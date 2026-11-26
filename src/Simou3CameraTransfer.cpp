// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "movesense/Simou3CameraTransfer.h"
#include "movesense/Simou3Camera.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"
#include "movesense/transfer_mode_def.h"

#include <algorithm>
#include <limits>

namespace movesense {

struct Simou3ChunkHdr {
    uint32_t fc = 0, chunkStart = 0, chunkEnd = 0, totalLen = 0;
    uint8_t label = 0, flags = 0, status = 0, fmt = 0;
    DualExposurePhase exposurePhase = DualExposurePhase::Unknown;
    uint16_t w = 0, h = 0;
    uint64_t tsend = 0, pts = 0, recvNow = 0;
    bool isHead = false;
};

static int64_t SteadyTimeUs()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static uint64_t simou3_now_us()
{
    static const int64_t kEpochOffsetUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - SteadyTimeUs();
    return static_cast<uint64_t>(SteadyTimeUs() + kEpochOffsetUs);
}

class Simou3CameraTransfer::TimeSync
{
public:
    explicit TimeSync(Simou3CameraTransfer& transfer) : m_transfer(transfer) {}

    void OnStreamStopped();
    int Align();
    void SetFixedOffset(int64_t offsetUs);
    int64_t GetOffset() const;

    std::mutex m_mutex;

private:
    int64_t OffsetAt(int64_t nowUs) const;

    Simou3CameraTransfer& m_transfer;
    std::mutex m_probeMutex;
    uint32_t m_sequence = 0;
    bool m_hasAligned = false;
    uint64_t m_generation = 0;
    int64_t m_offsetUs = 0;
    int64_t m_targetUs = 0;
    int64_t m_offsetSetUs = 0;
};

void Simou3CameraTransfer::TimeSync::OnStreamStopped()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ++m_generation;
}

int Simou3CameraTransfer::TimeSync::Align()
{
    constexpr int kPort = 5000;
    constexpr int kProbes = 16;
    constexpr int kMaxDelayUs = 5000;
    constexpr uint32_t kMagic = 0x53335453;
    struct SampleSet {
        int accepted = 0;
        int64_t delayUs = kMaxDelayUs + 1;
        int64_t offsetUs = 0;
    };

    std::lock_guard<std::mutex> probeLock(m_probeMutex);
    uint64_t generation = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        generation = m_generation;
    }

    SampleSet legacy, echoed;
    for (int i = 0; i < kProbes; ++i) {
        Simou3Net net;
        if (!net.openUdp(50)) {
            return -1;
        }

        unsigned char request[16] = {};
        const uint32_t sequence = ++m_sequence;
        const int64_t t1 = static_cast<int64_t>(simou3_now_us());
        memcpy(request, &kMagic, sizeof(kMagic));
        memcpy(request + 4, &sequence, sizeof(sequence));
        memcpy(request + 8, &t1, sizeof(t1));
        if (net.sendTo(m_transfer.mIP, kPort, request, sizeof(request)) != sizeof(request)) {
            continue;
        }

        unsigned char reply[64];
        std::string sender;
        int port = 0;
        const int size = net.recvFrom(reply, sizeof(reply), &sender, &port);
        const int64_t t4 = static_cast<int64_t>(simou3_now_us());
        if (sender != m_transfer.mIP || port != kPort || (size != 16 && size != 32)) {
            continue;
        }
        const bool echo = size == 32;
        if (echo && memcmp(reply, request, sizeof(request)) != 0) {
            continue;
        }

        int64_t t2 = 0;
        int64_t t3 = 0;
        const unsigned char* timestamps = reply + (echo ? sizeof(request) : 0);
        memcpy(&t2, timestamps, sizeof(t2));
        memcpy(&t3, timestamps + sizeof(t2), sizeof(t3));
        if (t2 <= 0 || t3 < t2 || t4 < t1 || t3 - t2 > t4 - t1) {
            continue;
        }
        const int64_t delay = (t4 - t1) - (t3 - t2);
        if (delay > kMaxDelayUs) {
            continue;
        }

        SampleSet& samples = echo ? echoed : legacy;
        ++samples.accepted;
        if (delay < samples.delayUs) {
            samples.delayUs = delay;
            samples.offsetUs = (t1 - t2) / 2 + (t4 - t3) / 2;
        }
    }

    const SampleSet& samples = echoed.accepted != 0 ? echoed : legacy;
    if (samples.accepted < 3) {
        simou3_log("alignTimeToHost: not enough valid replies");
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (generation != m_generation) {
        return -1;
    }
    const int64_t now = SteadyTimeUs();
    m_offsetUs = m_hasAligned ? OffsetAt(now) : samples.offsetUs;
    m_hasAligned = true;
    m_targetUs = samples.offsetUs;
    m_offsetSetUs = now;
    simou3_log("alignTimeToHost: offset=" + std::to_string(m_targetUs) +
               "us, delay=" + std::to_string(samples.delayUs) + "us, valid probes=" +
               std::to_string(samples.accepted));
    return 1;
}

void Simou3CameraTransfer::TimeSync::SetFixedOffset(int64_t offsetUs)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_offsetUs = offsetUs;
    m_targetUs = offsetUs;
    m_offsetSetUs = SteadyTimeUs();
}

int64_t Simou3CameraTransfer::TimeSync::OffsetAt(int64_t nowUs) const
{
    const int64_t limitUs = (nowUs - m_offsetSetUs) / 5000;
    return m_offsetUs + (std::max)(-limitUs, (std::min)(limitUs, m_targetUs - m_offsetUs));
}

int64_t Simou3CameraTransfer::TimeSync::GetOffset() const
{
    return m_hasAligned ? OffsetAt(SteadyTimeUs()) : m_offsetUs;
}

static uint64_t ShiftTime(uint64_t pts, int64_t offsetUs)
{
    if (pts == 0 || pts > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
        return pts;
    }
    if (offsetUs > 0 && pts > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)() - offsetUs)) {
        return 0;
    }
    const int64_t mapped = static_cast<int64_t>(pts) + offsetUs;
    return mapped > 0 ? static_cast<uint64_t>(mapped) : 0;
}

Simou3CameraTransfer::Simou3CameraTransfer(std::string ip, int port)
    : m_timeSync { std::unique_ptr<TimeSync>(new TimeSync(*this)) }
{
    mIP = ip;
    mPort = port;
}

Simou3CameraTransfer::~Simou3CameraTransfer()
{
    stopChunkRecv();
}

bool Simou3CameraTransfer::connectToCamera(int mode)
{
    if (!mSock.connectTcp(mIP, mPort)) {
        return false;
    }
    mTransferMode = mode;
    return true;
}

bool Simou3CameraTransfer::startChunkRecv()
{
    if (mRunning.load())
        return true;
    unsigned char msg[8] = { 'M', 'O', 'D', 'E' };
    memcpy(msg + 4, &mTransferMode, 4);
    if (mSock.sendBlock(msg, 8) <= 0) {
        simou3_log("startChunkRecv: send MODE failed");
        return false;
    }
    mSubMask = static_cast<uint32_t>(mTransferMode);
    mNewestFc = 0;
    mStIncomplete.store(0);
    mSlotFull.store(false);
    mRunning.store(true);
    mRecvChunkThread = std::thread(&Simou3CameraTransfer::recvLoop, this);
    return true;
}

void Simou3CameraTransfer::stopChunkRecv()
{
    mRunning.store(false);
    mSlotCv.notify_all();
    mSock.shutdownSock();
    if (mRecvChunkThread.joinable())
        mRecvChunkThread.join();
    m_timeSync.state->OnStreamStopped();
    mSock.closeSock();
    {
        std::lock_guard<std::mutex> lk(mBmtx);
        mBuckets.clear();
        mNewestFc = 0;
    }
    {
        std::lock_guard<std::mutex> lk(mSlotMtx);
        mSlot = MovesenseFrame();
        mSlotFull.store(false);
    }
    {
        std::lock_guard<std::mutex> lk(mImuMtx);
        mImuQ.clear();
    }
}

namespace {
#pragma pack(push, 1)
struct YdrHeader {
    uint32_t magic;
    uint16_t version, headerBytes;
    uint32_t totalBytes;
    uint16_t coordW, coordH, detectionCount, detectionRecordBytes;
    uint32_t detectionsOffset;
};
#pragma pack(pop)
const uint32_t kYdrMagic = 0x31524459u;
void fillDetections(MovesenseFrame& f)
{
    f.m_detections.clear();
    f.m_detCoordW = 0;
    f.m_detCoordH = 0;
    const MovesenseFrame::Plane& seg = f.m_planes[MovesenseFrame::SEG];
    if (!seg.complete() || seg.data.size() < sizeof(YdrHeader)) {
        return;
    }

    const YdrHeader* h = reinterpret_cast<const YdrHeader*>(seg.data.data());
    if (h->magic != kYdrMagic || h->coordW == 0 || h->coordH == 0) {
        return;
    }

    size_t need = static_cast<size_t>(h->detectionsOffset) + static_cast<size_t>(h->detectionCount) * sizeof(DetectionBox);
    if (need > seg.data.size()) {
        return;
    }

    const DetectionBox* d = reinterpret_cast<const DetectionBox*>(seg.data.data() + h->detectionsOffset);
    f.m_detCoordW = h->coordW;
    f.m_detCoordH = h->coordH;
    f.m_detections.assign(d, d + h->detectionCount);
}
} // namespace

bool Simou3CameraTransfer::getFrame(MovesenseFrame& out)
{
    if (!mSlotFull.load(std::memory_order_acquire)) {
        return false;
    }

    std::lock_guard<std::mutex> lk(mSlotMtx);
    if (!mSlotFull.load(std::memory_order_relaxed)) {
        return false;
    }

    out = std::move(mSlot);
    fillDetections(out);
    mSlotFull.store(false, std::memory_order_release);
    return true;
}

bool Simou3CameraTransfer::getFrame(MovesenseFrame& out, int timeoutMs)
{
    std::unique_lock<std::mutex> lk(mSlotMtx);
    auto ready = [this] { return mSlotFull.load(std::memory_order_relaxed) || !mRunning.load(); };
    if (timeoutMs < 0) {
        mSlotCv.wait(lk, ready);
    } else if (timeoutMs > 0) {
        mSlotCv.wait_for(lk, std::chrono::milliseconds(timeoutMs), ready);
    }

    if (!mSlotFull.load(std::memory_order_relaxed)) {
        return false;
    }

    out = std::move(mSlot);
    fillDetections(out);
    mSlotFull.store(false, std::memory_order_release);
    return true;
}

Simou3Stats Simou3CameraTransfer::getStats() const
{
    Simou3Stats s;
    s.emptyPkt = mStEmpty.load();
    s.abnormalPkt = mStAbn.load();
    s.incompleteDrop = mStIncomplete.load();
    s.overwrite = mStOverwrite.load();
    s.corrupt = mStCorrupt.load();
    return s;
}

void Simou3CameraTransfer::stashIMU(std::vector<uint8_t>&& buf)
{
    if (buf.empty()) {
        return;
    }
    Imu s {};
    if (static_cast<int>(buf.size()) >= IMU_SAMPLE_BYTES) {
        const uint8_t* p = buf.data();
        std::memcpy(&s.ax, p, 14);
        std::memcpy(&s.timestampUs, p + 16, 8);
    }

    std::lock_guard<std::mutex> timeLock(m_timeSync.state->m_mutex);
    const int64_t offset = m_timeSync.state->GetOffset();
    s.timestampUs = ShiftTime(s.timestampUs, offset);

    std::lock_guard<std::mutex> lk(mImuMtx);
    if (static_cast<int>(mImuQ.size()) >= IMU_CAP) {
        mImuQ.pop_front();
    }
    mImuQ.push_back(std::move(s));
}

void Simou3CameraTransfer::setPtsOffset(int64_t offsetUs)
{
    m_timeSync.state->SetFixedOffset(offsetUs);
}

int Simou3Camera::alignTimeToHost()
{
    return mSimou3CameraTransfer.m_timeSync.state->Align();
}

int Simou3CameraTransfer::getIMU(std::vector<Imu>& out, int num)
{
    if (!mRunning.load()) {
        return -1;
    }

    if (num < 0) {
        num = 0;
    }
    if (num > IMU_CAP) {
        num = IMU_CAP;
    }

    int got = 0;
    std::lock_guard<std::mutex> lk(mImuMtx);
    while (got < num && !mImuQ.empty()) {
        out.push_back(mImuQ.front());
        mImuQ.pop_front();
        got++;
    }
    return got;
}

bool Simou3CameraTransfer::frameComplete(const MovesenseFrame& f) const
{
    for (int i = 0; i < MovesenseFrame::PLANE_N; i++) {
        int label = MovesenseFrame::idxToLabel(i);
        if (!((mSubMask >> label) & 1u)) {
            continue;
        }

        if (!f.m_planes[i].complete()) {
            return false;
        }
    }
    return true;
}

void Simou3CameraTransfer::publish(MovesenseFrame&& f)
{
    bool degraded = f.m_degraded;
    if (degraded) {
        mStIncomplete.fetch_add(1);
        if (mDropDegraded.load(std::memory_order_relaxed)) {
            return;
        }
    }

    std::lock_guard<std::mutex> timeLock(m_timeSync.state->m_mutex);
    const int64_t offset = m_timeSync.state->GetOffset();
    if (offset != 0) {
        for (auto& plane : f.m_planes) {
            if (plane.has) {
                plane.pts = ShiftTime(plane.pts, offset);
            }
        }
    }

    std::lock_guard<std::mutex> lk(mSlotMtx);
    if (mRequireKnownExposure && f.exposurePhase() == DualExposurePhase::Unknown) {
        return;
    }
    if (mSlotFull.load(std::memory_order_relaxed))
        mStOverwrite.fetch_add(1);
    mSlot = std::move(f);
    mSlotFull.store(true, std::memory_order_release);
    mSlotCv.notify_one();
}

void Simou3CameraTransfer::SetRequireKnownExposure(bool enabled)
{
    std::lock_guard<std::mutex> lock(mSlotMtx);
    if (mRequireKnownExposure == enabled) {
        return;
    }
    mRequireKnownExposure = enabled;
    mSlotFull.store(false, std::memory_order_release);
}

void Simou3CameraTransfer::setDropDegradedFrame(bool onoff)
{
    mDropDegraded.store(onoff, std::memory_order_relaxed);
}

void Simou3CameraTransfer::stashChunk(const Simou3ChunkHdr& c, std::vector<uint8_t>&& buf)
{
    std::lock_guard<std::mutex> lk(mBmtx);
    if (c.fc > mNewestFc) {
        mNewestFc = c.fc;
    }

    int idx = MovesenseFrame::labelToIdx(c.label);
    if (idx < 0) {
        return;
    }

    MovesenseFrame& f = mBuckets[c.fc];
    f.m_frameCnt = c.fc;
    MovesenseFrame::Plane& p = f.m_planes[idx];

    if (c.isHead) {
        if (c.status == 0 && (c.exposurePhase == DualExposurePhase::Low || c.exposurePhase == DualExposurePhase::High)) {
            f.m_exposurePhase = c.exposurePhase;
        }
        p.has = true;
        p.recvStart = c.recvNow;
        p.pts = c.pts;
        p.tsend = c.tsend;
        p.status = c.status;
        p.totalLen = c.totalLen;
        if (idx != MovesenseFrame::SEG) {
            p.format = static_cast<PFormat>(c.fmt);
            p.width = c.w;
            p.height = c.h;
        }
        if (c.status == 1) {
            mStAbn.fetch_add(1);
        }

        if (c.status == 2) {
            mStEmpty.fetch_add(1);
        }

        if (c.status != 0) {
            f.m_degraded = true;
        } else {
            p.data.resize(c.totalLen);
        }
    }
    if (c.status == 0 && !buf.empty()) {
        if (static_cast<size_t>(c.chunkStart) + buf.size() <= p.data.size()) {
            memcpy(p.data.data() + c.chunkStart, buf.data(), buf.size());
            p.recvBytes += static_cast<uint32_t>(buf.size());
            if (p.recvBytes == p.totalLen) {
                p.recvEnd = c.recvNow;
            }
        } else {
            mStCorrupt.fetch_add(1);
        }
    }

    if (frameComplete(f)) {
        publish(std::move(f));
        mBuckets.erase(c.fc);
    }

    const uint32_t AGE_WIN = 8;
    while (!mBuckets.empty()) {
        auto it = mBuckets.begin();
        if (mNewestFc >= AGE_WIN && it->first <= mNewestFc - AGE_WIN) {
            mBuckets.erase(it);
            mStIncomplete.fetch_add(1);
        } else {
            break;
        }
    }
}

void Simou3CameraTransfer::recvLoop()
{
    while (mRunning.load()) {
        unsigned char lb;
        if (mSock.recvStream(&lb, 1, mRunning) <= 0) {
            break;
        }

        if (lb == 30) {
            unsigned char cb[2];
            if (mSock.recvStream(cb, 2, mRunning) <= 0) {
                break;
            }

            uint16_t cnt = static_cast<uint16_t>(static_cast<uint16_t>(cb[0]) | (static_cast<uint16_t>(cb[1]) << 8));
            uint32_t plen = static_cast<uint32_t>(cnt) * IMU_SAMPLE_BYTES;
            std::vector<uint8_t> ibuf(plen);
            if (plen && mSock.recvStream(ibuf.data(), plen, mRunning) <= 0) {
                break;
            }
            for (uint16_t i = 0; i < cnt; i++) {
                stashIMU(std::vector<uint8_t>(
                    ibuf.begin() + static_cast<size_t>(i) * IMU_SAMPLE_BYTES, ibuf.begin() + static_cast<size_t>((i + 1)) * IMU_SAMPLE_BYTES));
            }
            continue;
        }

        unsigned char pre[14];
        pre[0] = lb;
        if (mSock.recvStream(pre + 1, 13, mRunning) <= 0) {
            break;
        }

        Simou3ChunkHdr c;
        c.label = lb;
        memcpy(&c.fc, pre + 1, 4);
        c.flags = pre[5];
        memcpy(&c.chunkStart, pre + 6, 4);
        memcpy(&c.chunkEnd, pre + 10, 4);
        c.isHead = (c.flags & 0x02) != 0;

        if (c.isHead) {
            unsigned char ext[27];
            if (mSock.recvStream(ext, 27, mRunning) <= 0) {
                break;
            }

            memcpy(&c.tsend, ext, 8);
            memcpy(&c.pts, ext + 8, 8);
            c.status = ext[16];
            c.fmt = ext[17];
            c.exposurePhase = static_cast<DualExposurePhase>(ext[18]);
            memcpy(&c.w, ext + 19, 2);
            memcpy(&c.h, ext + 21, 2);
            memcpy(&c.totalLen, ext + 23, 4);
        }

        if (MovesenseFrame::labelToIdx(c.label) < 0 || c.chunkEnd < c.chunkStart || (c.isHead && c.chunkEnd > c.totalLen)) {
            mStCorrupt.fetch_add(1);
            simou3_log("recvLoop: corrupt chunk -> disconnect");
            break;
        }

        uint32_t clen = c.chunkEnd - c.chunkStart;
        std::vector<uint8_t> buf(clen);
        if (clen && mSock.recvStream(buf.data(), clen, mRunning) <= 0) {
            break;
        }
        c.recvNow = simou3_now_us();
        stashChunk(c, std::move(buf));
    }
    {
        std::lock_guard<std::mutex> lock(mSlotMtx);
        mRunning.store(false);
    }
    mSlotCv.notify_all();
}

} // namespace movesense
