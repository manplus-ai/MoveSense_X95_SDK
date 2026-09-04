// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include <cstdint>

struct CrcCtx {
    uint32_t value;
};

void crc_init(CrcCtx* ctx);
uint32_t crc_add_buffer(CrcCtx* ctx, const unsigned char* buffer, unsigned len);
