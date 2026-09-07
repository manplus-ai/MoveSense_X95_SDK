// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "Simou3Net.h"
#include "Simou3Types.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace movesense {

struct Simou3ChunkHdr;
class Simou3CameraTransfer {
public:
    Simou3CameraTransfer(std::string ip = "192.168.1.70", int port = 5001);
    ~Simou3CameraTransfer();
    Simou3CameraTransfer(const Simou3CameraTransfer&) = delete;
    Simou3CameraTransfer& operator=(const Simou3CameraTransfer&) = delete;
    bool connectToCamera(int mode);

    bool startChunkRecv();
    void stopChunkRecv();
    bool getFrame(MovesenseFrame& out);
    bool getFrame(MovesenseFrame& out, int timeoutMs);
    Simou3Stats getStats() const;

    int getIMU(std::vector<Imu>& out, int num = 200);
    void setPtsOffset(int64_t offsetUs);

public:
    std::string mIP;
    int mPort;

private:
    Simou3Net mSock;
    int mTransferMode;

    void recvLoop();
    void stashChunk(const Simou3ChunkHdr& c, std::vector<uint8_t>&& buf);
    void stashIMU(std::vector<uint8_t>&& buf);
    void publish(MovesenseFrame&& f);
    bool frameComplete(const MovesenseFrame& f) const;

    static const int IMU_CAP = 200;
    static const int IMU_SAMPLE_BYTES = 24;
    std::deque<Imu> mImuQ;
    std::mutex mImuMtx;

    std::thread mRecvChunkThread;
    std::atomic<bool> mRunning { false };
    uint32_t mSubMask = 0;
    std::map<uint32_t, MovesenseFrame> mBuckets;
    uint32_t mNewestFc = 0;
    std::mutex mBmtx;
    std::mutex mSlotMtx;
    std::condition_variable mSlotCv;
    MovesenseFrame mSlot;
    std::atomic<bool> mSlotFull { false };
    std::atomic<uint64_t> mStEmpty { 0 }, mStAbn { 0 }, mStIncomplete { 0 }, mStOverwrite { 0 }, mStCorrupt { 0 };
    std::atomic<int64_t> mPtsOffset { 0 };
};

} // namespace movesense
