// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "movesense/Simou3Camera.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"

#include <chrono>
#include <cstring>

namespace movesense {

static int64_t nowWallUs()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool Simou3Camera::isCameraOnline()
{
    return isCameraOnline(mIp);
}

Simou3Camera::Simou3Camera(std::string ip) : mSimou3CameraTransfer(ip, 5001), mSimou3CameraSettings(ip, 5002), mSimou3CameraScan(5004)
{
    Simou3Net::globalInit();
    mIp = ip;
}

Simou3Camera::~Simou3Camera()
{
    closeCamera();
}

int Simou3Camera::openCamera(int mode)
{
    mTransferMode = mode;
    if (!mSimou3CameraTransfer.connectToCamera(mTransferMode)) {
        simou3_log("openCamera: connect data stream failed");
        return -1;
    }
    if (!mSimou3CameraTransfer.startChunkRecv()) {
        simou3_log("openCamera: start receive failed");
        mSimou3CameraTransfer.stopChunkRecv();
        return -1;
    }
    return 1;
}

int Simou3Camera::closeCamera()
{
    mSimou3CameraTransfer.stopChunkRecv();
    return 1;
}

int Simou3Camera::openCameraSettings()
{
    int ret = mSimou3CameraSettings.connectToCamera();
    return ret;
}

int Simou3Camera::closeCameraSettings()
{
    return mSimou3CameraSettings.disconnectCamera();
}

int Simou3Camera::setTriggerMode(int mode)
{
    return mSimou3CameraSettings.setTriggerMode(mode);
}

int Simou3Camera::getTriggerMode(int& mode)
{
    return mSimou3CameraSettings.getTriggerMode(mode);
}
int Simou3Camera::getCameraType(int& type)
{
    return mSimou3CameraSettings.getCameraType(type);
}
int Simou3Camera::getDepthUnitMm(double& unitMm)
{
    return mSimou3CameraSettings.getDepthUnitMm(unitMm);
}

int Simou3Camera::setTriggerOut(int out)
{
    return mSimou3CameraSettings.setTriggerOut(out);
}
int Simou3Camera::getTriggerOut(int& out)
{
    return mSimou3CameraSettings.getTriggerOut(out);
}
int Simou3Camera::triggerFrame()
{
    return mSimou3CameraSettings.triggerFrame();
}
int Simou3Camera::setStereoExposure(unsigned expus)
{
    return mSimou3CameraSettings.setStereoExposure(expus);
}
int Simou3Camera::getStereoExposure(unsigned& expus)
{
    return mSimou3CameraSettings.getStereoExposure(expus);
}

int Simou3Camera::setStereoGain(float gain)
{
    return mSimou3CameraSettings.setStereoGain(gain);
}
int Simou3Camera::getStereoGain(float& gain)
{
    return mSimou3CameraSettings.getStereoGain(gain);
}

int Simou3Camera::setStereoAutoExpo(int onoff)
{
    return mSimou3CameraSettings.setStereoAutoExpo(onoff);
}
int Simou3Camera::setStereoAutoExpoDesiredBin(int brightness)
{
    return mSimou3CameraSettings.setStereoAutoExpoDesiredBin(brightness);
}

int Simou3Camera::setStereoMaxExposure(unsigned expus)
{
    return mSimou3CameraSettings.setStereoMaxExposure(expus);
}
int Simou3Camera::getStereoMaxExposure(unsigned& expus)
{
    return mSimou3CameraSettings.getStereoMaxExposure(expus);
}

int Simou3Camera::setStereoMinExposure(unsigned expus)
{
    return mSimou3CameraSettings.setStereoMinExposure(expus);
}
int Simou3Camera::getStereoMinExposure(unsigned& expus)
{
    return mSimou3CameraSettings.getStereoMinExposure(expus);
}

int Simou3Camera::setStereoMaxGain(float gain)
{
    return mSimou3CameraSettings.setStereoMaxGain(gain);
}
int Simou3Camera::getStereoMaxGain(float& gain)
{
    return mSimou3CameraSettings.getStereoMaxGain(gain);
}

int Simou3Camera::setStereoMinGain(float gain)
{
    return mSimou3CameraSettings.setStereoMinGain(gain);
}
int Simou3Camera::getStereoMinGain(float& gain)
{
    return mSimou3CameraSettings.getStereoMinGain(gain);
}

int Simou3Camera::setRGBAutoExpo(int onoff)
{
    return mSimou3CameraSettings.setRGBAutoExpo(onoff);
}
int Simou3Camera::setRGBAutoExpoDesiredBin(int brighness)
{
    return mSimou3CameraSettings.setRGBAutoExpoDesiredBin(brighness);
}
int Simou3Camera::setRGBExposure(unsigned expus)
{
    return mSimou3CameraSettings.setRGBExposure(expus);
}
int Simou3Camera::getRGBExposure(unsigned& expus)
{
    return mSimou3CameraSettings.getRGBExposure(expus);
}
int Simou3Camera::setRGBGain(float gain)
{
    return mSimou3CameraSettings.setRGBGain(gain);
}
int Simou3Camera::getRGBGain(float& gain)
{
    return mSimou3CameraSettings.getRGBGain(gain);
}
int Simou3Camera::setRGBMaxExposure(unsigned expus)
{
    return mSimou3CameraSettings.setRGBMaxExposure(expus);
}
int Simou3Camera::getRGBMaxExposure(unsigned& expus)
{
    return mSimou3CameraSettings.getRGBMaxExposure(expus);
}
int Simou3Camera::setRGBMinExposure(unsigned expus)
{
    return mSimou3CameraSettings.setRGBMinExposure(expus);
}
int Simou3Camera::getRGBMinExposure(unsigned& expus)
{
    return mSimou3CameraSettings.getRGBMinExposure(expus);
}
int Simou3Camera::setRGBMaxGain(float gain)
{
    return mSimou3CameraSettings.setRGBMaxGain(gain);
}
int Simou3Camera::getRGBMaxGain(float& gain)
{
    return mSimou3CameraSettings.getRGBMaxGain(gain);
}
int Simou3Camera::setRGBMinGain(float gain)
{
    return mSimou3CameraSettings.setRGBMinGain(gain);
}
int Simou3Camera::getRGBMinGain(float& gain)
{
    return mSimou3CameraSettings.getRGBMinGain(gain);
}

int Simou3Camera::setDOEPower(unsigned power)
{
    return mSimou3CameraSettings.setDOEPower(power);
}
int Simou3Camera::getDOEPower(unsigned& power)
{
    return mSimou3CameraSettings.getDOEPower(power);
}
int Simou3Camera::setRoi(RoiStream stream, bool enable, unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
    return mSimou3CameraSettings.setRoi(static_cast<unsigned>(stream), enable, x1, y1, x2, y2);
}
int Simou3Camera::getRoi(RoiStream stream, bool& enable, unsigned& x1, unsigned& y1, unsigned& x2, unsigned& y2)
{
    return mSimou3CameraSettings.getRoi(static_cast<unsigned>(stream), enable, x1, y1, x2, y2);
}

int Simou3Camera::getStereoColorAdjust(bool& enable, unsigned& brightness, unsigned& contrast, unsigned& saturation, unsigned& hue)
{
    return mSimou3CameraSettings.getColorAdjust(IspChannel::Stereo, enable, brightness, contrast, saturation, hue);
}

int Simou3Camera::getRGBColorAdjust(bool& enable, unsigned& brightness, unsigned& contrast, unsigned& saturation, unsigned& hue)
{
    return mSimou3CameraSettings.getColorAdjust(IspChannel::Rgb, enable, brightness, contrast, saturation, hue);
}

int Simou3Camera::getStereoSharpen(bool& enable, unsigned& strength, unsigned& textureStrength, unsigned& edgeStrength)
{
    return mSimou3CameraSettings.getSharpen(IspChannel::Stereo, enable, strength, textureStrength, edgeStrength);
}

int Simou3Camera::getRGBSharpen(bool& enable, unsigned& strength, unsigned& textureStrength, unsigned& edgeStrength)
{
    return mSimou3CameraSettings.getSharpen(IspChannel::Rgb, enable, strength, textureStrength, edgeStrength);
}

int Simou3Camera::getStereoAutoWb(bool& autoMode)
{
    return mSimou3CameraSettings.getWhiteBalanceAuto(IspChannel::Stereo, autoMode);
}

int Simou3Camera::getRGBAutoWb(bool& autoMode)
{
    return mSimou3CameraSettings.getWhiteBalanceAuto(IspChannel::Rgb, autoMode);
}

int Simou3Camera::getStereoWbGain(float& rGain, float& gGain, float& bGain)
{
    return mSimou3CameraSettings.getWhiteBalanceGain(IspChannel::Stereo, rGain, gGain, bGain);
}

int Simou3Camera::getRGBWbGain(float& rGain, float& gGain, float& bGain)
{
    return mSimou3CameraSettings.getWhiteBalanceGain(IspChannel::Rgb, rGain, gGain, bGain);
}

int Simou3Camera::setStereoColorAdjust(bool enable, unsigned brightness, unsigned contrast, unsigned saturation, unsigned hue)
{
    return mSimou3CameraSettings.setColorAdjust(IspChannel::Stereo, enable, brightness, contrast, saturation, hue);
}

int Simou3Camera::setRGBColorAdjust(bool enable, unsigned brightness, unsigned contrast, unsigned saturation, unsigned hue)
{
    return mSimou3CameraSettings.setColorAdjust(IspChannel::Rgb, enable, brightness, contrast, saturation, hue);
}

int Simou3Camera::setStereoSharpen(bool enable, unsigned strength)
{
    return mSimou3CameraSettings.setSharpen(IspChannel::Stereo, enable, strength);
}

int Simou3Camera::setRGBSharpen(bool enable, unsigned strength)
{
    return mSimou3CameraSettings.setSharpen(IspChannel::Rgb, enable, strength);
}

int Simou3Camera::setStereoAutoWb(bool autoMode)
{
    return mSimou3CameraSettings.setWhiteBalanceAuto(IspChannel::Stereo, autoMode);
}

int Simou3Camera::setRGBAutoWb(bool autoMode)
{
    return mSimou3CameraSettings.setWhiteBalanceAuto(IspChannel::Rgb, autoMode);
}

int Simou3Camera::setStereoWbGain(float rGain, float gGain, float bGain)
{
    return mSimou3CameraSettings.setWhiteBalanceGain(IspChannel::Stereo, rGain, gGain, bGain);
}

int Simou3Camera::setRGBWbGain(float rGain, float gGain, float bGain)
{
    return mSimou3CameraSettings.setWhiteBalanceGain(IspChannel::Rgb, rGain, gGain, bGain);
}

int Simou3Camera::setStereoCalibData(unsigned char* data, int len)
{
    return mSimou3CameraSettings.setStereoCalibData(data, len);
}
int Simou3Camera::getStereoCalibData(unsigned char* data, int len)
{
    return mSimou3CameraSettings.getStereoCalibData(data, len);
}

int Simou3Camera::setRGBCalibData(unsigned char* data, int len)
{
    return mSimou3CameraSettings.setRGBCalibData(data, len);
}
int Simou3Camera::getRGBCalibData(unsigned char* data, int len)
{
    return mSimou3CameraSettings.getRGBCalibData(data, len);
}
namespace {

void packIntrinsics(const calib::Intrinsics& value, float* out)
{
    out[0] = value.fx;
    out[1] = value.fy;
    out[2] = value.cx;
    out[3] = value.cy;
}

void unpackIntrinsics(const float* in, calib::Intrinsics& value)
{
    value.fx = in[0];
    value.fy = in[1];
    value.cx = in[2];
    value.cy = in[3];
}

void packImu(const calib::ImuCalibration& value, float* out)
{
    out[calib::kImuGyroNoiseDensity] = value.gyroNoiseDensity;
    out[calib::kImuGyroRandomWalk] = value.gyroRandomWalk;
    out[calib::kImuAccelNoiseDensity] = value.accelNoiseDensity;
    out[calib::kImuAccelRandomWalk] = value.accelRandomWalk;
    out[calib::kImuTimeshiftMs] = value.timeshiftMs;
    memcpy(out + calib::kImuRotationRowMajor, value.rotation, sizeof(value.rotation));
    memcpy(out + calib::kImuTranslationMm, value.translationMm, sizeof(value.translationMm));
}

void unpackImu(const float* in, calib::ImuCalibration& value)
{
    value.gyroNoiseDensity = in[calib::kImuGyroNoiseDensity];
    value.gyroRandomWalk = in[calib::kImuGyroRandomWalk];
    value.accelNoiseDensity = in[calib::kImuAccelNoiseDensity];
    value.accelRandomWalk = in[calib::kImuAccelRandomWalk];
    value.timeshiftMs = in[calib::kImuTimeshiftMs];
    memcpy(value.rotation, in + calib::kImuRotationRowMajor, sizeof(value.rotation));
    memcpy(value.translationMm, in + calib::kImuTranslationMm, sizeof(value.translationMm));
}

void packStereoCamera(const calib::StereoCamera& value, float* out, int intrinsicsAt, int inverseAt, int distortionAt)
{
    packIntrinsics(value.intrinsics, out + intrinsicsAt);
    memcpy(out + inverseAt, value.inverseRectify, sizeof(value.inverseRectify));
    memcpy(out + distortionAt, value.distortion, sizeof(value.distortion));
}

void unpackStereoCamera(const float* in, calib::StereoCamera& value, int intrinsicsAt, int inverseAt, int distortionAt)
{
    unpackIntrinsics(in + intrinsicsAt, value.intrinsics);
    memcpy(value.inverseRectify, in + inverseAt, sizeof(value.inverseRectify));
    memcpy(value.distortion, in + distortionAt, sizeof(value.distortion));
}

} // namespace

int Simou3Camera::setStereoCalibData(const calib::StereoCalibration& calibration)
{
    float data[calib::kStereoCalibFloats] = { 0.0f };
    packStereoCamera(calibration.left, data, calib::kStereoLeftIntrinsics, calib::kStereoLeftInverseRectifyRowMajor,
        calib::kStereoLeftDistortion);
    packStereoCamera(calibration.right, data, calib::kStereoRightIntrinsics, calib::kStereoRightInverseRectifyRowMajor,
        calib::kStereoRightDistortion);
    data[calib::kRectifiedFx] = calibration.rectified.fx;
    data[calib::kRectifiedFy] = calibration.rectified.fy;
    data[calib::kRectifiedCx] = calibration.rectified.cx;
    data[calib::kRectifiedCy] = calibration.rectified.cy;
    data[calib::kStereoNegativeBaselineMm] = calibration.negativeBaselineMm;
    packImu(calibration.imu, data + calib::kStereoImuSegment);

    return setStereoCalibData(reinterpret_cast<unsigned char*>(data), calib::kStereoCalibBytes);
}

int Simou3Camera::getStereoCalibData(calib::StereoCalibration& calibration)
{
    float data[calib::kStereoCalibFloats] = { 0.0f };
    const int ret = getStereoCalibData(reinterpret_cast<unsigned char*>(data), calib::kStereoCalibBytes);
    if (ret <= 0) {
        return ret;
    }

    unpackStereoCamera(data, calibration.left, calib::kStereoLeftIntrinsics, calib::kStereoLeftInverseRectifyRowMajor,
        calib::kStereoLeftDistortion);
    unpackStereoCamera(data, calibration.right, calib::kStereoRightIntrinsics, calib::kStereoRightInverseRectifyRowMajor,
        calib::kStereoRightDistortion);
    calibration.rectified.fx = data[calib::kRectifiedFx];
    calibration.rectified.fy = data[calib::kRectifiedFy];
    calibration.rectified.cx = data[calib::kRectifiedCx];
    calibration.rectified.cy = data[calib::kRectifiedCy];
    calibration.negativeBaselineMm = data[calib::kStereoNegativeBaselineMm];
    unpackImu(data + calib::kStereoImuSegment, calibration.imu);

    return ret;
}

int Simou3Camera::setRGBCalibData(const calib::RgbCalibration& calibration)
{
    float data[calib::kRgbCalibFloats] = { 0.0f };
    memcpy(data, calibration.reserved, sizeof(calibration.reserved));
    packIntrinsics(calibration.intrinsics, data + calib::kRgbIntrinsics);
    memcpy(data + calib::kRgbInverseRectifyRowMajor, calibration.inverseRectify, sizeof(calibration.inverseRectify));
    memcpy(data + calib::kRgbDistortion, calibration.distortion, sizeof(calibration.distortion));
    data[calib::kRectifiedFx] = calibration.rectified.fx;
    data[calib::kRectifiedFy] = calibration.rectified.fy;
    data[calib::kRectifiedCx] = calibration.rectified.cx;
    data[calib::kRectifiedCy] = calibration.rectified.cy;
    data[calib::kRgbReserved46] = calibration.reserved46;
    memcpy(data + calib::kRgbRotationRowMajor, calibration.payloadRotation, sizeof(calibration.payloadRotation));
    memcpy(data + calib::kRgbTranslationMm, calibration.registeredTranslation, sizeof(calibration.registeredTranslation));
    packImu(calibration.imu, data + calib::kRgbImuSegment);

    return setRGBCalibData(reinterpret_cast<unsigned char*>(data), calib::kRgbCalibBytes);
}

int Simou3Camera::getRGBCalibData(calib::RgbCalibration& calibration)
{
    float data[calib::kRgbCalibFloats] = { 0.0f };
    const int ret = getRGBCalibData(reinterpret_cast<unsigned char*>(data), calib::kRgbCalibBytes);
    if (ret <= 0) {
        return ret;
    }

    memcpy(calibration.reserved, data, sizeof(calibration.reserved));
    unpackIntrinsics(data + calib::kRgbIntrinsics, calibration.intrinsics);
    memcpy(calibration.inverseRectify, data + calib::kRgbInverseRectifyRowMajor, sizeof(calibration.inverseRectify));
    memcpy(calibration.distortion, data + calib::kRgbDistortion, sizeof(calibration.distortion));
    calibration.rectified.fx = data[calib::kRectifiedFx];
    calibration.rectified.fy = data[calib::kRectifiedFy];
    calibration.rectified.cx = data[calib::kRectifiedCx];
    calibration.rectified.cy = data[calib::kRectifiedCy];
    calibration.reserved46 = data[calib::kRgbReserved46];
    memcpy(calibration.payloadRotation, data + calib::kRgbRotationRowMajor, sizeof(calibration.payloadRotation));
    memcpy(calibration.registeredTranslation, data + calib::kRgbTranslationMm, sizeof(calibration.registeredTranslation));
    unpackImu(data + calib::kRgbImuSegment, calibration.imu);

    return ret;
}

int Simou3Camera::setFrameRate(int fps)
{
    return mSimou3CameraSettings.setFrameRate(fps);
}
int Simou3Camera::getFrameRate(int& fps)
{
    return mSimou3CameraSettings.getFrameRate(fps);
}

int Simou3Camera::setDepthDownsample(bool onoff)
{
    return mSimou3CameraSettings.setDepthDownsample(onoff);
}
int Simou3Camera::setRGBDownsample(bool onoff)
{
    return mSimou3CameraSettings.setRGBDownsample(onoff);
}
int Simou3Camera::setStereoDownsample(bool onoff)
{
    return mSimou3CameraSettings.setStereoDownsample(onoff);
}
int Simou3Camera::setDownsampleMode(int mode)
{
    return mSimou3CameraSettings.setDownsampleMode(mode);
}
int Simou3Camera::setChunkSize(int bytes)
{
    return mSimou3CameraSettings.setChunkSize(bytes);
}

int Simou3Camera::setRegistrationSwitch(bool onoff)
{
    return mSimou3CameraSettings.setRegistrationSwitch(onoff);
}

int Simou3Camera::SetNetworkConfig(const NetworkConfig& config) { return mSimou3CameraSettings.SetNetworkConfig(config); }

int Simou3Camera::GetNetworkConfig(NetworkConfig& config) { return mSimou3CameraSettings.GetNetworkConfig(config); }

int Simou3Camera::getMacAddress(uint8_t mac[6])
{
    return mSimou3CameraSettings.getMacAddress(mac);
}

int Simou3Camera::getMacAddress(std::string& mac)
{
    return mSimou3CameraSettings.getMacAddress(mac);
}

int Simou3Camera::getCameraSN(std::string& sn)
{
    return mSimou3CameraSettings.getCameraSN(sn);
}

int Simou3Camera::setCameraSN(const std::string& sn)
{
    return mSimou3CameraSettings.setCameraSN(sn);
}

int Simou3Camera::getHardwareVersion(std::string& hardware)
{
    return mSimou3CameraSettings.getHardwareVersion(hardware);
}

int Simou3Camera::getProductModelVersion(std::string& product)
{
    return mSimou3CameraSettings.getProductModelVersion(product);
}

int Simou3Camera::getAppVersion(std::string& version) { return mSimou3CameraSettings.getAppVersion(version); }

int Simou3Camera::GetKernelVersion(std::string& version) { return mSimou3CameraSettings.GetKernelVersion(version); }

int Simou3Camera::GetCameraLibVersion(std::string& version) { return mSimou3CameraSettings.GetCameraLibVersion(version); }

bool Simou3Camera::isCameraOnline(std::string ip)
{
    CameraInfo cameraInfo;
    return (mSimou3CameraScan.scanCamera(ip, cameraInfo) == 0);
}

int Simou3Camera::scanSingleCamera(std::string ip, CameraInfo& cameraInfo)
{
    return mSimou3CameraScan.scanCamera(ip, cameraInfo);
}

std::vector<CameraInfo> Simou3Camera::scanAllCameras(std::string broadcastIP)
{
    return mSimou3CameraScan.scanBroadcast(broadcastIP);
}

std::vector<CameraInfo> Simou3Camera::scanAllNetworkInterfaces()
{
    return mSimou3CameraScan.scanAllInterfaces();
}

void Simou3Camera::setScanTimeout(int timeoutMS)
{
    mSimou3CameraScan.setTimeout(timeoutMS);
}

void Simou3Camera::setScanPort(int port)
{
    mSimou3CameraScan.setScanPort(port);
}

std::vector<CameraInfo> Simou3Camera::scanCameras(std::string broadcastIP, int port, int timeoutMS)
{
    Simou3CameraScan scanner(port, timeoutMS);
    return scanner.scanBroadcast(broadcastIP);
}

std::vector<CameraInfo> Simou3Camera::scanCamerasAllInterfaces(int port, int timeoutMS)
{
    Simou3CameraScan scanner(port, timeoutMS);
    return scanner.scanAllInterfaces();
}

bool Simou3Camera::getFrame(MovesenseFrame& out)
{
    return mSimou3CameraTransfer.getFrame(out);
}

bool Simou3Camera::getFrame(MovesenseFrame& out, int timeoutMs)
{
    return mSimou3CameraTransfer.getFrame(out, timeoutMs);
}

Simou3Stats Simou3Camera::getStats()
{
    return mSimou3CameraTransfer.getStats();
}

void Simou3Camera::setDropDegradedFrame(bool onoff)
{
    mSimou3CameraTransfer.setDropDegradedFrame(onoff);
}

int Simou3Camera::getIMU(std::vector<Imu>& out, int num)
{
    return mSimou3CameraTransfer.getIMU(out, num);
}

int Simou3Camera::alignTimeToHost()
{
    Simou3Net net;
    if (!net.openUdp(50)) {
        simou3_log("alignTimeToHost: open udp failed");
        return -1;
    }

    const int kProbes = 16;
    const int kPort = 5000;
    bool got = false;
    int64_t bestDelay = 0, bestOffset = 0;

    for (int i = 0; i < kProbes; i++) {
        unsigned char req[16] = { 0 };
        uint32_t magic = 0x53335453;
        memcpy(req, &magic, 4);
        memcpy(req + 4, &i, 4);

        int64_t t1 = nowWallUs();
        if (net.sendTo(mIp, kPort, req, sizeof(req)) <= 0) {
            continue;
        }
        unsigned char rep[16];
        int r = net.recvFrom(rep, sizeof(rep));
        int64_t t4 = nowWallUs();
        if (r < 16) {
            continue;
        }

        uint64_t t2 = 0, t3 = 0;
        memcpy(&t2, rep, 8);
        memcpy(&t3, rep + 8, 8);

        int64_t delay = (t4 - t1) - static_cast<int64_t>(t3 - t2);
        int64_t theta = (static_cast<int64_t>(t2 - t1) + static_cast<int64_t>(t3 - t4)) / 2;
        int64_t offset = -theta;

        if (!got || delay < bestDelay) {
            got = true;
            bestDelay = delay;
            bestOffset = offset;
        }
    }

    net.closeSock();

    if (!got) {
        simou3_log("alignTimeToHost: no valid reply from camera");
        return -1;
    }

    mSimou3CameraTransfer.setPtsOffset(bestOffset);
    simou3_log("alignTimeToHost: offset=" + std::to_string(bestOffset) + "us (min delay=" + std::to_string(bestDelay) + "us)");
    return 1;
}

} // namespace movesense
