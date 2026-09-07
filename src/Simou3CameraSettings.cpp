// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "movesense/Simou3CameraSettings.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"
#include "crc.h"

#include <fstream>

Simou3CameraSettings::Simou3CameraSettings(std::string ip, int port) : mIP(ip), mPort(port) {}

int Simou3CameraSettings::connectToCamera()
{
    if (!mSock.connectTcp(mIP, mPort)) {
        return -1;
    }
    return 1;
}
int Simou3CameraSettings::disconnectCamera()
{
    mSock.closeSock();
    return 0;
}

int Simou3CameraSettings::setTriggerMode(int mode)
{
    unsigned short cmdType = CMD_TYPE_SET_TRIGGER_MODE;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &mode);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&mode, cmdLen);
    return ret;
}
int Simou3CameraSettings::getTriggerMode(int& mode)
{
    unsigned short cmdType = CMD_TYPE_GET_TRIGGER_MODE;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&mode, cmdLen);
    return ret;
}

int Simou3CameraSettings::getCameraType(int& type)
{
    unsigned short cmdType = CMD_TYPE_GET_CAMERA_TYPE;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&type, cmdLen);
    return ret;
}

int Simou3CameraSettings::setTriggerOut(int out)
{
    unsigned short cmdType = CMD_TYPE_SET_TRIGGER_OUT;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &out);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&out, cmdLen);
    return ret;
}
int Simou3CameraSettings::getTriggerOut(int& out)
{
    unsigned short cmdType = CMD_TYPE_GET_TRIGGER_OUT;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&out, cmdLen);
    return ret;
}

int Simou3CameraSettings::triggerFrame(int frameCnt)
{
    simou3_log("Simou3CameraSettings::triggerFrame: " + std::to_string(frameCnt));
    unsigned short cmdType = CMD_TYPE_TRIGGER_FRAME;
    unsigned short cmdLen = sizeof(int);
    int ret = sendCmd(cmdType, cmdLen, &frameCnt);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&frameCnt, cmdLen);
    return ret;
}

int Simou3CameraSettings::getFirmwareVersion(unsigned& firmware)
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_GET_FIRMWARE_VERSION;
    unsigned short cmdLen = sizeof(unsigned);

    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&firmware, cmdLen);
    return ret;
}

int Simou3CameraSettings::updateFirmware(std::string firmwareName)
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    std::ifstream inf_crc(firmwareName, std::ios::binary);
    if (!inf_crc.is_open()) {
        simou3_log("firmware file open failed.");
        return -1;
    }
    inf_crc.seekg(0, std::ios::end);
    std::streamoff fileLen = inf_crc.tellg();
    inf_crc.seekg(0, std::ios::beg);
    if (fileLen <= 0) {
        simou3_log("firmware file is empty.");
        return -1;
    }
    const std::streamoff MAX_FIRMWARE_BYTES = 64 * 1024 * 1024;
    if (fileLen > MAX_FIRMWARE_BYTES) {
        simou3_log("firmware file too large: " + std::to_string(static_cast<long long>(fileLen)));
        return -1;
    }
    int rawLen = static_cast<int>(fileLen);
    simou3_log("file len is " + std::to_string(rawLen));

    int updateLen = (rawLen + 255) / 256 * 256;
    std::vector<unsigned char> buffV(updateLen, 0xFF);
    unsigned char* buff = buffV.data();
    inf_crc.read(reinterpret_cast<char*>(buff), rawLen);
    std::streamsize gotLen = inf_crc.gcount();
    inf_crc.close();
    if (gotLen != rawLen) {
        simou3_log("firmware read incomplete: " + std::to_string(static_cast<long long>(gotLen)) + "/" + std::to_string(rawLen));
        return -1;
    }

    CrcCtx crcCtx;
    crc_init(&crcCtx);
    uint32_t crc = 0;
    for (int i = 0; i < updateLen / 256; i++) {
        crc = crc_add_buffer(&crcCtx, &buff[256 * i], 256);
    }
    simou3_log("file crc is " + std::to_string(crc));

    unsigned short cmdType = CMD_TYPE_UPDATE_HEADER;
    unsigned short cmdLen = 8;
    unsigned char updateHeader[8];
    memcpy(updateHeader, &updateLen, sizeof(unsigned));
    memcpy(updateHeader + 4, &crc, sizeof(unsigned));

    int ret = sendCmd(cmdType, cmdLen, &updateHeader);
    if (ret <= 0) {
        return ret;
    }

    {
        unsigned short cmdType = CMD_TYPE_UPDATE_ERASE;
        unsigned short cmdLen = 1;
        unsigned char cmdData[1] = { 0 };
        int ret = sendCmd(cmdType, cmdLen, cmdData);
        if (ret <= 0) {
            return -1;
        }
        ret = mSock.recvBlock(cmdData, 1);
        if (ret <= 0) {
            return -1;
        }
        if (cmdData[0] > 0) {
            simou3_log("erase done");
        }
    }
    {
        simou3_log("data transfer command");
        unsigned short cmdType = CMD_TYPE_UPDATE_DATA;
        unsigned short cmdLen = 1;
        unsigned char cmdData[1] = { 0 };
        int ret = sendCmd(cmdType, cmdLen, cmdData);
        if (ret <= 0) {
            return -1;
        }

        simou3_log("data transfer start");
        for (int i = 0; i < updateLen / 256; i++) {
            int ret = mSock.sendBlock(&(buff[256 * i]), 256);
            if (ret <= 0) {
                return -1;
            }
        }
        simou3_log("data transfer end");
        {
            int ret = mSock.recvBlock(cmdData, cmdLen);
            if (ret <= 0) {
                return -1;
            }
            if (cmdData[0] > 0) {
                simou3_log("update success.");
            } else {
                simou3_log("update failed.");
                return -1;
            }
        }
    }

    return 1;
}

int Simou3CameraSettings::setStereoExposure(unsigned expus)
{
    unsigned short cmdType = CMD_TYPE_SET_EXPOSURE_STEREO;
    unsigned short cmdLen = sizeof(unsigned);

    int ret = sendCmd(cmdType, cmdLen, &expus);
    if (ret <= 0) {
        return ret;
    }

    unsigned expusReal;
    ret = mSock.recvBlock(&expusReal, cmdLen);
    return ret;
}

int Simou3CameraSettings::getStereoExposure(unsigned& expus)
{
    unsigned short cmdType = CMD_TYPE_GET_EXPOSURE_STEREO;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoGain(float gain)
{
    unsigned short cmdType = CMD_TYPE_SET_GAIN_STEREO;
    unsigned short cmdLen = sizeof(float);
    int ret = sendCmd(cmdType, cmdLen, &gain);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::getStereoGain(float& gain)
{
    unsigned short cmdType = CMD_TYPE_GET_GAIN_STEREO;
    unsigned short cmdLen = sizeof(float);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoMaxExposure(unsigned expus)
{
    unsigned short cmdType = CMD_TYPE_SET_MAX_EXPOSURE_STEREO;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &expus);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::getStereoMaxExposure(unsigned& expus)
{
    unsigned short cmdType = CMD_TYPE_GET_MAX_EXPOSURE_STEREO;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoMinExposure(unsigned expus)
{
    unsigned short cmdType = CMD_TYPE_SET_MIN_EXPOSURE_STEREO;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &expus);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::getStereoMinExposure(unsigned& expus)
{
    unsigned short cmdType = CMD_TYPE_GET_MIN_EXPOSURE_STEREO;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoMaxGain(float gain)
{
    unsigned short cmdType = CMD_TYPE_SET_MAX_GAIN_STEREO;
    unsigned short cmdLen = sizeof(float);
    int ret = sendCmd(cmdType, cmdLen, &gain);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::getStereoMaxGain(float& gain)
{
    unsigned short cmdType = CMD_TYPE_GET_MAX_GAIN_STEREO;
    unsigned short cmdLen = sizeof(float);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoMinGain(float gain)
{
    unsigned short cmdType = CMD_TYPE_SET_MIN_GAIN_STEREO;
    unsigned short cmdLen = sizeof(float);
    int ret = sendCmd(cmdType, cmdLen, &gain);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::getStereoMinGain(float& gain)
{
    unsigned short cmdType = CMD_TYPE_GET_MIN_GAIN_STEREO;
    unsigned short cmdLen = sizeof(float);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoAutoExpo(int onoff)
{
    unsigned short cmdType = CMD_TYPE_AE_SWITCH_STEREO;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &onoff);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&onoff, cmdLen);
    return ret;
}
int Simou3CameraSettings::setStereoAutoExpoDesiredBin(int brightness)
{
    unsigned short cmdType = CMD_TYPE_AE_DESIRED_BIN_STEREO;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &brightness);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&brightness, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBAutoExpo(int onoff)
{
    unsigned short cmdType = CMD_TYPE_AE_SWITCH_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &onoff);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&onoff, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBAutoExpoDesiredBin(int brighness)
{
    unsigned short cmdType = CMD_TYPE_AE_DESIRED_BIN_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &brighness);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&brighness, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBExposure(unsigned expus)
{
    unsigned short cmdType = CMD_TYPE_SET_EXPOSURE_RGB;
    unsigned short cmdLen = sizeof(float);
    int ret = sendCmd(cmdType, cmdLen, &expus);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::getRGBExposure(unsigned& expus)
{
    unsigned short cmdType = CMD_TYPE_GET_EXPOSURE_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBGain(float gain)
{
    unsigned short cmdType = CMD_TYPE_SET_GAIN_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &gain);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::getRGBGain(float& gain)
{
    unsigned short cmdType = CMD_TYPE_GET_GAIN_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBMaxExposure(unsigned expus)
{
    unsigned short cmdType = CMD_TYPE_SET_MAX_EXPOSURE_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &expus);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::getRGBMaxExposure(unsigned& expus)
{
    unsigned short cmdType = CMD_TYPE_GET_MAX_EXPOSURE_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBMinExposure(unsigned expus)
{
    unsigned short cmdType = CMD_TYPE_SET_MIN_EXPOSURE_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &expus);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::getRGBMinExposure(unsigned& expus)
{
    unsigned short cmdType = CMD_TYPE_GET_MIN_EXPOSURE_RGB;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&expus, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBMaxGain(float gain)
{
    unsigned short cmdType = CMD_TYPE_SET_MAX_GAIN_RGB;
    unsigned short cmdLen = sizeof(float);
    int ret = sendCmd(cmdType, cmdLen, &gain);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::getRGBMaxGain(float& gain)
{
    unsigned short cmdType = CMD_TYPE_GET_MAX_GAIN_RGB;
    unsigned short cmdLen = sizeof(float);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBMinGain(float gain)
{
    unsigned short cmdType = CMD_TYPE_SET_MIN_GAIN_RGB;
    unsigned short cmdLen = sizeof(float);
    int ret = sendCmd(cmdType, cmdLen, &gain);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}
int Simou3CameraSettings::getRGBMinGain(float& gain)
{
    unsigned short cmdType = CMD_TYPE_GET_MIN_GAIN_RGB;
    unsigned short cmdLen = sizeof(float);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&gain, cmdLen);
    return ret;
}

int Simou3CameraSettings::setDOEPower(unsigned power)
{
    if (power > 255) {
        power = 255;
    }
    unsigned short cmdType = CMD_TYPE_SET_DOE_POWER;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendCmd(cmdType, cmdLen, &power);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&power, cmdLen);
    return ret;
}
int Simou3CameraSettings::getDOEPower(unsigned& power)
{
    unsigned short cmdType = CMD_TYPE_GET_DOE_POWER;
    unsigned short cmdLen = sizeof(unsigned);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&power, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoCalibData(unsigned char* data, int len)
{
    unsigned short cmdType = CMD_TYPE_SET_STEREO_CALIB_DATA;
    unsigned short cmdLen = len;
    int ret = sendCmd(cmdType, cmdLen, data);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(data, cmdLen);
    return ret;
}
int Simou3CameraSettings::getStereoCalibData(unsigned char* data, int len)
{
    unsigned short cmdType = CMD_TYPE_GET_STEREO_CALIB_DATA;
    unsigned short cmdLen = len;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(data, cmdLen);
    return ret;
}

int Simou3CameraSettings::setRGBCalibData(unsigned char* data, int len)
{
    unsigned short cmdType = CMD_TYPE_SET_RGB_CALIB_DATA;
    unsigned short cmdLen = len;
    int ret = sendCmd(cmdType, cmdLen, data);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(data, cmdLen);
    return ret;
}
int Simou3CameraSettings::getRGBCalibData(unsigned char* data, int len)
{
    unsigned short cmdType = CMD_TYPE_GET_RGB_CALIB_DATA;
    unsigned short cmdLen = len;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(data, cmdLen);
    return ret;
}

int Simou3CameraSettings::setFrameRate(int fps)
{
    unsigned short cmdType = CMD_TYPE_SET_FRAME_RATE;
    unsigned short cmdLen = sizeof(int);
    int ret = sendCmd(cmdType, cmdLen, &fps);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&fps, cmdLen);
    return ret;
}
int Simou3CameraSettings::setDownsampleMode(int mode)
{
    unsigned short cmdType = CMD_TYPE_SET_DOWNSAMPLE_MODE;
    unsigned short cmdLen = sizeof(int);
    int ret = sendCmd(cmdType, cmdLen, &mode);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&mode, cmdLen);
    return ret;
}
int Simou3CameraSettings::getFrameRate(int& fps)
{
    unsigned short cmdType = CMD_TYPE_GET_FRAME_RATE;
    unsigned short cmdLen = sizeof(int);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    ret = mSock.recvBlock(&fps, cmdLen);
    return ret;
}

int Simou3CameraSettings::setDepthDownsample(bool onoff)
{
    int downsample = onoff ? 1 : 0;
    unsigned short cmdType = CMD_TYPE_SET_DEPTH_DOWNSAMPLE;
    unsigned short cmdLen = sizeof(int);
    int ret = sendCmd(cmdType, cmdLen, &downsample);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&downsample, cmdLen);
    return ret;
}
int Simou3CameraSettings::setRGBDownsample(bool onoff)
{
    int downsample = onoff ? 1 : 0;
    unsigned short cmdType = CMD_TYPE_SET_RGB_DOWNSAMPLE;
    unsigned short cmdLen = sizeof(int);
    int ret = sendCmd(cmdType, cmdLen, &downsample);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&downsample, cmdLen);
    return ret;
}

int Simou3CameraSettings::setStereoDownsample(bool onoff)
{
    int downsample = onoff ? 1 : 0;
    unsigned short cmdType = CMD_TYPE_SET_STEREO_DOWNSAMPLE;
    unsigned short cmdLen = sizeof(int);
    int ret = sendCmd(cmdType, cmdLen, &downsample);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&downsample, cmdLen);
    return ret;
}

int Simou3CameraSettings::setChunkSize(int bytes)
{
    unsigned int sz = static_cast<unsigned int>(bytes);
    unsigned short cmdType = CMD_TYPE_SET_CHUNK_SIZE;
    unsigned short cmdLen = sizeof(unsigned int);
    int ret = sendCmd(cmdType, cmdLen, &sz);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&sz, cmdLen);
    return ret;
}

int Simou3CameraSettings::getAppVersion(uint8_t app[8])
{
    unsigned short cmdType = CMD_TYPE_GET_APP_VERSION;
    unsigned short cmdLen = 8;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(app, 8);
    return ret;
}

int Simou3CameraSettings::getAppVersion(std::string& app)
{
    uint8_t appData[9] = { 0 };
    int ret = getAppVersion(appData);
    if (ret <= 0) {
        return ret;
    }
    appData[8] = '\0';
    app = reinterpret_cast<char*>(appData);
    return ret;
}

int Simou3CameraSettings::setCameraSN(const uint8_t sn[16])
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_SET_CAMERA_SN;
    unsigned short cmdLen = 16;
    int ret = sendCmd(cmdType, cmdLen, sn);
    if (ret <= 0) {
        return ret;
    }
    uint8_t snResponse[16];
    ret = mSock.recvBlock(snResponse, 16);
    return ret;
}

int Simou3CameraSettings::setCameraSN(const std::string& sn)
{
    uint8_t snData[16] = { 0 };
    size_t len = sn.length();
    if (len > 16) {
        len = 16;
    }
    memcpy(snData, sn.c_str(), len);
    return setCameraSN(snData);
}

int Simou3CameraSettings::getCameraSN(uint8_t sn[16])
{
    unsigned short cmdType = CMD_TYPE_GET_CAMERA_SN;
    unsigned short cmdLen = 16;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(sn, 16);
    return ret;
}

int Simou3CameraSettings::getCameraSN(std::string& sn)
{
    uint8_t snData[17] = { 0 };
    int ret = getCameraSN(snData);
    if (ret <= 0) {
        return ret;
    }
    snData[16] = '\0';
    sn = reinterpret_cast<char*>(snData);
    return ret;
}

int Simou3CameraSettings::setFPGAVersion(const uint8_t fpga[8])
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_SET_FPGA_VERSION;
    unsigned short cmdLen = 8;
    int ret = sendCmd(cmdType, cmdLen, fpga);
    if (ret <= 0) {
        return ret;
    }
    uint8_t fpgaResponse[8];
    ret = mSock.recvBlock(fpgaResponse, 8);
    return ret;
}

int Simou3CameraSettings::setFPGAVersion(const std::string& fpga)
{
    uint8_t fpgaData[8] = { 0 };
    size_t len = fpga.length();
    if (len > 8) {
        len = 8;
    }
    memcpy(fpgaData, fpga.c_str(), len);
    return setFPGAVersion(fpgaData);
}

int Simou3CameraSettings::getFPGAVersion(uint8_t fpga[8])
{
    unsigned short cmdType = CMD_TYPE_GET_FPGA_VERSION;
    unsigned short cmdLen = 8;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(fpga, 8);
    return ret;
}

int Simou3CameraSettings::getFPGAVersion(std::string& fpga)
{
    uint8_t fpgaData[9] = { 0 };
    int ret = getFPGAVersion(fpgaData);
    if (ret <= 0) {
        return ret;
    }
    fpgaData[8] = '\0';
    fpga = reinterpret_cast<char*>(fpgaData);
    return ret;
}

int Simou3CameraSettings::setHardwareVersion(const uint8_t hardware[8])
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_SET_HARDWARE_VERSION;
    unsigned short cmdLen = 8;
    int ret = sendCmd(cmdType, cmdLen, hardware);
    if (ret <= 0) {
        return ret;
    }
    uint8_t hardwareResponse[8];
    ret = mSock.recvBlock(hardwareResponse, 8);
    return ret;
}

int Simou3CameraSettings::setHardwareVersion(const std::string& hardware)
{
    uint8_t hardwareData[8] = { 0 };
    size_t len = hardware.length();
    if (len > 8) {
        len = 8;
    }
    memcpy(hardwareData, hardware.c_str(), len);
    return setHardwareVersion(hardwareData);
}

int Simou3CameraSettings::getHardwareVersion(uint8_t hardware[8])
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_GET_HARDWARE_VERSION;
    unsigned short cmdLen = 8;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(hardware, 8);
    return ret;
}

int Simou3CameraSettings::getHardwareVersion(std::string& hardware)
{
    uint8_t hardwareData[9] = { 0 };
    int ret = getHardwareVersion(hardwareData);
    if (ret <= 0) {
        return ret;
    }
    hardwareData[8] = '\0';
    hardware = reinterpret_cast<char*>(hardwareData);
    return ret;
}

int Simou3CameraSettings::setProductModelVersion(const uint8_t product[16])
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_SET_PRODUCT_MODEL_VERSION;
    unsigned short cmdLen = 16;
    int ret = sendCmd(cmdType, cmdLen, product);
    if (ret <= 0) {
        return ret;
    }
    uint8_t productResponse[16];
    ret = mSock.recvBlock(productResponse, 16);
    return ret;
}

int Simou3CameraSettings::setProductModelVersion(const std::string& product)
{
    uint8_t productData[16] = { 0 };
    size_t len = product.length();
    if (len > 16) {
        len = 16;
    }
    memcpy(productData, product.c_str(), len);
    return setProductModelVersion(productData);
}

int Simou3CameraSettings::getProductModelVersion(uint8_t product[16])
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_GET_PRODUCT_MODEL_VERSION;
    unsigned short cmdLen = 16;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(product, 16);
    return ret;
}

int Simou3CameraSettings::getProductModelVersion(std::string& product)
{
    uint8_t productData[17] = { 0 };
    int ret = getProductModelVersion(productData);
    if (ret <= 0) {
        return ret;
    }
    productData[16] = '\0';
    product = reinterpret_cast<char*>(productData);
    return ret;
}

int Simou3CameraSettings::setDepthSwitch(bool onoff)
{
    unsigned short cmdType = CMD_TYPE_SET_DEPTH_SWITCH;
    unsigned short cmdLen = sizeof(int);
    int onoffLocal = onoff ? 1 : 0;
    int ret = sendCmd(cmdType, cmdLen, &onoffLocal);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&onoffLocal, cmdLen);
    return ret;
}

int Simou3CameraSettings::setRegistrationSwitch(bool onoff)
{
    unsigned short cmdType = CMD_TYPE_SET_REGISTRATION_SWITCH;
    unsigned short cmdLen = sizeof(int);
    int onoffLocal = onoff ? 1 : 0;
    int ret = sendCmd(cmdType, cmdLen, &onoffLocal);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(&onoffLocal, cmdLen);
    return ret;
}

int Simou3CameraSettings::setIPAddress(uint32_t ip, uint32_t mask, uint32_t gateway)
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_SET_IP;
    unsigned short cmdLen = sizeof(uint32_t) * 3;
    uint32_t ipData[3] = { ip, mask, gateway };
    int ret = sendCmd(cmdType, cmdLen, ipData);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(ipData, cmdLen);
    return ret;
}

int Simou3CameraSettings::getIPAddress(uint32_t& ip, uint32_t& mask, uint32_t& gateway)
{
    unsigned short cmdType = CMD_TYPE_GET_IP;
    unsigned short cmdLen = sizeof(uint32_t) * 3;
    uint32_t ipData[3];
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(ipData, cmdLen);
    ip = ipData[0];
    mask = ipData[1];
    gateway = ipData[2];
    return ret;
}

int Simou3CameraSettings::setIPAddress(std::string ip, std::string mask, std::string gateway)
{
    return setIPAddress(static_cast<uint32_t>(inet_addr(ip.c_str())), static_cast<uint32_t>(inet_addr(mask.c_str())),
        static_cast<uint32_t>(inet_addr(gateway.c_str())));
}

int Simou3CameraSettings::getIPAddress(std::string& ip, std::string& mask, std::string& gateway)
{
    uint32_t _ip;
    uint32_t _mask;
    uint32_t _gateway;
    int ret = getIPAddress(_ip, _mask, _gateway);
    if (ret <= 0) {
        return ret;
    }
    ip = inet_ntoa(*reinterpret_cast<in_addr*>(&_ip));
    mask = inet_ntoa(*reinterpret_cast<in_addr*>(&_mask));
    gateway = inet_ntoa(*reinterpret_cast<in_addr*>(&_gateway));
    return ret;
}

int Simou3CameraSettings::setMacAddress(uint8_t mac[6])
{
    return SIMOU3_ERR_NOT_SUPPORTED;
    unsigned short cmdType = CMD_TYPE_SET_MAC;
    unsigned short cmdLen = sizeof(uint8_t) * 6;
    int ret = sendCmd(cmdType, cmdLen, mac);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(mac, cmdLen);
    return ret;
}

int Simou3CameraSettings::getMacAddress(uint8_t mac[6])
{
    unsigned short cmdType = CMD_TYPE_GET_MAC;
    unsigned short cmdLen = sizeof(uint8_t) * 6;
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(mac, cmdLen);
    return ret;
}

int Simou3CameraSettings::setMacAddress(std::string mac)
{
    uint8_t macData[6];
    unsigned int temp[6];

    if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]) != 6) {
        if (sscanf(mac.c_str(), "%x-%x-%x-%x-%x-%x", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]) != 6) {
            return -1;
        }
    }

    for (int i = 0; i < 6; i++) {
        macData[i] = static_cast<uint8_t>((temp[i] & 0xFF));
    }

    return setMacAddress(macData);
}

int Simou3CameraSettings::getMacAddress(std::string& mac)
{
    uint8_t macData[6];
    int ret = getMacAddress(macData);
    if (ret <= 0) {
        return ret;
    }
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", macData[0], macData[1], macData[2], macData[3], macData[4], macData[5]);
    mac = macStr;
    return ret;
}

int Simou3CameraSettings::sendGetCmd(unsigned short cmdType, unsigned short len)
{
    std::vector<unsigned char> zero(len, 0);
    return sendCmd(cmdType, len, zero.data());
}

int Simou3CameraSettings::sendCmd(unsigned short cmdType, unsigned short len, const void* data)
{
    std::vector<unsigned char> sendBufV(7 + len);
    unsigned char* sendBuf = sendBufV.data();
    sendBuf[0] = 'C';
    sendBuf[1] = 'M';
    sendBuf[2] = 'D';

    unsigned short cmdTypeLocal = cmdType;
    unsigned short cmdLenLocal = len;
    memcpy(sendBuf + 3, &cmdTypeLocal, 2);
    memcpy(sendBuf + 5, &cmdLenLocal, 2);
    memcpy(sendBuf + 7, data, len);
    int ret = mSock.sendBlock(sendBuf, 7 + len);
    if (ret <= 0) {
        return ret;
    }
    return ret;
}
