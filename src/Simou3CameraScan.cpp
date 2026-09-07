// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "movesense/Simou3CameraScan.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"
#include "movesense/transfer_mode_def.h"

#include <set>

Simou3CameraScan::Simou3CameraScan(int port, int timeoutMS)
{
    mPort = port;
    mTimeoutMS = timeoutMS;
}

Simou3CameraScan::~Simou3CameraScan()
{
    closeSocket();
}

void Simou3CameraScan::setScanPort(int port)
{
    mPort = port;
}

void Simou3CameraScan::setTimeout(int timeoutMS)
{
    mTimeoutMS = timeoutMS;
}

int Simou3CameraScan::initSocket()
{
    if (!mSock.openUdp(mTimeoutMS)) {
        return -1;
    }
    return 0;
}

void Simou3CameraScan::closeSocket()
{
    mSock.closeSock();
}

std::vector<NetworkInterface> Simou3CameraScan::getAllNetworkInterfaces()
{
    return Simou3Net::enumInterfaces();
}

std::vector<CameraInfo> Simou3CameraScan::scanBroadcastOnInterface(const NetworkInterface& netif)
{
    if (netif.broadcast.empty()) {
        simou3_log("No broadcast address for interface: " + netif.name);
        return std::vector<CameraInfo>();
    }
    std::vector<CameraInfo> cameras;
    if (initSocket() < 0) {
        return cameras;
    }
    if (!mSock.enableBroadcast()) {
        closeSocket();
        return cameras;
    }
    if (!mSock.bindTo(netif.ip)) {
        closeSocket();
        return cameras;
    }

    char sendBuffer[8];
    char recvBuffer[100];
    uint32_t mode = MODE_SCAN;
    uint32_t len = 0;
    std::memcpy(sendBuffer, &mode, sizeof(mode));
    std::memcpy(sendBuffer + 4, &len, sizeof(len));

    if (mSock.sendTo(netif.broadcast, mPort, sendBuffer, 8) < 0) {
        simou3_log("Broadcast sendto failed: " + std::to_string(GET_SOCKET_ERROR()));
        closeSocket();
        return cameras;
    }

    while (true) {
        std::string senderIp;
        int senderPort = 0;
        memset(recvBuffer, 0, sizeof(recvBuffer));
        int recvLen = mSock.recvFrom(recvBuffer, sizeof(recvBuffer), &senderIp, &senderPort);
        if (recvLen < 0) {
            break;
        }

        if (recvLen >= 8) {
            uint32_t recvMode = 0;
            uint32_t recvDataLen = 0;
            std::memcpy(&recvMode, recvBuffer, sizeof(recvMode));
            std::memcpy(&recvDataLen, recvBuffer + 4, sizeof(recvDataLen));

            if (recvMode == MODE_SCAN) {
                CameraInfo cameraInfo;
                cameraInfo.ip = senderIp;
                cameraInfo.port = senderPort;
                cameraInfo.mode = recvMode;
                cameraInfo.len = recvDataLen;

                if (recvLen >= 18 && recvDataLen >= 10) {
                    const unsigned char* payload = reinterpret_cast<const unsigned char*>(recvBuffer + 8);
                    char macBuf[20];
                    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X", payload[0], payload[1], payload[2], payload[3], payload[4],
                        payload[5]);
                    cameraInfo.mac = macBuf;

                    struct in_addr gwAddr;
                    std::memcpy(&gwAddr, payload + 6, 4);
                    char gwStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &gwAddr, gwStr, INET_ADDRSTRLEN);
                    cameraInfo.gateway = gwStr;

                    if (recvLen >= 22 && recvDataLen >= 14) {
                        struct in_addr maskAddr;
                        std::memcpy(&maskAddr, payload + 10, 4);
                        char maskStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &maskAddr, maskStr, INET_ADDRSTRLEN);
                        cameraInfo.netmask = maskStr;
                    }
                }

                cameras.push_back(cameraInfo);
            }
        }
    }

    closeSocket();
    return cameras;
}

std::vector<CameraInfo> Simou3CameraScan::scanAllInterfaces()
{
    std::vector<CameraInfo> allCameras;
    std::set<std::string> uniqueIPs;

    std::vector<NetworkInterface> interfaces = getAllNetworkInterfaces();

    if (interfaces.empty()) {
        simou3_log("No network interfaces found");
        return allCameras;
    }

    simou3_log("Found " + std::to_string(interfaces.size()) + " network interface(s)");

    for (const auto& netif : interfaces) {
        std::vector<CameraInfo> cameras = scanBroadcastOnInterface(netif);

        for (const auto& cam : cameras) {
            if (uniqueIPs.find(cam.ip) == uniqueIPs.end()) {
                uniqueIPs.insert(cam.ip);
                allCameras.push_back(cam);
            }
        }
    }

    simou3_log("Total found " + std::to_string(allCameras.size()) + " unique camera(s)");

    return allCameras;
}

int Simou3CameraScan::sendScanCommand(std::string ip, CameraInfo& cameraInfo)
{
    char sendBuffer[8];
    char recvBuffer[100];
    uint32_t mode = MODE_SCAN;
    uint32_t len = 0;
    std::memcpy(sendBuffer, &mode, sizeof(mode));
    std::memcpy(sendBuffer + 4, &len, sizeof(len));

    if (mSock.sendTo(ip, mPort, sendBuffer, 8) < 0) {
        simou3_log("Sendto failed: " + std::to_string(GET_SOCKET_ERROR()));
        return -1;
    }

    std::string senderIp;
    int senderPort = 0;
    int recvLen = mSock.recvFrom(recvBuffer, sizeof(recvBuffer), &senderIp, &senderPort);
    if (recvLen < 0) {
        return -1;
    }

    if (recvLen >= 8) {
        uint32_t recvMode = 0;
        uint32_t recvDataLen = 0;
        std::memcpy(&recvMode, recvBuffer, sizeof(recvMode));
        std::memcpy(&recvDataLen, recvBuffer + 4, sizeof(recvDataLen));

        if (recvMode == MODE_SCAN) {
            cameraInfo.ip = senderIp;
            cameraInfo.port = senderPort;
            cameraInfo.mode = recvMode;
            cameraInfo.len = recvDataLen;

            if (recvLen >= 18 && recvDataLen >= 10) {
                const unsigned char* payload = reinterpret_cast<const unsigned char*>(recvBuffer + 8);
                char macBuf[20];
                snprintf(
                    macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5]);
                cameraInfo.mac = macBuf;

                struct in_addr gwAddr;
                std::memcpy(&gwAddr, payload + 6, 4);
                char gwStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &gwAddr, gwStr, INET_ADDRSTRLEN);
                cameraInfo.gateway = gwStr;
            }

            return 0;
        }
    }

    return -1;
}

int Simou3CameraScan::scanCamera(std::string ip, CameraInfo& cameraInfo)
{
    if (initSocket() < 0) {
        return -1;
    }

    int ret = sendScanCommand(ip, cameraInfo);

    closeSocket();

    return ret;
}

std::vector<CameraInfo> Simou3CameraScan::scanBroadcast(std::string broadcastIP)
{
    if (broadcastIP == "255.255.255.255") {
        return scanAllInterfaces();
    }
    std::vector<CameraInfo> cameras;
    if (initSocket() < 0) {
        return cameras;
    }
    if (!mSock.enableBroadcast()) {
        closeSocket();
        return cameras;
    }
    if (!mSock.bindTo("0.0.0.0")) {
        closeSocket();
        return cameras;
    }

    char sendBuffer[8];
    char recvBuffer[100];
    uint32_t mode = MODE_SCAN;
    uint32_t len = 0;
    std::memcpy(sendBuffer, &mode, sizeof(mode));
    std::memcpy(sendBuffer + 4, &len, sizeof(len));

    if (mSock.sendTo(broadcastIP, mPort, sendBuffer, 8) < 0) {
        simou3_log("Broadcast sendto failed: " + std::to_string(GET_SOCKET_ERROR()));
        closeSocket();
        return cameras;
    }

    while (true) {
        std::string senderIp;
        int senderPort = 0;
        memset(recvBuffer, 0, sizeof(recvBuffer));
        int recvLen = mSock.recvFrom(recvBuffer, sizeof(recvBuffer), &senderIp, &senderPort);
        if (recvLen < 0)
            break;

        if (recvLen >= 8) {
            uint32_t recvMode = 0;
            uint32_t recvDataLen = 0;
            std::memcpy(&recvMode, recvBuffer, sizeof(recvMode));
            std::memcpy(&recvDataLen, recvBuffer + 4, sizeof(recvDataLen));

            if (recvMode == MODE_SCAN) {
                CameraInfo cameraInfo;
                cameraInfo.ip = senderIp;
                cameraInfo.port = senderPort;
                cameraInfo.mode = recvMode;
                cameraInfo.len = recvDataLen;

                if (recvLen >= 18 && recvDataLen >= 10) {
                    const unsigned char* payload = reinterpret_cast<const unsigned char*>(recvBuffer + 8);
                    char macBuf[20];
                    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X", payload[0], payload[1], payload[2], payload[3], payload[4],
                        payload[5]);
                    cameraInfo.mac = macBuf;

                    struct in_addr gwAddr;
                    std::memcpy(&gwAddr, payload + 6, 4);
                    char gwStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &gwAddr, gwStr, INET_ADDRSTRLEN);
                    cameraInfo.gateway = gwStr;

                    if (recvLen >= 22 && recvDataLen >= 14) {
                        struct in_addr maskAddr;
                        std::memcpy(&maskAddr, payload + 10, 4);
                        char maskStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &maskAddr, maskStr, INET_ADDRSTRLEN);
                        cameraInfo.netmask = maskStr;
                    }
                }

                cameras.push_back(cameraInfo);
            }
        }
    }

    closeSocket();
    return cameras;
}
