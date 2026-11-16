<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved. -->

# MoveSense X95 SDK C API Reference

This document covers the interfaces declared in the public header `include/movesense/Simou3CameraC.h`. C programs link the same SDK library target, `MoveSense::X95_SDK`.

## Conventions


### Return values

Unless stated otherwise, interfaces returning `int` use these conventions.

| Return value | Meaning |
| --- | --- |
| `> 0` | Success. The interface may return `MS_X95_OK` (`1`), a byte count, or an item count. |
| `<= 0` | The call failed or no data was available. See each interface for its `0` case. |
| `-100` `MS_X95_ERR_NOT_SUPPORTED` | Feature unsupported |
| `-101` `MS_X95_ERR_ROI_INVALID` | Invalid ROI arguments |
| `-102` `MS_X95_ERR_ROI_REJECTED` | ROI rejected by the camera |
| `-103` `MS_X95_ERR_TRIGGER_MODE_INVALID` | Invalid trigger mode |
| `-104` `MS_X95_ERR_TRIGGER_REJECTED` | Trigger request rejected by the camera |
| `-105` `MS_X95_ERR_CONFIG_INVALID` | Invalid configuration argument |
| `-106` `MS_X95_ERR_STORAGE_FAILED` | Camera failed to persist the setting |
| `-107` `MS_X95_ERR_INFO_UNAVAILABLE` | Requested information is unavailable |
| `-108` `MS_X95_ERR_CONFIG_BUSY` | Camera is applying network settings |
| `-109` `MS_X95_ERR_CONFIG_APPLY_FAILED` | Camera failed to apply network settings |
| `-110` `MS_X95_ERR_INVALID_ARGUMENT` | Invalid C argument |
| `-111` `MS_X95_ERR_INVALID_HANDLE` | Required handle is null |
| `-112` `MS_X95_ERR_BUFFER_TOO_SMALL` | Output buffer is too small |
| `-113` `MS_X95_ERR_ALLOCATION` | Memory allocation failed |
| `-114` `MS_X95_ERR_CPP_EXCEPTION` | An SDK exception was caught at the C boundary |

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_error_message(int code)` | Describe a defined status code | `code` status code | Library-owned string |
| `ms_x95_get_last_error(void)` | Read the last detailed adapter error on this thread | None | Library-owned string |

Check the return value using the rules for the specific interface. When a positive value indicates success, use it as described for that interface. It may be a byte or item count. For example, an exposure setter returns `4` after receiving a four-byte reply. `ms_x95_error_message()` recognizes defined status codes. Passing a byte count such as `4` returns `"unknown error"`.

### Timeouts

TCP requests use the existing SDK timeouts. On the data channel, `ms_x95_camera_get_frame` has a separate per-call wait. Its `timeout_ms` argument is described in section 2.

### Call order

- Create a camera handle before opening either channel.
- Open the data channel before reading frames or IMU samples. Open the settings channel before reading or writing camera parameters.
- Reuse the frame handle to receive successive frames. Close each open channel and release acquired handles with their matching destroy function. Wait for calls using a handle to finish before destroying it.
- The following C example opens the stream once and reads 10 depth frames in a loop from a camera in auto trigger mode. Each read waits up to two seconds. See the README for build commands.

```c
#include <movesense/Simou3CameraC.h>
#include <stdio.h>

int main(void)
{
    const int kFrameCount = 10;
    const int kFrameTimeoutMs = 2000;
    ms_x95_camera_t *camera = NULL;
    ms_x95_frame_t *frame = NULL;
    ms_x95_plane_info_t info;
    int captured = 0;

    int result = ms_x95_camera_create("192.168.1.70", &camera);
    if (result <= 0) {
        return 1;
    }
    result = ms_x95_frame_create(&frame);
    if (result <= 0) {
        ms_x95_camera_destroy(camera);
        return 1;
    }

    result = ms_x95_camera_open_stream(camera, MS_X95_TRANSFER_MODE_DEPTH);
    if (result > 0) {
        for (; captured < kFrameCount; ++captured) {
            result = ms_x95_camera_get_frame(camera, frame, kFrameTimeoutMs);
            if (result <= 0) {
                fprintf(stderr, "Failed to receive frame %d.\n", captured + 1);
                break;
            }

            result = ms_x95_frame_get_plane_info(frame, MS_X95_STREAM_DEPTH, &info);
            if (result <= 0 || !info.has) {
                fprintf(stderr, "Frame %d has no depth image.\n", captured + 1);
                break;
            }

            printf("Depth frame %d: %u x %u\n", captured + 1, (unsigned)info.width, (unsigned)info.height);
        }
        ms_x95_camera_close_stream(camera);
    } else {
        fprintf(stderr, "Failed to open the depth stream.\n");
    }
    ms_x95_frame_destroy(frame);
    ms_x95_camera_destroy(camera);
    return captured == kFrameCount ? 0 : 1;
}
```

### Channels

The data channel (TCP 5001) and the settings channel (TCP 5002) can be used independently. ISP calls use `MS_X95_CHANNEL_STEREO` (`0`) for the stereo pair and `MS_X95_CHANNEL_RGB` (`1`) for the color camera. RGB camera settings apply to RGB-equipped models.

## 1. Connection and streaming

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_create(const char* ip, ms_x95_camera_t** out)` | Create a camera handle | `ip` camera address; `out` receives the handle | `>0` on success |
| `ms_x95_camera_destroy(ms_x95_camera_t* camera)` | Destroy a camera handle and release its channels | `camera` acquired handle or `NULL` | None |
| `ms_x95_camera_is_online(ms_x95_camera_t* camera)` | Probe the handle’s camera address | `camera` camera handle | `1` online, `0` offline |
| `ms_x95_camera_is_online_at(ms_x95_camera_t* camera, const char* ip)` | Probe a specified address | `ip` target address | `1` online, `0` offline |
| `ms_x95_camera_open_stream(ms_x95_camera_t* camera, uint32_t mode)` | Open the data channel | `mode` bitwise OR of `MS_X95_TRANSFER_MODE_*` masks | `>0` on success |
| `ms_x95_camera_close_stream(ms_x95_camera_t* camera)` | Stop receiving and close the data channel | `camera` camera handle | `>0` on success |
| `ms_x95_camera_open_settings(ms_x95_camera_t* camera)` / `ms_x95_camera_close_settings(ms_x95_camera_t* camera)` | Open or close the settings channel | `camera` camera handle | `>0` on success |

## 2. Frames and IMU

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_frame_create(ms_x95_frame_t** out)` | Allocate a reusable frame handle | `out` receives the handle | `>0` on success |
| `ms_x95_frame_destroy(ms_x95_frame_t* frame)` | Release a frame handle | `frame` acquired handle or `NULL` | None |
| `ms_x95_camera_get_frame(ms_x95_camera_t* camera, ms_x95_frame_t* frame, int32_t timeout_ms)` | Fetch one frame | `frame` output handle; `timeout_ms` is `0` to poll, positive for a bounded wait in ms, or negative to wait until a frame arrives or the stream closes | `1` frame received, `0` no frame |
| `ms_x95_frame_get_plane_info(const ms_x95_frame_t* frame, uint32_t stream, ms_x95_plane_info_t* info)` | Read stream metadata | `stream` is an `MS_X95_STREAM_*` value; `info` receives dimensions, format, PTS, byte counts and status | `>0` on success |
| `ms_x95_frame_copy_plane(const ms_x95_frame_t* frame, uint32_t stream, void* buffer, uint32_t capacity, uint32_t* written)` | Copy one plane to an application buffer | `stream` plane; `buffer` destination; `capacity` and `written` in bytes | `1` on success or `-112` if the buffer is too small |
| `ms_x95_frame_get_detections(const ms_x95_frame_t* frame, ms_x95_detection_box_t* detections, uint32_t capacity, uint32_t* count, int32_t* coord_width, int32_t* coord_height)` | Copy SEG detections | `detections`, `capacity` in entries, output `count`, `coord_width`, `coord_height` | `>0` on success or `-112` if the buffer is too small |
| `ms_x95_camera_get_stats(ms_x95_camera_t* camera, ms_x95_stats_t* stats)` | Read receive statistics | `stats` output structure | `>0` on success |
| `ms_x95_camera_set_drop_degraded_frame(ms_x95_camera_t* camera, int32_t onoff)` | Set whether incomplete frames are discarded | `onoff` nonzero to discard | `>0` on success |
| `ms_x95_camera_get_imu(ms_x95_camera_t* camera, ms_x95_imu_t* samples, uint32_t capacity, uint32_t* count)` | Read raw IMU samples | `samples` destination array, `capacity` in samples, `count` receives the number copied | `>0` sample count, `0` no samples |
| `ms_x95_camera_align_time_to_host(ms_x95_camera_t* camera)` | Align the camera clock to the host | `camera` camera handle | `>0` on success |
| `ms_x95_camera_get_depth_unit_mm(ms_x95_camera_t* camera, double* unit_mm)` | Read millimeters per depth count through the settings channel | `unit_mm` output in mm/count | `>0` on success |

Check `info.has` before copying. On `-112`, `written` holds the required byte count for a present plane. A failed or timed-out frame read may leave the previous data in the frame handle. Bytes copied to an application buffer are owned by the application.

### 2.1 Imu Structure

Subscribe with `MS_X95_TRANSFER_MODE_IMU` and use `ms_x95_camera_get_imu` to read an array of `ms_x95_imu_t`.

| Field | Type | Meaning |
| --- | --- | --- |
| `ax` / `ay` / `az` | `int16_t` | Signed raw accelerometer counts |
| `gx` / `gy` / `gz` | `int16_t` | Signed raw gyroscope counts |
| `temp` | `int16_t` | Raw temperature count |
| `timestamp_us` | `uint64_t` | Sample timestamp in µs |

### 2.2 Converting Raw Six-Axis Values to SI Units

The raw values use the same conversion as the C++ API. See [C++ API Reference, section 2.2](API_EN.md#22-converting-raw-six-axis-values-to-si-units). Convert `timestamp_us` to seconds by multiplying by `1e-6`.

## 3. Trigger

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_trigger_mode(ms_x95_camera_t* camera, int32_t mode)` / `ms_x95_camera_get_trigger_mode(ms_x95_camera_t* camera, int32_t* mode)` | Set or read trigger mode | `mode`: auto `0`, soft `1`, external `2` | `>0` on success |
| `ms_x95_camera_set_trigger_out(ms_x95_camera_t* camera, int32_t output)` / `ms_x95_camera_get_trigger_out(ms_x95_camera_t* camera, int32_t* output)` | Set or read the effective pulse output | `output`: `0` off, `1` on | `>0` on success |
| `ms_x95_camera_trigger_frame(ms_x95_camera_t* camera)` | Request one soft-triggered frame | `camera` camera handle, already streaming | `>0` on success or `-104` if rejected |

> Set the trigger mode before opening the data channel. In soft trigger mode, request a frame and read it with `ms_x95_camera_get_frame`. Separate successful trigger requests by at least one frame period. External trigger mode keeps pulse output disabled.

## 4. Device information

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_get_type(ms_x95_camera_t* camera, int32_t* type)` | Read the camera type | `type` output type code | `>0` on success |
| `ms_x95_camera_get_app_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | Read LocalApp version | `buffer`, `capacity`, `required` as described below | `>0` on success |
| `ms_x95_camera_get_serial_number(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | Read serial number | `buffer`, `capacity`, `required` as described below | `>0` on success |
| `ms_x95_camera_get_hardware_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | Read hardware version | `buffer`, `capacity`, `required` as described below | `>0` on success |
| `ms_x95_camera_get_product_model(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | Read product model | `buffer`, `capacity`, `required` as described below | `>0` on success |
| `ms_x95_camera_get_kernel_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | Read camera kernel version | `buffer`, `capacity`, `required` as described below | `>0` on success |
| `ms_x95_camera_get_library_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | Read camera library version | `buffer`, `capacity`, `required` as described below | `>0` on success |
| `ms_x95_camera_set_serial_number(ms_x95_camera_t* camera, const char* serial_number)` | Set the serial number | `serial_number` null-terminated string | `>0` on success |

String outputs use a caller-owned `buffer` and `capacity` in bytes. Pass `required` to receive the required capacity including the terminating null. The buffer-too-small code is `-112`.

## 5. Stereo exposure and gain

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_stereo_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_stereo_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | Set or read the manual exposure | `exposure_us` in µs | `>0` on success |
| `ms_x95_camera_set_stereo_max_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_stereo_max_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | Set or read the upper exposure limit | `exposure_us` in µs | `>0` on success |
| `ms_x95_camera_set_stereo_min_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_stereo_min_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | Set or read the lower exposure limit | `exposure_us` in µs | `>0` on success |
| `ms_x95_camera_set_stereo_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_stereo_gain(ms_x95_camera_t* camera, float* gain)` | Set or read the manual gain | `gain` in SDK gain units | `>0` on success |
| `ms_x95_camera_set_stereo_max_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_stereo_max_gain(ms_x95_camera_t* camera, float* gain)` | Set or read the upper gain limit | `gain` in SDK gain units | `>0` on success |
| `ms_x95_camera_set_stereo_min_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_stereo_min_gain(ms_x95_camera_t* camera, float* gain)` | Set or read the lower gain limit | `gain` in SDK gain units | `>0` on success |
| `ms_x95_camera_set_stereo_auto_exposure(ms_x95_camera_t* camera, int32_t onoff)` | Switch auto exposure | `onoff` `0` manual, `1` auto | `>0` on success |
| `ms_x95_camera_set_stereo_auto_exposure_brightness(ms_x95_camera_t* camera, int32_t brightness)` | Set the auto exposure brightness target | `brightness` target value | `>0` on success |

## 6. RGB exposure and gain

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_rgb_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_rgb_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | Set or read the manual exposure | `exposure_us` in µs | `>0` on success |
| `ms_x95_camera_set_rgb_max_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_rgb_max_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | Set or read the upper exposure limit | `exposure_us` in µs | `>0` on success |
| `ms_x95_camera_set_rgb_min_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_rgb_min_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | Set or read the lower exposure limit | `exposure_us` in µs | `>0` on success |
| `ms_x95_camera_set_rgb_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_rgb_gain(ms_x95_camera_t* camera, float* gain)` | Set or read the manual gain | `gain` in SDK gain units | `>0` on success |
| `ms_x95_camera_set_rgb_max_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_rgb_max_gain(ms_x95_camera_t* camera, float* gain)` | Set or read the upper gain limit | `gain` in SDK gain units | `>0` on success |
| `ms_x95_camera_set_rgb_min_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_rgb_min_gain(ms_x95_camera_t* camera, float* gain)` | Set or read the lower gain limit | `gain` in SDK gain units | `>0` on success |
| `ms_x95_camera_set_rgb_auto_exposure(ms_x95_camera_t* camera, int32_t onoff)` | Switch auto exposure | `onoff` `0` manual, `1` auto | `>0` on success |
| `ms_x95_camera_set_rgb_auto_exposure_brightness(ms_x95_camera_t* camera, int32_t brightness)` | Set the auto exposure brightness target | `brightness` target value | `>0` on success |

Gain is in `gain_128` units. A value of `128` represents 1× and `2048` represents 16×.

## 7. Image quality


### 7.1 Color adjustment

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_color_adjust(ms_x95_camera_t* camera, uint32_t channel, int32_t enable, uint32_t brightness, uint32_t contrast, uint32_t saturation, uint32_t hue)` / `ms_x95_camera_get_color_adjust(ms_x95_camera_t* camera, uint32_t channel, int32_t* enable, uint32_t* brightness, uint32_t* contrast, uint32_t* saturation, uint32_t* hue)` | Set or read color adjustment | `channel`, `enable`, `brightness`, `contrast`, `saturation`, `hue` | `>0` on success |

### 7.2 Sharpening

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_sharpen(ms_x95_camera_t* camera, uint32_t channel, int32_t enable, uint32_t strength)` | Set sharpening | `channel`, `enable`, `strength` | `>0` on success |
| `ms_x95_camera_get_sharpen(ms_x95_camera_t* camera, uint32_t channel, int32_t* enable, uint32_t* strength, uint32_t* texture_strength, uint32_t* edge_strength)` | Read sharpening | `channel`; outputs `enable`, `strength`, `texture_strength`, `edge_strength` | `>0` on success |

### 7.3 White balance

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_auto_white_balance(ms_x95_camera_t* camera, uint32_t channel, int32_t auto_mode)` / `ms_x95_camera_get_auto_white_balance(ms_x95_camera_t* camera, uint32_t channel, int32_t* auto_mode)` | Set or read auto white balance | `channel`, `auto_mode` | `>0` on success |
| `ms_x95_camera_set_white_balance_gain(ms_x95_camera_t* camera, uint32_t channel, float r_gain, float g_gain, float b_gain)` / `ms_x95_camera_get_white_balance_gain(ms_x95_camera_t* camera, uint32_t channel, float* r_gain, float* g_gain, float* b_gain)` | Set or read white balance gains | `channel`, `r_gain`, `g_gain`, `b_gain` | `>0` on success |

## 8. Calibration data

Open the settings channel before reading or writing calibration, and wait until calibration data is ready for reads. Stereo calibration refers to the raw left camera. RGB calibration refers to the raw RGB camera. For field definitions, units, and transforms see [Calibration Parameters](CALIBRATION_EN.md).

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_stereo_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length)` / `ms_x95_camera_get_stereo_calibration_blob(ms_x95_camera_t* camera, uint8_t* data, uint32_t capacity, uint32_t* written)` | Write or read the stereo calibration blob | `data` in little-endian payload order; `length` or `capacity` and `written` in bytes. The full base layout is 256 bytes. | `>0` on success |
| `ms_x95_camera_set_stereo_calibration(ms_x95_camera_t* camera, const ms_x95_stereo_calibration_t* calibration)` / `ms_x95_camera_get_stereo_calibration(ms_x95_camera_t* camera, ms_x95_stereo_calibration_t* calibration)` | Write stereo camera geometry or read the basic calibration fields | `calibration` input or output structure | `>0` on success |
| `ms_x95_camera_set_stereo_calibration_ex(ms_x95_camera_t* camera, const ms_x95_stereo_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics)` / `ms_x95_camera_get_stereo_calibration_ex(ms_x95_camera_t* camera, ms_x95_stereo_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics)` | Write or read stereo calibration with IMU intrinsics | `calibration` and `intrinsics`; reads also require `has_intrinsics` | `>0` on success |
| `ms_x95_camera_set_rgb_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length)` / `ms_x95_camera_get_rgb_calibration_blob(ms_x95_camera_t* camera, uint8_t* data, uint32_t capacity, uint32_t* written)` | Write or read the RGB calibration blob | `data` in little-endian payload order; `length` or `capacity` and `written` in bytes. The full base layout is 304 bytes. | `>0` on success |
| `ms_x95_camera_set_rgb_calibration(ms_x95_camera_t* camera, const ms_x95_rgb_calibration_t* calibration)` / `ms_x95_camera_get_rgb_calibration(ms_x95_camera_t* camera, ms_x95_rgb_calibration_t* calibration)` | Write RGB camera geometry or read the basic calibration fields | `calibration` input or output structure | `>0` on success |
| `ms_x95_camera_set_rgb_calibration_ex(ms_x95_camera_t* camera, const ms_x95_rgb_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics)` / `ms_x95_camera_get_rgb_calibration_ex(ms_x95_camera_t* camera, ms_x95_rgb_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics)` | Write or read RGB calibration with IMU intrinsics | `calibration` and `intrinsics`; reads also require `has_intrinsics` | `>0` on success |

Basic structured setters write geometry only and may invalidate the matching IMU calibration on supported firmware. Use the `_ex` interfaces for complete IMU calibration. Their `ms_x95_imu_intrinsics_t` contains `accel_m`, `gyro_m`, `gyro_a` and `gyro_c` as row-major 3×3 matrices. On a read, use the matrices only if `has_intrinsics` is `1`. Support depends on camera firmware.

## 9. Frame rate and downsampling

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_frame_rate(ms_x95_camera_t* camera, int32_t fps)` / `ms_x95_camera_get_frame_rate(ms_x95_camera_t* camera, int32_t* fps)` | Set or read the frame rate | `fps` frames per second | `>0` on success |
| `ms_x95_camera_set_downsample_mode(ms_x95_camera_t* camera, int32_t mode)` | Set the downsampling mode | `mode` `0` pre-downsample or `1` post-downsample | `>0` on success |
| `ms_x95_camera_set_depth_downsample(ms_x95_camera_t* camera, int32_t onoff)` | Toggle depth downsampling | `onoff` `0` off, `1` on | `>0` on success |
| `ms_x95_camera_set_rgb_downsample(ms_x95_camera_t* camera, int32_t onoff)` | Toggle RGB downsampling | `onoff` `0` off, `1` on | `>0` on success |
| `ms_x95_camera_set_stereo_downsample(ms_x95_camera_t* camera, int32_t onoff)` | Toggle stereo downsampling | `onoff` `0` off, `1` on | `>0` on success |
| `ms_x95_camera_set_chunk_size(ms_x95_camera_t* camera, int32_t bytes)` | Set data chunk size | `bytes` bytes per chunk | `>0` on success |
| `ms_x95_camera_set_registration_switch(ms_x95_camera_t* camera, int32_t onoff)` | Toggle registration | `onoff` `0` off, `1` on | `>0` on success |

## 10. ROI

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_roi(ms_x95_camera_t* camera, uint32_t stream, int32_t enable, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)` / `ms_x95_camera_get_roi(ms_x95_camera_t* camera, uint32_t stream, int32_t* enable, uint32_t* x1, uint32_t* y1, uint32_t* x2, uint32_t* y2)` | Set or read the stream ROI | `stream` image stream, `enable` switch, `x1`, `y1`, `x2`, `y2` ROI bounds | `>0` on success |

> ROI applies to the raw and rectified left, right and RGB image streams. SEG coordinates refer to the full image.

## 11. DOE projector

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_doe_power(ms_x95_camera_t* camera, uint32_t power)` / `ms_x95_camera_get_doe_power(ms_x95_camera_t* camera, uint32_t* power)` | Set or read DOE power | `power` DOE duty value | `>0` on success |

> Passive P-type cameras have no DOE projector.

## 12. Network configuration

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_set_network_config(ms_x95_camera_t* camera, const ms_x95_network_config_t* config)` / `ms_x95_camera_get_network_config(ms_x95_camera_t* camera, ms_x95_network_config_t* config)` | Set or read the complete network configuration | `config` with null-terminated `ip`, `netmask`, `gateway` | `>0` on success |
| `ms_x95_camera_get_mac_bytes(ms_x95_camera_t* camera, uint8_t mac[6])` | Read the current MAC address as six bytes | `mac` output array of six bytes | `>0` on success |
| `ms_x95_camera_get_mac_string(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | Read the current MAC address as text | `buffer`, `capacity`, `required` as in section 4 | `>0` on success |

The C interface uses the SDK’s network validation and persistence rules. An address change can close the current connection. Reconnect using the new address and read the configuration to confirm it.

## 13. Camera discovery

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_camera_scan_single(ms_x95_camera_t* camera, const char* ip, ms_x95_camera_info_t* info)` | Probe one camera | `ip` address, `info` output | `1` on success, `<0` on failure |
| `ms_x95_camera_scan_all(ms_x95_camera_t* camera, const char* broadcast_ip, ms_x95_scan_list_t** out)` / `ms_x95_camera_scan_all_interfaces(ms_x95_camera_t* camera, ms_x95_scan_list_t** out)` | Scan one broadcast domain or all local interfaces with an existing camera handle | `broadcast_ip` for the single-domain call; `out` receives the scan list | `>0` on success |
| `ms_x95_camera_set_scan_timeout(ms_x95_camera_t* camera, int32_t timeout_ms)` / `ms_x95_camera_set_scan_port(ms_x95_camera_t* camera, int32_t port)` | Set handle-based discovery timeout or port | `timeout_ms` in ms or `port` | `>0` on success |
| `ms_x95_scan_cameras(const char* broadcast_ip, int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out)` / `ms_x95_scan_cameras_all_interfaces(int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out)` | Scan one broadcast domain or all interfaces directly | `broadcast_ip` for the single-domain call, `port`, `timeout_ms`, `out` | `>0` on success |
| `ms_x95_scan_list_count(const ms_x95_scan_list_t* list)` / `ms_x95_interface_list_count(const ms_x95_interface_list_t* list)` | Read list length | `list` scan or network-interface list | Entry count |
| `ms_x95_scan_list_get(const ms_x95_scan_list_t* list, uint32_t index, ms_x95_camera_info_t* info)` / `ms_x95_interface_list_get(const ms_x95_interface_list_t* list, uint32_t index, ms_x95_network_interface_t* info)` | Copy one list entry | `list`, zero-based `index`, output `info` | `>0` on success |
| `ms_x95_scan_list_destroy(ms_x95_scan_list_t* list)` / `ms_x95_interface_list_destroy(ms_x95_interface_list_t* list)` | Release a list handle | `list` acquired handle or `NULL` | None |
| `ms_x95_get_network_interfaces(ms_x95_interface_list_t** out)` | Enumerate local network interfaces | `out` receives the interface list | `>0` on success |

Release each list returned through `out`, including empty lists. A copied entry is independent of the list.

## 14. SDK version

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `ms_x95_get_version(int* major, int* minor, int* patch)` | Read the linked SDK’s version numbers | `major`, `minor`, `patch` output pointers, all required | `>0` on success |
| `ms_x95_get_version_string(void)` | Read the linked SDK’s version string | None | Library-owned string |
