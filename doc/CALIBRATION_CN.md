# 标定参数说明

SDK 通过标定接口读取或写入双目、RGB 和 IMU 参数。本说明定义参数含义、单位、坐标关系及使用约定。

[English](CALIBRATION_EN.md)

## 1. 标定接口

以下为 `movesense::Simou3Camera` 的结构体接口，参数类型位于 `movesense::calib` 命名空间。

| 接口 | 作用 |
| --- | --- |
| `getStereoCalibData(calib::StereoCalibration& calibration)` | 读取双目标定参数 |
| `setStereoCalibData(const calib::StereoCalibration& calibration)` | 写入双目标定参数 |
| `getRGBCalibData(calib::RgbCalibration& calibration)` | 读取 RGB 标定参数 |
| `setRGBCalibData(const calib::RgbCalibration& calibration)` | 写入 RGB 标定参数 |

接口使用已建立的控制连接，相机须处于已打开状态。返回值 `>0` 表示成功，`<=0` 表示失败；读取失败时不使用输出参数。

SDK 的 Sample 会在相机开启并收到首帧后读取、打印双目及可用的 RGB 标定参数，包括 IMU 噪声、时偏、外参和内参矩阵。

## 2. 通用约定

- 原始相机内参对应 1280×960 标定图像域。
- 相机坐标系：x 向右、y 向下、z 沿光轴向前。
- 焦距和主点单位为像素；平移和基线单位为 mm；时间偏移单位为 ms。
- 所有 3×3 矩阵均由 9 个元素按行排列；平移由 3 个元素组成。
- 原始图像使用原始内参和畸变；矫正图像使用输出内参，两者的坐标系分别表示。

`calib::Intrinsics` 的四个字段为 `fx`、`fy`、`cx`、`cy`，对应：

```text
K = | fx   0   cx |
    |  0  fy   cy |
    |  0   0    1 |
```

## 3. 双目参数

`calib::StereoCalibration` 中：

| 字段 | 含义 |
| --- | --- |
| `left.intrinsics` / `right.intrinsics` | 原始左目 / 右目内参 |
| `left.distortion[8]` / `right.distortion[8]` | 对应原始图像的畸变系数 |
| `left.inverseRectify[9]` / `right.inverseRectify[9]` | 对应相机的矫正逆映射，含内参逆和旋转逆 |
| `rectified` | 左右目共用的矫正输出内参 |
| `negativeBaselineMm` | 负基线值；基线长度 `B = −negativeBaselineMm` |
| `imu` | IMU 标定参数，外参对应原始左目 |

8 个畸变系数采用 OpenCV rational 模型，顺序为：

```text
k1  k2  p1  p2  k3  k4  k5  k6
```

记输出内参为 `K_rect`，原始相机系到矫正相机系的旋转为 `R_rect`，则：

```text
inverseRectify = (K_rect · R_rect)⁻¹
ray_raw = inverseRectify · [u, v, 1]ᵀ
```

`ray_raw` 是矫正像素对应的原始相机系视线方向。`inverseRectify` 不能直接当作纯旋转矩阵。

水平双目的视差与深度关系为：

```text
Z = rectified.fx · B / d
```

`d` 为水平视差，单位像素；`Z` 和 `B` 单位均为 mm。

## 4. RGB 参数

RGB 参数适用于具备 RGB 镜头的机型。`calib::RgbCalibration` 中：

| 字段 | 含义 |
| --- | --- |
| `intrinsics` | 原始 RGB 内参 |
| `distortion[8]` | 原始 RGB 的 rational 畸变系数，排列与双目相同 |
| `inverseRectify[9]` | RGB 矫正逆映射 |
| `rectified` | RGB 输出投影内参 |
| `registeredTranslation[3]` | 配准平移，单位 mm |
| `imu` | IMU 标定参数，外参对应原始 RGB 相机 |
| `reserved` / `reserved46` / `payloadRotation` | 保留字段，回写时保留读取值 |

记 `K_out` 为 `rectified` 对应的相机矩阵，`Lr` 为左目矫正系，`C` 为原始 RGB 系：

```text
R_C_Lr = inverseRectify · K_out
t_C_Lr = R_C_Lr · registeredTranslation

p_C = R_C_Lr · p_Lr + t_C_Lr
```

`R_C_Lr` 为左目矫正系到原始 RGB 系的旋转，`t_C_Lr` 单位为 mm。`payloadRotation` 是保留字段，实际旋转关系由上述参数确定。

## 5. IMU 参数

两类相机标定结构中的 `imu` 均为 `calib::ImuCalibration`：

| 字段 | 单位 / 形状 | 含义 |
| --- | --- | --- |
| `gyroNoiseDensity` | rad/s/√Hz | 陀螺白噪声密度 |
| `gyroRandomWalk` | rad/s²/√Hz | 陀螺零偏随机游走参数 |
| `accelNoiseDensity` | m/s²/√Hz | 加速度计白噪声密度 |
| `accelRandomWalk` | m/s³/√Hz | 加速度计零偏随机游走参数 |
| `timeshiftMs` | ms | 相机与 IMU 的时间偏移 |
| `rotation[9]` | 3×3 | IMU 参考系到本通道原始相机系的旋转 |
| `translationMm[3]` | mm | IMU 原点在本通道原始相机系下的位置 |
| `accelM[9]` / `gyroM[9]` | 无量纲，3×3 | 加速度计 / 陀螺的标度因子与轴间误差矩阵 |
| `gyroA[9]` | (rad/s)/(m/s²)，3×3 | 陀螺加速度敏感项 |
| `gyroC[9]` | 无量纲，3×3 | IMU 参考系到陀螺参考轴系的旋转 |
| `hasIntrinsics` | bool | 是否具备完整 IMU 内参 |

噪声字段为原始参数值。时间偏移的符号定义为：

```text
t_imu_ms = t_cam_ms + timeshiftMs
```

此处两个时间戳均以 ms 表示。双目标定中的 `t_cam_ms` 对应左目，RGB 标定中对应 RGB 相机。

### 5.1 相机与 IMU 的外参

令 `R = rotation`、`t = translationMm`，点坐标均按列向量表示：

```text
p_camera = R · p_imu + t
p_imu = Rᵀ · (p_camera − t)
```

双目的相机参考是原始左目，RGB 的相机参考是原始 RGB。平移和点坐标使用一致的长度单位；这组外参不包含相机矫正旋转。

### 5.2 IMU 内参的数学关系

`accelM`、`gyroM` 为下三角矩阵。记 `M_a=accelM`、`M_g=gyroM`、`A=gyroA`、`C=gyroC`，忽略加性零偏和随机噪声时：

```text
a_measured = M_a · a_imu
w_measured = M_g · C · w_imu + A · C · a_imu

a_imu = M_a⁻¹ · a_measured
w_imu = Cᵀ · M_g⁻¹ · (w_measured − A · C · a_imu)
```

公式中的加速度计量测（比力）单位为 m/s²，角速度单位为 rad/s。内参矩阵不包含一次录制中估计出的加性零偏；修正后的量仍表达在 IMU 参考系中。

## 6. 使用约定

- 读取成功后检查 `imu.hasIntrinsics`；为 `false` 时，完整的 IMU 内参不可用。
- 写入结构体时，`imu.hasIntrinsics=true` 表示写入相机与完整 IMU 参数，要求相机支持完整 IMU 标定接口；为 `false` 时仅写相机几何参数，支持该行为的相机固件会使对应通道的 IMU 标定失效。
- 双目与 RGB 的 IMU 参数分别对应各自相机参考系，不能互换外参。
- 标定参数的读取与写入不改变 `getIMU` 返回的原始传感器数据。
