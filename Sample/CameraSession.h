// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "ModeCatalog.h"
#include "movesense/Simou3Camera.h"

#include <string>
#include <vector>

class CameraSession {
public:
    ~CameraSession();

    bool ScanAndSelect();
    bool Open(const ModeSpec& mode);
    void Close();

    bool GetFrame(MovesenseFrame& out, int timeoutMs)
    {
        return m_cam && m_cam->getFrame(out, timeoutMs);
    }

    int GetImu(std::vector<IMU>& out)
    {
        return m_cam ? m_cam->getIMU(out) : 0;
    }

    int CameraType() const
    {
        return m_cameraType;
    }

    const std::string& Ip() const
    {
        return m_ip;
    }

    Simou3Camera* Cam() const
    {
        return m_cam;
    }

private:
    void ApplyDownsample(const ModeSpec& mode);

    Simou3Camera* m_cam = nullptr;
    std::string m_ip;
    int m_cameraType = -1;
    bool m_opened = false;
};
