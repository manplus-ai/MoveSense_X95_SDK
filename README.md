# MoveSense X95 SDK

> **Status: 0.2.1 — early release.** The API is **not** stable yet and
> breaking changes are expected before 1.0.

Cross-platform (Linux / Windows) C++ SDK for the MoveSense X95-series
active / passive stereo depth camera. The active variant pairs two IR
global-shutter sensors (with an 850 nm filter) and a DOE speckle projector
for depth ranging; depth is computed on the
device and streamed to the host. A separate RGB stream is provided; its
calibration is used **only** for point-cloud coloring and does not take part
in ranging. An on-board IMU (~200 Hz) is streamed alongside the images
(~25 Hz), which makes the camera suitable for VIO / SLAM / point-cloud work.

The host talks to the camera over Ethernet using a private protocol:

| Channel | Transport | Purpose                                    |
| ------- | --------- | ------------------------------------------ |
| 5001    | TCP       | Data stream (image chunks + IMU)           |
| 5002    | TCP       | Parameter configuration (request/response) |
| 5000    | UDP       | Time synchronization                       |
| 5004    | UDP       | Device discovery                           |

## Layout

```
MoveSense_X95_SDK/
├── include/movesense/       Public headers
│   ├── Simou3Camera.h       camera API (umbrella header)
│   ├── Simou3CalibLayout.h  calibration blob field offsets
│   └── ...                  types, settings, transfer, scan, net
├── src/                SDK implementation
├── Sample/             Example program (depends on OpenCV, display only)
├── cmake/              Package config template
├── CMakeLists.txt
└── win_build.bat       One-click Windows build (Visual Studio)
```

## Dependencies

- CMake ≥ 3.10 and a C++17 compiler.
- **The SDK library has no third-party dependencies.**
- The `Sample` example is **off by default**. Enable it with
  `-DMOVESENSE_BUILD_SAMPLE=ON`; it uses OpenCV (via `find_package`) for image
  display only. The SDK library itself never depends on OpenCV.

## Build & Install

Linux:

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
sudo make install          # installs to /usr/local by default
```

Installed layout (relative to `CMAKE_INSTALL_PREFIX`, default `/usr/local`):

```
include/movesense/*.h                         public headers
lib/libMoveSense_X95_SDK.so(.0.2)(.0.2.1)     shared library
lib/cmake/MoveSense_X95_SDK/*.cmake           package config (for find_package)
```

Install elsewhere by adding `-DCMAKE_INSTALL_PREFIX=<dir>` to the cmake step,
e.g. `cmake .. -DCMAKE_INSTALL_PREFIX=/opt/movesense`. To remove what was
installed, run `sudo make uninstall` from the build directory.

The `Sample` example is **not built by default**. Add
`-DMOVESENSE_BUILD_SAMPLE=ON` to the cmake step to build it (requires OpenCV);
the binary is produced at `bin/Sample`.

Windows (builds with whichever of Visual Studio 2017 / 2019 is installed,
each in x64 + x32, Debug + Release):

```bat
win_build.bat
:: Release → win_build\<vs2017|vs2019>\<x64|x32>\Release\MoveSense_X95_SDK.dll  (+ .lib)
:: Debug   → win_build\<vs2017|vs2019>\<x64|x32>\Debug\MoveSense_X95_SDKD.dll   (+ .lib)
```

## Use it in your project

After installing, consume the SDK from CMake via `find_package`:

```cmake
find_package(MoveSense_X95_SDK REQUIRED)
target_link_libraries(your_app PRIVATE MoveSense::X95_SDK)
```

```cpp
#include <movesense/Simou3Camera.h>
```

If installed to a non-standard prefix, point CMake at it with
`-DCMAKE_PREFIX_PATH=<install-prefix>`.

On Windows, `win_build.bat` only builds the libraries (no install, no
find_package). Consume them directly: add `include/` to your include path,
link against `MoveSense_X95_SDK.lib`, and ship `MoveSense_X95_SDK.dll` next to
your executable.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) and
[NOTICE](NOTICE).

---

中文说明见 [README_CN.md](README_CN.md)。
更完整的 API 使用说明将在后续阶段补充。
