// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "CameraSession.h"
#include "movesense/Simou3Camera.h"

#include <atomic>
#include <cstdio>
#include <thread>
using namespace movesense;

class ImuReceiver {
public:
    ~ImuReceiver();

    void Start(CameraSession* session);
    void Stop();
    double Rate() const
    {
        return m_hz.load();
    }

    void ToggleRecord();
    bool Recording() const
    {
        return m_recording.load();
    }

private:
    void Loop();
    void ParseAndWrite(const Imu& s);

    CameraSession* m_session = nullptr;
    std::thread m_thread;
    std::atomic<bool> m_running { false };
    std::atomic<double> m_hz { 0.0 };

    std::atomic<bool> m_recording { false };
    FILE* m_csv = nullptr;
};
