// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include <string>
#include <vector>
#include "Simou3Net.h"

class Simou3CameraScan
{
public:
	Simou3CameraScan(int port = 5004, int timeoutMS = 2000);
	~Simou3CameraScan();

	int scanCamera(std::string ip, CameraInfo& cameraInfo);
	std::vector<CameraInfo> scanBroadcast(std::string broadcastIP = "255.255.255.255");
	std::vector<CameraInfo> scanAllInterfaces();
	std::vector<NetworkInterface> getAllNetworkInterfaces();
	void setScanPort(int port);
	void setTimeout(int timeoutMS);

private:
	int mPort;
	int mTimeoutMS;
	Simou3Net mSock;

	int sendScanCommand(std::string ip, CameraInfo& cameraInfo);
	std::vector<CameraInfo> scanBroadcastOnInterface(const NetworkInterface& netif);
	int initSocket();
	void closeSocket();
};
