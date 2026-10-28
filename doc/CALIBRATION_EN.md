# Calibration Parameters

The SDK provides interfaces to read and write stereo, RGB, and IMU calibration parameters. This document defines their meanings, units, coordinate relationships, and usage conventions.

[中文](CALIBRATION_CN.md)

## 1. Calibration Interfaces

The following structure-based interfaces belong to `movesense::Simou3Camera`. Parameter types are defined in the `movesense::calib` namespace.

| Interface | Purpose |
| --- | --- |
| `getStereoCalibData(calib::StereoCalibration& calibration)` | Read stereo calibration |
| `setStereoCalibData(const calib::StereoCalibration& calibration)` | Write stereo calibration |
| `getRGBCalibData(calib::RgbCalibration& calibration)` | Read RGB calibration |
| `setRGBCalibData(const calib::RgbCalibration& calibration)` | Write RGB calibration |

These interfaces use an established control connection, and the camera must be open. A return value `>0` indicates success; `<=0` indicates failure. Do not use output parameters from a failed read.

The SDK Sample reads and prints stereo and available RGB calibration after the camera opens and the first frame arrives, including IMU noise, time offset, extrinsics, and intrinsic matrices.

## 2. General Conventions

- Raw camera intrinsics correspond to the 1280×960 calibration image domain.
- Camera coordinates follow x right, y down, and z forward along the optical axis.
- Focal lengths and principal points are in pixels; translations and baseline are in mm; time offsets are in ms.
- Each 3×3 matrix contains 9 elements in row-major order; each translation contains 3 elements.
- Raw images use raw intrinsics and distortion. Rectified images use output intrinsics, with their coordinate frames defined separately.

The four fields of `calib::Intrinsics` are `fx`, `fy`, `cx`, and `cy`:

```text
K = | fx   0   cx |
    |  0  fy   cy |
    |  0   0    1 |
```

## 3. Stereo Parameters

Fields of `calib::StereoCalibration`:

| Field | Meaning |
| --- | --- |
| `left.intrinsics` / `right.intrinsics` | Raw left / right camera intrinsics |
| `left.distortion[8]` / `right.distortion[8]` | Distortion coefficients for the corresponding raw image |
| `left.inverseRectify[9]` / `right.inverseRectify[9]` | Inverse rectification mapping, including inverse intrinsics and inverse rotation |
| `rectified` | Rectified output intrinsics shared by both cameras |
| `negativeBaselineMm` | Negative baseline value; baseline length `B = −negativeBaselineMm` |
| `imu` | IMU calibration with extrinsics referenced to the raw left camera |

The 8 distortion coefficients follow the OpenCV rational model in this order:

```text
k1  k2  p1  p2  k3  k4  k5  k6
```

Let `K_rect` be the output intrinsic matrix and `R_rect` the rotation from the raw camera frame to the rectified frame:

```text
inverseRectify = (K_rect · R_rect)⁻¹
ray_raw = inverseRectify · [u, v, 1]ᵀ
```

`ray_raw` is the viewing direction of the rectified pixel expressed in the raw camera frame. `inverseRectify` is not a pure rotation matrix.

For horizontal stereo, disparity and depth satisfy:

```text
Z = rectified.fx · B / d
```

`d` is horizontal disparity in pixels; `Z` and `B` are in mm.

## 4. RGB Parameters

RGB parameters apply to models equipped with an RGB camera. Fields of `calib::RgbCalibration`:

| Field | Meaning |
| --- | --- |
| `intrinsics` | Raw RGB camera intrinsics |
| `distortion[8]` | Rational distortion coefficients for the raw RGB image, in the same order as stereo |
| `inverseRectify[9]` | RGB inverse rectification mapping |
| `rectified` | RGB output projection intrinsics |
| `registeredTranslation[3]` | Registration translation, in mm |
| `imu` | IMU calibration with extrinsics referenced to the raw RGB camera |
| `reserved` / `reserved46` / `payloadRotation` | Reserved fields; preserve the values read when writing back |

Let `K_out` be the camera matrix represented by `rectified`, `Lr` the rectified left camera frame, and `C` the raw RGB camera frame:

```text
R_C_Lr = inverseRectify · K_out
t_C_Lr = R_C_Lr · registeredTranslation

p_C = R_C_Lr · p_Lr + t_C_Lr
```

`R_C_Lr` rotates from the rectified left camera frame to the raw RGB frame; `t_C_Lr` is in mm. `payloadRotation` is a reserved field. The actual rotation relationship is determined by the parameters above.

## 5. IMU Parameters

The `imu` member of both camera calibration structures is a `calib::ImuCalibration`:

| Field | Units / dimensions | Meaning |
| --- | --- | --- |
| `gyroNoiseDensity` | rad/s/√Hz | Gyroscope white-noise density |
| `gyroRandomWalk` | rad/s²/√Hz | Gyroscope bias random-walk parameter |
| `accelNoiseDensity` | m/s²/√Hz | Accelerometer white-noise density |
| `accelRandomWalk` | m/s³/√Hz | Accelerometer bias random-walk parameter |
| `timeshiftMs` | ms | Camera-to-IMU time offset |
| `rotation[9]` | 3×3 | Rotation from the IMU reference frame to the channel's raw camera frame |
| `translationMm[3]` | mm | Position of the IMU origin in the channel's raw camera frame |
| `accelM[9]` / `gyroM[9]` | Dimensionless, 3×3 | Accelerometer / gyroscope scale-factor and cross-axis error matrices |
| `gyroA[9]` | (rad/s)/(m/s²), 3×3 | Gyroscope acceleration-sensitivity matrix |
| `gyroC[9]` | Dimensionless, 3×3 | Rotation from the IMU reference frame to the gyroscope reference axes |
| `hasIntrinsics` | bool | Whether complete IMU intrinsics are available |

Noise fields contain the original parameter values. The time-offset sign convention is:

```text
t_imu_ms = t_cam_ms + timeshiftMs
```

Both timestamps in this equation are in ms. In stereo calibration, `t_cam_ms` refers to the left camera; in RGB calibration, it refers to the RGB camera.

### 5.1 Camera and IMU Extrinsics

Let `R = rotation` and `t = translationMm`, with point coordinates represented as column vectors:

```text
p_camera = R · p_imu + t
p_imu = Rᵀ · (p_camera − t)
```

The stereo reference is the raw left camera; the RGB reference is the raw RGB camera. Translations and point coordinates use consistent length units. These extrinsics do not include a camera rectification rotation.

### 5.2 IMU Intrinsic Relationships

`accelM` and `gyroM` are lower-triangular matrices. Let `M_a=accelM`, `M_g=gyroM`, `A=gyroA`, and `C=gyroC`. Neglecting additive bias and random noise:

```text
a_measured = M_a · a_imu
w_measured = M_g · C · w_imu + A · C · a_imu

a_imu = M_a⁻¹ · a_measured
w_imu = Cᵀ · M_g⁻¹ · (w_measured − A · C · a_imu)
```

Accelerometer measurements (specific force) in these equations are in m/s², and angular velocity is in rad/s. The intrinsic matrices do not contain additive biases estimated during an individual recording. Corrected quantities remain expressed in the IMU reference frame.

## 6. Usage Conventions

- After a successful read, check `imu.hasIntrinsics`. A value of `false` means complete IMU intrinsics are unavailable.
- When writing a structure, `imu.hasIntrinsics=true` selects camera and complete IMU parameters and requires support for the complete IMU calibration interface. A value of `false` writes camera geometry only; camera firmware supporting this behavior invalidates the IMU calibration for that channel.
- Stereo and RGB IMU extrinsics refer to their respective camera frames and must not be interchanged.
- Reading or writing calibration parameters does not alter the raw sensor data returned by `getIMU`.
