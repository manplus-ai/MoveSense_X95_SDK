// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include <cstdio>
#include <string>

inline void simou3_log(const std::string& msg)
{
    printf("%s\n", msg.c_str());
}
