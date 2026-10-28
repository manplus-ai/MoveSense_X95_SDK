# MoveSense X95 SDK 接口说明

本文档覆盖公开头 `include/movesense/Simou3Camera.h` 的接口。

## 通用约定

### 返回值

除特别说明外,所有返回 `int` 的接口遵循同一约定:

| 返回值                                       | 含义                                       |
| -------------------------------------------- | ------------------------------------------ |
| `> 0`                                      | 成功(数值为该次收发的字节数,无其他含义)    |
| `<= 0`                                     | 失败(0 或负数表示连接断开、超时、对端拒绝) |
| `-100` `SIMOU3_ERR_NOT_SUPPORTED`        | 该接口暂不支持                             |
| `-101` `SIMOU3_ERR_ROI_INVALID`          | ROI 参数非法(SDK 本地校验未通过)           |
| `-102` `SIMOU3_ERR_ROI_REJECTED`         | ROI 被相机拒绝(回显值与请求不符)           |
| `-103` `SIMOU3_ERR_TRIGGER_MODE_INVALID` | 触发模式取值非法(SDK 本地校验未通过)       |
| `-104` `SIMOU3_ERR_TRIGGER_REJECTED`     | 触发请求被相机拒绝                         |
| `-105` `SIMOU3_ERR_CONFIG_INVALID`       | 参数非法(网络配置、SN、触发输出取值等)     |
| `-106` `SIMOU3_ERR_STORAGE_FAILED`       | 相机持久保存失败                           |
| `-107` `SIMOU3_ERR_INFO_UNAVAILABLE`     | 相机上无此项信息                           |
| `-108` `SIMOU3_ERR_CONFIG_BUSY`          | 相机正在改网,稍后重试                      |
| `-109` `SIMOU3_ERR_CONFIG_APPLY_FAILED`  | 网络配置应用失败                           |

判定成功请用 `ret > 0`,不要用 `ret == 0`。

### 超时

TCP 连接、发送、接收统一设 5 秒超时。控制通道(5002)上的请求-响应超时即返回失败(`<= 0`)。数据通道(5001)上 5 秒收不到数据只记一条日志并继续等待,不会断开连接;只有调用 `closeCamera` 或相机主动断开才会结束接收。

### 调用顺序

- 取图相关接口(`openCamera` / `getFrame` / `getIMU`)走数据通道,需先 `openCamera`
- 参数设置类接口走控制通道,需先 `openCameraSettings`
- 两个通道相互独立,可单独使用
- 读取标定参数时，相机需已开启且数据就绪

### 通道概念

`Stereo` 指双目，左右目共用一套参数；`RGB` 指彩色相机。
**被动式 P 型机**没有 RGB 相机:`setRGB*` 不会作用到硬件,`getRGB*` 的返回值没有实际意义。

## 一、连接与数据流

| 接口名称                           | 功能                       | 参数含义                                                                                                                                                                  | 返回值        |
| ---------------------------------- | -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------- |
| `Simou3Camera(std::string ip)`   | 构造相机对象               | `ip` 相机 IP,默认 `192.168.1.70`                                                                                                                                      | —            |
| `isCameraOnline()`               | 探测相机是否在线           | 无                                                                                                                                                                        | `true` 在线 |
| `isCameraOnline(std::string ip)` | 探测指定 IP 的相机是否在线 | `ip` 目标 IP                                                                                                                                                            | `true` 在线 |
| `openCamera(int mode)`           | 打开数据流并开始接收       | `mode` 订阅掩码,由 `transfer_mode_def.h` 的掩码宏按位或组成,如 `TRANSFER_MODE_L_RECTIFIED \| TRANSFER_MODE_DEPTH`(等价于 `(1u << TRANSFER_MODE_*_BIT)`,位宏仍保留) | `>0` 成功   |
| `closeCamera()`                  | 关闭数据流                 | 无                                                                                                                                                                        | `>0` 成功   |
| `openCameraSettings()`           | 打开控制通道(端口 5002)    | 无                                                                                                                                                                        | `>0` 成功   |
| `closeCameraSettings()`          | 关闭控制通道               | 无                                                                                                                                                                        | `>0` 成功   |

## 二、取图与 IMU

| 接口名称                                         | 功能                                          | 参数含义                                                                              | 返回值               |
| ------------------------------------------------ | --------------------------------------------- | ------------------------------------------------------------------------------------- | -------------------- |
| `getFrame(MovesenseFrame& out)`                | 取一帧(非阻塞,当前没有新帧立即返回 `false`) | `out` 输出帧                                                                        | `true` 取到        |
| `getFrame(MovesenseFrame& out, int timeoutMs)` | 取一帧(指定超时)                              | `out` 输出帧;`timeoutMs` 超时毫秒,`0` 不等待,`<0` 一直等到有帧或关闭          | `true` 取到        |
| `getStats()`                                   | 取收流统计                                    | 无                                                                                    | `Simou3Stats` 结构 |
| `setDropDegradedFrame(bool onoff)`             | 设置是否丢弃不完整帧                          | `onoff` `true` 丢弃残帧                                                           | 无                   |
| `getIMU(std::vector<Imu>& out, int num)`       | 取 IMU 数据                                   | `out` 输出数组;`num` 最多取几条,默认 200;SDK 最多缓存最近 200 条,取得慢会丢最旧的 | `>0` 实际条数      |
| `alignTimeToHost()`                            | 将相机时间对齐到主机                          | 无                                                                                    | `>0` 成功          |

### 2.1 Imu 结构体

`movesense::Imu` 定义于 `include/movesense/Simou3Types.h`。订阅模式须包含 `TRANSFER_MODE_IMU`，再通过 `getIMU` 获取样本。

| 字段                     | 类型         | 含义                                |
| ------------------------ | ------------ | ----------------------------------- |
| `ax` / `ay` / `az` | `int16_t`  | 加速度计 x / y / z 轴有符号原始计数 |
| `gx` / `gy` / `gz` | `int16_t`  | 陀螺仪 x / y / z 轴有符号原始计数   |
| `temp`                 | `int16_t`  | 温度原始计数                        |
| `timestampUs`          | `uint64_t` | IMU 样本时间戳，单位 µs            |

`getIMU` 向 `out` 追加本次取到的样本；重复使用同一个数组时，应先清空旧样本。返回 `0` 表示当前没有可取的样本，返回负数表示未开流或读取失败。

### 2.2 六轴原始值换算为 SI 单位

当前 X95 的 IMU 配置为加速度计 ±16 g、陀螺仪 ±2000 °/s，对应灵敏度分别为 2048 LSB/g 和 16.4 LSB/(°/s)。以 `g0 = 9.80665 m/s²` 作为标准重力加速度，换算公式为：

```text
a_measured = (9.80665 / 2048.0) · [ax, ay, az]ᵀ
w_measured = (π / (180.0 · 16.4)) · [gx, gy, gz]ᵀ
timestamp_s = 1e-6 · timestampUs
```

`a_measured` 单位为 m/s²，`w_measured` 单位为 rad/s，`timestamp_s` 单位为 s。若 IMU 量程改变，应使用对应量程的灵敏度。

换算后仍为未经过内参矫正的量测；加速度计量测为比力，静止时模长通常接近重力加速度，尚未去除重力。使用标定参数矫正这些量测，详见[标定参数说明](CALIBRATION_CN.md)第 5.2 节。SDK 不会自动将标定参数应用到 `getIMU` 返回的六轴原始值。

## 三、触发

| 接口名称                      | 功能                     | 参数含义                                                                                                                                                                                                                                   | 返回值                                                |
| ----------------------------- | ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------- |
| `setTriggerMode(int mode)`  | 设触发模式               | `mode` `kTriggerModeAuto`(`0`)自动触发:相机按 `setFrameRate` 的帧率连续出图;`kTriggerModeSoft`(`1`)软触发:每调一次 `triggerFrame()` 出一帧;`kTriggerModeExternal`(`2`)外触发:本机不出触发信号,由主相机的触发信号驱动出图 | `>0` 成功 / `-103` 取值非法 / `-104` 被相机拒绝 |
| `getTriggerMode(int& mode)` | 读当前触发模式           | `mode` 输出,取值同上                                                                                                                                                                                                                     | `>0` 成功                                           |
| `setTriggerOut(int out)`    | 设置触发脉冲输出         | `out` `0`关闭输出,`1`开启输出;外触发模式下请求仍会被接受,但有效值始终为 `0`                                                                                                                                                        | `>0` 成功 / `-105` 取值非法 / `-104` 被相机拒绝 |
| `getTriggerOut(int& out)`   | 读取有效触发脉冲输出状态 | `out` 输出,取值为 `0` 或 `1`;外触发模式始终返回 `0`                                                                                                                                                                                | `>0` 成功                                           |
| `triggerFrame()`            | 软触发出一帧             | 无                                                                                                                                                                                                                                         | `>0` 成功 / `-104` 被相机拒绝                     |

> 触发模式只在相机本次运行期间有效,相机重启后恢复为自动触发;断开重连**不会**重置,所以每次连接请在 `openCamera` 之前显式设置一次。
> 切换触发模式时,相机会丢弃尚未发出的帧。
>
> 软触发下相机不会主动出图:每次 `triggerFrame()` 成功后,用 `getFrame` 取到对应的那一帧(`frameCnt()` 加 1)。
> 两次 `triggerFrame()` 的间隔不得小于一帧时间,即 `1000 / 帧率` 毫秒(25 fps 时为 40 ms),所以软触发下 `setFrameRate` 设定的就是允许的最高触发频率。
>
> `triggerFrame()` 返回 `-104` 的情况:当前不是软触发模式;距上次成功触发不足一帧时间;相机尚未开流(首次 `openCamera` 之前)。
> `setTriggerOut(0)` 会在自动和软触发模式下关闭脉冲输出。软触发模式下关闭输出后,`triggerFrame()` 会被拒绝。外触发模式始终关闭脉冲输出,无论请求值是 `0` 还是 `1`。
>
> 多相机同步:各相机的触发线接在一起,一台作主相机,其余作从相机。主相机设为自动触发(`0`)或软触发(`1`),从相机设为外触发(`2`)。
> 主相机自动触发时,所有相机按主相机的帧率同步连续出图;主相机软触发时,主相机每次 `triggerFrame()` 成功,所有相机同时各出一帧。
> 从相机也要调用 `setFrameRate`,并设为与主相机相同的帧率:相机按帧率限制曝光时间上限,从相机帧率设低了,曝光可能超过主相机的触发周期,导致漏帧。

## 四、设备信息

| 接口                                         | 功能                                                                       | 返回值               |
| -------------------------------------------- | -------------------------------------------------------------------------- | -------------------- |
| getCameraType(int& type)                     | 读取机型，0=A，1=P，2=AP                                                   | >0 成功              |
| getDepthUnitMm(double& unitMm)               | 读取深度单位：每个深度计数代表的毫米数，取自相机配置 `Dpu.depth_unit_mm` | >0 成功，-107 无数据 |
| getAppVersion(std::string& version)          | 读取 LocalApp 版本                                                         | >0 成功              |
| GetKernelVersion(std::string& version)       | 读取运行中的 Linux 内核版本                                                | >0 成功              |
| GetCameraLibVersion(std::string& version)    | 读取 libMoveSenseCamera.so 版本                                            | >0 成功              |
| getMacAddress(std::string& mac)              | 读取 eth0 当前 MAC，只读                                                   | >0 成功              |
| getHardwareVersion(std::string& hardware)    | 读取生产配置硬件版本                                                       | >0 成功，-107 无数据 |
| getProductModelVersion(std::string& product) | 读取生产配置产品型号                                                       | >0 成功，-107 无数据 |
| getCameraSN(std::string& sn)                 | 读取完整 SN                                                                | >0 成功              |
| setCameraSN(const std::string& sn)           | 校验并持久保存 SN，无需断开                                                | >0 成功              |

SN 为 1～63 个 ASCII 字母、数字、下划线或连字符，不截断。硬件版本与产品型号为只读设备信息，每项最多 63 字节。

## 五、双目曝光与增益

| 接口名称                                        | 功能               | 参数含义                                               | 返回值      |
| ----------------------------------------------- | ------------------ | ------------------------------------------------------ | ----------- |
| `setStereoAutoExpo(int onoff)`                | 双目自动曝光开关   | `onoff` `1` 开 / `0` 关                          | `>0` 成功 |
| `setStereoAutoExpoDesiredBin(int brightness)` | 设自动曝光目标亮度 | `brightness` 目标亮度值                              | `>0` 成功 |
| `setStereoExposure(unsigned expus)`           | 设双目曝光时间     | `expus` 微秒，按有效范围限制                         | `>0` 成功 |
| `getStereoExposure(unsigned& expus)`          | 读双目曝光时间     | `expus` 输出微秒                                     | `>0` 成功 |
| `setStereoGain(float gain)`                   | 设双目增益         | `gain` 单位 gain_128:`128` = 1 倍,`2048` = 16 倍 | `>0` 成功 |
| `getStereoGain(float& gain)`                  | 读双目增益         | `gain` 输出,同上单位                                 | `>0` 成功 |
| `setStereoMaxExposure(unsigned expus)`        | 设曝光上限         | `expus` 微秒,钳进 `[100, 帧率上限]`                | `>0` 成功 |
| `getStereoMaxExposure(unsigned& expus)`       | 读曝光上限         | `expus` 输出微秒                                     | `>0` 成功 |
| `setStereoMinExposure(unsigned expus)`        | 设曝光下限         | `expus` 微秒                                         | `>0` 成功 |
| `getStereoMinExposure(unsigned& expus)`       | 读曝光下限         | `expus` 输出微秒                                     | `>0` 成功 |
| `setStereoMaxGain(float gain)`                | 设增益上限         | `gain` 钳进 `[128, 2048]`                          | `>0` 成功 |
| `getStereoMaxGain(float& gain)`               | 读增益上限         | `gain` 输出                                          | `>0` 成功 |
| `setStereoMinGain(float gain)`                | 设增益下限         | `gain` 钳进 `[128, 2048]`                          | `>0` 成功 |
| `getStereoMinGain(float& gain)`               | 读增益下限         | `gain` 输出                                          | `>0` 成功 |

## 六、RGB 曝光与增益

| 接口名称                                     | 功能               | 参数含义                      | 返回值      |
| -------------------------------------------- | ------------------ | ----------------------------- | ----------- |
| `setRGBAutoExpo(int onoff)`                | 中目自动曝光开关   | `onoff` `1` 开 / `0` 关 | `>0` 成功 |
| `setRGBAutoExpoDesiredBin(int brightness)` | 设自动曝光目标亮度 | `brightness` 目标亮度值     | `>0` 成功 |
| `setRGBExposure(unsigned expus)`           | 设中目曝光时间     | `expus` 微秒                | `>0` 成功 |
| `getRGBExposure(unsigned& expus)`          | 读中目曝光时间     | `expus` 输出微秒            | `>0` 成功 |
| `setRGBGain(float gain)`                   | 设中目增益         | `gain` 单位 gain_128        | `>0` 成功 |
| `getRGBGain(float& gain)`                  | 读中目增益         | `gain` 输出                 | `>0` 成功 |
| `setRGBMaxExposure(unsigned expus)`        | 设曝光上限         | `expus` 微秒                | `>0` 成功 |
| `getRGBMaxExposure(unsigned& expus)`       | 读曝光上限         | `expus` 输出                | `>0` 成功 |
| `setRGBMinExposure(unsigned expus)`        | 设曝光下限         | `expus` 微秒                | `>0` 成功 |
| `getRGBMinExposure(unsigned& expus)`       | 读曝光下限         | `expus` 输出                | `>0` 成功 |
| `setRGBMaxGain(float gain)`                | 设增益上限         | `gain` 钳进 `[128, 2048]` | `>0` 成功 |
| `getRGBMaxGain(float& gain)`               | 读增益上限         | `gain` 输出                 | `>0` 成功 |
| `setRGBMinGain(float gain)`                | 设增益下限         | `gain` 钳进 `[128, 2048]` | `>0` 成功 |
| `getRGBMinGain(float& gain)`               | 读增益下限         | `gain` 输出                 | `>0` 成功 |

## 七、画质调节

### 7.1 色彩调节

| 接口名称                                                                                                              | 功能                          | 参数含义                                                         | 返回值      |
| --------------------------------------------------------------------------------------------------------------------- | ----------------------------- | ---------------------------------------------------------------- | ----------- |
| `setStereoColorAdjust(bool enable, unsigned brightness, unsigned contrast, unsigned saturation, unsigned hue)`      | 设双目亮度/对比度/饱和度/色调 | `enable` 色彩调节总开关;其余四项范围 `[0,100]`,`50` 为中性 | `>0` 成功 |
| `getStereoColorAdjust(bool& enable, unsigned& brightness, unsigned& contrast, unsigned& saturation, unsigned& hue)` | 读双目色彩参数                | 五项输出                                                         | `>0` 成功 |
| `setRGBColorAdjust(...)`                                                                                            | 设中目色彩参数                | 同上                                                             | `>0` 成功 |
| `getRGBColorAdjust(...)`                                                                                            | 读中目色彩参数                | 同上                                                             | `>0` 成功 |

### 7.2 锐化

| 接口名称                                                                                                  | 功能       | 参数含义                                                                                                                     | 返回值      |
| --------------------------------------------------------------------------------------------------------- | ---------- | ---------------------------------------------------------------------------------------------------------------------------- | ----------- |
| `setStereoSharpen(bool enable, unsigned strength)`                                                      | 设双目锐化 | `enable` 锐化开关;`strength` 范围 `[0,100]`,**`50` = 相机出厂调优强度**,`0` 不锐化,`100` 约为出厂值的 2 倍 | `>0` 成功 |
| `getStereoSharpen(bool& enable, unsigned& strength, unsigned& textureStrength, unsigned& edgeStrength)` | 读双目锐化 | 前两项同上；`textureStrength` / `edgeStrength` 为实际纹理 / 边缘锐化强度诊断值                                           | `>0` 成功 |
| `setRGBSharpen(bool enable, unsigned strength)`                                                         | 设中目锐化 | 同上                                                                                                                         | `>0` 成功 |
| `getRGBSharpen(...)`                                                                                    | 读中目锐化 | 同上                                                                                                                         | `>0` 成功 |

> 出厂默认锐化为关闭状态。

### 7.3 白平衡

| 接口名称                                                      | 功能                | 参数含义                                                          | 返回值      |
| ------------------------------------------------------------- | ------------------- | ----------------------------------------------------------------- | ----------- |
| `setStereoAutoWb(bool autoMode)`                            | 双目白平衡自动/手动 | `autoMode` `true` 自动(AWB),`false` 手动                    | `>0` 成功 |
| `getStereoAutoWb(bool& autoMode)`                           | 读双目白平衡模式    | `autoMode` 输出                                                 | `>0` 成功 |
| `setStereoWbGain(float rGain, float gGain, float bGain)`    | 设双目白平衡增益    | 三路增益,单位**倍数**:`1.0f` = 1 倍。范围 `[0, 15.996]` | `>0` 成功 |
| `getStereoWbGain(float& rGain, float& gGain, float& bGain)` | 读双目白平衡增益    | 三项输出,单位倍数                                                 | `>0` 成功 |
| `setRGBAutoWb(bool autoMode)`                               | 中目白平衡自动/手动 | 同上                                                              | `>0` 成功 |
| `getRGBAutoWb(bool& autoMode)`                              | 读中目白平衡模式    | 同上                                                              | `>0` 成功 |
| `setRGBWbGain(float rGain, float gGain, float bGain)`       | 设中目白平衡增益    | 同上                                                              | `>0` 成功 |
| `getRGBWbGain(...)`                                         | 读中目白平衡增益    | 同上                                                              | `>0` 成功 |

## 八、标定数据

标定接口、参数含义、单位和坐标关系见[标定参数说明](CALIBRATION_CN.md)。

## 九、帧率与降采样

| 接口名称                              | 功能                 | 参数含义                                                                                                     | 返回值      |
| ------------------------------------- | -------------------- | ------------------------------------------------------------------------------------------------------------ | ----------- |
| `setFrameRate(int fps)`             | 设帧率               | `fps` 目标帧率,有效范围 `[1,25]`,范围外相机忽略并保持原值(仍返回成功);实际上限受订阅内容与降采样状态约束 | `>0` 成功 |
| `getFrameRate(int& fps)`            | 读帧率               | `fps` 输出                                                                                                 | `>0` 成功 |
| `setDownsampleMode(int mode)`       | 设降采样方式         | `0` 前降采样(算法前降分辨率)/ `1` 后降采样(发送前缩图)                                                   | `>0` 成功 |
| `setDepthDownsample(bool onoff)`    | 深度图降采样开关     | `onoff` `true` 开启                                                                                      | `>0` 成功 |
| `setRGBDownsample(bool onoff)`      | 中目降采样开关       | `onoff` `true` 开启                                                                                      | `>0` 成功 |
| `setStereoDownsample(bool onoff)`   | 双目降采样开关       | `onoff` `true` 开启                                                                                      | `>0` 成功 |
| `setChunkSize(int bytes)`           | 设 TCP 分包大小      | `bytes` 钳进 `[1024, 262144]`                                                                            | `>0` 成功 |
| `setRegistrationSwitch(bool onoff)` | 深度到彩色的配准开关 | `onoff` `true` 开启配准                                                                                  | `>0` 成功 |

## 十、ROI

| 接口名称                                                                                           | 功能           | 参数含义                                                                                                                                | 返回值                                                |
| -------------------------------------------------------------------------------------------------- | -------------- | --------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- |
| `setRoi(RoiStream stream, bool enable, unsigned x1, unsigned y1, unsigned x2, unsigned y2)`      | 设某一路的裁窗 | `stream` 见 `RoiStream` 枚举(六路);`enable` `false` 时坐标忽略;坐标为**0 基、左上闭、右下开**,须为偶数且窗口不小于 16×16 | `>0` 成功 / `-101` 参数非法 / `-102` 被相机拒绝 |
| `getRoi(RoiStream stream, bool& enable, unsigned& x1, unsigned& y1, unsigned& x2, unsigned& y2)` | 读某一路的裁窗 | 输出当前生效值                                                                                                                          | `>0` 成功                                           |

> 深度图不支持 ROI。
> 检测框(SEG)坐标以完整图像为基准。

## 十一、DOE 投射器

| 接口名称                         | 功能        | 参数含义                                                                                      | 返回值      |
| -------------------------------- | ----------- | --------------------------------------------------------------------------------------------- | ----------- |
| `setDOEPower(unsigned power)`  | 设 DOE 功率 | `power` `[0,255]`,超出按 `255`;`0` 关闭,其余每 32 为一档,共 8 档,档位越高开的通道越多 | `>0` 成功 |
| `getDOEPower(unsigned& power)` | 读 DOE 功率 | `power` 输出,为最近一次设置的 `[0,255]` 值                                                | `>0` 成功 |

> 被动式 P 型机没有 DOE,设置会被相机忽略。

## 十二、网络配置

| 接口                                                            | 功能                          | 返回值          |
| --------------------------------------------------------------- | ----------------------------- | --------------- |
| GetNetworkConfig(NetworkConfig& config)                         | 读取 eth0 当前 IP、掩码、网关 | >0 成功         |
| SetNetworkConfig(const NetworkConfig& config)                   | 整组校验、持久保存并动态应用  | >0 已接受并保存 |
| getMacAddress(uint8_t mac[6]) / getMacAddress(std::string& mac) | 读取当前 MAC                  | >0 成功         |

NetworkConfig 包含字符串 ip、netmask、gateway，三项必须完整提交。使用 IPv4 静态地址；掩码连续且主机地址不得为网络地址、广播地址、回环或组播地址。网关须同网段有效且不同于相机 IP，0.0.0.0 表示无默认网关。不开放 DHCP/DNS 或 MAC 修改。

设置成功表示请求已接受。网络地址生效时原连接可能断开，需使用新 IP 重新连接，并通过 GetNetworkConfig 确认当前配置。

错误：-105 参数非法，-106 持久保存失败，-107 信息不可用，-108 改网忙。所有连接类错误保留 <=0 返回约定。不得把超时或断开当作应用成功，需通过新地址回读确认。

## 十三、相机发现

| 接口名称                                                     | 功能                 | 参数含义                                                       | 返回值                               |
| ------------------------------------------------------------ | -------------------- | -------------------------------------------------------------- | ------------------------------------ |
| `scanSingleCamera(std::string ip, CameraInfo& cameraInfo)` | 探测指定 IP 的相机   | `ip` 目标;`cameraInfo` 输出信息(ip、mac、gateway、netmask) | `0` 成功,`<0` 失败(超时或无回应) |
| `scanAllCameras(std::string broadcastIP)`                  | 在指定广播域扫描     | `broadcastIP` 广播地址,默认 `255.255.255.255`              | 相机信息数组                         |
| `scanAllNetworkInterfaces()`                               | 遍历本机所有网卡扫描 | 无                                                             | 相机信息数组                         |
| `setScanTimeout(int timeoutMS)`                            | 设扫描超时           | `timeoutMS` 毫秒                                             | 无                                   |
| `setScanPort(int port)`                                    | 设扫描端口           | `port` 默认 5004                                             | 无                                   |
| `scanCameras(...)`(静态)                                   | 静态版广播扫描       | `broadcastIP` / `port` / `timeoutMS`                     | 相机信息数组                         |
| `scanCamerasAllInterfaces(...)`(静态)                      | 静态版全网卡扫描     | `port` / `timeoutMS`                                       | 相机信息数组                         |

> 多网卡机器建议用 `scanAllNetworkInterfaces()`,它会逐个网卡发广播,避免只扫到默认路由所在网段。

## 十四、SDK 版本

| 接口名称                                                   | 功能              | 参数含义                               | 返回值         |
| ---------------------------------------------------------- | ----------------- | -------------------------------------- | -------------- |
| `simou3_get_version(int* major, int* minor, int* patch)` | 读 SDK 版本号     | 三个输出,可传 `nullptr` 忽略其中某项 | 无             |
| `simou3_get_version_string()`                            | 读 SDK 版本字符串 | 无                                     | 如 `"0.5.2"` |

两个函数是 `movesense` 命名空间下的自由函数,不是 `Simou3Camera` 的成员。
