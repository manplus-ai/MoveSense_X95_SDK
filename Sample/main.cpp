// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "SampleApp.h"

#include <cstdio>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    SampleApp app;
    int rc = app.Run(argc, argv);

#ifdef _WIN32
    printf("\n[Sample] Finished (rc=%d), press any key to exit...\n", rc);
    fflush(stdout);
    _getch();
#endif
    return rc;
}
