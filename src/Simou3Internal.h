// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "Simou3Types.h"

#include <chrono>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include "iphlpapi.h"

#include <WS2tcpip.h>
#include <cstddef>
#define CLOSE_SOCKET closesocket
#define GET_SOCKET_ERROR() WSAGetLastError()
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define SOCKET_ERR SOCKET_ERROR
#else
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <string.h>
#define CLOSE_SOCKET close
#define GET_SOCKET_ERROR() errno
#define INVALID_SOCKET_VALUE -1
#define SOCKET_ERR -1
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable : 4996)
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
