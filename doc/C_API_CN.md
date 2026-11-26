<!-- SPDX-License-Identifier: Apache-2.0 -->

<!-- Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved. -->

# MoveSense X95 SDK C 语言接口说明

本文说明公开头文件 `include/movesense/Simou3CameraC.h` 声明的接口。C 程序链接同一个 SDK 库目标 `MoveSense::X95_SDK`。

## 通用约定

### 返回值

除单独注明外，返回 `int` 的接口遵循以下约定。

| 返回值                                       | 含义                                                        |
| -------------------------------------------- | ----------------------------------------------------------- |
| `> 0`                                      | 成功。具体接口返回 `MS_X95_OK`（`1`）、字节数或条目数。 |
| `<= 0`                                     | 调用失败或当前无数据。`0` 的含义见对应接口。              |
| `-100` `MS_X95_ERR_NOT_SUPPORTED`        | 功能未支持                                                  |
| `-101` `MS_X95_ERR_ROI_INVALID`          | ROI 参数无效                                                |
| `-102` `MS_X95_ERR_ROI_REJECTED`         | 相机拒绝 ROI                                                |
| `-103` `MS_X95_ERR_TRIGGER_MODE_INVALID` | 触发模式无效                                                |
| `-104` `MS_X95_ERR_TRIGGER_REJECTED`     | 相机拒绝触发请求                                            |
| `-105` `MS_X95_ERR_CONFIG_INVALID`       | 配置参数无效                                                |
| `-106` `MS_X95_ERR_STORAGE_FAILED`       | 相机保存配置失败                                            |
| `-107` `MS_X95_ERR_INFO_UNAVAILABLE`     | 所需信息不可用                                              |
| `-108` `MS_X95_ERR_CONFIG_BUSY`          | 相机正在应用网络配置                                        |
| `-109` `MS_X95_ERR_CONFIG_APPLY_FAILED`  | 相机应用网络配置失败                                        |
| `-110` `MS_X95_ERR_INVALID_ARGUMENT`     | C 接口参数无效                                              |
| `-111` `MS_X95_ERR_INVALID_HANDLE`       | 必要的句柄为空                                              |
| `-112` `MS_X95_ERR_BUFFER_TOO_SMALL`     | 输出缓冲区过小                                              |
| `-113` `MS_X95_ERR_ALLOCATION`           | 内存分配失败                                                |
| `-114` `MS_X95_ERR_CPP_EXCEPTION`        | C 封装捕获到底层 SDK 异常                                   |

| 接口名称                           | 功能                             | 参数含义        | 返回值         |
| ---------------------------------- | -------------------------------- | --------------- | -------------- |
| `ms_x95_error_message(int code)` | 查询已定义的状态码说明           | `code` 状态码 | 库管理的字符串 |
| `ms_x95_get_last_error(void)`    | 查询当前线程最近一次封装错误详情 | 无              | 库管理的字符串 |

先按各接口的返回值约定判断结果。对于以正数表示成功的接口，正数可能是字节数或条目数，应直接按该接口的说明处理。例如曝光设置成功返回 `4`，表示收到 4 字节应答。`ms_x95_error_message()` 只识别已定义的状态码，传入 `4` 这样的字节数会得到 `"unknown error"`。

### 超时

TCP 请求使用 SDK 现有超时机制。数据通道的 `ms_x95_camera_get_frame` 有独立的单次等待时间，`timeout_ms` 参数见第二节。

### 调用顺序

- 以下纯 C 示例从自动触发模式的相机连续获取 10 帧深度图。只开流一次，每帧最多等待 2 秒，编译方式见 README。

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

### 通道概念

数据通道（TCP 5001）和参数通道（TCP 5002）可分别使用。ISP 接口用 `MS_X95_CHANNEL_STEREO`（`0`）表示双目，用 `MS_X95_CHANNEL_RGB`（`1`）表示独立 RGB 相机。RGB 参数适用于配备独立 RGB 相机的机型。

## 一、连接与数据流

| 接口名称                                                                                                             | 功能                   | 参数含义                                              | 返回值                 |
| -------------------------------------------------------------------------------------------------------------------- | ---------------------- | ----------------------------------------------------- | ---------------------- |
| `ms_x95_camera_create(const char* ip, ms_x95_camera_t** out)`                                                      | 创建相机句柄           | `ip` 相机地址，`out` 输出句柄                     | `>0` 成功            |
| `ms_x95_camera_destroy(ms_x95_camera_t* camera)`                                                                   | 销毁相机句柄并释放通道 | `camera` 已创建的句柄或 `NULL`                    | 无                     |
| `ms_x95_camera_is_online(ms_x95_camera_t* camera)`                                                                 | 探测句柄对应的相机地址 | `camera` 相机句柄                                   | `1` 在线，`0` 离线 |
| `ms_x95_camera_is_online_at(ms_x95_camera_t* camera, const char* ip)`                                              | 探测指定地址           | `ip` 目标地址                                       | `1` 在线，`0` 离线 |
| `ms_x95_camera_open_stream(ms_x95_camera_t* camera, uint32_t mode)`                                                | 打开数据通道           | `mode` 将 `MS_X95_TRANSFER_MODE_*` 掩码按位或组合 | `>0` 成功            |
| `ms_x95_camera_close_stream(ms_x95_camera_t* camera)`                                                              | 停止接收并关闭数据通道 | `camera` 相机句柄                                   | `>0` 成功            |
| `ms_x95_camera_open_settings(ms_x95_camera_t* camera)` / `ms_x95_camera_close_settings(ms_x95_camera_t* camera)` | 打开或关闭参数通道     | `camera` 相机句柄                                   | `>0` 成功            |

## 二、取图与 IMU

| 接口名称                                                                                                                                                                          | 功能                     | 参数含义                                                                                          | 返回值                               |
| --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------ | ------------------------------------------------------------------------------------------------- | ------------------------------------ |
| `ms_x95_frame_create(ms_x95_frame_t** out)`                                                                                                                                     | 创建可重复使用的帧句柄   | `out` 输出帧句柄                                                                                | `>0` 成功                          |
| `ms_x95_frame_destroy(ms_x95_frame_t* frame)`                                                                                                                                   | 释放帧句柄               | `frame` 已创建的句柄或 `NULL`                                                                 | 无                                   |
| `ms_x95_camera_get_frame(ms_x95_camera_t* camera, ms_x95_frame_t* frame, int32_t timeout_ms)`                                                                                   | 获取一帧                 | `frame` 输出帧句柄；`timeout_ms` 为 `0` 时立即返回，正值为等待毫秒数，负值等待到有帧或关流  | `1` 取到帧，`0` 无帧             |
| `ms_x95_frame_get_plane_info(const ms_x95_frame_t* frame, uint32_t stream, ms_x95_plane_info_t* info)`                                                                          | 读取图像流信息           | `stream` 为 `MS_X95_STREAM_*`，`info` 输出尺寸、格式、PTS、字节数及状态                     | `>0` 成功                          |
| `ms_x95_frame_copy_plane(const ms_x95_frame_t* frame, uint32_t stream, void* buffer, uint32_t capacity, uint32_t* written)`                                                     | 复制一路图像到应用缓冲区 | `stream` 图像流，`buffer` 目标缓冲区，`capacity` 和 `written` 均以字节计                  | `1` 成功，缓冲区不足返回 `-112`  |
| `ms_x95_frame_get_detections(const ms_x95_frame_t* frame, ms_x95_detection_box_t* detections, uint32_t capacity, uint32_t* count, int32_t* coord_width, int32_t* coord_height)` | 复制 SEG 检测框          | `detections` 输出数组，`capacity` 为条数，另输出 `count`、`coord_width`、`coord_height` | `>0` 成功，缓冲区不足返回 `-112` |
| `ms_x95_camera_get_stats(ms_x95_camera_t* camera, ms_x95_stats_t* stats)`                                                                                                       | 读取接收统计             | `stats` 输出结构体                                                                              | `>0` 成功                          |
| `ms_x95_camera_set_drop_degraded_frame(ms_x95_camera_t* camera, int32_t onoff)`                                                                                                 | 设置是否丢弃不完整帧     | `onoff` 非零时丢弃                                                                              | `>0` 成功                          |
| `ms_x95_camera_get_imu(ms_x95_camera_t* camera, ms_x95_imu_t* samples, uint32_t capacity, uint32_t* count)`                                                                     | 读取原始 IMU 样本        | `samples` 输出数组，`capacity` 为样本容量，`count` 输出读取数量                             | `>0` 为样本数，`0` 为无数据      |
| `ms_x95_camera_align_time_to_host(ms_x95_camera_t* camera)`                                                                                                                     | 与主机对时               | `camera` 相机句柄                                                                               | `>0` 成功                          |
| `ms_x95_camera_get_depth_unit_mm(ms_x95_camera_t* camera, double* unit_mm)`                                                                                                     | 通过参数通道读取深度单位 | `unit_mm` 单位为毫米/计数                                                                       | `>0` 成功                          |

复制前检查 `info.has`。`ms_x95_frame_get_plane_info` 只读取尺寸、格式、PTS 等信息；`ms_x95_frame_copy_plane` 才把指定流的图像字节复制到调用方提供的数组或缓冲区。缓冲区由调用方分配、复用和释放，`capacity` 是容量，成功时 `written` 是实际复制的字节数。图像存在但缓冲区过小时，返回 `-112`，`written` 给出所需字节数。

复制完成后，应用缓冲区里的数据与帧句柄独立；后续复用或销毁帧句柄不会改变这份副本。取帧失败或超时时，帧句柄可能仍保留上一帧，必须先检查取帧返回值，不能把旧数据当作新帧。

### 2.1 Imu 结构体

订阅时包含 `MS_X95_TRANSFER_MODE_IMU`，再调用 `ms_x95_camera_get_imu` 读取 `ms_x95_imu_t` 数组。

| 字段                     | 类型         | 含义                       |
| ------------------------ | ------------ | -------------------------- |
| `ax` / `ay` / `az` | `int16_t`  | 三轴加速度计有符号原始计数 |
| `gx` / `gy` / `gz` | `int16_t`  | 三轴陀螺仪有符号原始计数   |
| `temp`                 | `int16_t`  | 温度原始计数               |
| `timestamp_us`         | `uint64_t` | 样本时间戳，单位微秒       |

### 2.2 六轴原始值换算为 SI 单位

原始值换算与 C++ API 相同，见 [C++ 接口说明第 2.2 节](API_CN.md#22-六轴原始值换算为-si-单位)。`timestamp_us` 乘以 `1e-6` 可换算为秒。

## 三、触发

| 接口名称                                                                                                                                                 | 功能                   | 参数含义                                           | 返回值                         |
| -------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------- | -------------------------------------------------- | ------------------------------ |
| `ms_x95_camera_set_trigger_mode(ms_x95_camera_t* camera, int32_t mode)` / `ms_x95_camera_get_trigger_mode(ms_x95_camera_t* camera, int32_t* mode)`   | 设置或读取触发模式     | `mode` 为自动 `0`、软触发 `1` 或外触发 `2` | `>0` 成功                    |
| `ms_x95_camera_set_trigger_out(ms_x95_camera_t* camera, int32_t output)` / `ms_x95_camera_get_trigger_out(ms_x95_camera_t* camera, int32_t* output)` | 设置或读取有效脉冲输出 | `output` 为 `0` 关闭、`1` 开启               | `>0` 成功                    |
| `ms_x95_camera_trigger_frame(ms_x95_camera_t* camera)`                                                                                                 | 软触发一帧             | `camera` 已开流的相机句柄                        | `>0` 成功，拒绝返回 `-104` |

> 开流前设置触发模式。软触发成功后用 `ms_x95_camera_get_frame` 取图，两次成功触发至少间隔一个帧周期。外触发模式的脉冲输出保持关闭。

## 四、设备信息

| 接口名称                                                                                                             | 功能              | 参数含义                                       | 返回值      |
| -------------------------------------------------------------------------------------------------------------------- | ----------------- | ---------------------------------------------- | ----------- |
| `ms_x95_camera_get_type(ms_x95_camera_t* camera, int32_t* type)`                                                   | 读取相机型号      | `type` 输出型号编号                          | `>0` 成功 |
| `ms_x95_camera_get_app_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)`      | 读取LocalApp 版本 | `buffer`、`capacity`、`required`，见下文 | `>0` 成功 |
| `ms_x95_camera_get_serial_number(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)`    | 读取序列号        | `buffer`、`capacity`、`required`，见下文 | `>0` 成功 |
| `ms_x95_camera_get_hardware_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)` | 读取硬件版本      | `buffer`、`capacity`、`required`，见下文 | `>0` 成功 |
| `ms_x95_camera_get_product_model(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)`    | 读取产品型号      | `buffer`、`capacity`、`required`，见下文 | `>0` 成功 |
| `ms_x95_camera_get_kernel_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)`   | 读取相机内核版本  | `buffer`、`capacity`、`required`，见下文 | `>0` 成功 |
| `ms_x95_camera_get_library_version(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)`  | 读取相机库版本    | `buffer`、`capacity`、`required`，见下文 | `>0` 成功 |
| `ms_x95_camera_set_serial_number(ms_x95_camera_t* camera, const char* serial_number)`                              | 设置序列号        | `serial_number` 以 `\0` 结尾的字符串       | `>0` 成功 |

字符串输出使用调用方提供的 `buffer`，`capacity` 以字节计。传入 `required` 接收所需容量，包含结尾的 `\0`。缓冲区不足时返回 `-112`。

## 五、双目曝光与增益

| 接口名称                                                                                                                                                                             | 功能                 | 参数含义                            | 返回值      |
| ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------- | ----------------------------------- | ----------- |
| `ms_x95_camera_set_stereo_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_stereo_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)`         | 设置或读取手动曝光   | `exposure_us`，微秒               | `>0` 成功 |
| `ms_x95_camera_set_stereo_max_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_stereo_max_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | 设置或读取曝光上限   | `exposure_us`，微秒               | `>0` 成功 |
| `ms_x95_camera_set_stereo_min_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_stereo_min_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | 设置或读取曝光下限   | `exposure_us`，微秒               | `>0` 成功 |
| `ms_x95_camera_set_stereo_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_stereo_gain(ms_x95_camera_t* camera, float* gain)`                                     | 设置或读取手动增益   | `gain`，SDK 增益单位              | `>0` 成功 |
| `ms_x95_camera_set_stereo_max_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_stereo_max_gain(ms_x95_camera_t* camera, float* gain)`                             | 设置或读取增益上限   | `gain`，SDK 增益单位              | `>0` 成功 |
| `ms_x95_camera_set_stereo_min_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_stereo_min_gain(ms_x95_camera_t* camera, float* gain)`                             | 设置或读取增益下限   | `gain`，SDK 增益单位              | `>0` 成功 |
| `ms_x95_camera_set_stereo_auto_exposure(ms_x95_camera_t* camera, int32_t onoff)`                                                                                                   | 切换自动曝光         | `onoff` 为 `0` 手动、`1` 自动 | `>0` 成功 |
| `ms_x95_camera_set_stereo_auto_exposure_brightness(ms_x95_camera_t* camera, int32_t brightness)`                                                                                   | 设置自动曝光目标亮度 | `brightness` 目标亮度             | `>0` 成功 |

### 5.1 高低曝光

| 接口                                                                                                             | 功能             | 参数                                                                                                    | 返回值      |
| ---------------------------------------------------------------------------------------------------------------- | ---------------- | ------------------------------------------------------------------------------------------------------- | ----------- |
| `ms_x95_camera_set_dual_exposure_enabled(ms_x95_camera_t* camera, int32_t enable)`                             | 设置高低曝光开关 | `1` 开启，`0` 关闭                                                                                  | `>0` 成功 |
| `ms_x95_camera_get_dual_exposure_enabled(ms_x95_camera_t* camera, int32_t* enable)`                            | 读取高低曝光开关 | `enable` 输出开关状态                                                                                 | `>0` 成功 |
| `ms_x95_camera_set_dual_exposure_config(ms_x95_camera_t* camera, const ms_x95_dual_exposure_config_t* config)` | 设置高低曝光配置 | `high_reference` 取 `MS_X95_DUAL_EXPOSURE_REFERENCE_HIGH` 或 `MS_X95_DUAL_EXPOSURE_REFERENCE_LOW` | `>0` 成功 |
| `ms_x95_camera_get_dual_exposure_config(ms_x95_camera_t* camera, ms_x95_dual_exposure_config_t* config)`       | 读取高低曝光配置 | 输出参考曝光档和两档曝光、增益                                                                          | `>0` 成功 |

曝光时间以微秒（µs）为单位，范围为 1～65535。增益用 128～2048 的编码值表示，128 对应 1 倍增益。高档的曝光时间和曝光时间乘增益的乘积都须大于低档。两档曝光过于接近时，相机可能无法区分高低曝光帧。

SDK 设置的高低曝光开关和档位在相机本次运行期间有效。相机重启时从配置文件重新读取。

| 接口                                                                             | 说明               | 参数                                                                                                                            | 返回值     |
| -------------------------------------------------------------------------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------- | ---------- |
| `ms_x95_frame_get_exposure_phase(const ms_x95_frame_t* frame, int32_t* phase)` | 读取整帧的曝光档位 | `phase` 输出 `MS_X95_EXPOSURE_PHASE_UNKNOWN`（0）、`MS_X95_EXPOSURE_PHASE_LOW`（1）或 `MS_X95_EXPOSURE_PHASE_HIGH`（2） | `1` 成功 |

首次开流时，如需高低曝光过滤，由调用方显式设置或回读触发模式和高低曝光开关。同一相机句柄重连数据通道时，SDK 在接收第一帧前重新回读相机状态。确认自动触发和高低曝光都已开启后，才过滤档位未知的帧。回读失败时，`ms_x95_camera_open_stream` 返回失败。相机明确返回不支持高低曝光时，按普通模式开流。取图成功后读取档位。

纯 RGB 流没有双目亮暗标签。启用过滤时，`ms_x95_camera_get_frame` 的 `timeout_ms` 为正数就会等到超时并返回 `0`。

## 六、RGB 曝光与增益

| 接口名称                                                                                                                                                                       | 功能                 | 参数含义                            | 返回值      |
| ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------- | ----------------------------------- | ----------- |
| `ms_x95_camera_set_rgb_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_rgb_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)`         | 设置或读取手动曝光   | `exposure_us`，微秒               | `>0` 成功 |
| `ms_x95_camera_set_rgb_max_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_rgb_max_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | 设置或读取曝光上限   | `exposure_us`，微秒               | `>0` 成功 |
| `ms_x95_camera_set_rgb_min_exposure(ms_x95_camera_t* camera, uint32_t exposure_us)` / `ms_x95_camera_get_rgb_min_exposure(ms_x95_camera_t* camera, uint32_t* exposure_us)` | 设置或读取曝光下限   | `exposure_us`，微秒               | `>0` 成功 |
| `ms_x95_camera_set_rgb_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_rgb_gain(ms_x95_camera_t* camera, float* gain)`                                     | 设置或读取手动增益   | `gain`，SDK 增益单位              | `>0` 成功 |
| `ms_x95_camera_set_rgb_max_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_rgb_max_gain(ms_x95_camera_t* camera, float* gain)`                             | 设置或读取增益上限   | `gain`，SDK 增益单位              | `>0` 成功 |
| `ms_x95_camera_set_rgb_min_gain(ms_x95_camera_t* camera, float gain)` / `ms_x95_camera_get_rgb_min_gain(ms_x95_camera_t* camera, float* gain)`                             | 设置或读取增益下限   | `gain`，SDK 增益单位              | `>0` 成功 |
| `ms_x95_camera_set_rgb_auto_exposure(ms_x95_camera_t* camera, int32_t onoff)`                                                                                                | 切换自动曝光         | `onoff` 为 `0` 手动、`1` 自动 | `>0` 成功 |
| `ms_x95_camera_set_rgb_auto_exposure_brightness(ms_x95_camera_t* camera, int32_t brightness)`                                                                                | 设置自动曝光目标亮度 | `brightness` 目标亮度             | `>0` 成功 |

增益使用 `gain_128` 单位，`128` 表示 1 倍，`2048` 表示 16 倍。

## 七、画质调节

### 7.1 色彩调节

| 接口名称                                                                                                                                                                                                                                                                                                                                                 | 功能               | 参数含义                                                      | 返回值      |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------ | ------------------------------------------------------------- | ----------- |
| `ms_x95_camera_set_color_adjust(ms_x95_camera_t* camera, uint32_t channel, int32_t enable, uint32_t brightness, uint32_t contrast, uint32_t saturation, uint32_t hue)` / `ms_x95_camera_get_color_adjust(ms_x95_camera_t* camera, uint32_t channel, int32_t* enable, uint32_t* brightness, uint32_t* contrast, uint32_t* saturation, uint32_t* hue)` | 设置或读取色彩调节 | `channel` 通道、`enable` 开关及亮度、对比度、饱和度、色调 | `>0` 成功 |

### 7.2 锐化

| 接口名称                                                                                                                                                           | 功能     | 参数含义                                               | 返回值      |
| ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------- | ------------------------------------------------------ | ----------- |
| `ms_x95_camera_set_sharpen(ms_x95_camera_t* camera, uint32_t channel, int32_t enable, uint32_t strength)`                                                        | 设置锐化 | `channel` 通道、`enable` 开关、`strength` 强度   | `>0` 成功 |
| `ms_x95_camera_get_sharpen(ms_x95_camera_t* camera, uint32_t channel, int32_t* enable, uint32_t* strength, uint32_t* texture_strength, uint32_t* edge_strength)` | 读取锐化 | `channel` 通道；输出开关、总强度、纹理强度、边缘强度 | `>0` 成功 |

### 7.3 白平衡

| 接口名称                                                                                                                                                                                                                                                         | 功能                 | 参数含义                                     | 返回值      |
| ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------- | -------------------------------------------- | ----------- |
| `ms_x95_camera_set_auto_white_balance(ms_x95_camera_t* camera, uint32_t channel, int32_t auto_mode)` / `ms_x95_camera_get_auto_white_balance(ms_x95_camera_t* camera, uint32_t channel, int32_t* auto_mode)`                                                 | 设置或读取自动白平衡 | `channel` 通道、`auto_mode` 自动模式开关 | `>0` 成功 |
| `ms_x95_camera_set_white_balance_gain(ms_x95_camera_t* camera, uint32_t channel, float r_gain, float g_gain, float b_gain)` / `ms_x95_camera_get_white_balance_gain(ms_x95_camera_t* camera, uint32_t channel, float* r_gain, float* g_gain, float* b_gain)` | 设置或读取白平衡增益 | `channel` 通道及 R、G、B 三路增益          | `>0` 成功 |

## 八、标定数据

读写标定前先打开参数通道，读取前等待标定数据就绪。双目标定以原始左目为参考，RGB 标定以原始 RGB 相机为参考。字段、单位和坐标变换见[标定参数说明](CALIBRATION_CN.md)。

| 接口名称                                                                                                                                                                                                                                                                                                                                       | 功能                               | 参数含义                                                                                           | 返回值      |
| ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------- | -------------------------------------------------------------------------------------------------- | ----------- |
| `ms_x95_camera_set_stereo_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length)` / `ms_x95_camera_get_stereo_calibration_blob(ms_x95_camera_t* camera, uint8_t* data, uint32_t capacity, uint32_t* written)`                                                                                                     | 写入或读取双目标定字节块           | `data` 按小端布局，`length` 或 `capacity` 和 `written` 均以字节计，完整基础布局为 256 字节 | `>0` 成功 |
| `ms_x95_camera_set_stereo_calibration(ms_x95_camera_t* camera, const ms_x95_stereo_calibration_t* calibration)` / `ms_x95_camera_get_stereo_calibration(ms_x95_camera_t* camera, ms_x95_stereo_calibration_t* calibration)`                                                                                                                | 写入双目相机几何或读取基础标定字段 | `calibration` 输入或输出标定结构体                                                               | `>0` 成功 |
| `ms_x95_camera_set_stereo_calibration_ex(ms_x95_camera_t* camera, const ms_x95_stereo_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics)` / `ms_x95_camera_get_stereo_calibration_ex(ms_x95_camera_t* camera, ms_x95_stereo_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics)` | 写入或读取带 IMU 内参的双目标定    | `calibration` 和 `intrinsics`，读取另需 `has_intrinsics` 输出                                | `>0` 成功 |
| `ms_x95_camera_set_rgb_calibration_blob(ms_x95_camera_t* camera, const uint8_t* data, uint32_t length)` / `ms_x95_camera_get_rgb_calibration_blob(ms_x95_camera_t* camera, uint8_t* data, uint32_t capacity, uint32_t* written)`                                                                                                           | 写入或读取RGB 标定字节块           | `data` 按小端布局，`length` 或 `capacity` 和 `written` 均以字节计，完整基础布局为 304 字节 | `>0` 成功 |
| `ms_x95_camera_set_rgb_calibration(ms_x95_camera_t* camera, const ms_x95_rgb_calibration_t* calibration)` / `ms_x95_camera_get_rgb_calibration(ms_x95_camera_t* camera, ms_x95_rgb_calibration_t* calibration)`                                                                                                                            | 写入RGB 相机几何或读取基础标定字段 | `calibration` 输入或输出标定结构体                                                               | `>0` 成功 |
| `ms_x95_camera_set_rgb_calibration_ex(ms_x95_camera_t* camera, const ms_x95_rgb_calibration_t* calibration, const ms_x95_imu_intrinsics_t* intrinsics)` / `ms_x95_camera_get_rgb_calibration_ex(ms_x95_camera_t* camera, ms_x95_rgb_calibration_t* calibration, ms_x95_imu_intrinsics_t* intrinsics, int32_t* has_intrinsics)`             | 写入或读取带 IMU 内参的RGB 标定    | `calibration` 和 `intrinsics`，读取另需 `has_intrinsics` 输出                                | `>0` 成功 |

基础结构体写接口只写相机几何，支持几何单独写入的固件会使对应的 IMU 标定失效。完整 IMU 标定使用 `_ex` 接口，`ms_x95_imu_intrinsics_t` 的 `accel_m`、`gyro_m`、`gyro_a`、`gyro_c` 均为按行存放的 3×3 矩阵。读取后仅在 `has_intrinsics` 为 `1` 时使用矩阵。具体支持情况取决于相机固件。

## 九、帧率与降采样

| 接口名称                                                                                                                                         | 功能               | 参数含义                                   | 返回值      |
| ------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------ | ------------------------------------------ | ----------- |
| `ms_x95_camera_set_frame_rate(ms_x95_camera_t* camera, int32_t fps)` / `ms_x95_camera_get_frame_rate(ms_x95_camera_t* camera, int32_t* fps)` | 设置或读取帧率     | `fps` 每秒帧数                           | `>0` 成功 |
| `ms_x95_camera_set_downsample_mode(ms_x95_camera_t* camera, int32_t mode)`                                                                     | 设置降采样模式     | `mode` 为 `0` 前降采样、`1` 后降采样 | `>0` 成功 |
| `ms_x95_camera_set_depth_downsample(ms_x95_camera_t* camera, int32_t onoff)`                                                                   | 设置深度降采样开关 | `onoff` 为 `0` 关闭、`1` 开启        | `>0` 成功 |
| `ms_x95_camera_set_rgb_downsample(ms_x95_camera_t* camera, int32_t onoff)`                                                                     | 设置RGB 降采样开关 | `onoff` 为 `0` 关闭、`1` 开启        | `>0` 成功 |
| `ms_x95_camera_set_stereo_downsample(ms_x95_camera_t* camera, int32_t onoff)`                                                                  | 设置双目降采样开关 | `onoff` 为 `0` 关闭、`1` 开启        | `>0` 成功 |
| `ms_x95_camera_set_chunk_size(ms_x95_camera_t* camera, int32_t bytes)`                                                                         | 设置数据分块大小   | `bytes` 每个分块的字节数                 | `>0` 成功 |
| `ms_x95_camera_set_registration_switch(ms_x95_camera_t* camera, int32_t onoff)`                                                                | 设置配准开关       | `onoff` 为 `0` 关闭、`1` 开启        | `>0` 成功 |

## 十、ROI

| 接口名称                                                                                                                                                                                                                                                                               | 功能                   | 参数含义                                                                    | 返回值      |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------- | --------------------------------------------------------------------------- | ----------- |
| `ms_x95_camera_set_roi(ms_x95_camera_t* camera, uint32_t stream, int32_t enable, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)` / `ms_x95_camera_get_roi(ms_x95_camera_t* camera, uint32_t stream, int32_t* enable, uint32_t* x1, uint32_t* y1, uint32_t* x2, uint32_t* y2)` | 设置或读取指定流的 ROI | `stream` 图像流、`enable` 开关、`x1`、`y1`、`x2`、`y2` ROI 坐标 | `>0` 成功 |

> ROI 适用于左目、右目和 RGB 的原始流与矫正流。SEG 检测框坐标以完整图像为基准。

## 十一、DOE 投射器

| 接口名称                                                                                                                                             | 功能                | 参数含义               | 返回值      |
| ---------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------- | ---------------------- | ----------- |
| `ms_x95_camera_set_doe_power(ms_x95_camera_t* camera, uint32_t power)` / `ms_x95_camera_get_doe_power(ms_x95_camera_t* camera, uint32_t* power)` | 设置或读取 DOE 功率 | `power` DOE 占空比值 | `>0` 成功 |

> 被动式 P 型机没有 DOE 投射器。

## 十二、网络配置

| 接口名称                                                                                                                                                                                              | 功能                       | 参数含义                                                                   | 返回值      |
| ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------- | -------------------------------------------------------------------------- | ----------- |
| `ms_x95_camera_set_network_config(ms_x95_camera_t* camera, const ms_x95_network_config_t* config)` / `ms_x95_camera_get_network_config(ms_x95_camera_t* camera, ms_x95_network_config_t* config)` | 整组设置或读取网络配置     | `config` 内各字符串以 `\0` 结尾，包含 `ip`、`netmask`、`gateway` | `>0` 成功 |
| `ms_x95_camera_get_mac_bytes(ms_x95_camera_t* camera, uint8_t mac[6])`                                                                                                                              | 读取当前 MAC 地址的 6 字节 | `mac` 输出数组，长度为 6 字节                                            | `>0` 成功 |
| `ms_x95_camera_get_mac_string(ms_x95_camera_t* camera, char* buffer, uint32_t capacity, uint32_t* required)`                                                                                        | 读取当前 MAC 地址字符串    | `buffer`、`capacity`、`required`，见第四节                           | `>0` 成功 |

C 接口沿用 SDK 的网络校验和持久化规则。修改 IP 可能断开当前连接，使用新地址重连后读取配置确认。

## 十三、相机发现

| 接口名称                                                                                                                                                                                                                    | 功能                                       | 参数含义                                                                | 返回值                  |
| --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------ | ----------------------------------------------------------------------- | ----------------------- |
| `ms_x95_camera_scan_single(ms_x95_camera_t* camera, const char* ip, ms_x95_camera_info_t* info)`                                                                                                                          | 探测一台相机                               | `ip` 相机地址，`info` 输出信息                                      | `1` 成功，`<0` 失败 |
| `ms_x95_camera_scan_all(ms_x95_camera_t* camera, const char* broadcast_ip, ms_x95_scan_list_t** out)` / `ms_x95_camera_scan_all_interfaces(ms_x95_camera_t* camera, ms_x95_scan_list_t** out)`                          | 用已有相机句柄扫描指定广播域或全部本机网卡 | 指定广播域时传 `broadcast_ip`，`out` 输出扫描列表                   | `>0` 成功             |
| `ms_x95_camera_set_scan_timeout(ms_x95_camera_t* camera, int32_t timeout_ms)` / `ms_x95_camera_set_scan_port(ms_x95_camera_t* camera, int32_t port)`                                                                    | 设置相机句柄的扫描超时或端口               | `timeout_ms` 毫秒数或 `port` 端口                                   | `>0` 成功             |
| `ms_x95_scan_cameras(const char* broadcast_ip, int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out)` / `ms_x95_scan_cameras_all_interfaces(int32_t port, int32_t timeout_ms, ms_x95_scan_list_t** out)`          | 直接扫描指定广播域或全部本机网卡           | 指定广播域时传 `broadcast_ip`，另传 `port`、`timeout_ms`、`out` | `>0` 成功             |
| `ms_x95_scan_list_count(const ms_x95_scan_list_t* list)` / `ms_x95_interface_list_count(const ms_x95_interface_list_t* list)`                                                                                           | 读取列表长度                               | `list` 扫描列表或网卡列表                                             | 条目数                  |
| `ms_x95_scan_list_get(const ms_x95_scan_list_t* list, uint32_t index, ms_x95_camera_info_t* info)` / `ms_x95_interface_list_get(const ms_x95_interface_list_t* list, uint32_t index, ms_x95_network_interface_t* info)` | 复制一个列表条目                           | `list` 列表，`index` 从零开始，`info` 输出条目                    | `>0` 成功             |
| `ms_x95_scan_list_destroy(ms_x95_scan_list_t* list)` / `ms_x95_interface_list_destroy(ms_x95_interface_list_t* list)`                                                                                                   | 释放列表句柄                               | `list` 已取得的句柄或 `NULL`                                        | 无                      |
| `ms_x95_get_network_interfaces(ms_x95_interface_list_t** out)`                                                                                                                                                            | 枚举本机网卡                               | `out` 输出网卡列表                                                    | `>0` 成功             |

通过 `out` 得到的列表均需释放，包括空列表。已复制的条目可在释放列表后继续使用。

## 十四、SDK 版本

| 接口名称                                                   | 功能                          | 参数含义                                              | 返回值         |
| ---------------------------------------------------------- | ----------------------------- | ----------------------------------------------------- | -------------- |
| `ms_x95_get_version(int* major, int* minor, int* patch)` | 读取当前链接的 SDK 版本号     | `major`、`minor`、`patch`，三个输出指针都需提供 | `>0` 成功    |
| `ms_x95_get_version_string(void)`                        | 读取当前链接的 SDK 版本字符串 | 无                                                    | 库管理的字符串 |
