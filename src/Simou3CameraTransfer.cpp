// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "Simou3CameraTransfer.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"
#include "transfer_mode_def.h"

struct Simou3ChunkHdr {
    uint32_t fc = 0, chunkStart = 0, chunkEnd = 0, totalLen = 0;
    uint8_t label = 0, flags = 0, status = 0, fmt = 0;
    uint16_t w = 0, h = 0;
    uint64_t tsend = 0, pts = 0, recvNow = 0;
    bool isHead = false;
};

Simou3CameraTransfer::Simou3CameraTransfer(std::string ip, int port)
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

static uint64_t simou3_now_us()
{
    auto now = std::chrono::system_clock::now();
    auto tp = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    return static_cast<uint64_t>(tp.time_since_epoch().count());
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
    mSock.closeSock();
    if (mRecvChunkThread.joinable())
        mRecvChunkThread.join();
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
    std::lock_guard<std::mutex> lk(mImuMtx);
    IMU s {};
    if (static_cast<int>(buf.size()) >= IMU_SAMPLE_BYTES) {
        const uint8_t* p = buf.data();
        std::memcpy(&s.ax, p, 14);
        std::memcpy(&s.timestampUs, p + 16, 8);
    }

    int64_t off = mPtsOffset.load(std::memory_order_relaxed);
    if (off != 0) {
        s.timestampUs = static_cast<uint64_t>(static_cast<int64_t>(s.timestampUs) + off);
    }

    if (static_cast<int>(mImuQ.size()) >= IMU_CAP) {
        mImuQ.pop_front();
    }
    mImuQ.push_back(std::move(s));
}

void Simou3CameraTransfer::setPtsOffset(int64_t offsetUs)
{
    mPtsOffset.store(offsetUs, std::memory_order_relaxed);
}

int Simou3CameraTransfer::getIMU(std::vector<IMU>& out, int num)
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
    }

    if (degraded) {
        return;
    }

    int64_t off = mPtsOffset.load(std::memory_order_relaxed);
    if (off != 0) {
        for (int i = 0; i < MovesenseFrame::PLANE_N; i++) {
            if (f.m_planes[i].has) {
                f.m_planes[i].pts = static_cast<uint64_t>(static_cast<int64_t>(f.m_planes[i].pts) + off);
            }
        }
    }

    std::lock_guard<std::mutex> lk(mSlotMtx);
    if (mSlotFull.load(std::memory_order_relaxed))
        mStOverwrite.fetch_add(1);
    mSlot = std::move(f);
    mSlotFull.store(true, std::memory_order_release);
    mSlotCv.notify_one();
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
        if (mSock.recvBlock(&lb, 1) <= 0) {
            break;
        }

        if (lb == 30) {
            unsigned char cb[2];
            if (mSock.recvBlock(cb, 2) <= 0) {
                break;
            }

            uint16_t cnt = static_cast<uint16_t>(static_cast<uint16_t>(cb[0]) | (static_cast<uint16_t>(cb[1]) << 8));
            uint32_t plen = static_cast<uint32_t>(cnt) * IMU_SAMPLE_BYTES;
            std::vector<uint8_t> ibuf(plen);
            if (plen && mSock.recvBlock(ibuf.data(), plen) <= 0) {
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
        if (mSock.recvBlock(pre + 1, 13) <= 0) {
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
            if (mSock.recvBlock(ext, 27) <= 0) {
                break;
            }

            memcpy(&c.tsend, ext, 8);
            memcpy(&c.pts, ext + 8, 8);
            c.status = ext[16];
            c.fmt = ext[17];
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
        if (clen && mSock.recvBlock(buf.data(), clen) <= 0) {
            break;
        }
        c.recvNow = simou3_now_us();
        stashChunk(c, std::move(buf));
    }
    mRunning.store(false);
}
