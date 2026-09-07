# MoveSense X95 SDK

> **状态:0.1.1 — 早期版本。** API **尚不稳定**,1.0 之前预计会有破坏性变更。
> 后续版本计划:引入 `simou3` 命名空间、统一错误码等并收窄对外公开的头文件接口面。

跨平台(Linux / Windows)C++ SDK,用于 MoveSense X95 系列主/被动式双目深度相机。
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
MoveSense_X95_SDK/
├── include/movesense/  公开头文件
│   └── Simou3Camera.h  API
├── src/                SDK 库实现
├── Sample/             示例程序(依赖 OpenCV,仅用于显示)
├── cmake/              package config 模板
├── CMakeLists.txt
└── win_build.bat       Windows 一键编译(Visual Studio)
```

## 依赖

- CMake ≥ 3.10,支持 C++17 的编译器。
- **SDK 库本体零第三方依赖。**
- `Sample` 示例**默认不编译**,用 `-DMOVESENSE_BUILD_SAMPLE=ON` 开启;
  它仅用 OpenCV(经 `find_package`)做图像显示。SDK 库本体不依赖 OpenCV。

## 编译与安装

Linux:

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
sudo make install          # 默认装到 /usr/local
```

安装后布局(相对 `CMAKE_INSTALL_PREFIX`,默认 `/usr/local`):

```
include/movesense/*.h                         公开头文件
lib/libMoveSense_X95_SDK.so(.0.1)(.0.1.1)     动态库
lib/cmake/MoveSense_X95_SDK/*.cmake           package config(供 find_package)
```

装到别处在 cmake 步加 `-DCMAKE_INSTALL_PREFIX=<dir>`,例如
`cmake .. -DCMAKE_INSTALL_PREFIX=/opt/movesense`。
卸载已安装的文件: 在 build 目录里执行 `sudo make uninstall`。

`Sample` 示例**默认不编译**。在 cmake 步加 `-DMOVESENSE_BUILD_SAMPLE=ON`
才会编(需要 OpenCV),产物在 `bin/Sample`。

Windows(装了 VS2017 / VS2019 哪个就用哪个编,每个都出 x64 + x32、Debug + Release):

```bat
win_build.bat
:: Release → win_build\<vs2017|vs2019>\<x64|x32>\Release\MoveSense_X95_SDK.dll  (+ .lib)
:: Debug   → win_build\<vs2017|vs2019>\<x64|x32>\Debug\MoveSense_X95_SDKD.dll   (+ .lib)
```

## 在你的工程里使用

安装后,通过 CMake 的 `find_package` 引入:

```cmake
find_package(MoveSense_X95_SDK REQUIRED)
target_link_libraries(your_app PRIVATE MoveSense::X95_SDK)
```

```cpp
#include <movesense/Simou3Camera.h>
```

装到非标准前缀时,用 `-DCMAKE_PREFIX_PATH=<安装前缀>` 指过去。

Windows 侧 `win_build.bat` 只编库(不 install、不走 find_package)。直接用即可:
把 `include/` 加进头文件搜索路径,链接 `MoveSense_X95_SDK.lib`,并把
`MoveSense_X95_SDK.dll` 放到可执行文件旁边。

## 许可

采用 Apache License 2.0。见 [LICENSE](LICENSE) 与 [NOTICE](NOTICE)。

---

更完整的 API 使用说明将在后续阶段补充。
