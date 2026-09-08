// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "CameraSession.h"

#include "movesense/Simou3CalibLayout.h"

#include <cmath>
#include <cstdio>
#include <iostream>

using namespace movesense;

namespace {
const int DEFAULT_FPS = 15;
const int DEFAULT_EXPO_US = 5000;
const int DEFAULT_GAIN_X = 4;
const int GAIN_UNIT = 128;

void PrintIntrinsics(const char* label, const float* v)
{
    printf("  %-10s fx=%.3f fy=%.3f cx=%.3f cy=%.3f\n", label, v[0], v[1], v[2], v[3]);
}

void PrintDistortion(const char* label, const float* v)
{
    printf("  %-10s", label);
    for (int i = 0; i < calib::kDistortionFloats; ++i) {
        printf(" %.6f", v[i]);
    }
    printf("\n");
}

void PrintMatrix3x3(const char* label, const float* v)
{
    printf("  %-10s [%.5f %.5f %.5f; %.5f %.5f %.5f; %.5f %.5f %.5f]\n", label, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);
}

void PrintImuSegment(const float* seg)
{
    bool allFinite = true;
    bool anyNonZero = false;
    for (int i = 0; i < calib::kImuSegmentFloats; ++i) {
        if (!std::isfinite(seg[i])) {
            allFinite = false;
            break;
        }
        if (seg[i] != 0.0f) {
            anyNonZero = true;
        }
    }

    if (!allFinite || !anyNonZero) {
        printf("  %-10s not calibrated (%s)\n", "imu:", allFinite ? "all zero" : "non-finite");
        return;
    }

    printf("  %-10s gyro_nd=%.3e gyro_rw=%.3e acc_nd=%.3e acc_rw=%.3e\n", "imu noise:", seg[calib::kImuGyroNoiseDensity],
        seg[calib::kImuGyroRandomWalk], seg[calib::kImuAccelNoiseDensity], seg[calib::kImuAccelRandomWalk]);
    printf("  %-10s %.4f ms\n", "imu ts:", seg[calib::kImuTimeshiftMs]);
    PrintMatrix3x3("imu R:", seg + calib::kImuRotationRowMajor);
    printf("  %-10s [%.4f %.4f %.4f] mm\n", "imu t:", seg[calib::kImuTranslationMm + 0], seg[calib::kImuTranslationMm + 1],
        seg[calib::kImuTranslationMm + 2]);
}
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

void CameraSession::PrintCalibration()
{
    float stereo[calib::kStereoCalibFloats] = { 0 };
    if (m_cam->getStereoCalibData(reinterpret_cast<unsigned char*>(stereo), calib::kStereoCalibBytes) <= 0) {
        printf("[Sample] getStereoCalibData failed\n");
        return;
    }

    printf("\n===== Stereo calibration (%d floats) =====\n", calib::kStereoCalibFloats);
    PrintIntrinsics("left M1:", stereo + calib::kStereoLeftIntrinsics);
    PrintDistortion("left D1:", stereo + calib::kStereoLeftDistortion);
    PrintMatrix3x3("left iR1:", stereo + calib::kStereoLeftInverseRectifyRowMajor);
    PrintIntrinsics("right M2:", stereo + calib::kStereoRightIntrinsics);
    PrintDistortion("right D2:", stereo + calib::kStereoRightDistortion);
    PrintMatrix3x3("right iR2:", stereo + calib::kStereoRightInverseRectifyRowMajor);
    PrintIntrinsics("rectified:", stereo + calib::kRectifiedFx);
    printf("  %-10s %.4f mm\n", "baseline:", std::fabs(stereo[calib::kStereoNegativeBaselineMm]));
    PrintImuSegment(stereo + calib::kStereoImuSegment);

    if (m_cameraType == 1) {
        printf("===== RGB calibration: passive (P) camera has no RGB lens =====\n");
        return;
    }

    float rgb[calib::kRgbCalibFloats] = { 0 };
    if (m_cam->getRGBCalibData(reinterpret_cast<unsigned char*>(rgb), calib::kRgbCalibBytes) <= 0) {
        printf("[Sample] getRGBCalibData failed\n");
        return;
    }

    printf("\n===== RGB calibration (%d floats) =====\n", calib::kRgbCalibFloats);
    PrintIntrinsics("rgb M2:", rgb + calib::kRgbIntrinsics);
    PrintDistortion("rgb D2:", rgb + calib::kRgbDistortion);
    PrintMatrix3x3("rgb iR2:", rgb + calib::kRgbInverseRectifyRowMajor);
    PrintIntrinsics("rectified:", rgb + calib::kRectifiedFx);
    PrintMatrix3x3("rgb R:", rgb + calib::kRgbRotationRowMajor);
    printf("  %-10s [%.4f %.4f %.4f] mm\n", "rgb T_reg:", rgb[calib::kRgbTranslationMm + 0], rgb[calib::kRgbTranslationMm + 1],
        rgb[calib::kRgbTranslationMm + 2]);
    PrintImuSegment(rgb + calib::kRgbImuSegment);
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

    PrintCalibration();

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
