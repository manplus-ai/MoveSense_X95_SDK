// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "movesense/Simou3CameraSettings.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"
#include <cstring>

#define CMD_TYPE_GET_CAMERA_TYPE 0x0019
#define CMD_TYPE_GET_DEPTH_UNIT 0x001C

#define CMD_TYPE_SET_IP 0x0009
#define CMD_TYPE_GET_IP 0x000A

#define CMD_TYPE_GET_MAC 0x000C

#define CMD_TYPE_GET_APP_VERSION 0x0010

#define CMD_TYPE_SET_CAMERA_SN 0x0011
#define CMD_TYPE_GET_CAMERA_SN 0x0012
#define CMD_TYPE_GET_FPGA_VERSION 0x0014
#define CMD_TYPE_GET_HARDWARE_VERSION 0x0016
#define CMD_TYPE_GET_PRODUCT_MODEL_VERSION 0x0018

#define CMD_TYPE_AE_SWITCH_STEREO 0x2021
#define CMD_TYPE_AE_DESIRED_BIN_STEREO 0x2022
#define CMD_TYPE_SET_EXPOSURE_STEREO 0x2023
#define CMD_TYPE_GET_EXPOSURE_STEREO 0x2024
#define CMD_TYPE_SET_GAIN_STEREO 0x2025
#define CMD_TYPE_GET_GAIN_STEREO 0x2026

#define CMD_TYPE_SET_MAX_EXPOSURE_STEREO 0x2028
#define CMD_TYPE_SET_MIN_EXPOSURE_STEREO 0x2029
#define CMD_TYPE_SET_MAX_GAIN_STEREO 0x202A
#define CMD_TYPE_SET_MIN_GAIN_STEREO 0x202B
#define CMD_TYPE_GET_MAX_EXPOSURE_STEREO 0x202C
#define CMD_TYPE_GET_MIN_EXPOSURE_STEREO 0x202D
#define CMD_TYPE_GET_MAX_GAIN_STEREO 0x202E
#define CMD_TYPE_GET_MIN_GAIN_STEREO 0x202F

#define CMD_TYPE_AE_SWITCH_RGB 0x2041
#define CMD_TYPE_AE_DESIRED_BIN_RGB 0x2042
#define CMD_TYPE_SET_EXPOSURE_RGB 0x2043
#define CMD_TYPE_GET_EXPOSURE_RGB 0x2044
#define CMD_TYPE_SET_GAIN_RGB 0x2045
#define CMD_TYPE_GET_GAIN_RGB 0x2046

#define CMD_TYPE_SET_MAX_EXPOSURE_RGB 0x2048
#define CMD_TYPE_SET_MIN_EXPOSURE_RGB 0x204A
#define CMD_TYPE_SET_MAX_GAIN_RGB 0x204C
#define CMD_TYPE_SET_MIN_GAIN_RGB 0x204E
#define CMD_TYPE_GET_MAX_EXPOSURE_RGB 0x2049
#define CMD_TYPE_GET_MIN_EXPOSURE_RGB 0x204B
#define CMD_TYPE_GET_MAX_GAIN_RGB 0x204D
#define CMD_TYPE_GET_MIN_GAIN_RGB 0x204F

#define CMD_TYPE_SET_DEPTH_DOWNSAMPLE 0x2061
#define CMD_TYPE_SET_DOWNSAMPLE_MODE 0x206D
#define CMD_TYPE_SET_RGB_DOWNSAMPLE 0x2063
#define CMD_TYPE_SET_STEREO_DOWNSAMPLE 0x2069
#define CMD_TYPE_SET_CHUNK_SIZE 0x206B

#define CMD_TYPE_SET_FRAME_RATE 0x3005
#define CMD_TYPE_GET_FRAME_RATE 0x3006
#define CMD_TYPE_SET_TRIGGER_MODE 0x3008
#define CMD_TYPE_GET_TRIGGER_MODE 0x3009
#define CMD_TYPE_SET_TRIGGER_OUT 0x300A
#define CMD_TYPE_GET_TRIGGER_OUT 0x300B
#define CMD_TYPE_TRIGGER_FRAME 0x300C

#define CMD_TYPE_SET_STEREO_CALIB_DATA 0x4001
#define CMD_TYPE_GET_STEREO_CALIB_DATA 0x4002
#define CMD_TYPE_SET_RGB_CALIB_DATA 0x4003
#define CMD_TYPE_GET_RGB_CALIB_DATA 0x4004

#define CMD_TYPE_SET_ROI 0x2071
#define CMD_TYPE_GET_ROI 0x2072

#define CMD_TYPE_SET_COLOR_ADJUST_STEREO 0x2031
#define CMD_TYPE_GET_COLOR_ADJUST_STEREO 0x2032
#define CMD_TYPE_SET_COLOR_ADJUST_RGB 0x2051
#define CMD_TYPE_GET_COLOR_ADJUST_RGB 0x2052
#define CMD_TYPE_SET_SHARPEN_STEREO 0x2033
#define CMD_TYPE_GET_SHARPEN_STEREO 0x2034
#define CMD_TYPE_SET_SHARPEN_RGB 0x2053
#define CMD_TYPE_GET_SHARPEN_RGB 0x2054
#define CMD_TYPE_SET_WB_AUTO_STEREO 0x2035
#define CMD_TYPE_GET_WB_AUTO_STEREO 0x2036
#define CMD_TYPE_SET_WB_AUTO_RGB 0x2055
#define CMD_TYPE_GET_WB_AUTO_RGB 0x2056
#define CMD_TYPE_SET_WB_GAIN_STEREO 0x2037
#define CMD_TYPE_GET_WB_GAIN_STEREO 0x2038
#define CMD_TYPE_SET_WB_GAIN_RGB 0x2057
#define CMD_TYPE_GET_WB_GAIN_RGB 0x2058

#define CMD_TYPE_SET_DOE_POWER 0x5001
#define CMD_TYPE_GET_DOE_POWER 0x5002

#define CMD_TYPE_SET_REGISTRATION_SWITCH 0x7002

namespace movesense {

namespace {

unsigned clampIspLevel(unsigned v)
{
    return (v > kIspLevelMax) ? kIspLevelMax : v;
}

float clampIspWbGain(float v)
{
    if (!(v > kIspWbGainMin)) {
        return kIspWbGainMin;
    }
    if (v > kIspWbGainMax) {
        return kIspWbGainMax;
    }
    return v;
}

} // namespace

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
    if (mode != kTriggerModeAuto && mode != kTriggerModeSoft && mode != kTriggerModeExternal) {
        return SIMOU3_ERR_TRIGGER_MODE_INVALID;
    }

    unsigned short cmdType = CMD_TYPE_SET_TRIGGER_MODE;
    unsigned short cmdLen = sizeof(int);
    int ret = sendCmd(cmdType, cmdLen, &mode);
    if (ret <= 0) {
        return ret;
    }

    int current = -1;
    ret = mSock.recvBlock(&current, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    if (current != mode) {
        return SIMOU3_ERR_TRIGGER_REJECTED;
    }
    return ret;
}
int Simou3CameraSettings::getTriggerMode(int& mode)
{
    unsigned short cmdType = CMD_TYPE_GET_TRIGGER_MODE;
    unsigned short cmdLen = sizeof(int);
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

int Simou3CameraSettings::getDepthUnitMm(double& unitMm)
{
    unsigned char data[12] = {};
    int result = ExchangeParameter(CMD_TYPE_GET_DEPTH_UNIT, data, sizeof(data));
    if (result <= 0) {
        return result;
    }

    double value = 0.0;
    memcpy(&value, data + 4, sizeof(value));
    if (!(value > 0.0)) {
        return SIMOU3_ERR_INFO_UNAVAILABLE;
    }

    unitMm = value;
    return result;
}

int Simou3CameraSettings::setTriggerOut(int out)
{
    if (out != 0 && out != 1) {
        return SIMOU3_ERR_CONFIG_INVALID;
    }

    const unsigned short cmdLen = sizeof(out);
    int ret = sendCmd(CMD_TYPE_SET_TRIGGER_OUT, cmdLen, &out);
    if (ret <= 0) {
        return ret;
    }

    int current = -1;
    ret = mSock.recvBlock(&current, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    if (current == out) {
        return ret;
    }
    if (out == 1 && current == 0) {
        int mode = -1;
        int modeRet = getTriggerMode(mode);
        if (modeRet > 0 && mode == kTriggerModeExternal) {
            return ret;
        }
    }
    return SIMOU3_ERR_TRIGGER_REJECTED;
}

int Simou3CameraSettings::getTriggerOut(int& out)
{
    const unsigned short cmdLen = sizeof(out);
    int ret = sendGetCmd(CMD_TYPE_GET_TRIGGER_OUT, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    int current = -1;
    ret = mSock.recvBlock(&current, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    if (current != 0 && current != 1) {
        return SIMOU3_ERR_TRIGGER_REJECTED;
    }
    out = current;
    return ret;
}

int Simou3CameraSettings::triggerFrame()
{
    unsigned short cmdType = CMD_TYPE_TRIGGER_FRAME;
    unsigned short cmdLen = sizeof(int);
    int ret = sendGetCmd(cmdType, cmdLen);
    if (ret <= 0) {
        return ret;
    }

    int fired = 0;
    ret = mSock.recvBlock(&fired, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    if (fired != 1) {
        return SIMOU3_ERR_TRIGGER_REJECTED;
    }
    return ret;
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

int Simou3CameraSettings::checkRoi(unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
    if (x1 > 65535u || y1 > 65535u || x2 > 65535u || y2 > 65535u) {
        return SIMOU3_ERR_ROI_INVALID;
    }
    if (x2 <= x1 || y2 <= y1) {
        return SIMOU3_ERR_ROI_INVALID;
    }
    if ((x2 - x1) < 16u || (y2 - y1) < 16u) {
        return SIMOU3_ERR_ROI_INVALID;
    }
    if (((x1 | y1 | x2 | y2) & 1u) != 0) {
        return SIMOU3_ERR_ROI_INVALID;
    }
    return 1;
}

int Simou3CameraSettings::setRoi(unsigned stream, bool enable, unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
    if (stream > 5u) {
        return SIMOU3_ERR_NOT_SUPPORTED;
    }
    if (enable) {
        int chk = checkRoi(x1, y1, x2, y2);
        if (chk <= 0) {
            return chk;
        }
    } else {
        x1 = y1 = x2 = y2 = 0;
    }
    uint32_t req[6] = { stream, enable ? 1u : 0u, x1, y1, x2, y2 };
    uint32_t rep[6] = { 0, 0, 0, 0, 0, 0 };
    unsigned short cmdType = CMD_TYPE_SET_ROI;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    if (memcmp(req, rep, sizeof(req)) != 0) {
        return SIMOU3_ERR_ROI_REJECTED;
    }
    return ret;
}

int Simou3CameraSettings::getRoi(unsigned stream, bool& enable, unsigned& x1, unsigned& y1, unsigned& x2, unsigned& y2)
{
    if (stream > 5u) {
        return SIMOU3_ERR_NOT_SUPPORTED;
    }
    uint32_t req[6] = { stream, 0, 0, 0, 0, 0 };
    uint32_t rep[6] = { 0, 0, 0, 0, 0, 0 };
    unsigned short cmdType = CMD_TYPE_GET_ROI;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    enable = (rep[1] != 0);
    x1 = rep[2];
    y1 = rep[3];
    x2 = rep[4];
    y2 = rep[5];
    return ret;
}

int Simou3CameraSettings::getColorAdjust(
    IspChannel channel, bool& enable, unsigned& brightness, unsigned& contrast, unsigned& saturation, unsigned& hue)
{
    uint32_t req[5] = { 0, 0, 0, 0, 0 };
    uint32_t rep[5] = { 0, 0, 0, 0, 0 };
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_GET_COLOR_ADJUST_RGB : CMD_TYPE_GET_COLOR_ADJUST_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    enable = (rep[0] != 0);
    brightness = rep[1];
    contrast = rep[2];
    saturation = rep[3];
    hue = rep[4];
    return ret;
}

int Simou3CameraSettings::getSharpen(IspChannel channel, bool& enable, unsigned& strength, unsigned& textureStrength, unsigned& edgeStrength)
{
    uint32_t req[4] = { 0, 0, 0, 0 };
    uint32_t rep[4] = { 0, 0, 0, 0 };
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_GET_SHARPEN_RGB : CMD_TYPE_GET_SHARPEN_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    enable = (rep[0] != 0);
    strength = rep[1];
    textureStrength = rep[2];
    edgeStrength = rep[3];
    return ret;
}

int Simou3CameraSettings::getWhiteBalanceAuto(IspChannel channel, bool& autoMode)
{
    uint32_t req[1] = { 0 };
    uint32_t rep[1] = { 0 };
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_GET_WB_AUTO_RGB : CMD_TYPE_GET_WB_AUTO_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    autoMode = (rep[0] != 0);
    return ret;
}

int Simou3CameraSettings::getWhiteBalanceGain(IspChannel channel, float& rGain, float& gGain, float& bGain)
{
    uint32_t req[3] = { 0, 0, 0 };
    uint32_t rep[3] = { 0, 0, 0 };
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_GET_WB_GAIN_RGB : CMD_TYPE_GET_WB_GAIN_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    if (ret <= 0) {
        return ret;
    }
    memcpy(&rGain, &rep[0], sizeof(float));
    memcpy(&gGain, &rep[1], sizeof(float));
    memcpy(&bGain, &rep[2], sizeof(float));
    return ret;
}

int Simou3CameraSettings::setColorAdjust(IspChannel channel, bool enable, unsigned brightness, unsigned contrast, unsigned saturation, unsigned hue)
{
    uint32_t req[5] = { enable ? 1u : 0u, clampIspLevel(brightness), clampIspLevel(contrast), clampIspLevel(saturation), clampIspLevel(hue) };
    uint32_t rep[5] = { 0, 0, 0, 0, 0 };
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_SET_COLOR_ADJUST_RGB : CMD_TYPE_SET_COLOR_ADJUST_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    return ret;
}

int Simou3CameraSettings::setSharpen(IspChannel channel, bool enable, unsigned strength)
{
    uint32_t req[4] = { enable ? 1u : 0u, clampIspLevel(strength), 0, 0 };
    uint32_t rep[4] = { 0, 0, 0, 0 };
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_SET_SHARPEN_RGB : CMD_TYPE_SET_SHARPEN_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    return ret;
}

int Simou3CameraSettings::setWhiteBalanceAuto(IspChannel channel, bool autoMode)
{
    uint32_t req[1] = { autoMode ? 1u : 0u };
    uint32_t rep[1] = { 0 };
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_SET_WB_AUTO_RGB : CMD_TYPE_SET_WB_AUTO_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
    return ret;
}

int Simou3CameraSettings::setWhiteBalanceGain(IspChannel channel, float rGain, float gGain, float bGain)
{
    uint32_t req[3] = { 0, 0, 0 };
    uint32_t rep[3] = { 0, 0, 0 };
    const float r = clampIspWbGain(rGain);
    const float g = clampIspWbGain(gGain);
    const float b = clampIspWbGain(bGain);
    memcpy(&req[0], &r, sizeof(float));
    memcpy(&req[1], &g, sizeof(float));
    memcpy(&req[2], &b, sizeof(float));
    unsigned short cmdType = (channel == IspChannel::Rgb) ? CMD_TYPE_SET_WB_GAIN_RGB : CMD_TYPE_SET_WB_GAIN_STEREO;
    unsigned short cmdLen = sizeof(req);
    int ret = sendCmd(cmdType, cmdLen, req);
    if (ret <= 0) {
        return ret;
    }
    ret = mSock.recvBlock(rep, cmdLen);
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

int Simou3CameraSettings::ExchangeParameter(unsigned short command, unsigned char* data, unsigned short length)
{
    int result = sendCmd(command, length, data);
    if (result != length + 7) {
        return result <= 0 ? result : -1;
    }
    result = mSock.recvBlock(data, length);
    if (result != length) {
        return result <= 0 ? result : -1;
    }
    int32_t status = 0;
    memcpy(&status, data, 4);
    return status == 1 ? result : (status < 0 ? status : -1);
}

int Simou3CameraSettings::ReadTextParameter(unsigned short command, std::string& value)
{
    unsigned char data[68] = {};
    int result = ExchangeParameter(command, data, sizeof(data));
    if (result <= 0) {
        return result;
    }
    const char* text = reinterpret_cast<const char*>(data + 4);
    const void* end = memchr(text, 0, 64);
    if (!end || text[0] == 0) {
        return SIMOU3_ERR_INFO_UNAVAILABLE;
    }
    value.assign(text, static_cast<const char*>(end) - text);
    return result;
}

int Simou3CameraSettings::getAppVersion(std::string& app) { return ReadTextParameter(CMD_TYPE_GET_APP_VERSION, app); }

int Simou3CameraSettings::getCameraSN(std::string& sn) { return ReadTextParameter(CMD_TYPE_GET_CAMERA_SN, sn); }

int Simou3CameraSettings::setCameraSN(const std::string& sn)
{
    if (sn.empty() || sn.size() >= 64) {
        return SIMOU3_ERR_CONFIG_INVALID;
    }
    for (unsigned char ch : sn) {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '-' || ch == '_')) {
            return SIMOU3_ERR_CONFIG_INVALID;
        }
    }
    unsigned char data[68] = {};
    memcpy(data + 4, sn.data(), sn.size());
    return ExchangeParameter(CMD_TYPE_SET_CAMERA_SN, data, sizeof(data));
}

int Simou3CameraSettings::getHardwareVersion(std::string& hardware) { return ReadTextParameter(CMD_TYPE_GET_HARDWARE_VERSION, hardware); }

int Simou3CameraSettings::getProductModelVersion(std::string& product)
{
    return ReadTextParameter(CMD_TYPE_GET_PRODUCT_MODEL_VERSION, product);
}

int Simou3CameraSettings::GetKernelVersion(std::string& version) { return ReadTextParameter(0x001A, version); }

int Simou3CameraSettings::GetCameraLibVersion(std::string& version) { return ReadTextParameter(0x001B, version); }

int Simou3CameraSettings::SetNetworkConfig(const NetworkConfig& config)
{
    uint32_t addresses[3] = {};
    if (inet_pton(AF_INET, config.ip.c_str(), &addresses[0]) != 1 || inet_pton(AF_INET, config.netmask.c_str(), &addresses[1]) != 1 ||
        inet_pton(AF_INET, config.gateway.c_str(), &addresses[2]) != 1) {
        return SIMOU3_ERR_CONFIG_INVALID;
    }
    uint32_t ip = ntohl(addresses[0]);
    uint32_t mask = ntohl(addresses[1]);
    uint32_t gateway = ntohl(addresses[2]);
    uint32_t inverse = ~mask;
    auto validHost = [mask](uint32_t address) {
        uint32_t host = address & ~mask;
        return (address >> 24) != 0 && (address >> 24) != 127 && (address >> 24) < 224 && host != 0 && host != ~mask;
    };
    if (mask == 0 || inverse == 0 || (inverse & (inverse + 1)) != 0 || !validHost(ip) ||
        (gateway != 0 && (!validHost(gateway) || (gateway & mask) != (ip & mask) || gateway == ip))) {
        return SIMOU3_ERR_CONFIG_INVALID;
    }
    unsigned char data[16] = {};
    memcpy(data + 4, addresses, sizeof(addresses));
    return ExchangeParameter(CMD_TYPE_SET_IP, data, sizeof(data));
}

int Simou3CameraSettings::GetNetworkConfig(NetworkConfig& config)
{
    unsigned char data[16] = {};
    int result = ExchangeParameter(CMD_TYPE_GET_IP, data, sizeof(data));
    if (result <= 0) {
        return result;
    }
    char ip[INET_ADDRSTRLEN];
    char mask[INET_ADDRSTRLEN];
    char gateway[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, data + 4, ip, sizeof(ip)) || !inet_ntop(AF_INET, data + 8, mask, sizeof(mask)) ||
        !inet_ntop(AF_INET, data + 12, gateway, sizeof(gateway))) {
        return -1;
    }
    config.ip = ip;
    config.netmask = mask;
    config.gateway = gateway;
    return result;
}

int Simou3CameraSettings::getMacAddress(uint8_t mac[6])
{
    unsigned char data[10] = {};
    int result = ExchangeParameter(CMD_TYPE_GET_MAC, data, sizeof(data));
    if (result > 0) {
        memcpy(mac, data + 4, 6);
    }
    return result;
}

int Simou3CameraSettings::getMacAddress(std::string& mac)
{
    uint8_t data[6] = {};
    int result = getMacAddress(data);
    if (result <= 0) {
        return result;
    }
    char text[18];
    snprintf(text, sizeof(text), "%02x:%02x:%02x:%02x:%02x:%02x", data[0], data[1], data[2], data[3], data[4], data[5]);
    mac = text;
    return result;
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

} // namespace movesense
