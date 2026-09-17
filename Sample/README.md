# Sample — 菜单式示例

## 文件结构

| 文件                | 职责                                                                                                                                                                    |
| ------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ModeCatalog.*`   | 13 个模式的规格表(订阅流/降采样/seg 画哪/ROI 落哪一路)+`Mask()` 拼 transferMode + 菜单打印                                                                            |
| `CameraSession.*` | 扫描选相机 → 按模式开(逐流下发降采样、按需下发 ROI、openCamera)→ 开流后按机型打印标定(A/AP 双目+RGB,P 仅双目;首次开流前相机未初始化取不到)→ 取帧/取 IMU;退出时清 ROI |
| `FrameReceiver.*` | 收帧线程 → 存最新帧;`TryGetLatest` / `Fps()`                                                                                                                       |
| `ImuReceiver.*`   | 收 IMU 线程 → SDK 已解析的`Imu` 结构 → CSV 录制;`Rate()`                                                                                                          |
| `SampleApp.*`     | 编排器:显示、SEG(YDR1)叠框(含 ROI 偏移/裁剪)、控制条、按键                                                                                                              |
| `main.cpp`        | 入口                                                                                                                                                                    |

## 13 个模式

```
[主动式 A/AP]
  0  lrrgb              双目灰+中目RGB 全分辨率(无深度/IMU/SEG)
  1  rgbd_imu_seg       RGB+深度+IMU+SEG 全分辨率
  2  rgbd_low_imu_seg   RGB+深度 前降(整体640)+IMU+SEG
  3  rgbd_mix_imu_seg   RGB全+深度后降640+IMU+SEG
  4  lrgbd_imu_seg      左目+RGB+深度+IMU+SEG 全分辨率
  5  lrgbd_low_imu_seg  左目+RGB+深度 前降(整体640)+IMU+SEG
  6  lrgbd_mix_imu_seg  左目全+RGB全+深度后降640+IMU+SEG
  7  lrrgb_seg_roi      双目灰+RGB 全分辨率+SEG, RGB 这一路裁 ROI(无深度/IMU)
[被动式 P]
  8  lr                 双目彩色 全分辨率(无深度/IMU/SEG)
  9  lrd_imu_seg        双目+深度+IMU+SEG 全分辨率
 10  lrd_low_imu_seg    双目+深度 前降(整体640)+IMU+SEG
 11  lrd_mix_imu_seg    双目全+深度后降640+IMU+SEG
 12  lr_seg_roi         双目彩色 全分辨率+SEG, 右目这一路裁 ROI(无深度/IMU)
```

- 主动式 seg 画中目 RGB,被动式 seg 画右目。

## ROI(7 / 12 两个模式)

- 开流前调 `setRoi(RoiStream, enable, x1, y1, x2, y2)`,示例固定裁 `[0,0]-[64,64]`(`CameraSession.cpp` 的 `ROI_ORIGIN_*`/`ROI_SIZE`),坐标 0 基、左上含右下不含。
  主动式裁 `RgbRect`,被动式裁 `RightRect`,左目不裁。任何模式开流前都先把六路 ROI 关掉(enable=false),退出时再关一次,不让上一个客户端留下的 ROI 影响本次。
- 裁后该路 `Plane.width/height` 就是 ROI 尺寸,显示/抓图直接用,不假设任何分辨率;窗口太小时放大显示(最近邻,不影响抓图)。
- SDK 先拦退化(线/点)、小于 16x16、奇数坐标(所有流统一要求偶数,NV21 色度 2x2);越界由相机判定并回显实际生效值,不一致返回 `SIMOU3_ERR_ROI_REJECTED`。
- SEG 框仍是全图坐标:按 `全图尺寸/detCoord` 缩放后减去 ROI 左上角再画,全图尺寸取同帧未裁的左目尺寸;
  完全落在 ROI 外的框不画,部分越界的裁到边界并用橙色标出。

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
