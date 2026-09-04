// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "CameraSession.h"

#include <cstdio>
#include <iostream>

namespace {
const int DEFAULT_FPS = 15;
const int DEFAULT_EXPO_US = 5000;
const int DEFAULT_GAIN_X = 4;
const int GAIN_UNIT = 128;
} // namespace

CameraSession::~CameraSession()
{
    Close();
}

bool CameraSession::ScanAndSelect()
{
    printf("[Sample] Scanning cameras (all interfaces)...\n");
    fflush(stdout);
    std::vector<CameraInfo> cams = Simou3Camera::scanCamerasAllInterfaces();
    if (cams.empty()) {
        printf("[Sample] No camera found\n");
        return false;
    }
    printf("\n===== Found %d camera(s) =====\n", (int)cams.size());
    for (size_t i = 0; i < cams.size(); ++i) {
        printf("  %zu  %-16s  mac=%s\n", i, cams[i].ip.c_str(), cams[i].mac.c_str());
    }
    printf("========================\nEnter camera index (Enter to confirm): ");
    fflush(stdout);
    int sel = -1;
    std::cin >> sel;
    if (sel < 0 || sel >= (int)cams.size()) {
        printf("[Sample] Invalid camera index\n");
        return false;
    }
    m_ip = cams[sel].ip;
    m_cam = new Simou3Camera(m_ip);
    printf("[Sample] Selected %s\n", m_ip.c_str());
    return true;
}

void CameraSession::ApplyDownsample(const ModeSpec& mode)
{
    m_cam->setStereoDownsample(mode.m_downStereo);
    m_cam->setRGBDownsample(mode.m_downRgb);
    m_cam->setDepthDownsample(mode.m_downDepth);
    m_cam->setDownsampleMode(mode.m_downMode >= 0 ? mode.m_downMode : 1);
}

bool CameraSession::Open(const ModeSpec& mode)
{
    if (!m_cam) {
        return false;
    }
    if (!m_cam->isCameraOnline()) {
        printf("[Sample] %s is offline\n", m_ip.c_str());
        return false;
    }

    if (m_cam->openCameraSettings() <= 0) {
        printf("[Sample] openCameraSettings failed\n");
        return false;
    }

    int type = -1;
    if (m_cam->getCameraType(type) > 0) {
        m_cameraType = type;
        bool cameraPassive = (type == 1);
        if (cameraPassive != mode.m_passive) {
            printf("[Sample][WARN] mode %s is %s, but camera type=%s\n", mode.name.c_str(), mode.m_passive ? "passive" : "active",
                cameraPassive ? "passive" : "active");
        }
    }

    ApplyDownsample(mode);
    m_cam->setFrameRate(DEFAULT_FPS);
    m_cam->setTriggerMode(1);

    if (m_cam->openCamera((int)mode.Mask()) <= 0) {
        printf("[Sample] openCamera failed (mask=0x%x)\n", mode.Mask());
        return false;
    }
    m_opened = true;

    m_cam->setStereoAutoExpo(0);
    m_cam->setStereoExposure((unsigned)DEFAULT_EXPO_US);
    m_cam->setStereoGain((float)(DEFAULT_GAIN_X * GAIN_UNIT));
    if (mode.m_hasRgb) {
        m_cam->setRGBAutoExpo(0);
        m_cam->setRGBExposure((unsigned)DEFAULT_EXPO_US);
        m_cam->setRGBGain((float)(DEFAULT_GAIN_X * GAIN_UNIT));
    }
    printf("[Sample] Opened: mode=%s mask=0x%x type=%d\n", mode.name.c_str(), mode.Mask(), m_cameraType);
    return true;
}

void CameraSession::Close()
{
    if (m_cam) {
        if (m_opened) {
            m_cam->closeCamera();
        }
        m_cam->closeCameraSettings();
        delete m_cam;
        m_cam = nullptr;
    }
    m_opened = false;
}
