// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "Simou3Camera.h"

#ifndef SIMOU3_VERSION_MAJOR
#define SIMOU3_VERSION_MAJOR 0
#endif
#ifndef SIMOU3_VERSION_MINOR
#define SIMOU3_VERSION_MINOR 0
#endif
#ifndef SIMOU3_VERSION_PATCH
#define SIMOU3_VERSION_PATCH 0
#endif

#define SIMOU3_STR2(x) #x
#define SIMOU3_STR(x)  SIMOU3_STR2(x)

void simou3_get_version(int* major, int* minor, int* patch)
{
	if (major) {
		*major = SIMOU3_VERSION_MAJOR;
	}
	if (minor) {
		*minor = SIMOU3_VERSION_MINOR;
	}
	if (patch) {
		*patch = SIMOU3_VERSION_PATCH;
	}
}

const char* simou3_get_version_string()
{
	static const char kVersion[] =
		SIMOU3_STR(SIMOU3_VERSION_MAJOR) "." SIMOU3_STR(SIMOU3_VERSION_MINOR) "." SIMOU3_STR(SIMOU3_VERSION_PATCH);
	return kVersion;
}
