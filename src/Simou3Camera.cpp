// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "Simou3Camera.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"

#include <chrono>
#include <cstring>

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

int Simou3Camera::setTriggerOut(int out)
{
    return mSimou3CameraSettings.setTriggerOut(out);
}
int Simou3Camera::getTriggerOut(int& out)
{
    return mSimou3CameraSettings.getTriggerOut(out);
}
int Simou3Camera::triggerFrame(int frameCnt)
{
    return mSimou3CameraSettings.triggerFrame(frameCnt);
}
int Simou3Camera::getFirmwareVersion(unsigned& firmware)
{
    return mSimou3CameraSettings.getFirmwareVersion(firmware);
}
int Simou3Camera::updateFirmware(std::string firmwareName)
{
    return mSimou3CameraSettings.updateFirmware(firmwareName);
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

int Simou3Camera::setDepthSwitch(bool onoff)
{
    return mSimou3CameraSettings.setDepthSwitch(onoff);
}

int Simou3Camera::setRegistrationSwitch(bool onoff)
{
    return mSimou3CameraSettings.setRegistrationSwitch(onoff);
}

int Simou3Camera::setIPAddress(uint32_t ip, uint32_t mask, uint32_t gateway)
{
    return mSimou3CameraSettings.setIPAddress(ip, mask, gateway);
}

int Simou3Camera::getIPAddress(uint32_t& ip, uint32_t& mask, uint32_t& gateway)
{
    return mSimou3CameraSettings.getIPAddress(ip, mask, gateway);
}

int Simou3Camera::setIPAddress(std::string ip, std::string mask, std::string gateway)
{
    return mSimou3CameraSettings.setIPAddress(ip, mask, gateway);
}

int Simou3Camera::getIPAddress(std::string& ip, std::string& mask, std::string& gateway)
{
    return mSimou3CameraSettings.getIPAddress(ip, mask, gateway);
}

int Simou3Camera::setMacAddress(uint8_t mac[6])
{
    return mSimou3CameraSettings.setMacAddress(mac);
}

int Simou3Camera::getMacAddress(uint8_t mac[6])
{
    return mSimou3CameraSettings.getMacAddress(mac);
}

int Simou3Camera::setMacAddress(std::string mac)
{
    return mSimou3CameraSettings.setMacAddress(mac);
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

int Simou3Camera::getFPGAVersion(std::string& fpga)
{
    return mSimou3CameraSettings.getFPGAVersion(fpga);
}

int Simou3Camera::setFPGAVersion(const std::string& fpga)
{
    return mSimou3CameraSettings.setFPGAVersion(fpga);
}

int Simou3Camera::getHardwareVersion(std::string& hardware)
{
    return mSimou3CameraSettings.getHardwareVersion(hardware);
}

int Simou3Camera::setHardwareVersion(const std::string& hardware)
{
    return mSimou3CameraSettings.setHardwareVersion(hardware);
}

int Simou3Camera::getProductModelVersion(std::string& product)
{
    return mSimou3CameraSettings.getProductModelVersion(product);
}

int Simou3Camera::setProductModelVersion(const std::string& product)
{
    return mSimou3CameraSettings.setProductModelVersion(product);
}

int Simou3Camera::getAppVersion(std::string& version)
{
    return mSimou3CameraSettings.getAppVersion(version);
}

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
int Simou3Camera::getIMU(std::vector<IMU>& out, int num)
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
