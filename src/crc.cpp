// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "crc.h"

namespace {
struct CrcTable {
    uint32_t t[256];
    CrcTable()
    {
        for (int i = 0; i < 256; i++) {
            uint32_t crc = static_cast<uint32_t>(i);
            for (int j = 0; j < 8; j++) {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320u;
                else
                    crc >>= 1;
            }
            t[i] = crc;
        }
    }
};

const CrcTable& crcTable()
{
    static const CrcTable inst;
    return inst;
}
} // namespace

void crc_init(CrcCtx* ctx)
{
    ctx->value = 0;
}

uint32_t crc_add_buffer(CrcCtx* ctx, const unsigned char* buffer, unsigned len)
{
    const uint32_t* T = crcTable().t;
    for (unsigned i = 0; i < len; i++) {
        ctx->value = (ctx->value >> 8) ^ T[(ctx->value & 0xFF) ^ buffer[i]];
    }
    return ctx->value;
}
