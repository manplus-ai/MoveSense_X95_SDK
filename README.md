# Simou3 Camera SDK

> **Status: 0.1.0 — early release.** The API is **not** stable yet and
> breaking changes are expected before 1.0. Planned for upcoming releases:
> a `simou3` namespace, unified error codes and more, plus a smaller public
> header surface.

Cross-platform (Linux / Windows) C++ SDK for the Simou3 X95-series
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
simou3-camera-sdk_-src/
├── include/            Public headers (what integrators include)
│   └── Simou3Camera.h  Umbrella header — the main SDK API
├── src/                SDK library implementation (+ internal headers)
├── Sample/          Example program (depends on OpenCV, display only)
├── CMakeLists.txt
├── build.sh            One-click Linux build
└── win_build.bat       One-click Windows build (Visual Studio)
```

## Dependencies

- CMake ≥ 3.10 and a C++17 compiler.
- **The SDK library has no third-party dependencies.**
- The `Sample` example uses OpenCV for image display only. OpenCV
  is located via `find_package`; if it is missing, the sample is skipped and
  the SDK library still builds.

## Build

Linux:

```sh
./build.sh
# → linux_build/libSimou3CameraSDK.so
# → bin/Sample   (only if OpenCV is installed)
```

Windows (builds with whichever of Visual Studio 2017 / 2019 is installed,
each in x64 + x32, Debug + Release):

```bat
win_build.bat
:: Release → win_build\<vs2017|vs2019>\<x64|x32>\Release\Simou3CameraSDK.dll  (+ .lib)
:: Debug   → win_build\<vs2017|vs2019>\<x64|x32>\Debug\Simou3CameraSDKD.dll   (+ .lib)
```

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) and
[NOTICE](NOTICE).

---

中文说明见 [README_CN.md](README_CN.md)。
更完整的 API 使用说明将在后续阶段补充。
