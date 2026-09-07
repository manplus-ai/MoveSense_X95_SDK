// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "Simou3CameraScan.h"
#include "Simou3CameraSettings.h"
#include "Simou3CameraTransfer.h"
#include "Simou3Types.h"

#include <string>
#include <vector>

namespace movesense {

class Simou3Camera {
public:
    Simou3Camera(std::string ip = "192.168.1.70");
    ~Simou3Camera();

    bool isCameraOnline();
    int openCamera(int mode);
    int closeCamera();

    bool getFrame(MovesenseFrame& out);
    bool getFrame(MovesenseFrame& out, int timeoutMs);
    Simou3Stats getStats();
    int getIMU(std::vector<Imu>& out, int num = 200);
    int alignTimeToHost();

    int openCameraSettings();
    int closeCameraSettings();

    int setTriggerMode(int mode);
    int getTriggerMode(int& mode);
    int setTriggerOut(int out);
    int getTriggerOut(int& out);
    int triggerFrame(int frameCnt);

    int getFirmwareVersion(unsigned& firmware);
    int getCameraType(int& type);
    int updateFirmware(std::string firmwareName);
    int setStereoExposure(unsigned expus);
    int getStereoExposure(unsigned& expus);

    int setStereoGain(float gain);
    int getStereoGain(float& gain);

    int setStereoMaxExposure(unsigned expus);
    int getStereoMaxExposure(unsigned& expus);
    int setStereoMinExposure(unsigned expus);
    int getStereoMinExposure(unsigned& expus);
    int setStereoMaxGain(float gain);
    int getStereoMaxGain(float& gain);
    int setStereoMinGain(float gain);
    int getStereoMinGain(float& gain);

    int setStereoAutoExpo(int onoff);
    int setStereoAutoExpoDesiredBin(int brightness);

    int setRGBAutoExpo(int onoff);
    int setRGBAutoExpoDesiredBin(int brighness);
    int setRGBExposure(unsigned expus);
    int getRGBExposure(unsigned& expus);
    int setRGBGain(float gain);
    int getRGBGain(float& gain);
    int setRGBMaxExposure(unsigned expus);
    int getRGBMaxExposure(unsigned& expus);
    int setRGBMinExposure(unsigned expus);
    int getRGBMinExposure(unsigned& expus);
    int setRGBMaxGain(float gain);
    int getRGBMaxGain(float& gain);
    int setRGBMinGain(float gain);
    int getRGBMinGain(float& gain);

    int setDOEPower(unsigned power);
    int getDOEPower(unsigned& power);

    int setStereoCalibData(unsigned char* data, int len = 47 * 4);
    int getStereoCalibData(unsigned char* data, int len = 47 * 4);

    int setRGBCalibData(unsigned char* data, int len = 59 * 4);
    int getRGBCalibData(unsigned char* data, int len = 59 * 4);

    int setFrameRate(int fps);
    int getFrameRate(int& fps);
    int setDownsampleMode(int mode);

    int setDepthDownsample(bool onoff);
    int setRGBDownsample(bool onoff);
    int setStereoDownsample(bool onoff);

    int setChunkSize(int bytes);

    int setRegistrationSwitch(bool onoff);

    int setIPAddress(uint32_t ip, uint32_t mask, uint32_t gateway);
    int getIPAddress(uint32_t& ip, uint32_t& mask, uint32_t& gateway);

    int setIPAddress(std::string ip, std::string mask, std::string gateway);
    int getIPAddress(std::string& ip, std::string& mask, std::string& gateway);

    int setMacAddress(uint8_t mac[6]);
    int getMacAddress(uint8_t mac[6]);

    int setMacAddress(std::string mac);
    int getMacAddress(std::string& mac);

    int getAppVersion(std::string& version);
    int setCameraSN(const std::string& sn);
    int getCameraSN(std::string& sn);
    int setFPGAVersion(const std::string& fpga);
    int getFPGAVersion(std::string& fpga);
    int setHardwareVersion(const std::string& hardware);
    int getHardwareVersion(std::string& hardware);
    int setProductModelVersion(const std::string& product);
    int getProductModelVersion(std::string& product);

    bool isCameraOnline(std::string ip);

    int scanSingleCamera(std::string ip, CameraInfo& cameraInfo);

    std::vector<CameraInfo> scanAllCameras(std::string broadcastIP = "255.255.255.255");

    std::vector<CameraInfo> scanAllNetworkInterfaces();

    void setScanTimeout(int timeoutMS);

    void setScanPort(int port);

    static std::vector<CameraInfo> scanCameras(std::string broadcastIP = "255.255.255.255", int port = 5004, int timeoutMS = 2000);

    static std::vector<CameraInfo> scanCamerasAllInterfaces(int port = 5004, int timeoutMS = 2000);

private:
    Simou3CameraTransfer mSimou3CameraTransfer;
    Simou3CameraSettings mSimou3CameraSettings;
    Simou3CameraScan mSimou3CameraScan;

    std::string mIp;
    int mTransferMode;
};

void simou3_get_version(int* major, int* minor, int* patch);
const char* simou3_get_version_string();

} // namespace movesense
