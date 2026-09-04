// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "Simou3Net.h"

#include "Simou3Internal.h"
#include "Simou3Log.h"

#include <cstring>

Simou3Net::~Simou3Net()
{
    closeSock();
}

bool Simou3Net::valid() const
{
    return mSock != (socket_t)-1;
}

void Simou3Net::closeSock()
{
    if (mSock != (socket_t)-1) {
        CLOSE_SOCKET(mSock);
        mSock = (socket_t)-1;
    }
}

void Simou3Net::globalInit()
{
#ifdef _WIN32
    struct WsaGuard {
        WsaGuard()
        {
            WSADATA d;
            WSAStartup(MAKEWORD(2, 2), &d);
        }
        ~WsaGuard()
        {
            WSACleanup();
        }
    };
    static WsaGuard g;
#endif
}

int Simou3Net::recvBlock(void* buf, int len)
{
    int need = len;
    unsigned char* p = static_cast<unsigned char*>(buf);
    while (1) {
        int n = recv(mSock, reinterpret_cast<char*>(p) + (len - need), need, 0);
        if (n < 0) {
            simou3_log("recvBlock -1, ERROR: " + std::to_string(GET_SOCKET_ERROR()));
            return -1;
        }
        if (n == 0) {
            simou3_log("recvBlock -2, ERROR: " + std::to_string(GET_SOCKET_ERROR()));
            return -2;
        }
        need -= n;
        if (need == 0) {
            break;
        }
    }
    return len;
}

int Simou3Net::sendBlock(const void* buf, int len)
{
    int need = len;
    const unsigned char* p = static_cast<const unsigned char*>(buf);
    while (1) {
        int n = send(mSock, reinterpret_cast<const char*>(p) + (len - need), need, 0);
        if (n < 0) {
            simou3_log("sendBlock -1");
            return -1;
        }
        if (n == 0) {
            simou3_log("sendBlock -2");
            return -2;
        }
        need -= n;
        if (need == 0) {
            break;
        }
    }
    return len;
}

bool Simou3Net::connectTcp(const std::string& ip, int port, bool noDelay)
{
    globalInit();
    closeSock();
    socket_t sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET_VALUE) {
        simou3_log("Socket creation failed: " + std::to_string(GET_SOCKET_ERROR()));
        return false;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = PF_INET;
#ifdef _WIN32
    inet_pton(AF_INET, PCSTR(ip.c_str()), &addr.sin_addr.s_addr);
#else
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
#endif
    addr.sin_port = htons(port);
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        simou3_log("Connect failed.");
        CLOSE_SOCKET(sock);
        return false;
    }
    if (noDelay) {
        int optval = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&optval), sizeof(optval));
    }
    mSock = sock;
    return true;
}

bool Simou3Net::openUdp(int recvTimeoutMs)
{
    globalInit();
    closeSock();
    mSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (mSock == INVALID_SOCKET_VALUE) {
        simou3_log("Socket creation failed: " + std::to_string(GET_SOCKET_ERROR()));
        mSock = (socket_t)-1;
        return false;
    }
    if (recvTimeoutMs > 0) {
#ifdef _WIN32
        DWORD tv = recvTimeoutMs;
        setsockopt(mSock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
        struct timeval tv;
        tv.tv_sec = recvTimeoutMs / 1000;
        tv.tv_usec = (recvTimeoutMs % 1000) * 1000;
        setsockopt(mSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    }
    return true;
}

bool Simou3Net::enableBroadcast()
{
    int on = 1;
    if (setsockopt(mSock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&on), sizeof(on)) == SOCKET_ERR) {
        simou3_log("Set broadcast failed: " + std::to_string(GET_SOCKET_ERROR()));
        return false;
    }
    int reuse = 1;
    setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    return true;
}

bool Simou3Net::bindTo(const std::string& ip, int port)
{
    sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = inet_addr(ip.c_str());
    local.sin_port = htons(port);
    if (bind(mSock, reinterpret_cast<struct sockaddr*>(&local), sizeof(local)) == SOCKET_ERR) {
        simou3_log("Bind to " + ip + " failed: " + std::to_string(GET_SOCKET_ERROR()));
        return false;
    }
    return true;
}

int Simou3Net::sendTo(const std::string& ip, int port, const void* buf, int len)
{
    sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr(ip.c_str());
    to.sin_port = htons(port);
    return sendto(mSock, reinterpret_cast<const char*>(buf), len, 0, reinterpret_cast<struct sockaddr*>(&to), sizeof(to));
}

int Simou3Net::recvFrom(void* buf, int len, std::string* senderIp, int* senderPort)
{
    sockaddr_in from;
    socklen_t fromLen = sizeof(from);
    int n = recvfrom(mSock, reinterpret_cast<char*>(buf), len, 0, reinterpret_cast<struct sockaddr*>(&from), &fromLen);
    if (n >= 0) {
        if (senderIp) {
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(from.sin_addr), ipStr, INET_ADDRSTRLEN);
            *senderIp = ipStr;
        }
        if (senderPort) {
            *senderPort = ntohs(from.sin_port);
        }
    }
    return n;
}

std::vector<NetworkInterface> Simou3Net::enumInterfaces()
{
    std::vector<NetworkInterface> interfaces;

#ifdef _WIN32
    ULONG bufferSize = 15000;
    std::vector<unsigned char> addrBuf(bufferSize);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(addrBuf.data());

    ULONG result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &bufferSize);

    if (result == ERROR_BUFFER_OVERFLOW) {
        addrBuf.resize(bufferSize);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(addrBuf.data());
        result = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &bufferSize);
    }

    if (result == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            if (pCurrAddresses->OperStatus == IfOperStatusUp) {
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                while (pUnicast) {
                    if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                        NetworkInterface netif;

                        char name[256];
                        wcstombs(name, pCurrAddresses->FriendlyName, sizeof(name));
                        netif.name = name;

                        sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                        netif.ip = ipStr;

                        ULONG mask = 0xFFFFFFFF << (32 - pUnicast->OnLinkPrefixLength);
                        struct in_addr maskAddr;
                        maskAddr.s_addr = htonl(mask);
                        inet_ntop(AF_INET, &maskAddr, ipStr, INET_ADDRSTRLEN);
                        netif.netmask = ipStr;

                        ULONG ipAddr = ntohl(sa_in->sin_addr.s_addr);
                        ULONG broadcastAddr = ipAddr | (~mask);

                        struct in_addr broadcast;
                        broadcast.s_addr = htonl(broadcastAddr);
                        inet_ntop(AF_INET, &broadcast, ipStr, INET_ADDRSTRLEN);
                        netif.broadcast = ipStr;

                        interfaces.push_back(netif);

                        simou3_log("Found interface: " + netif.name + " IP: " + netif.ip + " Broadcast: " + netif.broadcast);
                    }
                    pUnicast = pUnicast->Next;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        simou3_log("GetAdaptersAddresses failed with error: " + std::to_string(result));
    }

#else
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        simou3_log(std::string("getifaddrs failed: ") + strerror(errno));
        return interfaces;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (ifa->ifa_flags & IFF_LOOPBACK)
                continue;

            if (!(ifa->ifa_flags & IFF_UP))
                continue;

            NetworkInterface netif;
            netif.name = ifa->ifa_name;

            struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN);
            netif.ip = ipStr;

            if (ifa->ifa_netmask) {
                struct sockaddr_in* netmask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask);
                inet_ntop(AF_INET, &(netmask->sin_addr), ipStr, INET_ADDRSTRLEN);
                netif.netmask = ipStr;
            }

            if (ifa->ifa_flags & IFF_BROADCAST && ifa->ifa_broadaddr) {
                struct sockaddr_in* broadcast = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_broadaddr);
                inet_ntop(AF_INET, &(broadcast->sin_addr), ipStr, INET_ADDRSTRLEN);
                netif.broadcast = ipStr;
            }

            interfaces.push_back(netif);

            simou3_log("Found interface: " + netif.name + " IP: " + netif.ip + " Broadcast: " + netif.broadcast);
        }
    }

    freeifaddrs(ifaddr);
#endif

    return interfaces;
}
