// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(MOVESENSE_X95_SDK_BUILD)
#define MS_X95_API __declspec(dllexport)
#else
#define MS_X95_API __declspec(dllimport)
#endif
#define MS_X95_CALL __cdecl
#else
#define MS_X95_API __attribute__((visibility("default")))
#define MS_X95_CALL
#endif

#define MS_X95_OK 1
#define MS_X95_NO_DATA 0
#define MS_X95_ERR_NOT_SUPPORTED (-100)
#define MS_X95_ERR_ROI_INVALID (-101)
#define MS_X95_ERR_ROI_REJECTED (-102)
#define MS_X95_ERR_TRIGGER_MODE_INVALID (-103)
#define MS_X95_ERR_TRIGGER_REJECTED (-104)
#define MS_X95_ERR_CONFIG_INVALID (-105)
#define MS_X95_ERR_STORAGE_FAILED (-106)
#define MS_X95_ERR_INFO_UNAVAILABLE (-107)
#define MS_X95_ERR_CONFIG_BUSY (-108)
#define MS_X95_ERR_CONFIG_APPLY_FAILED (-109)
#define MS_X95_ERR_INVALID_ARGUMENT (-110)
#define MS_X95_ERR_INVALID_HANDLE (-111)
#define MS_X95_ERR_BUFFER_TOO_SMALL (-112)
#define MS_X95_ERR_ALLOCATION (-113)
#define MS_X95_ERR_CPP_EXCEPTION (-114)

#define MS_X95_TEXT_MAX 64

#define MS_X95_TRANSFER_MODE_L_RAW (1u << 0)
#define MS_X95_TRANSFER_MODE_R_RAW (1u << 1)
#define MS_X95_TRANSFER_MODE_RGB_RAW (1u << 2)
#define MS_X95_TRANSFER_MODE_L_RECT (1u << 3)
#define MS_X95_TRANSFER_MODE_R_RECT (1u << 4)
#define MS_X95_TRANSFER_MODE_RGB_RECT (1u << 5)
#define MS_X95_TRANSFER_MODE_DEPTH (1u << 10)
#define MS_X95_TRANSFER_MODE_SEGMENTATION (1u << 20)
#define MS_X95_TRANSFER_MODE_IMU (1u << 30)

#define MS_X95_TRIGGER_MODE_AUTO 0
#define MS_X95_TRIGGER_MODE_SOFT 1
#define MS_X95_TRIGGER_MODE_EXTERNAL 2
#define MS_X95_CHANNEL_STEREO 0
#define MS_X95_CHANNEL_RGB 1
#define MS_X95_DUAL_EXPOSURE_REFERENCE_LOW 0
#define MS_X95_DUAL_EXPOSURE_REFERENCE_HIGH 1

typedef struct ms_x95_camera ms_x95_camera_t;
typedef struct ms_x95_frame ms_x95_frame_t;
typedef struct ms_x95_scan_list ms_x95_scan_list_t;
typedef struct ms_x95_interface_list ms_x95_interface_list_t;

typedef struct ms_x95_dual_exposure_config {
    int32_t high_reference;
    uint32_t high_exposure_us;
    uint32_t high_gain;
    uint32_t low_exposure_us;
    uint32_t low_gain;
} ms_x95_dual_exposure_config_t;

typedef enum ms_x95_stream {
    MS_X95_STREAM_LEFT_RAW = 0,
    MS_X95_STREAM_RIGHT_RAW = 1,
    MS_X95_STREAM_RGB_RAW = 2,
    MS_X95_STREAM_LEFT_RECT = 3,
    MS_X95_STREAM_RIGHT_RECT = 4,
    MS_X95_STREAM_RGB_RECT = 5,
    MS_X95_STREAM_DEPTH = 10,
    MS_X95_STREAM_SEGMENTATION = 20
} ms_x95_stream_t;

typedef enum ms_x95_format { MS_X95_FORMAT_Y8 = 0, MS_X95_FORMAT_NV21 = 1, MS_X95_FORMAT_DEPTH16 = 2 } ms_x95_format_t;

typedef enum ms_x95_exposure_phase {
    MS_X95_EXPOSURE_PHASE_UNKNOWN = 0,
    MS_X95_EXPOSURE_PHASE_LOW = 1,
    MS_X95_EXPOSURE_PHASE_HIGH = 2
} ms_x95_exposure_phase_t;

typedef struct ms_x95_plane_info {
    uint8_t has;
    uint8_t status;
    uint16_t width;
    uint16_t height;
    uint32_t format;
    uint64_t pts;
    uint64_t tsend;
    uint64_t recv_start;
    uint64_t recv_end;
    uint32_t total_len;
    uint32_t recv_bytes;
    uint8_t complete;
    uint8_t degraded;
} ms_x95_plane_info_t;

typedef struct ms_x95_detection_box {
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    int32_t class_id;
} ms_x95_detection_box_t;

typedef struct ms_x95_imu {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    int16_t temp;
    uint64_t timestamp_us;
} ms_x95_imu_t;

typedef struct ms_x95_stats {
    uint64_t empty_pkt;
    uint64_t abnormal_pkt;
    uint64_t incomplete_drop;
    uint64_t overwrite;
    uint64_t corrupt;
} ms_x95_stats_t;

typedef struct ms_x95_network_config {
    char ip[MS_X95_TEXT_MAX];
    char netmask[MS_X95_TEXT_MAX];
    char gateway[MS_X95_TEXT_MAX];
} ms_x95_network_config_t;

typedef struct ms_x95_camera_info {
    char ip[MS_X95_TEXT_MAX];
    int32_t port;
    uint32_t mode;
    uint32_t len;
    char mac[MS_X95_TEXT_MAX];
    char gateway[MS_X95_TEXT_MAX];
    char netmask[MS_X95_TEXT_MAX];
} ms_x95_camera_info_t;

typedef struct ms_x95_network_interface {
    char name[MS_X95_TEXT_MAX];
    char ip[MS_X95_TEXT_MAX];
    char netmask[MS_X95_TEXT_MAX];
    char broadcast[MS_X95_TEXT_MAX];
} ms_x95_network_interface_t;

typedef struct ms_x95_intrinsics {
    float fx;
    float fy;
    float cx;
    float cy;
} ms_x95_intrinsics_t;

typedef struct ms_x95_imu_calibration {
    float gyro_noise_density;
    float gyro_random_walk;
    float accel_noise_density;
    float accel_random_walk;
    float timeshift_ms;
    float rotation[9];
    float translation_mm[3];
} ms_x95_imu_calibration_t;

typedef struct ms_x95_imu_intrinsics {
    float accel_m[9];
    float gyro_m[9];
    float gyro_a[9];
    float gyro_c[9];
} ms_x95_imu_intrinsics_t;

typedef struct ms_x95_stereo_camera_calibration {
    ms_x95_intrinsics_t intrinsics;
    float inverse_rectify[9];
    float distortion[8];
} ms_x95_stereo_camera_calibration_t;

typedef struct ms_x95_stereo_calibration {
    ms_x95_stereo_camera_calibration_t left;
    ms_x95_stereo_camera_calibration_t right;
    ms_x95_intrinsics_t rectified;
    float negative_baseline_mm;
    ms_x95_imu_calibration_t imu;
} ms_x95_stereo_calibration_t;

typedef struct ms_x95_rgb_calibration {
    float reserved[21];
    ms_x95_intrinsics_t intrinsics;
    float inverse_rectify[9];
    float distortion[8];
    ms_x95_intrinsics_t rectified;
    float reserved46;
    float payload_rotation[9];
    float registered_translation[3];
    ms_x95_imu_calibration_t imu;
} ms_x95_rgb_calibration_t;

#ifdef __cplusplus
extern "C" {
#endif

MS_X95_API int MS_X95_CALL ms_x95_get_version(int* major, int* minor, int* patch);
MS_X95_API const char* MS_X95_CALL ms_x95_get_version_string(void);
MS_X95_API const char* MS_X95_CALL ms_x95_get_last_error(void);
MS_X95_API const char* MS_X95_CALL ms_x95_error_message(int code);

MS_X95_API int MS_X95_CALL ms_x95_camera_create(const char* ip, ms_x95_camera_t** out);
MS_X95_API void MS_X95_CALL ms_x95_camera_destroy(ms_x95_camera_t* camera);
MS_X95_API int MS_X95_CALL ms_x95_camera_is_online(ms_x95_camera_t* camera);
MS_X95_API int MS_X95_CALL ms_x95_camera_is_online_at(ms_x95_camera_t* camera, const char* ip);
MS_X95_API int MS_X95_CALL ms_x95_camera_open_stream(ms_x95_camera_t* camera, uint32_t mode);
MS_X95_API int MS_X95_CALL ms_x95_camera_close_stream(ms_x95_camera_t* camera);
MS_X95_API int MS_X95_CALL ms_x95_camera_open_settings(ms_x95_camera_t* camera);
MS_X95_API int MS_X95_CALL ms_x95_camera_close_settings(ms_x95_camera_t* camera);

MS_X95_API int MS_X95_CALL ms_x95_frame_create(ms_x95_frame_t** out);
MS_X95_API void MS_X95_CALL ms_x95_frame_destroy(ms_x95_frame_t* frame);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_frame(ms_x95_camera_t* camera, ms_x95_frame_t* frame, int32_t timeout_ms);
MS_X95_API int MS_X95_CALL ms_x95_frame_get_exposure_phase(const ms_x95_frame_t* frame, int32_t* phase);
MS_X95_API int MS_X95_CALL ms_x95_frame_get_plane_info(const ms_x95_frame_t* frame, uint32_t stream, ms_x95_plane_info_t* info);
MS_X95_API int MS_X95_CALL ms_x95_frame_copy_plane(const ms_x95_frame_t* frame, uint32_t stream, void* buffer, uint32_t capacity, uint32_t* written);
MS_X95_API int MS_X95_CALL ms_x95_frame_get_detections(
    const ms_x95_frame_t* frame, ms_x95_detection_box_t* detections, uint32_t capacity, uint32_t* count, int32_t* coord_width, int32_t* coord_height);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stats(ms_x95_camera_t* camera, ms_x95_stats_t* stats);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_drop_degraded_frame(ms_x95_camera_t* camera, int32_t onoff);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_imu(ms_x95_camera_t* camera, ms_x95_imu_t* samples, uint32_t capacity, uint32_t* count);
MS_X95_API int MS_X95_CALL ms_x95_camera_align_time_to_host(ms_x95_camera_t* camera);

MS_X95_API int MS_X95_CALL ms_x95_camera_set_trigger_mode(ms_x95_camera_t* camera, int32_t mode);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_trigger_mode(ms_x95_camera_t* camera, int32_t* mode);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_dual_exposure_enabled(ms_x95_camera_t* camera, int32_t enable);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_dual_exposure_enabled(ms_x95_camera_t* camera, int32_t* enable);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_dual_exposure_config(ms_x95_camera_t* camera, const ms_x95_dual_exposure_config_t* config);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_dual_exposure_config(ms_x95_camera_t* camera, ms_x95_dual_exposure_config_t* config);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_trigger_out(ms_x95_camera_t* camera, int32_t output);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_trigger_out(ms_x95_camera_t* camera, int32_t* output);
MS_X95_API int MS_X95_CALL ms_x95_camera_trigger_frame(ms_x95_camera_t* camera);

MS_X95_API int MS_X95_CALL ms_x95_camera_get_type(ms_x95_camera_t* camera, int32_t* type);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_depth_unit_mm(ms_x95_camera_t* camera, double* unit_mm);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_exposure(ms_x95_camera_t* camera, uint32_t exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_gain(ms_x95_camera_t* camera, float gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_gain(ms_x95_camera_t* camera, float* gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_max_exposure(ms_x95_camera_t* camera, uint32_t exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_max_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_min_exposure(ms_x95_camera_t* camera, uint32_t exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_min_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_max_gain(ms_x95_camera_t* camera, float gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_max_gain(ms_x95_camera_t* camera, float* gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_min_gain(ms_x95_camera_t* camera, float gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_min_gain(ms_x95_camera_t* camera, float* gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_auto_exposure(ms_x95_camera_t* camera, int32_t onoff);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_auto_exposure_brightness(ms_x95_camera_t* camera, int32_t brightness);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_auto_exposure(ms_x95_camera_t* camera, int32_t onoff);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_auto_exposure_brightness(ms_x95_camera_t* camera, int32_t brightness);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_exposure(ms_x95_camera_t* camera, uint32_t exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_gain(ms_x95_camera_t* camera, float gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_gain(ms_x95_camera_t* camera, float* gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_max_exposure(ms_x95_camera_t* camera, uint32_t exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_max_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_min_exposure(ms_x95_camera_t* camera, uint32_t exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_min_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_max_gain(ms_x95_camera_t* camera, float gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_max_gain(ms_x95_camera_t* camera, float* gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_min_gain(ms_x95_camera_t* camera, float gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_min_gain(ms_x95_camera_t* camera, float* gain);

MS_X95_API int MS_X95_CALL ms_x95_camera_set_doe_power(ms_x95_camera_t* camera, uint32_t power);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_doe_power(ms_x95_camera_t* camera, uint32_t* power);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_roi(
    ms_x95_camera_t* camera, uint32_t stream, int32_t enable, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_roi(
    ms_x95_camera_t* camera, uint32_t stream, int32_t* enable, uint32_t* x1, uint32_t* y1, uint32_t* x2, uint32_t* y2);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_color_adjust(
    ms_x95_camera_t* camera, uint32_t channel, int32_t enable, uint32_t brightness, uint32_t contrast, uint32_t saturation, uint32_t hue);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_color_adjust(
    ms_x95_camera_t* camera, uint32_t channel, int32_t* enable, uint32_t* brightness, uint32_t* contrast, uint32_t* saturation, uint32_t* hue);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_sharpen(ms_x95_camera_t* camera, uint32_t channel, int32_t enable, uint32_t strength);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_sharpen(
    ms_x95_camera_t* camera, uint32_t channel, int32_t* enable, uint32_t* strength, uint32_t* texture_strength, uint32_t* edge_strength);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_auto_white_balance(ms_x95_camera_t* camera, uint32_t channel, int32_t auto_mode);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_auto_white_balance(ms_x95_camera_t* camera, uint32_t channel, int32_t* auto_mode);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_white_balance_gain(ms_x95_camera_t* camera, uint32_t channel, float r_gain, float g_gain, float b_gain);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_white_balance_gain(
    ms_x95_camera_t* camera, uint32_t channel, float* r_gain, float* g_gain, float* b_gain);

MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_calibration_blob(ms_x95_camera_t* camera, uint8_t* data, uint32_t capacity, uint32_t* written);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_calibration_blob(ms_x95_camera_t* camera, uint8_t* data, uint32_t capacity, uint32_t* written);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_calibration(ms_x95_camera_t* camera, const ms_x95_stereo_calibration_t* calibration);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_calibration(ms_x95_camera_t* camera, ms_x95_stereo_calibration_t* calibration);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_calibration(ms_x95_camera_t* camera, const ms_x95_rgb_calibration_t* calibration);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_calibration(ms_x95_camera_t* camera, ms_x95_rgb_calibration_t* calibration);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_calibration_ex(
    ms_x95_camera_t* camera, const ms_x95_stereo_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_stereo_calibration_ex(
    ms_x95_camera_t* camera, ms_x95_stereo_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_calibration_ex(
    ms_x95_camera_t* camera, const ms_x95_rgb_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_rgb_calibration_ex(
    ms_x95_camera_t* camera, ms_x95_rgb_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics);

MS_X95_API int MS_X95_CALL ms_x95_camera_set_frame_rate(ms_x95_camera_t* camera, int32_t fps);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_frame_rate(ms_x95_camera_t* camera, int32_t* fps);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_downsample_mode(ms_x95_camera_t* camera, int32_t mode);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_depth_downsample(ms_x95_camera_t* camera, int32_t onoff);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_rgb_downsample(ms_x95_camera_t* camera, int32_t onoff);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_stereo_downsample(ms_x95_camera_t* camera, int32_t onoff);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_chunk_size(ms_x95_camera_t* camera, int32_t bytes);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_registration_switch(ms_x95_camera_t* camera, int32_t onoff);

MS_X95_API int MS_X95_CALL ms_x95_camera_set_network_config(ms_x95_camera_t* camera, const ms_x95_network_config_t* config);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_network_config(ms_x95_camera_t* camera, ms_x95_network_config_t* config);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_mac_bytes(ms_x95_camera_t* camera, uint8_t mac[6]);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_mac_string(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_app_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_serial_number(ms_x95_camera_t* camera, const char* serial_number);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_serial_number(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_hardware_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_product_model(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_kernel_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required);
MS_X95_API int MS_X95_CALL ms_x95_camera_get_library_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required);

MS_X95_API int MS_X95_CALL ms_x95_camera_scan_single(ms_x95_camera_t* camera, const char* ip, ms_x95_camera_info_t* info);
MS_X95_API int MS_X95_CALL ms_x95_camera_scan_all(ms_x95_camera_t* camera, const char* broadcast_ip, ms_x95_scan_list_t** out);
MS_X95_API int MS_X95_CALL ms_x95_camera_scan_all_interfaces(ms_x95_camera_t* camera, ms_x95_scan_list_t** out);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_scan_timeout(ms_x95_camera_t* camera, int32_t timeout_ms);
MS_X95_API int MS_X95_CALL ms_x95_camera_set_scan_port(ms_x95_camera_t* camera, int32_t port);
MS_X95_API int MS_X95_CALL ms_x95_scan_cameras(const char* broadcast_ip, int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out);
MS_X95_API int MS_X95_CALL ms_x95_scan_cameras_all_interfaces(int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out);
MS_X95_API uint32_t MS_X95_CALL ms_x95_scan_list_count(const ms_x95_scan_list_t* list);
MS_X95_API int MS_X95_CALL ms_x95_scan_list_get(const ms_x95_scan_list_t* list, uint32_t index, ms_x95_camera_info_t* info);
MS_X95_API void MS_X95_CALL ms_x95_scan_list_destroy(ms_x95_scan_list_t* list);
MS_X95_API int MS_X95_CALL ms_x95_get_network_interfaces(ms_x95_interface_list_t** out);
MS_X95_API uint32_t MS_X95_CALL ms_x95_interface_list_count(const ms_x95_interface_list_t* list);
MS_X95_API int MS_X95_CALL ms_x95_interface_list_get(const ms_x95_interface_list_t* list, uint32_t index, ms_x95_network_interface_t* info);
MS_X95_API void MS_X95_CALL ms_x95_interface_list_destroy(ms_x95_interface_list_t* list);

#ifdef __cplusplus
}
#endif
