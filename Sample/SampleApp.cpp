// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "SampleApp.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace movesense;

namespace fs = std::filesystem;

namespace {

const char* WIN_CTRL = "ctrl";
const int GAIN_UNIT = 128;
const int EXPO_MAX_US = 65535;
const int FPS_MIN = 5;
const int FPS_MAX = 25;

std::atomic<bool> s_abort { false };
void OnSigint(int)
{
    s_abort.store(true);
}

std::string ExeDir()
{
#ifdef _WIN32
    char buf[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    return fs::path(buf).parent_path().string();
#else
    return ".";
#endif
}

bool PlaneToMat(const MovesenseFrame::Plane& p, cv::Mat& out)
{
    if (!p.has || p.data.empty() || p.width <= 0 || p.height <= 0) {
        return false;
    }
    size_t needY = (size_t)p.width * p.height;
    if (p.data.size() >= needY * 3 / 2) {
        cv::Mat yuv(p.height * 3 / 2, p.width, CV_8UC1, (void*)p.data.data());
        cv::cvtColor(yuv, out, cv::COLOR_YUV2BGR_NV21);
        return true;
    }
    if (p.data.size() >= needY) {
        cv::Mat gray(p.height, p.width, CV_8UC1, (void*)p.data.data());
        out = gray.clone();
        return true;
    }
    return false;
}

bool DepthToMat16(const MovesenseFrame::Plane& p, cv::Mat& out)
{
    if (!p.has || p.data.empty() || p.width <= 0 || p.height <= 0) {
        return false;
    }
    if (p.data.size() < (size_t)p.width * p.height * 2) {
        return false;
    }
    cv::Mat d16(p.height, p.width, CV_16UC1, (void*)p.data.data());
    out = d16.clone();
    return true;
}

void ShowPlane(const char* win, const MovesenseFrame::Plane& p)
{
    cv::Mat img;
    if (PlaneToMat(p, img)) {
        cv::imshow(win, img);
    }
}

void ShowDepth(const char* win, const MovesenseFrame::Plane& p)
{
    cv::Mat d16;
    if (!DepthToMat16(p, d16)) {
        return;
    }
    cv::Mat d8, color;
    d16.convertTo(d8, CV_8UC1, 1.0 / 20.0);
    cv::applyColorMap(d8, color, cv::COLORMAP_JET);
    cv::imshow(win, color);
}

const char* kCocoNames[80] = { "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant",
    "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard",
    "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop",
    "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors",
    "teddy bear", "hair drier", "toothbrush" };
const char* CocoName(int classId)
{
    return (classId >= 0 && classId < 80) ? kCocoNames[classId] : "?";
}

void DrawSeg(cv::Mat& bgr, const std::vector<DetectionBox>& dets, int coordW, int coordH)
{
    if (dets.empty() || coordW <= 0 || coordH <= 0) {
        return;
    }
    float sx = (float)bgr.cols / (float)coordW;
    float sy = (float)bgr.rows / (float)coordH;
    for (const DetectionBox& b : dets) {
        cv::Point p1((int)(b.x1 * sx), (int)(b.y1 * sy));
        cv::Point p2((int)(b.x2 * sx), (int)(b.y2 * sy));
        cv::rectangle(bgr, p1, p2, cv::Scalar(0, 255, 0), 2);
        char text[64];
        snprintf(text, sizeof(text), "%s %.2f", CocoName(b.classId), b.score);
        cv::putText(bgr, text, cv::Point(p1.x, p1.y - 4), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
    }
}

void ShowPlaneWithSeg(const char* win, const MovesenseFrame::Plane& img, const std::vector<DetectionBox>& dets, int coordW, int coordH)
{
    cv::Mat bgr;
    if (!PlaneToMat(img, bgr)) {
        return;
    }
    DrawSeg(bgr, dets, coordW, coordH);
    cv::imshow(win, bgr);
}

} // namespace

int SampleApp::Run(int argc, char** argv)
{
    std::signal(SIGINT, OnSigint);

    if (!SelectMode(argc, argv)) {
        return 0;
    }
    printf("[Sample] mode = %s (%s)\n", m_mode->name.c_str(), m_mode->m_desc.c_str());
    fflush(stdout);

    if (!m_session.ScanAndSelect()) {
        return 0;
    }

    if (!m_session.Open(*m_mode)) {
        return 0;
    }

    SetupControls();
    printf("[Sample] free-run; c=grab / r=record IMU / q|ESC=quit\n");
    fflush(stdout);

    m_frameReceiver.Start(&m_session);
    if (m_mode->m_hasImu) {
        m_imuReceiver.Start(&m_session);
    }

    bool running = true;
    bool captureReq = false;
    std::chrono::steady_clock::time_point lastStatus = std::chrono::steady_clock::now();

    while (running && !s_abort.load()) {
        MovesenseFrame frame;
        if (m_frameReceiver.TryGetLatest(frame)) {
            RenderFrame(frame);
            if (captureReq) {
                Snapshot(frame);
                captureReq = false;
            }
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - lastStatus).count() >= 1.0) {
            printf("[status] %.1f fps", m_frameReceiver.Fps());
            if (m_mode->m_hasImu) {
                printf(" | IMU %.0f Hz", m_imuReceiver.Rate());
            }
            printf("%s\n", m_recording ? " [REC]" : "");
            fflush(stdout);
            lastStatus = now;
        }

        int key = cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 27) {
            running = false;
        } else if (key == 'c' || key == 'C') {
            captureReq = true;
        } else if (key == 'r' || key == 'R') {
            ToggleRecord();
        }
    }

    m_frameReceiver.Stop();
    m_imuReceiver.Stop();
    m_session.Close();
    return 0;
}

bool SampleApp::SelectMode(int argc, char** argv)
{
    if (argc > 1) {
        int idx = atoi(argv[1]);
        m_mode = ModeCatalog::Get(idx);
        if (!m_mode) {
            printf("[Sample] Invalid mode index %d\n", idx);
            return false;
        }
        return true;
    }
    ModeCatalog::PrintMenu();
    printf("Enter mode index (Enter to confirm): ");
    fflush(stdout);
    int idx = -1;
    std::cin >> idx;
    m_mode = ModeCatalog::Get(idx);
    if (!m_mode) {
        printf("[Sample] Invalid mode index\n");
        return false;
    }
    return true;
}

void SampleApp::SetupControls()
{
    cv::namedWindow(WIN_CTRL, cv::WINDOW_NORMAL);
    cv::resizeWindow(WIN_CTRL, 520, 420);
    cv::createTrackbar("Stereo AutoExpo", WIN_CTRL, &m_tbStereoAuto, 1, OnStereoAuto, this);
    cv::createTrackbar("Stereo Expo(us)", WIN_CTRL, &m_tbStereoExpo, EXPO_MAX_US, OnStereoExpo, this);
    cv::createTrackbar("Stereo Gain(x)", WIN_CTRL, &m_tbStereoGain, 16, OnStereoGain, this);
    cv::createTrackbar("Stereo MaxExpo(us)", WIN_CTRL, &m_tbStereoMaxExpo, EXPO_MAX_US, OnStereoMaxExpo, this);
    cv::setTrackbarMin("Stereo MaxExpo(us)", WIN_CTRL, 100);
    cv::createTrackbar("Stereo MaxGain(x)", WIN_CTRL, &m_tbStereoMaxGain, 32, OnStereoMaxGain, this);
    if (!m_mode->m_passive) {
        cv::createTrackbar("RGB AutoExpo", WIN_CTRL, &m_tbRgbAuto, 1, OnRgbAuto, this);
        cv::createTrackbar("RGB Expo(us)", WIN_CTRL, &m_tbRgbExpo, EXPO_MAX_US, OnRgbExpo, this);
        cv::createTrackbar("RGB Gain(x)", WIN_CTRL, &m_tbRgbGain, 16, OnRgbGain, this);
        cv::createTrackbar("RGB MaxExpo(us)", WIN_CTRL, &m_tbRgbMaxExpo, EXPO_MAX_US, OnRgbMaxExpo, this);
        cv::setTrackbarMin("RGB MaxExpo(us)", WIN_CTRL, 100);
        cv::createTrackbar("RGB MaxGain(x)", WIN_CTRL, &m_tbRgbMaxGain, 32, OnRgbMaxGain, this);
        cv::createTrackbar("DOE(0-255)", WIN_CTRL, &m_tbDoePower, 255, OnDoePower, this);
    }
    cv::createTrackbar("FPS", WIN_CTRL, &m_tbFps, FPS_MAX, OnFps, this);
    cv::setTrackbarMin("FPS", WIN_CTRL, FPS_MIN);
}

void SampleApp::RenderFrame(const MovesenseFrame& frame)
{
    if (!m_resolutionPrinted && (frame.depth().has || frame.rgbRect().has || frame.leftRect().has)) {
        printf("[resolution] L=%dx%d R=%dx%d RGB=%dx%d depth=%dx%d\n", frame.leftRect().width, frame.leftRect().height, frame.rightRect().width,
            frame.rightRect().height, frame.rgbRect().width, frame.rgbRect().height, frame.depth().width, frame.depth().height);
        fflush(stdout);
        m_resolutionPrinted = true;
    }

    bool segOnRgb = m_mode->m_hasSeg && !m_mode->m_segOnRight;
    bool segOnRight = m_mode->m_hasSeg && m_mode->m_segOnRight;

    if (m_mode->m_hasL) {
        ShowPlane("L", frame.leftRect());
    }
    if (m_mode->m_hasRgb) {
        if (segOnRgb) {
            ShowPlaneWithSeg("RGB", frame.rgbRect(), frame.detections(), frame.detCoordW(), frame.detCoordH());
        } else {
            ShowPlane("RGB", frame.rgbRect());
        }
    }
    if (m_mode->m_hasR) {
        if (segOnRight) {
            ShowPlaneWithSeg("R", frame.rightRect(), frame.detections(), frame.detCoordW(), frame.detCoordH());
        } else {
            ShowPlane("R", frame.rightRect());
        }
    }
    if (m_mode->m_hasDepth) {
        ShowDepth("Depth", frame.depth());
    }
}

void SampleApp::Snapshot(const MovesenseFrame& frame)
{
    int idx = m_snapIdx++;
    char name[128];
    cv::Mat img;
    if (m_mode->m_hasRgb && PlaneToMat(frame.rgbRect(), img)) {
        snprintf(name, sizeof(name), "snap_%03d_rgb.png", idx);
        cv::imwrite(name, img);
    }
    if (m_mode->m_hasL && PlaneToMat(frame.leftRect(), img)) {
        snprintf(name, sizeof(name), "snap_%03d_L.png", idx);
        cv::imwrite(name, img);
    }
    if (m_mode->m_hasR && PlaneToMat(frame.rightRect(), img)) {
        snprintf(name, sizeof(name), "snap_%03d_R.png", idx);
        cv::imwrite(name, img);
    }
    cv::Mat d16;
    if (m_mode->m_hasDepth && DepthToMat16(frame.depth(), d16)) {
        snprintf(name, sizeof(name), "snap_%03d_depth.png", idx);
        cv::imwrite(name, d16);
    }
    printf("[grab] snap_%03d_* saved (dir=%s)\n", idx, ExeDir().c_str());
    fflush(stdout);
}

void SampleApp::ToggleRecord()
{
    if (!m_mode->m_hasImu) {
        printf("[record] current mode has no IMU\n");
        fflush(stdout);
        return;
    }
    m_imuReceiver.ToggleRecord();
    m_recording = m_imuReceiver.Recording();
}

void SampleApp::OnStereoAuto(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setStereoAutoExpo(value ? 1 : 0);
    }
}
void SampleApp::OnStereoExpo(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setStereoExposure((unsigned)value);
    }
}
void SampleApp::OnStereoGain(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setStereoGain((float)(value * GAIN_UNIT));
    }
}
void SampleApp::OnStereoMaxExpo(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setStereoMaxExposure((unsigned)value);
    }
}
void SampleApp::OnStereoMaxGain(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setStereoMaxGain((float)(value * GAIN_UNIT));
    }
}
void SampleApp::OnRgbAuto(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setRGBAutoExpo(value ? 1 : 0);
    }
}
void SampleApp::OnRgbExpo(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setRGBExposure((unsigned)value);
    }
}
void SampleApp::OnRgbGain(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setRGBGain((float)(value * GAIN_UNIT));
    }
}
void SampleApp::OnRgbMaxExpo(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setRGBMaxExposure((unsigned)value);
    }
}
void SampleApp::OnRgbMaxGain(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam) {
        cam->setRGBMaxGain((float)(value * GAIN_UNIT));
    }
}
void SampleApp::OnFps(int value, void* userdata)
{
    Simou3Camera* cam = static_cast<SampleApp*>(userdata)->m_session.Cam();
    if (cam && value >= FPS_MIN) {
        cam->setFrameRate(value);
    }
}

void SampleApp::ApplyDoe()
{
    Simou3Camera* cam = m_session.Cam();
    if (!cam) {
        return;
    }
    cam->setDOEPower((unsigned)m_tbDoePower);
}
void SampleApp::OnDoePower(int, void* userdata)
{
    static_cast<SampleApp*>(userdata)->ApplyDoe();
}
