# MoveSense X95 SDK

> **Status: 0.5.2 — early release.** The API is **not** stable yet and
> breaking changes are expected before 1.0.

Cross-platform (Linux / Windows) C++ SDK for MoveSense X95 cameras. It provides image, depth, and raw IMU data access, along with camera settings, calibration parameters, device information, and network configuration interfaces. RGB interfaces apply to models equipped with an RGB camera.

The SDK uses the following network channels:

| Channel | Transport | Purpose                                    |
| ------- | --------- | ------------------------------------------ |
| 5001    | TCP       | Data stream (image chunks + IMU)           |
| 5002    | TCP       | Parameter configuration (request/response) |
| 5000    | UDP       | Time synchronization                       |
| 5004    | UDP       | Device discovery                           |

API reference: [doc/API_EN.md](doc/API_EN.md) (中文: [doc/API_CN.md](doc/API_CN.md)).

Calibration parameters: [doc/CALIBRATION_EN.md](doc/CALIBRATION_EN.md) (中文: [doc/CALIBRATION_CN.md](doc/CALIBRATION_CN.md)).

## Layout

```
MoveSense_X95_SDK/
├── include/movesense/       Public headers
│   ├── Simou3Camera.h       camera API (umbrella header)
│   ├── Simou3CalibLayout.h  calibration structures and layout
│   └── ...                  types, settings, transfer, scan, net
├── src/                SDK implementation
├── doc/                API and calibration parameter reference (English / Chinese)
├── Sample/             Image acquisition, camera control, and calibration printing
├── cmake/              Package config template
├── CMakeLists.txt
└── win_build.bat       One-click Windows build (Visual Studio)
```

## Dependencies

- CMake ≥ 3.10 and a C++17 compiler.
- **The SDK library has no third-party dependencies.**
- `Sample` uses OpenCV for image display and processing. The SDK library itself does not depend on OpenCV.

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
lib/libMoveSense_X95_SDK.so(.0.5)(.0.5.2)     shared library
lib/cmake/MoveSense_X95_SDK/*.cmake           package config (for find_package)
```

Install elsewhere by adding `-DCMAKE_INSTALL_PREFIX=<dir>` to the cmake step,
e.g. `cmake .. -DCMAKE_INSTALL_PREFIX=/opt/movesense`. To remove what was
installed, run `sudo make uninstall` from the build directory.

The `Sample` example is **not built by default**. Add
`-DMOVESENSE_BUILD_SAMPLE=ON` to the cmake step to build it (requires OpenCV);
on Linux, the binary is produced at `bin/Sample` inside the build directory.

After the camera opens and the first frame arrives, the example prints stereo and available RGB calibration, including IMU noise, time offset, raw-camera extrinsics, and all four intrinsic matrices.

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

On Windows, `win_build.bat` builds the SDK libraries and also builds Sample
when OpenCV is found; it does not install the SDK. To use the SDK directly, add `include/` to your include path,
link against `MoveSense_X95_SDK.lib`, and ship `MoveSense_X95_SDK.dll` next to
your executable.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) and
[NOTICE](NOTICE).

---

中文说明见 [README_CN.md](README_CN.md)。
