# MoveSense X95 SDK API Reference

This document covers the interfaces declared in the public header `include/movesense/Simou3Camera.h`.

## Conventions

### Return values

Unless stated otherwise, every interface returning `int` follows the same convention:

| Return value | Meaning |
| --- | --- |
| `> 0` | Success (the value is the byte count transferred; it carries no other meaning) |
| `<= 0` | Failure (0 or negative means connection closed, timeout, or rejected by the peer) |
| `-100` `SIMOU3_ERR_NOT_SUPPORTED` | Interface not supported yet |
| `-101` `SIMOU3_ERR_ROI_INVALID` | Invalid ROI arguments (rejected by local SDK validation) |
| `-102` `SIMOU3_ERR_ROI_REJECTED` | ROI rejected by the camera (echoed value differs from the request) |
| `-103` `SIMOU3_ERR_TRIGGER_MODE_INVALID` | Invalid trigger mode (rejected by local SDK validation) |
| `-104` `SIMOU3_ERR_TRIGGER_REJECTED` | Trigger request rejected by the camera |
| `-105` `SIMOU3_ERR_CONFIG_INVALID` | Invalid argument (network config, SN, trigger output value, etc.) |
| `-106` `SIMOU3_ERR_STORAGE_FAILED` | The camera failed to persist the setting |
| `-107` `SIMOU3_ERR_INFO_UNAVAILABLE` | The camera has no such information |
| `-108` `SIMOU3_ERR_CONFIG_BUSY` | The camera is changing its network; retry later |
| `-109` `SIMOU3_ERR_CONFIG_APPLY_FAILED` | Applying the network configuration failed |

Test for success with `ret > 0`, not with `ret == 0`.

### Timeouts

TCP connect, send and receive use a 5-second timeout. A request/response on the control channel (5002) that times out returns failure (`<= 0`). On the data channel (5001), 5 seconds without data only logs a message and the SDK keeps waiting; the connection is not dropped. Receiving ends only when you call `closeCamera` or the camera disconnects.

### Call order

- Streaming interfaces (`openCamera` / `getFrame` / `getIMU`) use the data channel and require `openCamera` first
- Parameter interfaces use the control channel and require `openCameraSettings` first
- The two channels are independent and may be used separately

### Channels

`Stereo` refers to the stereo pair (left and right share one parameter set; both ISP pipes are written
internally); `RGB` refers to the color camera.
A **passive P-type unit** has no RGB camera: `setRGB*` has no effect on the hardware and the values returned by `getRGB*` are meaningless.

## 1. Connection and streaming

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `Simou3Camera(std::string ip)` | Construct a camera object | `ip` camera address, default `192.168.1.70` | — |
| `isCameraOnline()` | Probe whether the camera is online | none | `true` if online |
| `isCameraOnline(std::string ip)` | Probe whether the camera at a given address is online | `ip` target address | `true` if online |
| `openCamera(int mode)` | Open the data stream and start receiving | `mode` subscription mask, OR-ed from the mask macros in `transfer_mode_def.h`, e.g. `TRANSFER_MODE_L_RECTIFIED \| TRANSFER_MODE_DEPTH` (same as `(1u << TRANSFER_MODE_*_BIT)`; the `*_BIT` macros are kept) | `>0` success |
| `closeCamera()` | Close the data stream | none | `>0` success |
| `openCameraSettings()` | Open the control channel (port 5002) | none | `>0` success |
| `closeCameraSettings()` | Close the control channel | none | `>0` success |

## 2. Frames and IMU

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `getFrame(MovesenseFrame& out)` | Fetch one frame (non-blocking; returns `false` at once when there is no new frame) | `out` output frame | `true` if a frame was received |
| `getFrame(MovesenseFrame& out, int timeoutMs)` | Fetch one frame (explicit timeout) | `out` output frame; `timeoutMs` timeout in ms, `0` does not wait, `<0` waits until a frame arrives or the stream is closed | `true` if a frame was received |
| `getStats()` | Read receive statistics | none | `Simou3Stats` struct |
| `setDropDegradedFrame(bool onoff)` | Set whether incomplete frames are discarded | `onoff` `true` drops partial frames | none |
| `getIMU(std::vector<Imu>& out, int num)` | Fetch IMU samples | `out` output vector; `num` max samples, default 200; the SDK keeps only the latest 200 samples, so a slow reader loses the oldest | `>0` number of samples |
| `alignTimeToHost()` | Align the camera clock to the host | none | `>0` success |

## 3. Trigger

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setTriggerMode(int mode)` | Set the trigger mode | `mode` `kTriggerModeAuto` (`0`) auto trigger: the camera streams continuously at the `setFrameRate` rate; `kTriggerModeSoft` (`1`) soft trigger: one frame per `triggerFrame()` call; `kTriggerModeExternal` (`2`) external trigger: this camera outputs no trigger signal and captures on the master camera's trigger signal | `>0` success / `-103` invalid value / `-104` rejected by the camera |
| `getTriggerMode(int& mode)` | Read the current trigger mode | `mode` output, same values as above | `>0` success |
| `setTriggerOut(int out)` | Enable or disable trigger pulse output | `out` `0` disables output, `1` enables output; in external trigger mode the request is accepted but the effective value is always `0` | `>0` success / `-105` invalid value / `-104` rejected by the camera |
| `getTriggerOut(int& out)` | Read the effective trigger pulse output state | `out` output, `0` or `1`; external trigger mode always reports `0` | `>0` success |
| `triggerFrame()` | Soft-trigger one frame | none | `>0` success / `-104` rejected by the camera |

> The trigger mode lasts only for the current camera run and falls back to auto trigger after a camera restart. It is **not** reset on reconnect, so set it explicitly before `openCamera` on every connection.
> Switching the trigger mode discards frames the camera has not sent yet.
>
> In soft trigger mode the camera produces no frames on its own: after each successful `triggerFrame()`, fetch the resulting frame with `getFrame` (`frameCnt()` advances by 1).
> Two `triggerFrame()` calls must be at least one frame period apart, i.e. `1000 / fps` ms (40 ms at 25 fps), so in soft trigger mode `setFrameRate` sets the highest allowed trigger rate.
>
> `triggerFrame()` returns `-104` when: the camera is not in soft trigger mode; less than one frame period has passed since the last successful trigger; or the camera is not streaming yet (before the first `openCamera`).
> `setTriggerOut(0)` disables pulse output in auto and soft trigger modes. In soft trigger mode, `triggerFrame()` is rejected while output is disabled. External trigger mode always keeps pulse output disabled, regardless of the requested value.
>
> Multi-camera sync: wire the cameras' trigger lines together, with one master and the others as slaves. Set the master to auto trigger (`0`) or soft trigger (`1`) and every slave to external trigger (`2`).
> With the master in auto trigger, all cameras stream in sync at the master's frame rate; with the master in soft trigger, each successful `triggerFrame()` on the master makes every camera capture one frame at the same time.
> Slaves must also call `setFrameRate` with the same frame rate as the master: the camera caps exposure time from the frame rate, so a slave set to a lower rate may expose longer than the master's trigger period and miss frames.

## 4. Device information

| API | Function | Return |
| --- | --- | --- |
| getCameraType(int& type) | Read the camera type, 0=A, 1=P, 2=AP | >0 success |
| getDepthUnitMm(double& unitMm) | Read the depth unit: millimetres represented by one depth count, taken from the camera config `Dpu.depth_unit_mm` | >0 success, -107 unavailable |
| getAppVersion(std::string& version) | Read LocalApp version | >0 success |
| GetKernelVersion(std::string& version) | Read running Linux kernel version | >0 success |
| GetCameraLibVersion(std::string& version) | Read libMoveSenseCamera.so version | >0 success |
| getMacAddress(std::string& mac) | Read current eth0 MAC, read-only | >0 success |
| getHardwareVersion(std::string& hardware) | Read production hardware version | >0 success, -107 unavailable |
| getProductModelVersion(std::string& product) | Read production product model | >0 success, -107 unavailable |
| getCameraSN(std::string& sn) | Read complete serial number | >0 success |
| setCameraSN(const std::string& sn) | Validate and persist SN without disconnecting | >0 success |

SN accepts 1 to 63 ASCII letters, digits, underscores or hyphens without truncation. It is stored in /home/Param/CameraProbuctSn using SN, a space and the value. Reads fall back to /usr/local/bin/CameraProbuctSn only when the new file is absent and do not create files. Production supplies hardware_version= and product_model= in /home/Param/device.conf, up to 63 bytes per value. MAC, hardware, model and FPGA setters are not exposed.

Since 0.5.0 these APIs and parameter replies have breaking changes. Update camera and SDK together; legacy fixed-size replies are unsupported.

## 5. Stereo exposure and gain

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setStereoAutoExpo(int onoff)` | Toggle stereo auto exposure | `onoff` `1` on / `0` off | `>0` success |
| `setStereoAutoExpoDesiredBin(int brightness)` | Set the auto-exposure target brightness | `brightness` target brightness value | `>0` success |
| `setStereoExposure(unsigned expus)` | Set stereo exposure time | `expus` microseconds, clamped into the valid range | `>0` success |
| `getStereoExposure(unsigned& expus)` | Read stereo exposure time | `expus` output in microseconds | `>0` success |
| `setStereoGain(float gain)` | Set stereo gain | `gain` in gain_128 units: `128` = 1x, `2048` = 16x | `>0` success |
| `getStereoGain(float& gain)` | Read stereo gain | `gain` output, same unit | `>0` success |
| `setStereoMaxExposure(unsigned expus)` | Set the exposure upper bound | `expus` microseconds, clamped to `[100, frame-rate limit]` | `>0` success |
| `getStereoMaxExposure(unsigned& expus)` | Read the exposure upper bound | `expus` output in microseconds | `>0` success |
| `setStereoMinExposure(unsigned expus)` | Set the exposure lower bound | `expus` microseconds | `>0` success |
| `getStereoMinExposure(unsigned& expus)` | Read the exposure lower bound | `expus` output in microseconds | `>0` success |
| `setStereoMaxGain(float gain)` | Set the gain upper bound | `gain` clamped to `[128, 2048]` | `>0` success |
| `getStereoMaxGain(float& gain)` | Read the gain upper bound | `gain` output | `>0` success |
| `setStereoMinGain(float gain)` | Set the gain lower bound | `gain` clamped to `[128, 2048]` | `>0` success |
| `getStereoMinGain(float& gain)` | Read the gain lower bound | `gain` output | `>0` success |

## 6. RGB exposure and gain

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setRGBAutoExpo(int onoff)` | Toggle RGB auto exposure | `onoff` `1` on / `0` off | `>0` success |
| `setRGBAutoExpoDesiredBin(int brightness)` | Set the auto-exposure target brightness | `brightness` target brightness value | `>0` success |
| `setRGBExposure(unsigned expus)` | Set RGB exposure time | `expus` microseconds | `>0` success |
| `getRGBExposure(unsigned& expus)` | Read RGB exposure time | `expus` output in microseconds | `>0` success |
| `setRGBGain(float gain)` | Set RGB gain | `gain` in gain_128 units | `>0` success |
| `getRGBGain(float& gain)` | Read RGB gain | `gain` output | `>0` success |
| `setRGBMaxExposure(unsigned expus)` | Set the exposure upper bound | `expus` microseconds | `>0` success |
| `getRGBMaxExposure(unsigned& expus)` | Read the exposure upper bound | `expus` output | `>0` success |
| `setRGBMinExposure(unsigned expus)` | Set the exposure lower bound | `expus` microseconds | `>0` success |
| `getRGBMinExposure(unsigned& expus)` | Read the exposure lower bound | `expus` output | `>0` success |
| `setRGBMaxGain(float gain)` | Set the gain upper bound | `gain` clamped to `[128, 2048]` | `>0` success |
| `getRGBMaxGain(float& gain)` | Read the gain upper bound | `gain` output | `>0` success |
| `setRGBMinGain(float gain)` | Set the gain lower bound | `gain` clamped to `[128, 2048]` | `>0` success |
| `getRGBMinGain(float& gain)` | Read the gain lower bound | `gain` output | `>0` success |

## 7. Image quality

### 7.1 Color adjustment

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setStereoColorAdjust(bool enable, unsigned brightness, unsigned contrast, unsigned saturation, unsigned hue)` | Set stereo brightness / contrast / saturation / hue | `enable` master switch for color adjustment; the other four are `[0,100]` with `50` as neutral | `>0` success |
| `getStereoColorAdjust(bool& enable, unsigned& brightness, unsigned& contrast, unsigned& saturation, unsigned& hue)` | Read stereo color parameters | five outputs | `>0` success |
| `setRGBColorAdjust(...)` | Set RGB color parameters | same as above | `>0` success |
| `getRGBColorAdjust(...)` | Read RGB color parameters | same as above | `>0` success |

### 7.2 Sharpening

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setStereoSharpen(bool enable, unsigned strength)` | Set stereo sharpening | `enable` sharpening switch; `strength` in `[0,100]`, where **`50` = the factory-tuned strength**, `0` no sharpening, `100` roughly twice the factory value | `>0` success |
| `getStereoSharpen(bool& enable, unsigned& strength, unsigned& textureStrength, unsigned& edgeStrength)` | Read stereo sharpening | first two as above; `textureStrength` / `edgeStrength` are the first entries of the actual ISP curves, for diagnostics only | `>0` success |
| `setRGBSharpen(bool enable, unsigned strength)` | Set RGB sharpening | same as above | `>0` success |
| `getRGBSharpen(...)` | Read RGB sharpening | same as above | `>0` success |

> Sharpening is disabled by default.

### 7.3 White balance

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setStereoAutoWb(bool autoMode)` | Switch stereo white balance auto/manual | `autoMode` `true` automatic (AWB), `false` manual | `>0` success |
| `getStereoAutoWb(bool& autoMode)` | Read the stereo white balance mode | `autoMode` output | `>0` success |
| `setStereoWbGain(float rGain, float gGain, float bGain)` | Set stereo white balance gains | three gains as **multipliers**: `1.0f` = 1x. Range `[0, 15.996]` | `>0` success |
| `getStereoWbGain(float& rGain, float& gGain, float& bGain)` | Read stereo white balance gains | three outputs, as multipliers | `>0` success |
| `setRGBAutoWb(bool autoMode)` | Switch RGB white balance auto/manual | same as above | `>0` success |
| `getRGBAutoWb(bool& autoMode)` | Read the RGB white balance mode | same as above | `>0` success |
| `setRGBWbGain(float rGain, float gGain, float bGain)` | Set RGB white balance gains | same as above | `>0` success |
| `getRGBWbGain(...)` | Read RGB white balance gains | same as above | `>0` success |

## 8. Calibration data

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setStereoCalibData(unsigned char* data, int len)` | Write stereo calibration data | `data` calibration blob; `len` byte count, default `calib::kStereoCalibBytes` | `>0` success |
| `getStereoCalibData(unsigned char* data, int len)` | Read stereo calibration data | same as above | `>0` success |
| `setRGBCalibData(unsigned char* data, int len)` | Write RGB calibration data | `len` defaults to `calib::kRgbCalibBytes` | `>0` success |
| `getRGBCalibData(unsigned char* data, int len)` | Read RGB calibration data | same as above | `>0` success |

Besides the byte-array interfaces there are struct overloads taking `calib::StereoCalibration` / `calib::RgbCalibration` from `Simou3CalibLayout.h`. The SDK packs the struct into the float array using the calibration layout and calls the byte interfaces above, so the return values are the same.

| Interface | Function | Returns |
| --- | --- | --- |
| `setStereoCalibData(const calib::StereoCalibration& c)` | Write stereo calibration (struct) | `>0` success |
| `getStereoCalibData(calib::StereoCalibration& c)` | Read stereo calibration (struct) | `>0` success |
| `setRGBCalibData(const calib::RgbCalibration& c)` | Write RGB calibration (struct) | `>0` success |
| `getRGBCalibData(calib::RgbCalibration& c)` | Read RGB calibration (struct) | `>0` success |

Struct fields:

- `Intrinsics`: `fx` `fy` `cx` `cy`, in pixels.
- `ImuCalibration`: `gyroNoiseDensity` `gyroRandomWalk` `accelNoiseDensity` `accelRandomWalk` are noise parameters; `timeshiftMs` is the IMU-to-camera time offset in milliseconds; `rotation` is a 3x3 rotation, row-major; `translationMm` is the translation in millimetres.
- `StereoCamera` (one eye): `intrinsics` raw intrinsics; `inverseRectify` is the 3x3 inverse rectification rotation, row-major; `distortion` holds 8 distortion coefficients.
- `StereoCalibration`: `left` / `right` are `StereoCamera`; `rectified` holds the rectified intrinsics; `negativeBaselineMm` is the negative baseline length in millimetres; `imu` is an `ImuCalibration`.
- `RgbCalibration`: `intrinsics` `inverseRectify` `distortion` `rectified` as above; `registeredTranslation` is the registration translation in millimetres; `imu` is an `ImuCalibration`; `reserved` / `reserved46` / `payloadRotation` are reserved or fixed fields: write back what you read (`payloadRotation` defaults to identity).

## 9. Frame rate and downsampling

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setFrameRate(int fps)` | Set the frame rate | `fps` target rate, valid range `[1,25]`; values outside it are ignored by the camera and the old value is kept (still returns success); the effective ceiling is bounded by the subscription and the downsampling state | `>0` success |
| `getFrameRate(int& fps)` | Read the frame rate | `fps` output | `>0` success |
| `setDownsampleMode(int mode)` | Select the downsampling stage | `0` pre-downsample (before the algorithm) / `1` post-downsample (resize before sending) | `>0` success |
| `setDepthDownsample(bool onoff)` | Toggle depth downsampling | `onoff` `true` enables | `>0` success |
| `setRGBDownsample(bool onoff)` | Toggle RGB downsampling | `onoff` `true` enables | `>0` success |
| `setStereoDownsample(bool onoff)` | Toggle stereo downsampling | `onoff` `true` enables | `>0` success |
| `setChunkSize(int bytes)` | Set the TCP chunk size | `bytes` clamped to `[1024, 262144]` | `>0` success |
| `setRegistrationSwitch(bool onoff)` | Toggle depth-to-color registration | `onoff` `true` enables registration | `>0` success |

> `setStereoDownsample` triggers an internal reconfiguration on the camera; call it last within a group of downsampling settings.

## 10. ROI

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setRoi(RoiStream stream, bool enable, unsigned x1, unsigned y1, unsigned x2, unsigned y2)` | Set the crop window of one stream | `stream` one of the six `RoiStream` values; coordinates are ignored when `enable` is `false`; coordinates are **0-based, top-left inclusive, bottom-right exclusive**, must be even, and the window must not be smaller than 16x16 | `>0` success / `-101` invalid arguments / `-102` rejected by the camera |
| `getRoi(RoiStream stream, bool& enable, unsigned& x1, unsigned& y1, unsigned& x2, unsigned& y2)` | Read the crop window of one stream | outputs the values currently in effect | `>0` success |

> Depth does not support ROI.
> Detection boxes (SEG) are still in full-frame coordinates; subtract the ROI origin before drawing.

## 11. DOE projector

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setDOEPower(unsigned power)` | Set the DOE power | `power` in `[0,255]`, values above are treated as `255`; `0` turns it off, otherwise every 32 is one step, 8 steps in total, and a higher step opens more channels | `>0` success |
| `getDOEPower(unsigned& power)` | Read the DOE power | `power` output, the last value set in `[0,255]` | `>0` success |

> A passive P-type unit has no DOE and ignores the request.

## 12. Network configuration

| API | Function | Return |
| --- | --- | --- |
| GetNetworkConfig(NetworkConfig& config) | Read current eth0 IPv4 address, mask and gateway | >0 success |
| SetNetworkConfig(const NetworkConfig& config) | Validate, persist and dynamically apply the complete group | >0 accepted and saved |
| getMacAddress(uint8_t mac[6]) / getMacAddress(std::string& mac) | Read current MAC | >0 success |

NetworkConfig has string fields ip, netmask and gateway. Submit all three. Only static IPv4 is supported. Masks must be contiguous; network, broadcast, loopback and multicast host addresses are rejected. A nonzero gateway must be a valid host in the same subnet and differ from the camera IP; 0.0.0.0 removes the default gateway. DHCP, DNS and MAC writes are not exposed.

After acknowledgement the camera disconnects ports 5001/5002, completes image cleanup and applies the address and route. Close the old session, create a camera object at the new IP, read back configuration and acquire a frame to confirm application. Unchanged running values are saved without disconnecting. Application failure attempts to restore the previous network and file; acknowledgement does not guarantee later application.

Files: /home/Param/network.conf is dynamic; /home/Param/network.default is the production fallback. Startup fully validates the dynamic file and falls back on absence or corruption. SDK atomically replaces only the dynamic file. Default is 192.168.1.70/24 with gateway 192.168.1.1; the device MAC is preserved.

Errors: -105 invalid input, -106 storage failure, -107 unavailable information, -108 configuration busy. Connection errors retain the <=0 convention. A timeout or disconnection is not proof of application; verify at the new address.

## 13. Camera discovery

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `scanSingleCamera(std::string ip, CameraInfo& cameraInfo)` | Probe the camera at a given address | `ip` target; `cameraInfo` output (ip, mac, gateway, netmask) | `0` success, `<0` failure (timeout or no reply) |
| `scanAllCameras(std::string broadcastIP)` | Scan one broadcast domain | `broadcastIP` broadcast address, default `255.255.255.255` | array of camera info |
| `scanAllNetworkInterfaces()` | Scan across every local network interface | none | array of camera info |
| `setScanTimeout(int timeoutMS)` | Set the scan timeout | `timeoutMS` milliseconds | none |
| `setScanPort(int port)` | Set the scan port | `port`, default 5004 | none |
| `scanCameras(...)` (static) | Static broadcast scan | `broadcastIP` / `port` / `timeoutMS` | array of camera info |
| `scanCamerasAllInterfaces(...)` (static) | Static all-interface scan | `port` / `timeoutMS` | array of camera info |

> On a multi-homed machine prefer `scanAllNetworkInterfaces()`: it broadcasts from each interface in turn, so it does not only find cameras on the default-route subnet.

## 14. SDK version

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `simou3_get_version(int* major, int* minor, int* patch)` | Read the SDK version numbers | three outputs; any of them may be `nullptr` | none |
| `simou3_get_version_string()` | Read the SDK version string | none | e.g. `"0.5.1"` |

Both are free functions in the `movesense` namespace, not members of `Simou3Camera`.
