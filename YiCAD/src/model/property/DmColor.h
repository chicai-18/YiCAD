/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmColor.h
/// @brief 颜色类，支持RGBA颜色值及CAD索引颜色

#ifndef DMCOLOR_H
#define DMCOLOR_H

#include <QString>
#include "Datamodel.h"
#include "DmFlags.h"

// Color class.
class DmColor : public DmFlags
{
TYPESYSTEM_HEADER();
public:
    DmColor();
    DmColor(int r, int g, int b);
    DmColor(int r, int g, int b, int a);
    DmColor(const DmColor& c);
    explicit DmColor(unsigned int f);

    /// @brief 获取红色分量
    /// @return 红色分量值 (0-255)
    int red() const;

    /// @brief 获取绿色分量
    /// @return 绿色分量值 (0-255)
    int green() const;

    /// @brief 获取蓝色分量
    /// @return 蓝色分量值 (0-255)
    int blue() const;

    /// @brief 获取透明度分量
    /// @return 透明度分量值 (0-255)
    int alpha() const;

    void setRed(int r);
    void setGreen(int g);
    void setBlue(int b);
    void setAlpha(int a);
    void setRgb(int r, int g, int b);
    void setRgba(int r, int g, int b, int a);

    /// @brief 从CAD颜色索引创建DmColor
    /// @param colorIdx CAD颜色索引
    /// @param r 红色分量
    /// @param g 绿色分量
    /// @param b 蓝色分量
    /// @return DmColor对象
    static DmColor fromCAD(int colorIdx, int r, int g, int b);

    /// @brief 将DmColor转换为CAD颜色索引和RGB值
    /// @param color DmColor对象
    /// @param cadColorIdx 输出：CAD颜色索引
    /// @param cadR 输出：红色分量
    /// @param cadG 输出：绿色分量
    /// @param cadB 输出：蓝色分量
    static void toCAD(const DmColor& color, int& cadColorIdx, int& cadR, int& cadG, int& cadB);

    /// @brief 获取去除标志位的颜色副本
    /// @return 不含标志的DmColor
    DmColor stripFlags() const;

    /// @brief 判断颜色是否由图层定义
    /// @return 如果随层则返回true
    bool isByLayer() const;

    /// @brief 判断颜色是否由块定义
    /// @return 如果随块则返回true
    bool isByBlock() const;

    // These 3 methods are used for plugins
    int toIntColor(void) const;
    void fromIntColor(int co);
    int colorDistance(const DmColor& c) const;

    enum
    {
        Black = 0,
        // Minimum acceptable distance between two colors before visibility
        // enhancement is required. Determined empirically.
        MinColorDistance = 20,
    };

    DmColor& operator=(const DmColor& c);

    bool operator==(const DmColor& c) const;
    bool operator!=(const DmColor& c) const;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    int r = 0; ///< 红色分量
    int g = 0; ///< 绿色分量
    int b = 0; ///< 蓝色分量
    int a = 255; ///< 透明度分量
};

inline DmColor::DmColor(): DmFlags(), r(0), g(0), b(0), a(255)
{}

inline DmColor::DmColor(int r, int g, int b): DmFlags(), r(r), g(g), b(b), a(255)
{}

inline DmColor::DmColor(int r, int g, int b, int a): DmFlags(), r(r), g(g), b(b), a(a)
{}

inline DmColor::DmColor(const DmColor& c): DmFlags(), r(c.red()), g(c.green()), b(c.blue()), a(c.alpha())
{
    setFlags(c.getFlags());
}

inline DmColor::DmColor(unsigned int f): DmFlags(f), r(0), g(0), b(0), a(255)
{}

inline int DmColor::red() const
{
    return r;
}

inline int DmColor::green() const
{
    return g;
}

inline int DmColor::blue() const
{
    return b;
}

inline int DmColor::alpha() const
{
    return a;
}

inline void DmColor::setRed(int r)
{
    this->r = r;
}

inline void DmColor::setGreen(int g)
{
    this->g = g;
}

inline void DmColor::setBlue(int b)
{
    this->b = b;
}

inline void DmColor::setAlpha(int a)
{
    this->a = a;
}

inline void DmColor::setRgb(int r, int g, int b)
{
    this->r = r;
    this->g = g;
    this->b = b;
}

inline void DmColor::setRgba(int r, int g, int b, int a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

inline bool DmColor::operator==(const DmColor& c) const
{
    return (red() == c.red() && green() == c.green() && blue() == c.blue() && getFlags() == c.getFlags());
}

inline bool DmColor::operator!=(const DmColor& c) const
{
    return !operator==(c);
}

/// @return true if the color is defined by layer.
inline bool DmColor::isByLayer() const
{
    return getFlag(DM::FlagByLayer);
}

/// @return true if the color is defined by block.
inline bool DmColor::isByBlock() const
{
    return getFlag(DM::FlagByBlock);
}

MAKE_HASHABLE(DmColor, t.red(), t.green(), t.blue(), t.getFlags());

namespace DM
{
    /// 索引颜色表
    static const DmColor indexColors[256] = {
          {  0,  0,  0}, // unused
          {255,  0,  0}, // 1 红
          {255,255,  0}, // 2 黄
          {  0,255,  0}, // 3 绿
          {  0,255,255}, // 4 青
          {  0,  0,255}, // 5 蓝
          {255,  0,255}, // 6 洋红
          {  0,  0,  0}, // 7 黑白
          {128,128,128}, // 8 50% 灰
          {192,192,192}, // 9 75% 灰
          {255,  0,  0},
          {255,127,127},
          {204,  0,  0},
          {204,102,102},
          {153,  0,  0},
          {153, 76, 76}, // 15
          {127,  0,  0},
          {127, 63, 63},
          { 76,  0,  0},
          { 76, 38, 38},
          {255, 63,  0}, // 20
          {255,159,127},
          {204, 51,  0},
          {204,127,102},
          {153, 38,  0},
          {153, 95, 76}, // 25
          {127, 31,  0},
          {127, 79, 63},
          { 76, 19,  0},
          { 76, 47, 38},
          {255,127,  0}, // 30
          {255,191,127},
          {204,102,  0},
          {204,153,102},
          {153, 76,  0},
          {153,114, 76}, // 35
          {127, 63,  0},
          {127, 95, 63},
          { 76, 38,  0},
          { 76, 57, 38},
          {255,191,  0}, // 40
          {255,223,127},
          {204,153,  0},
          {204,178,102},
          {153,114,  0},
          {153,133, 76}, // 45
          {127, 95,  0},
          {127,111, 63},
          { 76, 57,  0},
          { 76, 66, 38},
          {255,255,  0}, // 50
          {255,255,127},
          {204,204,  0},
          {204,204,102},
          {153,153,  0},
          {153,153, 76}, // 55
          {127,127,  0},
          {127,127, 63},
          { 76, 76,  0},
          { 76, 76, 38},
          {191,255,  0}, // 60
          {223,255,127},
          {153,204,  0},
          {178,204,102},
          {114,153,  0},
          {133,153, 76}, // 65
          { 95,127,  0},
          {111,127, 63},
          { 57, 76,  0},
          { 66, 76, 38},
          {127,255,  0}, // 70
          {191,255,127},
          {102,204,  0},
          {153,204,102},
          { 76,153,  0},
          {114,153, 76}, // 75
          { 63,127,  0},
          { 95,127, 63},
          { 38, 76,  0},
          { 57, 76, 38},
          { 63,255,  0}, // 80
          {159,255,127},
          { 51,204,  0},
          {127,204,102},
          { 38,153,  0},
          { 95,153, 76}, // 85
          { 31,127,  0},
          { 79,127, 63},
          { 19, 76,  0},
          { 47, 76, 38},
          {  0,255,  0}, // 90
          {127,255,127},
          {  0,204,  0},
          {102,204,102},
          {  0,153,  0},
          { 76,153, 76}, // 95
          {  0,127,  0},
          { 63,127, 63},
          {  0, 76,  0},
          { 38, 76, 38},
          {  0,255, 63}, // 100
          {127,255,159},
          {  0,204, 51},
          {102,204,127},
          {  0,153, 38},
          { 76,153, 95}, // 105
          {  0,127, 31},
          { 63,127, 79},
          {  0, 76, 19},
          { 38, 76, 47},
          {  0,255,127}, // 110
          {127,255,191},
          {  0,204,102},
          {102,204,153},
          {  0,153, 76},
          { 76,153,114}, // 115
          {  0,127, 63},
          { 63,127, 95},
          {  0, 76, 38},
          { 38, 76, 57},
          {  0,255,191}, // 120
          {127,255,223},
          {  0,204,153},
          {102,204,178},
          {  0,153,114},
          { 76,153,133}, // 125
          {  0,127, 95},
          { 63,127,111},
          {  0, 76, 57},
          { 38, 76, 66},
          {  0,255,255}, // 130
          {127,255,255},
          {  0,204,204},
          {102,204,204},
          {  0,153,153},
          { 76,153,153}, // 135
          {  0,127,127},
          { 63,127,127},
          {  0, 76, 76},
          { 38, 76, 76},
          {  0,191,255}, // 140
          {127,223,255},
          {  0,153,204},
          {102,178,204},
          {  0,114,153},
          { 76,133,153}, // 145
          {  0, 95,127},
          { 63,111,127},
          {  0, 57, 76},
          { 38, 66, 76},
          {  0,127,255}, // 150
          {127,191,255},
          {  0,102,204},
          {102,153,204},
          {  0, 76,153},
          { 76,114,153}, // 155
          {  0, 63,127},
          { 63, 95,127},
          {  0, 38, 76},
          { 38, 57, 76},
          {  0, 66,255}, // 160
          {127,159,255},
          {  0, 51,204},
          {102,127,204},
          {  0, 38,153},
          { 76, 95,153}, // 165
          {  0, 31,127},
          { 63, 79,127},
          {  0, 19, 76},
          { 38, 47, 76},
          {  0,  0,255}, // 170
          {127,127,255},
          {  0,  0,204},
          {102,102,204},
          {  0,  0,153},
          { 76, 76,153}, // 175
          {  0,  0,127},
          { 63, 63,127},
          {  0,  0, 76},
          { 38, 38, 76},
          { 63,  0,255}, // 180
          {159,127,255},
          { 50,  0,204},
          {127,102,204},
          { 38,  0,153},
          { 95, 76,153}, // 185
          { 31,  0,127},
          { 79, 63,127},
          { 19,  0, 76},
          { 47, 38, 76},
          {127,  0,255}, // 190
          {191,127,255},
          {102,  0,204},
          {153,102,204},
          { 76,  0,153},
          {114, 76,153}, // 195
          { 63,  0,127},
          { 95, 63,127},
          { 38,  0, 76},
          { 57, 38, 76},
          {191,  0,255}, // 200
          {223,127,255},
          {153,  0,204},
          {178,102,204},
          {114,  0,153},
          {133, 76,153}, // 205
          { 95,  0,127},
          {111, 63,127},
          { 57,  0, 76},
          { 66, 38, 76},
          {255,  0,255}, // 210
          {255,127,255},
          {204,  0,204},
          {204,102,204},
          {153,  0,153},
          {153, 76,153}, // 215
          {127,  0,127},
          {127, 63,127},
          { 76,  0, 76},
          { 76, 38, 76},
          {255,  0,191}, // 220
          {255,127,223},
          {204,  0,153},
          {204,102,178},
          {153,  0,114},
          {153, 76,133}, // 225
          {127,  0, 95},
          {127, 63, 11},
          { 76,  0, 57},
          { 76, 38, 66},
          {255,  0,127}, // 230
          {255,127,191},
          {204,  0,102},
          {204,102,153},
          {153,  0, 76},
          {153, 76,114}, // 235
          {127,  0, 63},
          {127, 63, 95},
          { 76,  0, 38},
          { 76, 38, 57},
          {255,  0, 63}, // 240
          {255,127,159},
          {204,  0, 51},
          {204,102,127},
          {153,  0, 38},
          {153, 76, 95}, // 245
          {127,  0, 31},
          {127, 63, 79},
          { 76,  0, 19},
          { 76, 38, 47},
          { 51, 51, 51}, // 250
          { 91, 91, 91},
          {132,132,132},
          {173,173,173},
          {214,214,214},
          {255,255,255}  // 255
    };
}

#endif
