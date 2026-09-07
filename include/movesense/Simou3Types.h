// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <Winsock2.h>
typedef SOCKET socket_t;
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
typedef int socket_t;
#endif

#define SIMOU3_ERR_NOT_SUPPORTED (-100)

#define CMD_TYPE_GET_FIRMWARE_VERSION 0x0006
#define CMD_TYPE_GET_CAMERA_TYPE 0x0019

#define CMD_TYPE_SET_IP 0x0009
#define CMD_TYPE_GET_IP 0x000A

#define CMD_TYPE_SET_MAC 0x000B
#define CMD_TYPE_GET_MAC 0x000C

#define CMD_TYPE_GET_APP_VERSION 0x0010

#define CMD_TYPE_SET_CAMERA_SN 0x0011
#define CMD_TYPE_GET_CAMERA_SN 0x0012
#define CMD_TYPE_SET_FPGA_VERSION 0x0013
#define CMD_TYPE_GET_FPGA_VERSION 0x0014
#define CMD_TYPE_SET_HARDWARE_VERSION 0x0015
#define CMD_TYPE_GET_HARDWARE_VERSION 0x0016
#define CMD_TYPE_SET_PRODUCT_MODEL_VERSION 0x0017
#define CMD_TYPE_GET_PRODUCT_MODEL_VERSION 0x0018

#define CMD_TYPE_UPDATE_HEADER 0x1000
#define CMD_TYPE_UPDATE_ERASE 0x1001
#define CMD_TYPE_UPDATE_DATA 0x1002

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

#define CMD_TYPE_SET_DOE_POWER 0x5001
#define CMD_TYPE_GET_DOE_POWER 0x5002

#define CMD_TYPE_SET_DEPTH_SWITCH          0x7001
#define CMD_TYPE_SET_REGISTRATION_SWITCH   0x7002


struct CameraInfo {
	std::string ip;
	int port;
	uint32_t mode;
	uint32_t len;
	std::string mac;
	std::string gateway;
	std::string netmask;

	CameraInfo() : ip(""), port(0), mode(0), len(0), mac(""), gateway(""), netmask("") {}
};
struct NetworkInterface {
	std::string name;
	std::string ip;
	std::string netmask;
	std::string broadcast;

	NetworkInterface() : name(""), ip(""), netmask(""), broadcast("") {}
};

enum class PFormat : uint8_t { PF_Y8 = 0, PF_NV21 = 1, PF_DEPTH16 = 2 };

#pragma pack(push, 1)
struct DetectionBox {
	float   x1, y1, x2, y2;
	float   score;
	int32_t classId;
};
#pragma pack(pop)

class MovesenseFrame
{
public:
	struct Plane {
		bool     has = false;
		uint8_t  status = 0;
		uint16_t width = 0, height = 0;
		PFormat  format = PFormat::PF_Y8;
		uint64_t pts = 0, tsend = 0;
		uint64_t recvStart = 0, recvEnd = 0;
		std::vector<uint8_t> data;
		uint32_t totalLen = 0, recvBytes = 0;
		bool complete() const { return has && (status != 0 || recvBytes == totalLen); }
		bool degraded() const { return has && status != 0; }
	};
	enum { L_RAW = 0, R_RAW, RGB_RAW, L_RECT, R_RECT, RGB_RECT, DEPTH, SEG, PLANE_N };

	uint32_t frameCnt() const { return m_frameCnt; }
	const Plane& plane(int idx)  const { return m_planes[idx]; }
	const Plane& leftRaw()   const { return m_planes[L_RAW]; }
	const Plane& rightRaw()  const { return m_planes[R_RAW]; }
	const Plane& rgbRaw()    const { return m_planes[RGB_RAW]; }
	const Plane& leftRect()  const { return m_planes[L_RECT]; }
	const Plane& rightRect() const { return m_planes[R_RECT]; }
	const Plane& rgbRect()   const { return m_planes[RGB_RECT]; }
	const Plane& depth()     const { return m_planes[DEPTH]; }

	const std::vector<DetectionBox>& detections() const { return m_detections; }
	int detCoordW() const { return m_detCoordW; }
	int detCoordH() const { return m_detCoordH; }

	static int labelToIdx(uint8_t label) {
		switch (label) {
		case 0: return L_RAW;  case 1: return R_RAW;  case 2: return RGB_RAW;
		case 3: return L_RECT; case 4: return R_RECT; case 5: return RGB_RECT;
		case 10: return DEPTH; case 20: return SEG;   default: return -1;
		}
	}
	static int idxToLabel(int idx) {
		static const int L[PLANE_N] = { 0,1,2,3,4,5,10,20 }; return L[idx];
	}

	uint32_t m_frameCnt = 0;
	bool     m_degraded = false;
	Plane    m_planes[PLANE_N];
	std::vector<DetectionBox> m_detections;
	int      m_detCoordW = 0, m_detCoordH = 0;
};

struct IMU {
	int16_t  ax, ay, az;
	int16_t  gx, gy, gz;
	int16_t  temp;
	uint64_t timestampUs;
};

struct Simou3Stats {
	uint64_t emptyPkt = 0;
	uint64_t abnormalPkt = 0;
	uint64_t incompleteDrop = 0;
	uint64_t overwrite = 0;
	uint64_t corrupt = 0;
};
