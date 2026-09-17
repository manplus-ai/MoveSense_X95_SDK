// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#include "ModeCatalog.h"

#include "movesense/Simou3Types.h"
#include "movesense/transfer_mode_def.h"

#include <cstdio>

namespace {

const int NO_ROI = -1;
const int ROI_RGB_RECT = static_cast<int>(movesense::RoiStream::RgbRect);
const int ROI_RIGHT_RECT = static_cast<int>(movesense::RoiStream::RightRect);

const ModeSpec MODES[] = {
    { "lrrgb", false, 1, 1, 1, 0, 0, 0, 0, 0, 0, -1, 0, NO_ROI, "Stereo gray + center RGB, full res (no depth/IMU/SEG)" },
    { "rgbd_imu_seg", false, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, NO_ROI, "RGB+depth+IMU+SEG, full res" },
    { "rgbd_low_imu_seg", false, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, NO_ROI, "RGB+depth, pre-downsample (whole 640) +IMU+SEG" },
    { "rgbd_mix_imu_seg", false, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, NO_ROI, "RGB full + depth post-downsample 640 +IMU+SEG" },
    { "lrgbd_imu_seg", false, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, NO_ROI, "Left+RGB+depth+IMU+SEG, full res" },
    { "lrgbd_low_imu_seg", false, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, NO_ROI, "Left+RGB+depth, pre-downsample (whole 640) +IMU+SEG" },
    { "lrgbd_mix_imu_seg", false, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, NO_ROI, "Left full + RGB full + depth post-downsample 640 +IMU+SEG" },
    { "lrrgb_seg_roi", false, 1, 1, 1, 0, 0, 1, 0, 0, 0, -1, 0, ROI_RGB_RECT, "Stereo gray + RGB full res, SEG on RGB, RGB cropped by ROI (no depth/IMU)" },
    { "lr", true, 1, 1, 0, 0, 0, 0, 0, 0, 0, -1, 1, NO_ROI, "Stereo color, full res (no depth/IMU/SEG)" },
    { "lrd_imu_seg", true, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, NO_ROI, "Stereo+depth+IMU+SEG, full res" },
    { "lrd_low_imu_seg", true, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, NO_ROI, "Stereo+depth, pre-downsample (whole 640) +IMU+SEG" },
    { "lrd_mix_imu_seg", true, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, NO_ROI, "Stereo full + depth post-downsample 640 +IMU+SEG" },
    { "lr_seg_roi", true, 1, 1, 0, 0, 0, 1, 0, 0, 0, -1, 1, ROI_RIGHT_RECT, "Stereo color full res, SEG on right, right cropped by ROI (no depth/IMU)" },
};
const int MODE_N = (int)(sizeof(MODES) / sizeof(MODES[0]));

} // namespace

unsigned ModeSpec::Mask() const
{
    unsigned mask = 0;
    if (m_hasL) {
        mask |= (1u << TRANSFER_MODE_L_RECTIFIED_BIT);
    }

    if (m_hasR) {
        mask |= (1u << TRANSFER_MODE_R_RECTIFIED_BIT);
    }

    if (m_hasRgb) {
        mask |= (1u << TRANSFER_MODE_RGB_RECTIFIED_BIT);
    }

    if (m_hasDepth) {
        mask |= (1u << TRANSFER_MODE_DEPTH_BIT);
    }

    if (m_hasSeg) {
        mask |= (1u << TRANSFER_MODE_SEG_BIT);
    }

    if (m_hasImu) {
        mask |= (1u << TRANSFER_MODE_IMU_BIT);
    }

    return mask;
}

int ModeCatalog::Count()
{
    return MODE_N;
}

const ModeSpec* ModeCatalog::Get(int idx)
{
    return (idx >= 0 && idx < MODE_N) ? &MODES[idx] : nullptr;
}

void ModeCatalog::PrintMenu()
{
    printf("\n===================== Mode list =====================\n");
    bool passiveHeaderPrinted = false;
    for (int i = 0; i < MODE_N; ++i) {
        if (!MODES[i].m_passive && i == 0) {
            printf("  [Active A/AP]\n");
        }
        if (MODES[i].m_passive && !passiveHeaderPrinted) {
            printf("  [Passive P]\n");
            passiveHeaderPrinted = true;
        }
        printf("  %2d  %-20s %s\n", i, MODES[i].name.c_str(), MODES[i].m_desc.c_str());
    }
    printf("====================================================\n");
}
