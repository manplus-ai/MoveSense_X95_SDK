# 司眸三代相机 SDK

> **状态:0.1.0 — 早期版本。** API **尚不稳定**,1.0 之前预计会有破坏性变更。
> 后续版本计划:引入 `simou3` 命名空间、统一错误码等并收窄对外公开的头文件接口面。

跨平台(Linux / Windows)C++ SDK,用于司眸三代X95系列主/被动式双目深度相机。
深度在设备端算好后传给上位机。另有一路 RGB,其标定参数**仅用于点云上色,不参与
测距**。板载 IMU与图像一并下传。

上位机通过以太网、私有协议与相机通信:

| 通道 | 传输 | 用途                     |
| ---- | ---- | ------------------------ |
| 5001 | TCP  | 数据流(图像 chunk + IMU) |
| 5002 | TCP  | 参数配置(请求-响应)      |
| 5000 | UDP  | 时间同步                 |
| 5004 | UDP  | 设备扫描                 |

## 目录结构

```
simou3-camera-sdk_-src/
├── include/            对外公开头文件(集成方 include 这些)
│   └── Simou3Camera.h  主 API(umbrella 头)
├── src/                SDK 库实现(含内部头)
├── Sample/          示例程序(依赖 OpenCV,仅用于显示)
├── CMakeLists.txt
├── build.sh            Linux 一键编译
└── win_build.bat       Windows 一键编译(Visual Studio)
```

## 依赖

- CMake ≥ 3.10,支持 C++17 的编译器。
- **SDK 库本体零第三方依赖。**
- `Sample` 示例仅用 OpenCV 做图像显示,经 `find_package` 查找;
  找不到则跳过示例,SDK 库照常编译。

## 编译

Linux:

```sh
./build.sh
# → linux_build/libSimou3CameraSDK.so
# → bin/Sample   (仅当系统装了 OpenCV)
```

Windows(装了 VS2017 / VS2019 哪个就用哪个编,每个都出 x64 + x32、Debug + Release):

```bat
win_build.bat
:: Release → win_build\<vs2017|vs2019>\<x64|x32>\Release\Simou3CameraSDK.dll  (+ .lib)
:: Debug   → win_build\<vs2017|vs2019>\<x64|x32>\Debug\Simou3CameraSDKD.dll   (+ .lib)
```

## 许可

采用 Apache License 2.0。见 [LICENSE](LICENSE) 与 [NOTICE](NOTICE)。

---

更完整的 API 使用说明将在后续阶段补充。
