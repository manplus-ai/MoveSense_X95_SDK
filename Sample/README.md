# Sample — 菜单式示例

## 文件结构

| 文件                | 职责                                                                            |
| ------------------- | ------------------------------------------------------------------------------- |
| `ModeCatalog.*`   | 11 个模式的规格表(订阅流/降采样/seg 画哪)+`Mask()` 拼 transferMode + 菜单打印 |
| `CameraSession.*` | 纯扫描选相机 → 按模式开(逐流下发降采样)→ 取帧/取 IMU                          |
| `FrameReceiver.*` | 收帧线程 → 存最新帧;`TryGetLatest` / `Fps()`                               |
| `ImuReceiver.*`   | 收 IMU 线程 → 解析 24B → CSV 录制;`Rate()`                                  |
| `SampleApp.*`     | 编排器:显示、SEG(YDR1)叠框、控制条、按键                                        |
| `main.cpp`        | 入口                                                                            |

## 11 个模式

```
[主动式 A/AP]
  0  lrrgb              双目灰+中目RGB 全分辨率(无深度/IMU/SEG)
  1  rgbd_imu_seg       RGB+深度+IMU+SEG 全分辨率
  2  rgbd_low_imu_seg   RGB+深度 前降(整体640)+IMU+SEG
  3  rgbd_mix_imu_seg   RGB全+深度后降640+IMU+SEG
  4  lrgbd_imu_seg      左目+RGB+深度+IMU+SEG 全分辨率
  5  lrgbd_low_imu_seg  左目+RGB+深度 前降(整体640)+IMU+SEG
  6  lrgbd_mix_imu_seg  左目全+RGB全+深度后降640+IMU+SEG
[被动式 P]
  7  lr                 双目彩色 全分辨率(无深度/IMU/SEG)
  8  lrd_imu_seg        双目+深度+IMU+SEG 全分辨率
  9  lrd_low_imu_seg    双目+深度 前降(整体640)+IMU+SEG
 10  lrd_mix_imu_seg    双目全+深度后降640+IMU+SEG
```

- `low` = 前降(match 跑 640,整体缩)`mix` = 后降(全分辨率算完只缩传输)
- 主动式 seg 画中目 RGB,被动式 seg 画右目。

## 用法

```
Sample            # 交互选模式 + 选相机
Sample 1          # 直接用模式 1(仍扫描选相机)
```

运行时按键:`c` 抓帧 / `r` 录 IMU(imu.csv)/ `q`|ESC 退出。控制条:双目曝光/增益恒有,RGB 仅主动式,FPS。

## 图像格式

相机出 **NV21**(YVU_SEMIPLANAR_420),显示用 `COLOR_YUV2BGR_NV21` 解码。深度 U16(mm)。SEG 为 YDR1 检测结果(magic `0x31524459`)。

## 编译

随 SDK 顶层 CMake 一起编(顶层已 `add_subdirectory(Sample)`):

```
cmake -S . -B build && cmake --build build       # Windows: 产物旁自动拷 dll
```

Linux 需先 `sudo apt install libopencv-dev`。
