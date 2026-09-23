# MoveSense X95 SDK 接口说明

本文档覆盖公开头 `include/movesense/Simou3Camera.h` 的接口。

## 通用约定

### 返回值

除特别说明外,所有返回 `int` 的接口遵循同一约定:

| 返回值                                | 含义                                       |
| ------------------------------------- | ------------------------------------------ |
| `> 0`                               | 成功(数值为该次收发的字节数,无其他含义)    |
| `<= 0`                              | 失败(0 或负数表示连接断开、超时、对端拒绝) |
| `-100` `SIMOU3_ERR_NOT_SUPPORTED` | 该接口暂不支持                             |
| `-101` `SIMOU3_ERR_ROI_INVALID`   | ROI 参数非法(SDK 本地校验未通过)           |
| `-102` `SIMOU3_ERR_ROI_REJECTED`  | ROI 被相机拒绝(回显值与请求不符)           |

判定成功请用 `ret > 0`,不要用 `ret == 0`。

### 调用顺序

- 取图相关接口(`openCamera` / `getFrame` / `getIMU`)走数据通道,需先 `openCamera`
- 参数设置类接口走控制通道,需先 `openCameraSettings`
- 两个通道相互独立,可单独使用

### 通道概念

`Stereo` 指双目(左右目共用一套参数,内部同时写两个 ISP pipe);`RGB` 指彩色相机。
**被动式 P 型机** `setRGB*` / `getRGB*` 返回不支持,不会误碰硬件。

## 一、连接与数据流

| 接口名称                           | 功能                       | 参数含义                                                                           | 返回值        |
| ---------------------------------- | -------------------------- | ---------------------------------------------------------------------------------- | ------------- |
| `Simou3Camera(std::string ip)`   | 构造相机对象               | `ip` 相机 IP,默认 `192.168.1.70`                                               | —            |
| `isCameraOnline()`               | 探测相机是否在线           | 无                                                                                 | `true` 在线 |
| `isCameraOnline(std::string ip)` | 探测指定 IP 的相机是否在线 | `ip` 目标 IP                                                                     | `true` 在线 |
| `openCamera(int mode)`           | 打开数据流并开始接收       | `mode` 订阅掩码,由 `transfer_mode_def.h` 的 `TRANSFER_MODE_*_BIT` 按位或组成 | `>0` 成功   |
| `closeCamera()`                  | 关闭数据流                 | 无                                                                                 | `>0` 成功   |
| `openCameraSettings()`           | 打开控制通道(端口 5002)    | 无                                                                                 | `>0` 成功   |
| `closeCameraSettings()`          | 关闭控制通道               | 无                                                                                 | `>0` 成功   |

## 二、取图与 IMU

| 接口名称                                         | 功能                   | 参数含义                                     | 返回值               |
| ------------------------------------------------ | ---------------------- | -------------------------------------------- | -------------------- |
| `getFrame(MovesenseFrame& out)`                | 取一帧(阻塞至默认超时) | `out` 输出帧                               | `true` 取到        |
| `getFrame(MovesenseFrame& out, int timeoutMs)` | 取一帧(指定超时)       | `out` 输出帧;`timeoutMs` 超时毫秒        | `true` 取到        |
| `getStats()`                                   | 取收流统计             | 无                                           | `Simou3Stats` 结构 |
| `setDropDegradedFrame(bool onoff)`             | 设置是否丢弃不完整帧   | `onoff` `true` 丢弃残帧                  | 无                   |
| `getIMU(std::vector<Imu>& out, int num)`       | 取 IMU 数据            | `out` 输出数组;`num` 最多取几条,默认 200 | `>0` 实际条数      |
| `alignTimeToHost()`                            | 将相机时间对齐到主机   | 无                                           | `>0` 成功          |

## 三、触发

| 接口名称                       | 功能               | 参数含义 | 返回值   |
| ------------------------------ | ------------------ | -------- | -------- |
| `setTriggerMode(int mode)`   | **暂不支持** | —       | `-100` |
| `getTriggerMode(int& mode)`  | **暂不支持** | —       | `-100` |
| `setTriggerOut(int out)`     | **暂不支持** | —       | `-100` |
| `getTriggerOut(int& out)`    | **暂不支持** | —       | `-100` |
| `triggerFrame(int frameCnt)` | **暂不支持** | —       | `-100` |

## 四、设备信息

| 接口名称                                               | 功能               | 参数含义                                                | 返回值      |
| ------------------------------------------------------ | ------------------ | ------------------------------------------------------- | ----------- |
| `getCameraType(int& type)`                           | 读机型             | `type` 输出:`1` = 被动式 P 型(无中目),其他 = 含中目 | `>0` 成功 |
| `getAppVersion(std::string& version)`                | 读应用程序版本     | `version` 输出版本串                                  | `>0` 成功 |
| `getCameraSN(std::string& sn)`                       | 读相机序列号       | `sn` 输出序列号                                       | `>0` 成功 |
| `getFPGAVersion(std::string& fpga)`                  | 读 FPGA 版本       | `fpga` 输出版本串                                     | `>0` 成功 |
| `getFirmwareVersion(unsigned& firmware)`             | **暂不支持** | —                                                      | `-100`    |
| `updateFirmware(std::string firmwareName)`           | **暂不支持** | —                                                      | `-100`    |
| `setCameraSN(const std::string& sn)`                 | **暂不支持** | —                                                      | `-100`    |
| `setFPGAVersion(const std::string& fpga)`            | **暂不支持** | —                                                      | `-100`    |
| `setHardwareVersion(const std::string& hardware)`    | **暂不支持** | —                                                      | `-100`    |
| `getHardwareVersion(std::string& hardware)`          | **暂不支持** | —                                                      | `-100`    |
| `setProductModelVersion(const std::string& product)` | **暂不支持** | —                                                      | `-100`    |
| `getProductModelVersion(std::string& product)`       | **暂不支持** | —                                                      | `-100`    |

## 五、双目曝光与增益

| 接口名称                                        | 功能               | 参数含义                                               | 返回值      |
| ----------------------------------------------- | ------------------ | ------------------------------------------------------ | ----------- |
| `setStereoAutoExpo(int onoff)`                | 双目自动曝光开关   | `onoff` `1` 开 / `0` 关                          | `>0` 成功 |
| `setStereoAutoExpoDesiredBin(int brightness)` | 设自动曝光目标亮度 | `brightness` 目标亮度值                              | `>0` 成功 |
| `setStereoExposure(unsigned expus)`           | 设双目曝光时间     | `expus` 微秒,内部钳进有效区间                        | `>0` 成功 |
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
| `getStereoSharpen(bool& enable, unsigned& strength, unsigned& textureStrength, unsigned& edgeStrength)` | 读双目锐化 | 前两项同上;`textureStrength` / `edgeStrength` 为 ISP 内部实际曲线首档值,仅供诊断                                         | `>0` 成功 |
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

| 接口名称                                             | 功能           | 参数含义                                                            | 返回值      |
| ---------------------------------------------------- | -------------- | ------------------------------------------------------------------- | ----------- |
| `setStereoCalibData(unsigned char* data, int len)` | 写双目标定数据 | `data` 标定 blob;`len` 字节数,默认 `calib::kStereoCalibBytes` | `>0` 成功 |
| `getStereoCalibData(unsigned char* data, int len)` | 读双目标定数据 | 同上                                                                | `>0` 成功 |
| `setRGBCalibData(unsigned char* data, int len)`    | 写中目标定数据 | `len` 默认 `calib::kRgbCalibBytes`                              | `>0` 成功 |
| `getRGBCalibData(unsigned char* data, int len)`    | 读中目标定数据 | 同上                                                                | `>0` 成功 |

## 九、帧率与降采样

| 接口名称                              | 功能                 | 参数含义                                                   | 返回值      |
| ------------------------------------- | -------------------- | ---------------------------------------------------------- | ----------- |
| `setFrameRate(int fps)`             | 设帧率               | `fps` 目标帧率,受订阅内容与降采样状态约束                | `>0` 成功 |
| `getFrameRate(int& fps)`            | 读帧率               | `fps` 输出                                               | `>0` 成功 |
| `setDownsampleMode(int mode)`       | 设降采样方式         | `0` 前降采样(算法前降分辨率)/ `1` 后降采样(发送前缩图) | `>0` 成功 |
| `setDepthDownsample(bool onoff)`    | 深度图降采样开关     | `onoff` `true` 开启                                    | `>0` 成功 |
| `setRGBDownsample(bool onoff)`      | 中目降采样开关       | `onoff` `true` 开启                                    | `>0` 成功 |
| `setStereoDownsample(bool onoff)`   | 双目降采样开关       | `onoff` `true` 开启                                    | `>0` 成功 |
| `setChunkSize(int bytes)`           | 设 TCP 分包大小      | `bytes` 钳进 `[1024, 262144]`                          | `>0` 成功 |
| `setRegistrationSwitch(bool onoff)` | 深度到彩色的配准开关 | `onoff` `true` 开启配准                                | `>0` 成功 |

> `setStereoDownsample` 会触发相机内部重新配置,建议放在一组降采样设置的最后调用。

## 十、ROI

| 接口名称                                                                                           | 功能           | 参数含义                                                                                                                                | 返回值                                                |
| -------------------------------------------------------------------------------------------------- | -------------- | --------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- |
| `setRoi(RoiStream stream, bool enable, unsigned x1, unsigned y1, unsigned x2, unsigned y2)`      | 设某一路的裁窗 | `stream` 见 `RoiStream` 枚举(六路);`enable` `false` 时坐标忽略;坐标为**0 基、左上闭、右下开**,须为偶数且窗口不小于 16×16 | `>0` 成功 / `-101` 参数非法 / `-102` 被相机拒绝 |
| `getRoi(RoiStream stream, bool& enable, unsigned& x1, unsigned& y1, unsigned& x2, unsigned& y2)` | 读某一路的裁窗 | 输出当前生效值                                                                                                                          | `>0` 成功                                           |

> 深度图不支持 ROI。
> 检测框(SEG)坐标仍是全画幅坐标,绘制前需减去 ROI 原点。

## 十一、网络配置

| 接口名称                                                                   | 功能               | 参数含义            | 返回值      |
| -------------------------------------------------------------------------- | ------------------ | ------------------- | ----------- |
| `getIPAddress(uint32_t& ip, uint32_t& mask, uint32_t& gateway)`          | 读 IP 配置(整型)   | 三项输出,网络字节序 | `>0` 成功 |
| `getIPAddress(std::string& ip, std::string& mask, std::string& gateway)` | 读 IP 配置(点分串) | 三项输出            | `>0` 成功 |
| `getMacAddress(uint8_t mac[6])`                                          | 读 MAC(字节数组)   | `mac` 输出 6 字节 | `>0` 成功 |
| `getMacAddress(std::string& mac)`                                        | 读 MAC(字符串)     | `mac` 输出        | `>0` 成功 |
| `setIPAddress(...)`                                                      | **暂不支持** | —                  | `-100`    |
| `setMacAddress(...)`                                                     | **暂不支持** | —                  | `-100`    |

## 十二、相机发现

| 接口名称                                                     | 功能                 | 参数含义                                          | 返回值       |
| ------------------------------------------------------------ | -------------------- | ------------------------------------------------- | ------------ |
| `scanSingleCamera(std::string ip, CameraInfo& cameraInfo)` | 探测指定 IP 的相机   | `ip` 目标;`cameraInfo` 输出信息               | `>0` 成功  |
| `scanAllCameras(std::string broadcastIP)`                  | 在指定广播域扫描     | `broadcastIP` 广播地址,默认 `255.255.255.255` | 相机信息数组 |
| `scanAllNetworkInterfaces()`                               | 遍历本机所有网卡扫描 | 无                                                | 相机信息数组 |
| `setScanTimeout(int timeoutMS)`                            | 设扫描超时           | `timeoutMS` 毫秒                                | 无           |
| `setScanPort(int port)`                                    | 设扫描端口           | `port` 默认 5004                                | 无           |
| `scanCameras(...)`(静态)                                   | 静态版广播扫描       | `broadcastIP` / `port` / `timeoutMS`        | 相机信息数组 |
| `scanCamerasAllInterfaces(...)`(静态)                      | 静态版全网卡扫描     | `port` / `timeoutMS`                          | 相机信息数组 |

> 多网卡机器建议用 `scanAllNetworkInterfaces()`,它会逐个网卡发广播,避免只扫到默认路由所在网段。
