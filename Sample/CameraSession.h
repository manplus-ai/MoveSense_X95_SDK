// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "ModeCatalog.h"
#include "movesense/Simou3Camera.h"

#include <string>
#include <vector>

using namespace movesense;

struct RoiRect {
    bool active = false;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
};

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

    int GetImu(std::vector<Imu>& out)
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

    const RoiRect& Roi() const
    {
        return m_roi;
    }

private:
    void ApplyDownsample(const ModeSpec& mode);
    bool ApplyRoi(const ModeSpec& mode);
    void ClearRoi();
    void PrintCalibration();

    Simou3Camera* m_cam = nullptr;
    std::string m_ip;
    int m_cameraType = -1;
    bool m_opened = false;
    int m_roiStream = -1;
    RoiRect m_roi;
};
