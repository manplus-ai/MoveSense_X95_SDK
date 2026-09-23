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

Test for success with `ret > 0`, not with `ret == 0`.

### Call order

- Streaming interfaces (`openCamera` / `getFrame` / `getIMU`) use the data channel and require `openCamera` first
- Parameter interfaces use the control channel and require `openCameraSettings` first
- The two channels are independent and may be used separately

### Channels

`Stereo` refers to the stereo pair (left and right share one parameter set; both ISP pipes are written
internally); `RGB` refers to the color camera.
On a **passive P-type unit**, `setRGB*` / `getRGB*` return not-supported and never touch the hardware.

## 1. Connection and streaming

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `Simou3Camera(std::string ip)` | Construct a camera object | `ip` camera address, default `192.168.1.70` | — |
| `isCameraOnline()` | Probe whether the camera is online | none | `true` if online |
| `isCameraOnline(std::string ip)` | Probe whether the camera at a given address is online | `ip` target address | `true` if online |
| `openCamera(int mode)` | Open the data stream and start receiving | `mode` subscription mask, OR-ed from the `TRANSFER_MODE_*_BIT` values in `transfer_mode_def.h` | `>0` success |
| `closeCamera()` | Close the data stream | none | `>0` success |
| `openCameraSettings()` | Open the control channel (port 5002) | none | `>0` success |
| `closeCameraSettings()` | Close the control channel | none | `>0` success |

## 2. Frames and IMU

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `getFrame(MovesenseFrame& out)` | Fetch one frame (blocks until the default timeout) | `out` output frame | `true` if a frame was received |
| `getFrame(MovesenseFrame& out, int timeoutMs)` | Fetch one frame (explicit timeout) | `out` output frame; `timeoutMs` timeout in ms | `true` if a frame was received |
| `getStats()` | Read receive statistics | none | `Simou3Stats` struct |
| `setDropDegradedFrame(bool onoff)` | Set whether incomplete frames are discarded | `onoff` `true` drops partial frames | none |
| `getIMU(std::vector<Imu>& out, int num)` | Fetch IMU samples | `out` output vector; `num` max samples, default 200 | `>0` number of samples |
| `alignTimeToHost()` | Align the camera clock to the host | none | `>0` success |

## 3. Trigger

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setTriggerMode(int mode)` | **Not supported yet** | — | `-100` |
| `getTriggerMode(int& mode)` | **Not supported yet** | — | `-100` |
| `setTriggerOut(int out)` | **Not supported yet** | — | `-100` |
| `getTriggerOut(int& out)` | **Not supported yet** | — | `-100` |
| `triggerFrame(int frameCnt)` | **Not supported yet** | — | `-100` |

## 4. Device information

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `getCameraType(int& type)` | Read the model | `type` output: `1` = passive P-type (no center camera), otherwise the unit has one | `>0` success |
| `getAppVersion(std::string& version)` | Read the application version | `version` output string | `>0` success |
| `getCameraSN(std::string& sn)` | Read the serial number | `sn` output string | `>0` success |
| `getFPGAVersion(std::string& fpga)` | Read the FPGA version | `fpga` output string | `>0` success |
| `getFirmwareVersion(unsigned& firmware)` | **Not supported yet** | — | `-100` |
| `updateFirmware(std::string firmwareName)` | **Not supported yet** | — | `-100` |
| `setCameraSN(const std::string& sn)` | **Not supported yet** | — | `-100` |
| `setFPGAVersion(const std::string& fpga)` | **Not supported yet** | — | `-100` |
| `setHardwareVersion(const std::string& hardware)` | **Not supported yet** | — | `-100` |
| `getHardwareVersion(std::string& hardware)` | **Not supported yet** | — | `-100` |
| `setProductModelVersion(const std::string& product)` | **Not supported yet** | — | `-100` |
| `getProductModelVersion(std::string& product)` | **Not supported yet** | — | `-100` |

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

## 9. Frame rate and downsampling

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `setFrameRate(int fps)` | Set the frame rate | `fps` target rate, bounded by the subscription and the downsampling state | `>0` success |
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

## 11. Network configuration

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `getIPAddress(uint32_t& ip, uint32_t& mask, uint32_t& gateway)` | Read the IP settings (integer form) | three outputs, network byte order | `>0` success |
| `getIPAddress(std::string& ip, std::string& mask, std::string& gateway)` | Read the IP settings (dotted form) | three outputs | `>0` success |
| `getMacAddress(uint8_t mac[6])` | Read the MAC address (byte array) | `mac` 6-byte output | `>0` success |
| `getMacAddress(std::string& mac)` | Read the MAC address (string) | `mac` output | `>0` success |
| `setIPAddress(...)` | **Not supported yet** | — | `-100` |
| `setMacAddress(...)` | **Not supported yet** | — | `-100` |

## 12. Camera discovery

| Interface | Function | Parameters | Returns |
| --- | --- | --- | --- |
| `scanSingleCamera(std::string ip, CameraInfo& cameraInfo)` | Probe the camera at a given address | `ip` target; `cameraInfo` output info | `>0` success |
| `scanAllCameras(std::string broadcastIP)` | Scan one broadcast domain | `broadcastIP` broadcast address, default `255.255.255.255` | array of camera info |
| `scanAllNetworkInterfaces()` | Scan across every local network interface | none | array of camera info |
| `setScanTimeout(int timeoutMS)` | Set the scan timeout | `timeoutMS` milliseconds | none |
| `setScanPort(int port)` | Set the scan port | `port`, default 5004 | none |
| `scanCameras(...)` (static) | Static broadcast scan | `broadcastIP` / `port` / `timeoutMS` | array of camera info |
| `scanCamerasAllInterfaces(...)` (static) | Static all-interface scan | `port` / `timeoutMS` | array of camera info |

> On a multi-homed machine prefer `scanAllNetworkInterfaces()`: it broadcasts from each interface in turn, so it does not only find cameras on the default-route subnet.
