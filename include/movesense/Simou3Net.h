// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "Simou3Types.h"

#include <atomic>

namespace movesense {

class Simou3Net {
public:
    Simou3Net() = default;
    ~Simou3Net();
    Simou3Net(const Simou3Net&) = delete;
    Simou3Net& operator=(const Simou3Net&) = delete;

    bool connectTcp(const std::string& ip, int port, bool noDelay = true);
    int recvBlock(void* buf, int len);
    int recvStream(void* buf, int len, const std::atomic<bool>& running);
    int sendBlock(const void* buf, int len);

    bool openUdp(int recvTimeoutMs = 0);
    bool enableBroadcast();
    bool bindTo(const std::string& ip, int port = 0);
    int sendTo(const std::string& ip, int port, const void* buf, int len);
    int recvFrom(void* buf, int len, std::string* senderIp = nullptr, int* senderPort = nullptr);

    void shutdownSock();
    void closeSock();
    bool valid() const;

    static void globalInit();
    static std::vector<NetworkInterface> enumInterfaces();

private:
    socket_t mSock = (socket_t)-1;
};

} // namespace movesense
