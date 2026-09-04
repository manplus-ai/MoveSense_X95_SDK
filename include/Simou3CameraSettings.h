// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include <string>
#include <cstdint>
#include "Simou3Net.h"

class Simou3CameraSettings
{
public:
	Simou3CameraSettings(std::string ip = "192.168.1.70", int port = 5002);

	int connectToCamera();
	int disconnectCamera();

	int setTriggerMode(int mode);
	int getTriggerMode(int& mode);

	int setTriggerOut(int out);
	int getTriggerOut(int& out);

	int triggerFrame(int frameCnt);
	int getFirmwareVersion(unsigned &firmware);
	int getCameraType(int& type);
	int updateFirmware(std::string firmwareName);

	int setStereoExposure(unsigned expus);
	int getStereoExposure(unsigned &expus);

	int setStereoGain(float gain);
	int getStereoGain(float &gain);

	int setStereoMaxExposure(unsigned expus);
	int getStereoMaxExposure(unsigned &expus);
	int setStereoMinExposure(unsigned expus);
	int getStereoMinExposure(unsigned &expus);
	int setStereoMaxGain(float gain);
	int getStereoMaxGain(float &gain);
	int setStereoMinGain(float gain);
	int getStereoMinGain(float &gain);

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


	int setStereoCalibData(unsigned char* data, int len=47*4);
	int getStereoCalibData(unsigned char* data, int len=47*4);

	int setRGBCalibData(unsigned char* data, int len=59*4);
	int getRGBCalibData(unsigned char* data, int len=59*4);

	int setFrameRate(int fps);
	int getFrameRate(int& fps);
	int setDownsampleMode(int mode);

	int setDepthDownsample(bool onoff);
	int setRGBDownsample(bool onoff);
	int setStereoDownsample(bool onoff);

	int setChunkSize(int bytes);

	int setDepthSwitch(bool onoff);
	int setRegistrationSwitch(bool onoff);

	int setIPAddress(uint32_t ip, uint32_t mask, uint32_t gateway);
	int getIPAddress(uint32_t& ip, uint32_t& mask, uint32_t& gateway);

	int setIPAddress(std::string ip, std::string mask, std::string gateway);
	int getIPAddress(std::string& ip, std::string& mask, std::string& gateway);

	int setMacAddress(uint8_t mac[6]);
	int getMacAddress(uint8_t mac[6]);

	int setMacAddress(std::string mac);
	int getMacAddress(std::string& mac);

	int getAppVersion(uint8_t app[8]);
	int getAppVersion(std::string& app);

	int setCameraSN(const uint8_t sn[16]);
	int setCameraSN(const std::string& sn);

	int getCameraSN(uint8_t sn[16]);
	int getCameraSN(std::string& sn);

	int setFPGAVersion(const uint8_t fpga[8]);
	int setFPGAVersion(const std::string& fpga);

	int getFPGAVersion(uint8_t fpga[8]);
	int getFPGAVersion(std::string& fpga);

	int setHardwareVersion(const uint8_t hardware[8]);
	int setHardwareVersion(const std::string& hardware);

	int getHardwareVersion(uint8_t hardware[8]);
	int getHardwareVersion(std::string& hardware);

	int setProductModelVersion(const uint8_t product[16]);
	int setProductModelVersion(const std::string& product);

	int getProductModelVersion(uint8_t product[16]);
	int getProductModelVersion(std::string& product);

public:
	std::string mIP;
	int mPort;
private:
	Simou3Net mSock;
	int sendCmd(unsigned short cmdType, unsigned short len, const void * data);
	int sendGetCmd(unsigned short cmdType, unsigned short len);
};
