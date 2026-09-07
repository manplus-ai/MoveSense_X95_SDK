// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "movesense/Simou3Camera.h"

#ifndef MOVESENSE_VERSION_MAJOR
#define MOVESENSE_VERSION_MAJOR 0
#endif
#ifndef MOVESENSE_VERSION_MINOR
#define MOVESENSE_VERSION_MINOR 0
#endif
#ifndef MOVESENSE_VERSION_PATCH
#define MOVESENSE_VERSION_PATCH 0
#endif

#define SIMOU3_STR2(x) #x
#define SIMOU3_STR(x) SIMOU3_STR2(x)

namespace movesense {

void simou3_get_version(int* major, int* minor, int* patch)
{
    if (major) {
        *major = MOVESENSE_VERSION_MAJOR;
    }
    if (minor) {
        *minor = MOVESENSE_VERSION_MINOR;
    }
    if (patch) {
        *patch = MOVESENSE_VERSION_PATCH;
    }
}

const char* simou3_get_version_string()
{
    static const char kVersion[] =
        SIMOU3_STR(MOVESENSE_VERSION_MAJOR) "." SIMOU3_STR(MOVESENSE_VERSION_MINOR) "." SIMOU3_STR(MOVESENSE_VERSION_PATCH);
    return kVersion;
}

} // namespace movesense
