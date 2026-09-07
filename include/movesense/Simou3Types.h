// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Winsock2.h>
typedef SOCKET socket_t;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#endif

#define SIMOU3_ERR_NOT_SUPPORTED (-100)

namespace movesense {

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
    float x1, y1, x2, y2;
    float score;
    int32_t classId;
};
#pragma pack(pop)

class MovesenseFrame {
public:
    struct Plane {
        bool has = false;
        uint8_t status = 0;
        uint16_t width = 0, height = 0;
        PFormat format = PFormat::PF_Y8;
        uint64_t pts = 0, tsend = 0;
        uint64_t recvStart = 0, recvEnd = 0;
        std::vector<uint8_t> data;
        uint32_t totalLen = 0, recvBytes = 0;
        bool complete() const
        {
            return has && (status != 0 || recvBytes == totalLen);
        }
        bool degraded() const
        {
            return has && status != 0;
        }
    };
    enum { L_RAW = 0, R_RAW, RGB_RAW, L_RECT, R_RECT, RGB_RECT, DEPTH, SEG, PLANE_N };

    uint32_t frameCnt() const
    {
        return m_frameCnt;
    }
    const Plane& leftRaw() const
    {
        return m_planes[L_RAW];
    }
    const Plane& rightRaw() const
    {
        return m_planes[R_RAW];
    }
    const Plane& rgbRaw() const
    {
        return m_planes[RGB_RAW];
    }
    const Plane& leftRect() const
    {
        return m_planes[L_RECT];
    }
    const Plane& rightRect() const
    {
        return m_planes[R_RECT];
    }
    const Plane& rgbRect() const
    {
        return m_planes[RGB_RECT];
    }
    const Plane& depth() const
    {
        return m_planes[DEPTH];
    }
    const Plane& seg() const
    {
        return m_planes[SEG];
    }

    const std::vector<DetectionBox>& detections() const
    {
        return m_detections;
    }
    int detCoordW() const
    {
        return m_detCoordW;
    }
    int detCoordH() const
    {
        return m_detCoordH;
    }

    static int labelToIdx(uint8_t label)
    {
        switch (label) {
            case 0:
                return L_RAW;
            case 1:
                return R_RAW;
            case 2:
                return RGB_RAW;
            case 3:
                return L_RECT;
            case 4:
                return R_RECT;
            case 5:
                return RGB_RECT;
            case 10:
                return DEPTH;
            case 20:
                return SEG;
            default:
                return -1;
        }
    }
    static int idxToLabel(int idx)
    {
        static const int L[PLANE_N] = { 0, 1, 2, 3, 4, 5, 10, 20 };
        return L[idx];
    }

    uint32_t m_frameCnt = 0;
    bool m_degraded = false;
    Plane m_planes[PLANE_N];
    std::vector<DetectionBox> m_detections;
    int m_detCoordW = 0, m_detCoordH = 0;
};

struct Imu {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t temp;
    uint64_t timestampUs;
};

struct Simou3Stats {
    uint64_t emptyPkt = 0;
    uint64_t abnormalPkt = 0;
    uint64_t incompleteDrop = 0;
    uint64_t overwrite = 0;
    uint64_t corrupt = 0;
};

} // namespace movesense
