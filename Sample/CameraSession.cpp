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
const int ROI_ORIGIN_X = 0;
const int ROI_ORIGIN_Y = 0;
const int ROI_SIZE = 64;

void PrintIntrinsics(const char* label, const calib::Intrinsics& value)
{
    printf("  %s fx=%.9g fy=%.9g cx=%.9g cy=%.9g px\n", label, value.fx, value.fy, value.cx, value.cy);
}

void PrintDistortion(const char* label, const float* values)
{
    printf("  %s", label);
    for (int index = 0; index < calib::kDistortionFloats; index++) {
        printf(" %.9g", values[index]);
    }
    printf("\n");
}

void PrintMatrix3x3(const char* label, const float* values)
{
    printf("  %s\n", label);
    for (int row = 0; row < 3; row++) {
        printf("    [%.9g %.9g %.9g]\n", values[row * 3], values[row * 3 + 1], values[row * 3 + 2]);
    }
}

void PrintImuCalibration(const calib::ImuCalibration& imu, const char* cameraFrame)
{
    printf("  imu.hasIntrinsics=%s\n", imu.hasIntrinsics ? "true" : "false");
    bool valid = std::isfinite(imu.gyroNoiseDensity) && imu.gyroNoiseDensity > 0.0f && std::isfinite(imu.gyroRandomWalk) &&
                 imu.gyroRandomWalk > 0.0f && std::isfinite(imu.accelNoiseDensity) && imu.accelNoiseDensity > 0.0f &&
                 std::isfinite(imu.accelRandomWalk) && imu.accelRandomWalk > 0.0f && std::isfinite(imu.timeshiftMs);
    for (float value : imu.rotation) {
        valid = valid && std::isfinite(value);
    }
    for (float value : imu.translationMm) {
        valid = valid && std::isfinite(value);
    }
    if (!valid) {
        printf("  IMU calibration unavailable\n");
        return;
    }
    printf("  imu.gyroNoiseDensity=%.9g rad/s/sqrt(Hz)\n", imu.gyroNoiseDensity);
    printf("  imu.gyroRandomWalk=%.9g rad/s^2/sqrt(Hz)\n", imu.gyroRandomWalk);
    printf("  imu.accelNoiseDensity=%.9g m/s^2/sqrt(Hz)\n", imu.accelNoiseDensity);
    printf("  imu.accelRandomWalk=%.9g m/s^3/sqrt(Hz)\n", imu.accelRandomWalk);
    printf("  imu.timeshiftMs=%.9g ms (t_imu = t_camera + shift)\n", imu.timeshiftMs);
    printf("  IMU extrinsics: IMU -> %s\n", cameraFrame);
    PrintMatrix3x3("imu.rotation:", imu.rotation);
    printf("  imu.translationMm=[%.9g %.9g %.9g] mm\n", imu.translationMm[0], imu.translationMm[1], imu.translationMm[2]);
    if (!imu.hasIntrinsics) {
        printf("  IMU intrinsic matrices unavailable\n");
        return;
    }
    PrintMatrix3x3("imu.accelM:", imu.accelM);
    PrintMatrix3x3("imu.gyroM:", imu.gyroM);
    PrintMatrix3x3("imu.gyroA ((rad/s)/(m/s^2)):", imu.gyroA);
    PrintMatrix3x3("imu.gyroC:", imu.gyroC);
}

const char* RoiStreamName(int stream)
{
    switch (stream) {
        case static_cast<int>(RoiStream::LeftRaw):
            return "left_raw";
        case static_cast<int>(RoiStream::RightRaw):
            return "right_raw";
        case static_cast<int>(RoiStream::RgbRaw):
            return "rgb_raw";
        case static_cast<int>(RoiStream::LeftRect):
            return "left_rect";
        case static_cast<int>(RoiStream::RightRect):
            return "right_rect";
        case static_cast<int>(RoiStream::RgbRect):
            return "rgb_rect";
        default:
            return "?";
    }
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
    calib::StereoCalibration stereo;
    const int stereoResult = m_cam->getStereoCalibData(stereo);
    if (stereoResult > 0) {
        printf("\n===== Stereo calibration =====\n");
        printf("Raw left camera\n");
        PrintIntrinsics("intrinsics:", stereo.left.intrinsics);
        PrintDistortion("distortion:", stereo.left.distortion);
        PrintMatrix3x3("inverseRectify:", stereo.left.inverseRectify);
        printf("Raw right camera\n");
        PrintIntrinsics("intrinsics:", stereo.right.intrinsics);
        PrintDistortion("distortion:", stereo.right.distortion);
        PrintMatrix3x3("inverseRectify:", stereo.right.inverseRectify);
        PrintIntrinsics("rectified:", stereo.rectified);
        printf("  negativeBaselineMm=%.9g mm, baseline=%.9g mm\n", stereo.negativeBaselineMm, std::fabs(stereo.negativeBaselineMm));
        PrintImuCalibration(stereo.imu, "raw left camera");
    } else {
        printf("[Sample] getStereoCalibData failed rc=%d\n", stereoResult);
    }

    if (m_cameraType == 1) {
        printf("\n===== RGB calibration: unavailable on a passive camera =====\n");
    } else {
        calib::RgbCalibration rgb;
        const int rgbResult = m_cam->getRGBCalibData(rgb);
        if (rgbResult > 0) {
            printf("\n===== RGB calibration =====\n");
            PrintIntrinsics("raw intrinsics:", rgb.intrinsics);
            PrintDistortion("distortion:", rgb.distortion);
            PrintMatrix3x3("inverseRectify:", rgb.inverseRectify);
            PrintIntrinsics("output intrinsics:", rgb.rectified);
            printf("  registeredTranslation=[%.9g %.9g %.9g] mm\n", rgb.registeredTranslation[0], rgb.registeredTranslation[1],
                rgb.registeredTranslation[2]);
            PrintMatrix3x3("payloadRotation (reserved):", rgb.payloadRotation);
            PrintImuCalibration(rgb.imu, "raw RGB camera");
        } else {
            printf("[Sample] getRGBCalibData failed rc=%d\n", rgbResult);
        }
    }
    printf("\n");
    fflush(stdout);
}

void CameraSession::ApplyDownsample(const ModeSpec& mode)
{
    m_cam->setStereoDownsample(mode.m_downStereo);
    m_cam->setRGBDownsample(mode.m_downRgb);
    m_cam->setDepthDownsample(mode.m_downDepth);
    m_cam->setDownsampleMode(mode.m_downMode >= 0 ? mode.m_downMode : 1);
}

bool CameraSession::ApplyRoi(const ModeSpec& mode)
{
    m_roi = RoiRect();
    m_roiStream = -1;
    for (int st = 0; st <= static_cast<int>(RoiStream::RgbRect); ++st) {
        m_cam->setRoi(static_cast<RoiStream>(st), false, 0, 0, 0, 0);
    }
    if (!mode.HasRoi()) {
        return true;
    }
    RoiStream stream = static_cast<RoiStream>(mode.m_roiStream);
    unsigned x1 = ROI_ORIGIN_X, y1 = ROI_ORIGIN_Y;
    unsigned x2 = ROI_ORIGIN_X + ROI_SIZE, y2 = ROI_ORIGIN_Y + ROI_SIZE;
    int rc = m_cam->setRoi(stream, true, x1, y1, x2, y2);
    if (rc <= 0) {
        printf("[Sample] setRoi(%s, [%u,%u]-[%u,%u]) failed rc=%d%s\n", RoiStreamName(mode.m_roiStream), x1, y1, x2, y2, rc,
            rc == SIMOU3_ERR_ROI_REJECTED  ? " (camera rejected)"
            : rc == SIMOU3_ERR_ROI_INVALID ? " (invalid rect)"
                                           : "");
        return false;
    }
    m_roiStream = mode.m_roiStream;
    m_roi.active = true;
    m_roi.x1 = (int)x1;
    m_roi.y1 = (int)y1;
    m_roi.x2 = (int)x2;
    m_roi.y2 = (int)y2;
    printf("[Sample] ROI %s = [%u,%u]-[%u,%u]\n", RoiStreamName(mode.m_roiStream), x1, y1, x2, y2);
    return true;
}

void CameraSession::ClearRoi()
{
    if (m_cam && m_roiStream >= 0) {
        m_cam->setRoi(static_cast<RoiStream>(m_roiStream), false, 0, 0, 0, 0);
        printf("[Sample] ROI %s cleared\n", RoiStreamName(m_roiStream));
    }
    m_roiStream = -1;
    m_roi = RoiRect();
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
    m_cam->setTriggerMode(kTriggerModeAuto);
    if (!ApplyRoi(mode)) {
        return false;
    }

    if (m_cam->openCamera((int)mode.Mask()) <= 0) {
        printf("[Sample] openCamera failed (mask=0x%x)\n", mode.Mask());
        ClearRoi();
        return false;
    }
    m_opened = true;

    MovesenseFrame firstFrame;
    if (!m_cam->getFrame(firstFrame, 5000)) {
        printf("[Sample] No frame received within 5000 ms after opening; calibration is not ready\n");
        fflush(stdout);
        Close();
        return false;
    }
    PrintCalibration();

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
        ClearRoi();
        if (m_opened) {
            m_cam->closeCamera();
        }
        m_cam->closeCameraSettings();
        delete m_cam;
        m_cam = nullptr;
    }
    m_opened = false;
}
