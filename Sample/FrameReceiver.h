// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "CameraSession.h"
#include "movesense/Simou3Camera.h"

#include <atomic>
#include <mutex>
#include <thread>
using namespace movesense;

class FrameReceiver {
public:
    ~FrameReceiver();

    void Start(CameraSession* session);
    void Stop();
    bool TryGetLatest(MovesenseFrame& out);
    double Fps() const
    {
        return m_fps.load();
    }

private:
    void Loop();

    CameraSession* m_session = nullptr;
    std::thread m_thread;
    std::atomic<bool> m_running { false };

    std::mutex m_mtx;
    MovesenseFrame m_latest;
    bool m_hasLatest = false;

    std::atomic<double> m_fps { 0.0 };
};
