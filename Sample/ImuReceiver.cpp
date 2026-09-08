// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "ImuReceiver.h"

#include <chrono>
#include <cstring>
#include <vector>

using namespace movesense;

ImuReceiver::~ImuReceiver()
{
    Stop();
    if (m_csv) {
        fclose(m_csv);
        m_csv = nullptr;
    }
}

void ImuReceiver::Start(CameraSession* session)
{
    if (m_running.load()) {
        return;
    }
    m_session = session;
    m_running.store(true);
    m_thread = std::thread(&ImuReceiver::Loop, this);
}

void ImuReceiver::Stop()
{
    m_running.store(false);
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ImuReceiver::ToggleRecord()
{
    if (m_recording.load()) {
        m_recording.store(false);
        if (m_csv) {
            fclose(m_csv);
            m_csv = nullptr;
        }
        printf("[IMU] Stop recording\n");
    } else {
        m_csv = fopen("imu.csv", "w");
        if (m_csv) {
            fprintf(m_csv, "ts_us,ax,ay,az,gx,gy,gz\n");
            m_recording.store(true);
            printf("[IMU] Start recording -> imu.csv\n");
        }
    }
    fflush(stdout);
}

void ImuReceiver::ParseAndWrite(const Imu& s)
{
    if (m_recording.load() && m_csv) {
        fprintf(m_csv, "%llu,%d,%d,%d,%d,%d,%d\n", (unsigned long long)s.timestampUs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz);
    }
}

void ImuReceiver::Loop()
{
    int cnt = 0;
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    while (m_running.load()) {
        std::vector<Imu> samples;
        int n = m_session->GetImu(samples);
        if (n > 0) {
            for (const Imu& s : samples) {
                ParseAndWrite(s);
            }
            cnt += n;
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            double sec = std::chrono::duration<double>(now - t0).count();
            if (sec >= 1.0) {
                m_hz.store(cnt / sec);
                cnt = 0;
                t0 = now;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}
