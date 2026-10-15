// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once

namespace movesense {
namespace calib {

constexpr int kStereoCalibFloats = 64;
constexpr int kRgbCalibFloats = 76;
constexpr int kStereoCalibBytes = kStereoCalibFloats * 4;
constexpr int kRgbCalibBytes = kRgbCalibFloats * 4;

constexpr int kIntrinsicsFloats = 4;
constexpr int kDistortionFloats = 8;
constexpr int kRotationFloats = 9;
constexpr int kTranslationFloats = 3;
constexpr int kImuSegmentFloats = 17;

constexpr int kStereoLeftIntrinsics = 0;
constexpr int kStereoLeftInverseRectifyRowMajor = 4;
constexpr int kStereoLeftDistortion = 13;
constexpr int kStereoRightIntrinsics = 21;
constexpr int kStereoRightInverseRectifyRowMajor = 25;
constexpr int kStereoRightDistortion = 34;

constexpr int kRectifiedFx = 42;
constexpr int kRectifiedFy = 43;
constexpr int kRectifiedCx = 44;
constexpr int kRectifiedCy = 45;

constexpr int kStereoNegativeBaselineMm = 46;
constexpr int kStereoImuSegment = 47;

constexpr int kRgbIntrinsics = 21;
constexpr int kRgbInverseRectifyRowMajor = 25;
constexpr int kRgbDistortion = 34;
constexpr int kRgbReserved46 = 46;
constexpr int kRgbRotationRowMajor = 47;
constexpr int kRgbTranslationMm = 56;
constexpr int kRgbImuSegment = 59;

constexpr int kImuGyroNoiseDensity = 0;
constexpr int kImuGyroRandomWalk = 1;
constexpr int kImuAccelNoiseDensity = 2;
constexpr int kImuAccelRandomWalk = 3;
constexpr int kImuTimeshiftMs = 4;
constexpr int kImuRotationRowMajor = 5;
constexpr int kImuTranslationMm = 14;

constexpr int kRgbReservedFloats = 21;

struct Intrinsics {
    float fx = 0.0f;
    float fy = 0.0f;
    float cx = 0.0f;
    float cy = 0.0f;
};

struct ImuCalibration {
    float gyroNoiseDensity = 0.0f;
    float gyroRandomWalk = 0.0f;
    float accelNoiseDensity = 0.0f;
    float accelRandomWalk = 0.0f;
    float timeshiftMs = 0.0f;
    float rotation[kRotationFloats] = { 0.0f };
    float translationMm[kTranslationFloats] = { 0.0f };
};

struct StereoCamera {
    Intrinsics intrinsics;
    float inverseRectify[kRotationFloats] = { 0.0f };
    float distortion[kDistortionFloats] = { 0.0f };
};

struct StereoCalibration {
    StereoCamera left;
    StereoCamera right;
    Intrinsics rectified;
    float negativeBaselineMm = 0.0f;
    ImuCalibration imu;
};

struct RgbCalibration {
    float reserved[kRgbReservedFloats] = { 0.0f };
    Intrinsics intrinsics;
    float inverseRectify[kRotationFloats] = { 0.0f };
    float distortion[kDistortionFloats] = { 0.0f };
    Intrinsics rectified;
    float reserved46 = 0.0f;
    float payloadRotation[kRotationFloats] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    float registeredTranslation[kTranslationFloats] = { 0.0f };
    ImuCalibration imu;
};

} // namespace calib
} // namespace movesense
