# MoveSense X95 SDK

> **状态:0.5.2 — 早期版本。** API **尚不稳定**,1.0 之前预计会有破坏性变更。

用于 MoveSense X95 系列相机的跨平台（Linux / Windows）C++ SDK，提供图像、深度和原始 IMU 数据读取，以及相机参数、标定参数、设备信息和网络配置接口。RGB 接口适用于具备 RGB 镜头的机型。

SDK 使用以下网络通道：

| 通道 | 传输 | 用途                     |
| ---- | ---- | ------------------------ |
| 5001 | TCP  | 数据流(图像 chunk + IMU) |
| 5002 | TCP  | 参数配置(请求-响应)      |
| 5000 | UDP  | 时间同步                 |
| 5004 | UDP  | 设备扫描                 |

接口速查见 [doc/API_CN.md](doc/API_CN.md)(English: [doc/API_EN.md](doc/API_EN.md))。

标定参数说明见 [doc/CALIBRATION_CN.md](doc/CALIBRATION_CN.md)(English: [doc/CALIBRATION_EN.md](doc/CALIBRATION_EN.md))。

## 目录结构

```
MoveSense_X95_SDK/
├── include/movesense/       公开头文件
│   ├── Simou3Camera.h       相机 API(总入口头)
│   ├── Simou3CalibLayout.h  标定参数结构与布局
│   └── ...                  类型 / 参数 / 传输 / 扫描 / 网络
├── src/                SDK 库实现
├── doc/                接口与标定参数说明(中英文)
├── Sample/             示例程序（取图、参数控制、标定打印）
├── cmake/              package config 模板
├── CMakeLists.txt
└── win_build.bat       Windows 一键编译(Visual Studio)
```

## 依赖

- CMake ≥ 3.10,支持 C++17 的编译器。
- **SDK 库本体零第三方依赖。**
- `Sample` 使用 OpenCV 进行图像显示与处理；SDK 库本体不依赖 OpenCV。

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
lib/libMoveSense_X95_SDK.so(.0.5)(.0.5.2)     动态库
lib/cmake/MoveSense_X95_SDK/*.cmake           package config(供 find_package)
```

装到别处在 cmake 步加 `-DCMAKE_INSTALL_PREFIX=<dir>`,例如
`cmake .. -DCMAKE_INSTALL_PREFIX=/opt/movesense`。
卸载已安装的文件: 在 build 目录里执行 `sudo make uninstall`。

`Sample` 示例**默认不编译**。在 cmake 步加 `-DMOVESENSE_BUILD_SAMPLE=ON`
才会编(需要 OpenCV)。Linux 产物位于构建目录下的 `bin/Sample`。

示例在相机开启并收到首帧后打印双目及可用的 RGB 标定参数，包含 IMU 噪声、时偏、原始相机系外参和四个内参矩阵。

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

Windows 侧 `win_build.bat` 编译 SDK 库，找到 OpenCV 时也会编译 Sample，不执行安装。直接使用 SDK 时:
把 `include/` 加进头文件搜索路径,链接 `MoveSense_X95_SDK.lib`,并把
`MoveSense_X95_SDK.dll` 放到可执行文件旁边。

## 许可

采用 Apache License 2.0。见 [LICENSE](LICENSE) 与 [NOTICE](NOTICE)。
