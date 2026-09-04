// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include "CameraSession.h"
#include "FrameReceiver.h"
#include "ImuReceiver.h"
#include "ModeCatalog.h"
#include "Simou3Camera.h"

class SampleApp {
public:
    int Run(int argc, char** argv);

private:
    bool SelectMode(int argc, char** argv);
    void SetupControls();
    void RenderFrame(const MovesenseFrame& frame);
    void Snapshot(const MovesenseFrame& frame);
    void ToggleRecord();

    static void OnStereoAuto(int value, void* userdata);
    static void OnStereoExpo(int value, void* userdata);
    static void OnStereoGain(int value, void* userdata);
    static void OnStereoMaxExpo(int value, void* userdata);
    static void OnStereoMaxGain(int value, void* userdata);
    static void OnRgbAuto(int value, void* userdata);
    static void OnRgbExpo(int value, void* userdata);
    static void OnRgbGain(int value, void* userdata);
    static void OnRgbMaxExpo(int value, void* userdata);
    static void OnRgbMaxGain(int value, void* userdata);
    static void OnFps(int value, void* userdata);
    static void OnDoePower(int value, void* userdata);
    void ApplyDoe();

    const ModeSpec* m_mode = nullptr;
    CameraSession m_session;
    FrameReceiver m_frameReceiver;
    ImuReceiver m_imuReceiver;

    int m_tbStereoAuto = 0, m_tbStereoExpo = 5000, m_tbStereoGain = 4;
    int m_tbRgbAuto = 0, m_tbRgbExpo = 5000, m_tbRgbGain = 4;
    int m_tbStereoMaxExpo = 65535, m_tbStereoMaxGain = 16;
    int m_tbRgbMaxExpo = 65535, m_tbRgbMaxGain = 16;
    int m_tbFps = 15;
    int m_tbDoePower = 0;
    bool m_recording = false;
    bool m_resolutionPrinted = false;
    int m_snapIdx = 0;
};
