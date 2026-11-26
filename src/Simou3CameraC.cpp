// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All
// rights reserved.

#include "movesense/Simou3CameraC.h"

#include "movesense/Simou3Camera.h"
#include "movesense/Simou3Types.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct ms_x95_camera {
    explicit ms_x95_camera(const char* ip) : impl(ip ? ip : "192.168.1.70") {}
    movesense::Simou3Camera impl;
};

struct ms_x95_frame {
    movesense::MovesenseFrame impl;
};

struct ms_x95_scan_list {
    std::vector<movesense::CameraInfo> cameras;
};

struct ms_x95_interface_list {
    std::vector<movesense::NetworkInterface> interfaces;
};

namespace {

thread_local std::string g_lastError;

int fail(int code, const char* message) noexcept
{
    try {
        g_lastError = message ? message : "unknown error";
    } catch (...) {
        g_lastError.clear();
    }
    return code;
}

template<typename F>
int guard(F&& function) noexcept
{
    try {
        return function();
    } catch (const std::bad_alloc&) {
        return fail(MS_X95_ERR_ALLOCATION, "allocation failed");
    } catch (const std::exception& error) {
        return fail(MS_X95_ERR_CPP_EXCEPTION, error.what());
    } catch (...) {
        return fail(MS_X95_ERR_CPP_EXCEPTION, "unknown C++ exception");
    }
}

bool validCamera(const ms_x95_camera_t* camera)
{
    return camera != nullptr;
}

bool validFrame(const ms_x95_frame_t* frame)
{
    return frame != nullptr;
}

int copyString(const std::string& value, char* buffer, uint32_t capacity, uint32_t* required)
{
    if (!required) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "required is null");
    }
    const uint64_t need = static_cast<uint64_t>(value.size()) + 1u;
    if (need > UINT32_MAX) {
        return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "string is too large");
    }
    *required = static_cast<uint32_t>(need);
    if (!buffer || capacity < *required) {
        return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "buffer is too small");
    }
    std::memcpy(buffer, value.c_str(), *required);
    return MS_X95_OK;
}

int copyFixed(const std::string& value, char* buffer, size_t capacity)
{
    if (value.size() + 1 > capacity) {
        return MS_X95_ERR_BUFFER_TOO_SMALL;
    }
    std::memcpy(buffer, value.c_str(), value.size() + 1);
    return MS_X95_OK;
}

int toChannel(uint32_t value, movesense::IspChannel& channel)
{
    if (value > 1) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "invalid ISP channel");
    }
    channel = value == 0 ? movesense::IspChannel::Stereo : movesense::IspChannel::Rgb;
    return MS_X95_OK;
}

int toRoiStream(uint32_t value, movesense::RoiStream& stream)
{
    if (value > 5) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "invalid ROI stream");
    }
    stream = static_cast<movesense::RoiStream>(value);
    return MS_X95_OK;
}

const movesense::MovesenseFrame::Plane* plane(const ms_x95_frame_t* frame, uint32_t stream)
{
    if (!validFrame(frame) || stream > UINT8_MAX) {
        return nullptr;
    }
    const int index = movesense::MovesenseFrame::labelToIdx(static_cast<uint8_t>(stream));
    if (index < 0) {
        return nullptr;
    }
    return &frame->impl.m_planes[index];
}

void fillPlaneInfo(const movesense::MovesenseFrame::Plane& source, ms_x95_plane_info_t& target)
{
    target.has = source.has ? 1 : 0;
    target.status = source.status;
    target.width = source.width;
    target.height = source.height;
    target.format = static_cast<uint32_t>(source.format);
    target.pts = source.pts;
    target.tsend = source.tsend;
    target.recv_start = source.recvStart;
    target.recv_end = source.recvEnd;
    target.total_len = source.totalLen;
    target.recv_bytes = source.recvBytes;
    target.complete = source.complete() ? 1 : 0;
    target.degraded = source.degraded() ? 1 : 0;
}

int copyCameraInfo(const movesense::CameraInfo& source, ms_x95_camera_info_t& target)
{
    std::memset(&target, 0, sizeof(target));
    target.port = source.port;
    target.mode = source.mode;
    target.len = source.len;
    int result = copyFixed(source.ip, target.ip, sizeof(target.ip));
    if (result <= 0) {
        return result;
    }
    result = copyFixed(source.mac, target.mac, sizeof(target.mac));
    if (result <= 0) {
        return result;
    }
    result = copyFixed(source.gateway, target.gateway, sizeof(target.gateway));
    if (result <= 0) {
        return result;
    }
    return copyFixed(source.netmask, target.netmask, sizeof(target.netmask));
}

int copyInterface(const movesense::NetworkInterface& source, ms_x95_network_interface_t& target)
{
    std::memset(&target, 0, sizeof(target));
    int result = copyFixed(source.name, target.name, sizeof(target.name));
    if (result <= 0) {
        return result;
    }
    result = copyFixed(source.ip, target.ip, sizeof(target.ip));
    if (result <= 0) {
        return result;
    }
    result = copyFixed(source.netmask, target.netmask, sizeof(target.netmask));
    if (result <= 0) {
        return result;
    }
    return copyFixed(source.broadcast, target.broadcast, sizeof(target.broadcast));
}

void packIntrinsics(const ms_x95_intrinsics_t& source, movesense::calib::Intrinsics& target)
{
    target.fx = source.fx;
    target.fy = source.fy;
    target.cx = source.cx;
    target.cy = source.cy;
}

void unpackIntrinsics(const movesense::calib::Intrinsics& source, ms_x95_intrinsics_t& target)
{
    target.fx = source.fx;
    target.fy = source.fy;
    target.cx = source.cx;
    target.cy = source.cy;
}

void packImu(const ms_x95_imu_calibration_t& source, movesense::calib::ImuCalibration& target)
{
    target.gyroNoiseDensity = source.gyro_noise_density;
    target.gyroRandomWalk = source.gyro_random_walk;
    target.accelNoiseDensity = source.accel_noise_density;
    target.accelRandomWalk = source.accel_random_walk;
    target.timeshiftMs = source.timeshift_ms;
    std::memcpy(target.rotation, source.rotation, sizeof(target.rotation));
    std::memcpy(target.translationMm, source.translation_mm, sizeof(target.translationMm));
}

void unpackImu(const movesense::calib::ImuCalibration& source, ms_x95_imu_calibration_t& target)
{
    target.gyro_noise_density = source.gyroNoiseDensity;
    target.gyro_random_walk = source.gyroRandomWalk;
    target.accel_noise_density = source.accelNoiseDensity;
    target.accel_random_walk = source.accelRandomWalk;
    target.timeshift_ms = source.timeshiftMs;
    std::memcpy(target.rotation, source.rotation, sizeof(target.rotation));
    std::memcpy(target.translation_mm, source.translationMm, sizeof(target.translation_mm));
}

void packImuIntrinsics(const ms_x95_imu_intrinsics_t& source, movesense::calib::ImuCalibration& target)
{
    std::memcpy(target.accelM, source.accel_m, sizeof(target.accelM));
    std::memcpy(target.gyroM, source.gyro_m, sizeof(target.gyroM));
    std::memcpy(target.gyroA, source.gyro_a, sizeof(target.gyroA));
    std::memcpy(target.gyroC, source.gyro_c, sizeof(target.gyroC));
    target.hasIntrinsics = true;
}

void unpackImuIntrinsics(const movesense::calib::ImuCalibration& source, ms_x95_imu_intrinsics_t& target, int32_t& hasIntrinsics)
{
    hasIntrinsics = source.hasIntrinsics ? 1 : 0;
    if (!source.hasIntrinsics) {
        target = {};
        return;
    }

    std::memcpy(target.accel_m, source.accelM, sizeof(target.accel_m));
    std::memcpy(target.gyro_m, source.gyroM, sizeof(target.gyro_m));
    std::memcpy(target.gyro_a, source.gyroA, sizeof(target.gyro_a));
    std::memcpy(target.gyro_c, source.gyroC, sizeof(target.gyro_c));
}

void packStereoCamera(const ms_x95_stereo_camera_calibration_t& source, movesense::calib::StereoCamera& target)
{
    packIntrinsics(source.intrinsics, target.intrinsics);
    std::memcpy(target.inverseRectify, source.inverse_rectify, sizeof(target.inverseRectify));
    std::memcpy(target.distortion, source.distortion, sizeof(target.distortion));
}

void unpackStereoCamera(const movesense::calib::StereoCamera& source, ms_x95_stereo_camera_calibration_t& target)
{
    unpackIntrinsics(source.intrinsics, target.intrinsics);
    std::memcpy(target.inverse_rectify, source.inverseRectify, sizeof(target.inverse_rectify));
    std::memcpy(target.distortion, source.distortion, sizeof(target.distortion));
}

void packStereoCalibration(const ms_x95_stereo_calibration_t& source, movesense::calib::StereoCalibration& target)
{
    packStereoCamera(source.left, target.left);
    packStereoCamera(source.right, target.right);
    packIntrinsics(source.rectified, target.rectified);
    target.negativeBaselineMm = source.negative_baseline_mm;
    packImu(source.imu, target.imu);
}

void unpackStereoCalibration(const movesense::calib::StereoCalibration& source, ms_x95_stereo_calibration_t& target)
{
    unpackStereoCamera(source.left, target.left);
    unpackStereoCamera(source.right, target.right);
    unpackIntrinsics(source.rectified, target.rectified);
    target.negative_baseline_mm = source.negativeBaselineMm;
    unpackImu(source.imu, target.imu);
}

void packRgbCalibration(const ms_x95_rgb_calibration_t& source, movesense::calib::RgbCalibration& target)
{
    std::memcpy(target.reserved, source.reserved, sizeof(target.reserved));
    packIntrinsics(source.intrinsics, target.intrinsics);
    std::memcpy(target.inverseRectify, source.inverse_rectify, sizeof(target.inverseRectify));
    std::memcpy(target.distortion, source.distortion, sizeof(target.distortion));
    packIntrinsics(source.rectified, target.rectified);
    target.reserved46 = source.reserved46;
    std::memcpy(target.payloadRotation, source.payload_rotation, sizeof(target.payloadRotation));
    std::memcpy(target.registeredTranslation, source.registered_translation, sizeof(target.registeredTranslation));
    packImu(source.imu, target.imu);
}

void unpackRgbCalibration(const movesense::calib::RgbCalibration& source, ms_x95_rgb_calibration_t& target)
{
    std::memcpy(target.reserved, source.reserved, sizeof(target.reserved));
    unpackIntrinsics(source.intrinsics, target.intrinsics);
    std::memcpy(target.inverse_rectify, source.inverseRectify, sizeof(target.inverse_rectify));
    std::memcpy(target.distortion, source.distortion, sizeof(target.distortion));
    unpackIntrinsics(source.rectified, target.rectified);
    target.reserved46 = source.reserved46;
    std::memcpy(target.payload_rotation, source.payloadRotation, sizeof(target.payload_rotation));
    std::memcpy(target.registered_translation, source.registeredTranslation, sizeof(target.registered_translation));
    unpackImu(source.imu, target.imu);
}

int copyBlob(const unsigned char* source, uint32_t size, uint8_t* buffer, uint32_t capacity, uint32_t* written)
{
    if (!written) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "written is null");
    }
    *written = size;
    if (!buffer || capacity < size) {
        return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "buffer is too small");
    }
    std::memcpy(buffer, source, size);
    return MS_X95_OK;
}

} // namespace

extern "C" MS_X95_API int MS_X95_CALL ms_x95_get_version(int* major, int* minor, int* patch)
{
    if (!major || !minor || !patch) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "version output is null");
    }
    movesense::simou3_get_version(major, minor, patch);
    return MS_X95_OK;
}

extern "C" MS_X95_API const char* MS_X95_CALL ms_x95_get_version_string(void)
{
    return movesense::simou3_get_version_string();
}

extern "C" MS_X95_API const char* MS_X95_CALL ms_x95_get_last_error(void)
{
    return g_lastError.c_str();
}

extern "C" MS_X95_API const char* MS_X95_CALL ms_x95_error_message(int code)
{
    switch (code) {
        case MS_X95_OK:
            return "success";
        case MS_X95_NO_DATA:
            return "no data";
        case MS_X95_ERR_NOT_SUPPORTED:
            return "not supported";
        case MS_X95_ERR_ROI_INVALID:
            return "invalid ROI";
        case MS_X95_ERR_ROI_REJECTED:
            return "ROI rejected";
        case MS_X95_ERR_TRIGGER_MODE_INVALID:
            return "invalid trigger mode";
        case MS_X95_ERR_TRIGGER_REJECTED:
            return "trigger rejected";
        case MS_X95_ERR_CONFIG_INVALID:
            return "invalid configuration";
        case MS_X95_ERR_STORAGE_FAILED:
            return "storage failed";
        case MS_X95_ERR_INFO_UNAVAILABLE:
            return "information unavailable";
        case MS_X95_ERR_CONFIG_BUSY:
            return "configuration busy";
        case MS_X95_ERR_CONFIG_APPLY_FAILED:
            return "configuration apply failed";
        case MS_X95_ERR_INVALID_ARGUMENT:
            return "invalid argument";
        case MS_X95_ERR_INVALID_HANDLE:
            return "invalid handle";
        case MS_X95_ERR_BUFFER_TOO_SMALL:
            return "buffer too small";
        case MS_X95_ERR_ALLOCATION:
            return "allocation failed";
        case MS_X95_ERR_CPP_EXCEPTION:
            return "C++ exception";
        default:
            return "unknown error";
    }
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_create(const char* ip, ms_x95_camera_t** out)
{
    if (!out) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera output is null");
    }
    *out = nullptr;
    return guard([&]() {
        *out = new ms_x95_camera(ip);
        return MS_X95_OK;
    });
}

extern "C" MS_X95_API void MS_X95_CALL ms_x95_camera_destroy(ms_x95_camera_t* camera)
{
    delete camera;
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_is_online(ms_x95_camera_t* camera)
{
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    return guard([&]() {
        return camera->impl.isCameraOnline() ? 1 : 0;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_is_online_at(ms_x95_camera_t* camera, const char* ip)
{
    if (!validCamera(camera) || !ip) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or IP is null");
    }
    return guard([&]() {
        return camera->impl.isCameraOnline(std::string(ip)) ? 1 : 0;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_open_stream(ms_x95_camera_t* camera, uint32_t mode)
{
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    return guard([&]() {
        return camera->impl.openCamera(static_cast<int>(mode));
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_close_stream(ms_x95_camera_t* camera)
{
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    return guard([&]() {
        return camera->impl.closeCamera();
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_open_settings(ms_x95_camera_t* camera)
{
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    return guard([&]() {
        return camera->impl.openCameraSettings();
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_close_settings(ms_x95_camera_t* camera)
{
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    return guard([&]() {
        const int result = camera->impl.closeCameraSettings();
        return result == 0 ? MS_X95_OK : result;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_frame_create(ms_x95_frame_t** out)
{
    if (!out) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "frame output is null");
    }
    *out = nullptr;
    return guard([&]() {
        *out = new ms_x95_frame();
        return MS_X95_OK;
    });
}

extern "C" MS_X95_API void MS_X95_CALL ms_x95_frame_destroy(ms_x95_frame_t* frame)
{
    delete frame;
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_frame(ms_x95_camera_t* camera, ms_x95_frame_t* frame, int32_t timeout_ms)
{
    if (!validCamera(camera) || !validFrame(frame)) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or frame is null");
    }
    return guard([&]() {
        return camera->impl.getFrame(frame->impl, timeout_ms) ? MS_X95_OK : MS_X95_NO_DATA;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_frame_get_exposure_phase(const ms_x95_frame_t* frame, int32_t* phase)
{
    if (!validFrame(frame) || !phase) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "frame or output is null");
    }
    *phase = static_cast<int32_t>(frame->impl.exposurePhase());
    return MS_X95_OK;
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_frame_get_plane_info(const ms_x95_frame_t* frame, uint32_t stream, ms_x95_plane_info_t* info)
{
    if (!validFrame(frame) || !info) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "frame or output is null");
    }
    const auto* value = plane(frame, stream);
    if (!value) {
        return fail(MS_X95_ERR_INFO_UNAVAILABLE, "stream is not present");
    }
    fillPlaneInfo(*value, *info);
    return MS_X95_OK;
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_frame_copy_plane(
    const ms_x95_frame_t* frame, uint32_t stream, void* buffer, uint32_t capacity, uint32_t* written)
{
    if (!validFrame(frame)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "frame handle is null");
    }
    const auto* value = plane(frame, stream);
    if (!value || !value->has) {
        return fail(MS_X95_ERR_INFO_UNAVAILABLE, "stream is not present");
    }
    return copyBlob(value->data.data(), static_cast<uint32_t>(value->data.size()), static_cast<uint8_t*>(buffer), capacity, written);
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_frame_get_detections(
    const ms_x95_frame_t* frame, ms_x95_detection_box_t* detections, uint32_t capacity, uint32_t* count, int32_t* coord_width, int32_t* coord_height)
{
    if (!validFrame(frame) || !count || !coord_width || !coord_height) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "frame output is null");
    }

    const auto& values = frame->impl.detections();
    *count = static_cast<uint32_t>(values.size());
    *coord_width = frame->impl.detCoordW();
    *coord_height = frame->impl.detCoordH();
    if (values.empty()) {
        return MS_X95_OK;
    }
    if (!detections || capacity < values.size()) {
        return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "detection buffer is too small");
    }

    for (size_t i = 0; i < values.size(); ++i) {
        detections[i].x1 = values[i].x1;
        detections[i].y1 = values[i].y1;
        detections[i].x2 = values[i].x2;
        detections[i].y2 = values[i].y2;
        detections[i].score = values[i].score;
        detections[i].class_id = values[i].classId;
    }

    return MS_X95_OK;
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_stats(ms_x95_camera_t* camera, ms_x95_stats_t* stats)
{
    if (!validCamera(camera) || !stats) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or output is null");
    }
    return guard([&]() {
        const auto value = camera->impl.getStats();
        stats->empty_pkt = value.emptyPkt;
        stats->abnormal_pkt = value.abnormalPkt;
        stats->incomplete_drop = value.incompleteDrop;
        stats->overwrite = value.overwrite;
        stats->corrupt = value.corrupt;
        return MS_X95_OK;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_drop_degraded_frame(ms_x95_camera_t* camera, int32_t onoff)
{
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    return guard([&]() {
        camera->impl.setDropDegradedFrame(onoff != 0);
        return MS_X95_OK;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_imu(ms_x95_camera_t* camera, ms_x95_imu_t* samples, uint32_t capacity, uint32_t* count)
{
    if (!validCamera(camera) || !count) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or count is null");
    }
    *count = 0;
    if (!samples && capacity != 0) {
        return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "IMU buffer is too small");
    }

    return guard([&]() {
        std::vector<movesense::Imu> values;
        const int limit = capacity > static_cast<uint32_t>(INT_MAX) ? INT_MAX : static_cast<int>(capacity);
        const int result = camera->impl.getIMU(values, limit);
        if (result <= 0) {
            return result;
        }
        if (values.size() > capacity) {
            return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "IMU buffer is too small");
        }

        for (size_t i = 0; i < values.size(); ++i) {
            samples[i].ax = values[i].ax;
            samples[i].ay = values[i].ay;
            samples[i].az = values[i].az;
            samples[i].gx = values[i].gx;
            samples[i].gy = values[i].gy;
            samples[i].gz = values[i].gz;
            samples[i].temp = values[i].temp;
            samples[i].timestamp_us = values[i].timestampUs;
        }
        *count = static_cast<uint32_t>(values.size());
        return result;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_align_time_to_host(ms_x95_camera_t* camera)
{
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    return guard([&]() {
        return camera->impl.alignTimeToHost();
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_trigger_mode(ms_x95_camera_t* camera, int32_t mode)
{
    return validCamera(camera) ? guard([&]() {
        return camera->impl.setTriggerMode(mode);
    })
                               : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_trigger_mode(ms_x95_camera_t* camera, int32_t* mode)
{
    if (!validCamera(camera) || !mode) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or mode is null");
    }
    return guard([&]() {
        int value = 0;
        int r = camera->impl.getTriggerMode(value);
        *mode = value;
        return r;
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_dual_exposure_enabled(ms_x95_camera_t* camera, int32_t enable)
{
    if (!validCamera(camera) || (enable != 0 && enable != 1)) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera is null or enable is outside 0 and 1");
    }
    return guard([&]() {
        return camera->impl.setDualExposureEnabled(enable == 1);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_dual_exposure_enabled(ms_x95_camera_t* camera, int32_t* enable)
{
    if (!validCamera(camera) || enable == nullptr) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or enable is null");
    }
    return guard([&]() {
        bool current = false;
        const int result = camera->impl.getDualExposureEnabled(current);
        if (result > 0) {
            *enable = current ? 1 : 0;
        }
        return result;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_dual_exposure_config(ms_x95_camera_t* camera, const ms_x95_dual_exposure_config_t* config)
{
    if (!validCamera(camera) || config == nullptr) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or config is null");
    }
    if (config->high_reference != MS_X95_DUAL_EXPOSURE_REFERENCE_LOW && config->high_reference != MS_X95_DUAL_EXPOSURE_REFERENCE_HIGH) {
        return fail(MS_X95_ERR_CONFIG_INVALID, "invalid dual exposure reference");
    }
    return guard([&]() {
        movesense::DualExposureConfig requested;
        requested.highReference = (config->high_reference == MS_X95_DUAL_EXPOSURE_REFERENCE_HIGH);
        requested.highExposureUs = config->high_exposure_us;
        requested.highGain = config->high_gain;
        requested.lowExposureUs = config->low_exposure_us;
        requested.lowGain = config->low_gain;
        return camera->impl.setDualExposureConfig(requested);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_dual_exposure_config(ms_x95_camera_t* camera, ms_x95_dual_exposure_config_t* config)
{
    if (!validCamera(camera) || config == nullptr) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or config is null");
    }
    return guard([&]() {
        movesense::DualExposureConfig current;
        const int result = camera->impl.getDualExposureConfig(current);
        if (result > 0) {
            config->high_reference = current.highReference ? MS_X95_DUAL_EXPOSURE_REFERENCE_HIGH : MS_X95_DUAL_EXPOSURE_REFERENCE_LOW;
            config->high_exposure_us = current.highExposureUs;
            config->high_gain = current.highGain;
            config->low_exposure_us = current.lowExposureUs;
            config->low_gain = current.lowGain;
        }
        return result;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_trigger_out(ms_x95_camera_t* camera, int32_t output)
{
    return validCamera(camera) ? guard([&]() {
        return camera->impl.setTriggerOut(output);
    })
                               : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_trigger_out(ms_x95_camera_t* camera, int32_t* output)
{
    if (!validCamera(camera) || !output) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or output is null");
    }
    return guard([&]() {
        int value = 0;
        int r = camera->impl.getTriggerOut(value);
        *output = value;
        return r;
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_trigger_frame(ms_x95_camera_t* camera)
{
    return validCamera(camera) ? guard([&]() {
        return camera->impl.triggerFrame();
    })
                               : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}

#define MS_WRAP_GET_INT(name, method, out_type)                                        \
    extern "C" MS_X95_API int MS_X95_CALL name(ms_x95_camera_t* camera, out_type* out) \
    {                                                                                  \
        if (!validCamera(camera) || !out) {                                            \
            return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or output is null");      \
        }                                                                              \
                                                                                       \
        return guard([&]() {                                                           \
            out_type value = 0;                                                        \
            int r = camera->impl.method(value);                                        \
            *out = value;                                                              \
            return r;                                                                  \
        });                                                                            \
    }
#define MS_WRAP_SET_UINT(name, method)                                                         \
    extern "C" MS_X95_API int MS_X95_CALL name(ms_x95_camera_t* camera, uint32_t value)        \
    {                                                                                          \
        return validCamera(camera) ? guard([&]() {                                             \
            return camera->impl.method(value);                                                 \
        })                                                                                     \
                                   : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null"); \
    }
#define MS_WRAP_GET_UINT(name, method) MS_WRAP_GET_INT(name, method, uint32_t)
#define MS_WRAP_SET_FLOAT(name, method)                                                        \
    extern "C" MS_X95_API int MS_X95_CALL name(ms_x95_camera_t* camera, float value)           \
    {                                                                                          \
        return validCamera(camera) ? guard([&]() {                                             \
            return camera->impl.method(value);                                                 \
        })                                                                                     \
                                   : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null"); \
    }
#define MS_WRAP_GET_FLOAT(name, method) MS_WRAP_GET_INT(name, method, float)

MS_WRAP_GET_INT(ms_x95_camera_get_type, getCameraType, int32_t)
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_depth_unit_mm(ms_x95_camera_t* camera, double* unit_mm)
{
    if (!validCamera(camera) || !unit_mm) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or output is null");
    }
    return guard([&]() {
        return camera->impl.getDepthUnitMm(*unit_mm);
    });
}
MS_WRAP_SET_UINT(ms_x95_camera_set_stereo_exposure, setStereoExposure)
MS_WRAP_GET_UINT(ms_x95_camera_get_stereo_exposure, getStereoExposure)
MS_WRAP_SET_FLOAT(ms_x95_camera_set_stereo_gain, setStereoGain)
MS_WRAP_GET_FLOAT(ms_x95_camera_get_stereo_gain, getStereoGain)
MS_WRAP_SET_UINT(ms_x95_camera_set_stereo_max_exposure, setStereoMaxExposure)
MS_WRAP_GET_UINT(ms_x95_camera_get_stereo_max_exposure, getStereoMaxExposure)
MS_WRAP_SET_UINT(ms_x95_camera_set_stereo_min_exposure, setStereoMinExposure)
MS_WRAP_GET_UINT(ms_x95_camera_get_stereo_min_exposure, getStereoMinExposure)
MS_WRAP_SET_FLOAT(ms_x95_camera_set_stereo_max_gain, setStereoMaxGain)
MS_WRAP_GET_FLOAT(ms_x95_camera_get_stereo_max_gain, getStereoMaxGain)
MS_WRAP_SET_FLOAT(ms_x95_camera_set_stereo_min_gain, setStereoMinGain)
MS_WRAP_GET_FLOAT(ms_x95_camera_get_stereo_min_gain, getStereoMinGain)

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_auto_exposure(ms_x95_camera_t* c, int32_t v)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setStereoAutoExpo(v);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_auto_exposure_brightness(ms_x95_camera_t* c, int32_t v)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setStereoAutoExpoDesiredBin(v);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_auto_exposure(ms_x95_camera_t* c, int32_t v)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setRGBAutoExpo(v);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_auto_exposure_brightness(ms_x95_camera_t* c, int32_t v)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setRGBAutoExpoDesiredBin(v);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
MS_WRAP_SET_UINT(ms_x95_camera_set_rgb_exposure, setRGBExposure)
MS_WRAP_GET_UINT(ms_x95_camera_get_rgb_exposure, getRGBExposure)
MS_WRAP_SET_FLOAT(ms_x95_camera_set_rgb_gain, setRGBGain)
MS_WRAP_GET_FLOAT(ms_x95_camera_get_rgb_gain, getRGBGain)
MS_WRAP_SET_UINT(ms_x95_camera_set_rgb_max_exposure, setRGBMaxExposure)
MS_WRAP_GET_UINT(ms_x95_camera_get_rgb_max_exposure, getRGBMaxExposure)
MS_WRAP_SET_UINT(ms_x95_camera_set_rgb_min_exposure, setRGBMinExposure)
MS_WRAP_GET_UINT(ms_x95_camera_get_rgb_min_exposure, getRGBMinExposure)
MS_WRAP_SET_FLOAT(ms_x95_camera_set_rgb_max_gain, setRGBMaxGain)
MS_WRAP_GET_FLOAT(ms_x95_camera_get_rgb_max_gain, getRGBMaxGain)
MS_WRAP_SET_FLOAT(ms_x95_camera_set_rgb_min_gain, setRGBMinGain)
MS_WRAP_GET_FLOAT(ms_x95_camera_get_rgb_min_gain, getRGBMinGain)
MS_WRAP_SET_UINT(ms_x95_camera_set_doe_power, setDOEPower)
MS_WRAP_GET_UINT(ms_x95_camera_get_doe_power, getDOEPower)

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_roi(
    ms_x95_camera_t* c, uint32_t stream, int32_t enable, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
    if (!validCamera(c)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    movesense::RoiStream value;
    int check = toRoiStream(stream, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        return c->impl.setRoi(value, enable != 0, x1, y1, x2, y2);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_roi(
    ms_x95_camera_t* c, uint32_t stream, int32_t* enable, uint32_t* x1, uint32_t* y1, uint32_t* x2, uint32_t* y2)
{
    if (!validCamera(c) || !enable || !x1 || !y1 || !x2 || !y2) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "ROI output is null");
    }

    movesense::RoiStream value;
    int check = toRoiStream(stream, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        bool e = false;
        unsigned a = 0;
        unsigned b = 0;
        unsigned d = 0;
        unsigned f = 0;
        int r = c->impl.getRoi(value, e, a, b, d, f);

        *enable = e ? 1 : 0;
        *x1 = a;
        *y1 = b;
        *x2 = d;
        *y2 = f;
        return r;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_color_adjust(
    ms_x95_camera_t* c, uint32_t channel, int32_t enable, uint32_t brightness, uint32_t contrast, uint32_t saturation, uint32_t hue)
{
    if (!validCamera(c)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        return value == movesense::IspChannel::Stereo ? c->impl.setStereoColorAdjust(enable != 0, brightness, contrast, saturation, hue)
                                                      : c->impl.setRGBColorAdjust(enable != 0, brightness, contrast, saturation, hue);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_color_adjust(
    ms_x95_camera_t* c, uint32_t channel, int32_t* enable, uint32_t* brightness, uint32_t* contrast, uint32_t* saturation, uint32_t* hue)
{
    if (!validCamera(c) || !enable || !brightness || !contrast || !saturation || !hue) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "color output is null");
    }

    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        bool e = false;
        unsigned b = 0;
        unsigned c2 = 0;
        unsigned s = 0;
        unsigned h = 0;
        int r = value == movesense::IspChannel::Stereo ? c->impl.getStereoColorAdjust(e, b, c2, s, h) : c->impl.getRGBColorAdjust(e, b, c2, s, h);

        *enable = e ? 1 : 0;
        *brightness = b;
        *contrast = c2;
        *saturation = s;
        *hue = h;
        return r;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_sharpen(ms_x95_camera_t* c, uint32_t channel, int32_t enable, uint32_t strength)
{
    if (!validCamera(c)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        return value == movesense::IspChannel::Stereo ? c->impl.setStereoSharpen(enable != 0, strength)
                                                      : c->impl.setRGBSharpen(enable != 0, strength);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_sharpen(
    ms_x95_camera_t* c, uint32_t channel, int32_t* enable, uint32_t* strength, uint32_t* texture_strength, uint32_t* edge_strength)
{
    if (!validCamera(c) || !enable || !strength || !texture_strength || !edge_strength) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "sharpen output is null");
    }

    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        bool e = false;
        unsigned s = 0;
        unsigned t = 0;
        unsigned edge = 0;
        int r = value == movesense::IspChannel::Stereo ? c->impl.getStereoSharpen(e, s, t, edge) : c->impl.getRGBSharpen(e, s, t, edge);

        *enable = e ? 1 : 0;
        *strength = s;
        *texture_strength = t;
        *edge_strength = edge;
        return r;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_auto_white_balance(ms_x95_camera_t* c, uint32_t channel, int32_t auto_mode)
{
    if (!validCamera(c)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        return value == movesense::IspChannel::Stereo ? c->impl.setStereoAutoWb(auto_mode != 0) : c->impl.setRGBAutoWb(auto_mode != 0);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_auto_white_balance(ms_x95_camera_t* c, uint32_t channel, int32_t* auto_mode)
{
    if (!validCamera(c) || !auto_mode) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "white balance output is null");
    }
    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        bool mode = false;
        int r = value == movesense::IspChannel::Stereo ? c->impl.getStereoAutoWb(mode) : c->impl.getRGBAutoWb(mode);
        *auto_mode = mode ? 1 : 0;
        return r;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_white_balance_gain(
    ms_x95_camera_t* c, uint32_t channel, float r_gain, float g_gain, float b_gain)
{
    if (!validCamera(c)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        return value == movesense::IspChannel::Stereo ? c->impl.setStereoWbGain(r_gain, g_gain, b_gain)
                                                      : c->impl.setRGBWbGain(r_gain, g_gain, b_gain);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_white_balance_gain(
    ms_x95_camera_t* c, uint32_t channel, float* r_gain, float* g_gain, float* b_gain)
{
    if (!validCamera(c) || !r_gain || !g_gain || !b_gain) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "white balance output is null");
    }
    movesense::IspChannel value;
    int check = toChannel(channel, value);
    if (check <= 0) {
        return check;
    }
    return guard([&]() {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        int result = value == movesense::IspChannel::Stereo ? c->impl.getStereoWbGain(r, g, b) : c->impl.getRGBWbGain(r, g, b);
        *r_gain = r;
        *g_gain = g;
        *b_gain = b;
        return result;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length)
{
    if (!validCamera(camera) || !data || length == 0 || length > UINT16_MAX) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "invalid calibration buffer or length");
    }

    return guard([&]() {
        std::vector<unsigned char> copy(data, data + length);
        return camera->impl.setStereoCalibData(copy.data(), static_cast<int>(length));
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_calibration_blob(
    ms_x95_camera_t* c, uint8_t* data, uint32_t capacity, uint32_t* written)
{
    if (!validCamera(c) || !written) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration output is null");
    }
    if (capacity < movesense::calib::kStereoCalibBytes || !data) {
        *written = movesense::calib::kStereoCalibBytes;
        return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "calibration buffer is too small");
    }
    return guard([&]() {
        int r = c->impl.getStereoCalibData(data, movesense::calib::kStereoCalibBytes);
        *written = r > 0 ? movesense::calib::kStereoCalibBytes : 0;
        return r;
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length)
{
    if (!validCamera(camera) || !data || length == 0 || length > UINT16_MAX) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "invalid calibration buffer or length");
    }

    return guard([&]() {
        std::vector<unsigned char> copy(data, data + length);
        return camera->impl.setRGBCalibData(copy.data(), static_cast<int>(length));
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_calibration_blob(ms_x95_camera_t* c, uint8_t* data, uint32_t capacity, uint32_t* written)
{
    if (!validCamera(c) || !written) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration output is null");
    }
    if (capacity < movesense::calib::kRgbCalibBytes || !data) {
        *written = movesense::calib::kRgbCalibBytes;
        return fail(MS_X95_ERR_BUFFER_TOO_SMALL, "calibration buffer is too small");
    }
    return guard([&]() {
        int r = c->impl.getRGBCalibData(data, movesense::calib::kRgbCalibBytes);
        *written = r > 0 ? movesense::calib::kRgbCalibBytes : 0;
        return r;
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_calibration(ms_x95_camera_t* c, const ms_x95_stereo_calibration_t* value)
{
    if (!validCamera(c) || !value) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration is null");
    }
    return guard([&]() {
        movesense::calib::StereoCalibration v;
        packStereoCalibration(*value, v);
        return c->impl.setStereoCalibData(v);
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_calibration(ms_x95_camera_t* c, ms_x95_stereo_calibration_t* value)
{
    if (!validCamera(c) || !value) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration is null");
    }
    return guard([&]() {
        movesense::calib::StereoCalibration v;
        int r = c->impl.getStereoCalibData(v);
        if (r > 0) {
            unpackStereoCalibration(v, *value);
        }
        return r;
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_calibration(ms_x95_camera_t* c, const ms_x95_rgb_calibration_t* value)
{
    if (!validCamera(c) || !value) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration is null");
    }
    return guard([&]() {
        movesense::calib::RgbCalibration v;
        packRgbCalibration(*value, v);
        return c->impl.setRGBCalibData(v);
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_calibration(ms_x95_camera_t* c, ms_x95_rgb_calibration_t* value)
{
    if (!validCamera(c) || !value) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration is null");
    }
    return guard([&]() {
        movesense::calib::RgbCalibration v;
        int r = c->impl.getRGBCalibData(v);
        if (r > 0) {
            unpackRgbCalibration(v, *value);
        }
        return r;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_calibration_ex(
    ms_x95_camera_t* camera, const ms_x95_stereo_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics)
{
    if (!validCamera(camera) || !calibration || !intrinsics) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration or IMU intrinsics is null");
    }

    return guard([&]() {
        movesense::calib::StereoCalibration value;
        packStereoCalibration(*calibration, value);
        packImuIntrinsics(*intrinsics, value.imu);
        return camera->impl.setStereoCalibData(value);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_calibration_ex(
    ms_x95_camera_t* camera, ms_x95_stereo_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics)
{
    if (!validCamera(camera) || !calibration || !intrinsics || !has_intrinsics) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration output is null");
    }

    return guard([&]() {
        movesense::calib::StereoCalibration value;
        const int result = camera->impl.getStereoCalibData(value);
        if (result > 0) {
            unpackStereoCalibration(value, *calibration);
            unpackImuIntrinsics(value.imu, *intrinsics, *has_intrinsics);
        }
        return result;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_calibration_ex(
    ms_x95_camera_t* camera, const ms_x95_rgb_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics)
{
    if (!validCamera(camera) || !calibration || !intrinsics) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration or IMU intrinsics is null");
    }

    return guard([&]() {
        movesense::calib::RgbCalibration value;
        packRgbCalibration(*calibration, value);
        packImuIntrinsics(*intrinsics, value.imu);
        return camera->impl.setRGBCalibData(value);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_calibration_ex(
    ms_x95_camera_t* camera, ms_x95_rgb_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics)
{
    if (!validCamera(camera) || !calibration || !intrinsics || !has_intrinsics) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "calibration output is null");
    }

    return guard([&]() {
        movesense::calib::RgbCalibration value;
        const int result = camera->impl.getRGBCalibData(value);
        if (result > 0) {
            unpackRgbCalibration(value, *calibration);
            unpackImuIntrinsics(value.imu, *intrinsics, *has_intrinsics);
        }
        return result;
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_frame_rate(ms_x95_camera_t* c, int32_t value)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setFrameRate(value);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_frame_rate(ms_x95_camera_t* c, int32_t* value)
{
    if (!validCamera(c) || !value) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera or output is null");
    }
    return guard([&]() {
        int v = 0;
        int r = c->impl.getFrameRate(v);
        *value = v;
        return r;
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_downsample_mode(ms_x95_camera_t* c, int32_t value)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setDownsampleMode(value);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_depth_downsample(ms_x95_camera_t* c, int32_t value)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setDepthDownsample(value != 0);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_downsample(ms_x95_camera_t* c, int32_t value)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setRGBDownsample(value != 0);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_downsample(ms_x95_camera_t* c, int32_t value)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setStereoDownsample(value != 0);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_chunk_size(ms_x95_camera_t* c, int32_t value)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setChunkSize(value);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_registration_switch(ms_x95_camera_t* c, int32_t value)
{
    return validCamera(c) ? guard([&]() {
        return c->impl.setRegistrationSwitch(value != 0);
    })
                          : fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_network_config(ms_x95_camera_t* c, const ms_x95_network_config_t* config)
{
    if (!validCamera(c) || !config) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "network config is null");
    }
    return guard([&]() {
        movesense::NetworkConfig v { config->ip, config->netmask, config->gateway };
        return c->impl.SetNetworkConfig(v);
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_network_config(ms_x95_camera_t* c, ms_x95_network_config_t* config)
{
    if (!validCamera(c) || !config) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "network config is null");
    }

    return guard([&]() {
        movesense::NetworkConfig v;
        int r = c->impl.GetNetworkConfig(v);

        if (r > 0) {
            std::memset(config, 0, sizeof(*config));
            int copy = copyFixed(v.ip, config->ip, sizeof(config->ip));
            if (copy <= 0) {
                return copy;
            }
            copy = copyFixed(v.netmask, config->netmask, sizeof(config->netmask));
            if (copy <= 0) {
                return copy;
            }
            copy = copyFixed(v.gateway, config->gateway, sizeof(config->gateway));
            if (copy <= 0) {
                return copy;
            }
        }
        return r;
    });
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_get_mac_bytes(ms_x95_camera_t* c, uint8_t mac[6])
{
    if (!validCamera(c) || !mac) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "MAC output is null");
    }
    return guard([&]() {
        return c->impl.getMacAddress(mac);
    });
}

#define MS_TEXT_GETTER(name, method)                                                                                    \
    extern "C" MS_X95_API int MS_X95_CALL name(ms_x95_camera_t* c, char* buffer, uint32_t capacity, uint32_t* required) \
    {                                                                                                                   \
        if (!validCamera(c)) {                                                                                          \
            return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");                                            \
        }                                                                                                               \
                                                                                                                        \
        return guard([&]() {                                                                                            \
            std::string value;                                                                                          \
            int r = c->impl.method(value);                                                                              \
            if (r <= 0) {                                                                                               \
                return r;                                                                                               \
            }                                                                                                           \
            return copyString(value, buffer, capacity, required);                                                       \
        });                                                                                                             \
    }

MS_TEXT_GETTER(ms_x95_camera_get_mac_string, getMacAddress)
MS_TEXT_GETTER(ms_x95_camera_get_app_version, getAppVersion)
MS_TEXT_GETTER(ms_x95_camera_get_serial_number, getCameraSN)
MS_TEXT_GETTER(ms_x95_camera_get_hardware_version, getHardwareVersion)
MS_TEXT_GETTER(ms_x95_camera_get_product_model, getProductModelVersion)
MS_TEXT_GETTER(ms_x95_camera_get_kernel_version, GetKernelVersion)
MS_TEXT_GETTER(ms_x95_camera_get_library_version, GetCameraLibVersion)

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_serial_number(ms_x95_camera_t* c, const char* value)
{
    if (!validCamera(c) || !value) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "serial number is null");
    }
    return guard([&]() {
        return c->impl.setCameraSN(value);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_scan_single(ms_x95_camera_t* c, const char* ip, ms_x95_camera_info_t* info)
{
    if (!validCamera(c) || !ip || !info) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "scan argument is null");
    }
    return guard([&]() {
        movesense::CameraInfo value;
        int r = c->impl.scanSingleCamera(ip, value);
        if (r == 0) {
            r = copyCameraInfo(value, *info);
        }
        return r;
    });
}

static int makeScanList(std::vector<movesense::CameraInfo>&& values, ms_x95_scan_list_t** out)
{
    std::unique_ptr<ms_x95_scan_list_t> list(new ms_x95_scan_list_t());
    list->cameras = std::move(values);
    *out = list.release();
    return MS_X95_OK;
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_scan_all(ms_x95_camera_t* camera, const char* broadcast_ip, ms_x95_scan_list_t** out)
{
    if (!out) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "scan output is null");
    }
    *out = nullptr;
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera handle is null");
    }

    return guard([&]() {
        return makeScanList(camera->impl.scanAllCameras(broadcast_ip ? broadcast_ip : "255.255.255.255"), out);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_scan_all_interfaces(ms_x95_camera_t* camera, ms_x95_scan_list_t** out)
{
    if (!out) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "scan output is null");
    }
    *out = nullptr;
    if (!validCamera(camera)) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "camera handle is null");
    }

    return guard([&]() {
        return makeScanList(camera->impl.scanAllNetworkInterfaces(), out);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_scan_timeout(ms_x95_camera_t* c, int32_t timeout_ms)
{
    if (!validCamera(c)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    c->impl.setScanTimeout(timeout_ms);
    return MS_X95_OK;
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_camera_set_scan_port(ms_x95_camera_t* c, int32_t port)
{
    if (!validCamera(c)) {
        return fail(MS_X95_ERR_INVALID_HANDLE, "camera handle is null");
    }
    c->impl.setScanPort(port);
    return MS_X95_OK;
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_scan_cameras(const char* broadcast_ip, int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out)
{
    if (!out) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "scan output is null");
    }
    *out = nullptr;

    return guard([&]() {
        return makeScanList(movesense::Simou3Camera::scanCameras(broadcast_ip ? broadcast_ip : "255.255.255.255", port, timeout_ms), out);
    });
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_scan_cameras_all_interfaces(int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out)
{
    if (!out) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "scan output is null");
    }
    *out = nullptr;

    return guard([&]() {
        return makeScanList(movesense::Simou3Camera::scanCamerasAllInterfaces(port, timeout_ms), out);
    });
}

extern "C" MS_X95_API uint32_t MS_X95_CALL ms_x95_scan_list_count(const ms_x95_scan_list_t* list)
{
    return list ? static_cast<uint32_t>(list->cameras.size()) : 0;
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_scan_list_get(const ms_x95_scan_list_t* list, uint32_t index, ms_x95_camera_info_t* info)
{
    if (!list || !info) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "scan list or output is null");
    }
    if (index >= list->cameras.size()) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "scan index is out of range");
    }
    return copyCameraInfo(list->cameras[index], *info);
}
extern "C" MS_X95_API void MS_X95_CALL ms_x95_scan_list_destroy(ms_x95_scan_list_t* list)
{
    delete list;
}

extern "C" MS_X95_API int MS_X95_CALL ms_x95_get_network_interfaces(ms_x95_interface_list_t** out)
{
    if (!out) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "interface output is null");
    }
    *out = nullptr;

    return guard([&]() {
        std::unique_ptr<ms_x95_interface_list_t> list(new ms_x95_interface_list_t());
        list->interfaces = movesense::Simou3Net::enumInterfaces();
        *out = list.release();
        return MS_X95_OK;
    });
}

extern "C" MS_X95_API uint32_t MS_X95_CALL ms_x95_interface_list_count(const ms_x95_interface_list_t* list)
{
    return list ? static_cast<uint32_t>(list->interfaces.size()) : 0;
}
extern "C" MS_X95_API int MS_X95_CALL ms_x95_interface_list_get(const ms_x95_interface_list_t* list, uint32_t index, ms_x95_network_interface_t* info)
{
    if (!list || !info) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "interface list or output is null");
    }
    if (index >= list->interfaces.size()) {
        return fail(MS_X95_ERR_INVALID_ARGUMENT, "interface index is out of range");
    }
    return copyInterface(list->interfaces[index], *info);
}
extern "C" MS_X95_API void MS_X95_CALL ms_x95_interface_list_destroy(ms_x95_interface_list_t* list)
{
    delete list;
}

#undef MS_WRAP_GET_INT
#undef MS_WRAP_SET_UINT
#undef MS_WRAP_GET_UINT
#undef MS_WRAP_SET_FLOAT
#undef MS_WRAP_GET_FLOAT
#undef MS_TEXT_GETTER
