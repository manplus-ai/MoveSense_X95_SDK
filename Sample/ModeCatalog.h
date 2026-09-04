// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Humanplus Intelligent Robotics Technology Co.,Ltd. All rights reserved.

#pragma once
#include <string>

struct ModeSpec {
    std::string name;
    bool m_passive;
    bool m_hasL, m_hasR, m_hasRgb, m_hasDepth, m_hasImu, m_hasSeg;
    bool m_downStereo, m_downRgb, m_downDepth;
    int m_downMode;
    bool m_segOnRight;
    std::string m_desc;

    bool HasStereo() const
    {
        return m_hasL || m_hasR;
    }
    unsigned Mask() const;
};

class ModeCatalog {
public:
    static int Count();
    static const ModeSpec* Get(int idx);
    static void PrintMenu();
};
