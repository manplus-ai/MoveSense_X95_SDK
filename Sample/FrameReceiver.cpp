// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "FrameReceiver.h"

#include <chrono>

FrameReceiver::~FrameReceiver()
{
    Stop();
}

void FrameReceiver::Start(CameraSession* session)
{
    if (m_running.load()) {
        return;
    }
    m_session = session;
    m_running.store(true);
    m_thread = std::thread(&FrameReceiver::Loop, this);
}

void FrameReceiver::Stop()
{
    m_running.store(false);
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool FrameReceiver::TryGetLatest(MovesenseFrame& out)
{
    std::lock_guard<std::mutex> lk(m_mtx);
    if (!m_hasLatest) {
        return false;
    }
    out = std::move(m_latest);
    m_hasLatest = false;
    return true;
}

void FrameReceiver::Loop()
{
    int cnt = 0;
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    while (m_running.load()) {
        MovesenseFrame f;
        if (m_session->GetFrame(f, 200)) {
            {
                std::lock_guard<std::mutex> lk(m_mtx);
                m_latest = std::move(f);
                m_hasLatest = true;
            }
            ++cnt;
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            double sec = std::chrono::duration<double>(now - t0).count();
            if (sec >= 1.0) {
                m_fps.store(cnt / sec);
                cnt = 0;
                t0 = now;
            }
        }
    }
}
