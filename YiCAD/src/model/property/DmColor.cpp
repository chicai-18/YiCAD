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


/// @file DmColor.cpp
/// @brief 颜色类实现

#include "DmColor.h"
#include <iostream>
#include <cmath>

TYPESYSTEM_SOURCE_ABSTRACT(DmColor, DmFlags, 0)

DmColor DmColor::fromCAD(int colorIdx, int r, int g, int b)
{
    DmColor color;
    if (colorIdx == 0)   //随块
    {
        color = DmColor(DM::FlagByBlock);
    }
    else if (colorIdx == 256)   //随层
    {
        color = DmColor(DM::FlagByLayer);
    }
    else
    {
        color = DmColor(r, g, b);
    }
    return color;
}

void DmColor::toCAD(const DmColor& color, int& cadColorIdx, int& cadR, int& cadG, int& cadB)
{
    cadColorIdx = -1;
    if (color.isByBlock())
    {
        cadColorIdx = 0;
    }
    else if (color.isByLayer())
    {
        cadColorIdx = 256;
    }
    cadR = color.red();
    cadG = color.green();
    cadB = color.blue();
}

/// @brief 获取去除标志位的颜色副本
DmColor DmColor::stripFlags() const
{
    return DmColor(red(), green(), blue());
}

// This method is used for plugins
int DmColor::toIntColor(void) const
{
    constexpr int kByLayerValue = -1;
    constexpr int kByBlockValue = -2;
    if (isByLayer())
    {
        return kByLayerValue;
    }
    if (isByBlock())
    {
        return kByBlockValue;
    }
    int cd = (red() << 16) + (green() << 8) + blue();
    return cd;
}

// This method is used for plugins
void DmColor::fromIntColor(int co)
{
    constexpr int kByLayerValue = -1;
    constexpr int kByBlockValue = -2;
    constexpr int kColorMask = 0xFF;
    if (co == kByLayerValue)
    {
        setFlags(DM::FlagByLayer);
    }
    else if (co == kByBlockValue)
    {
        setFlags(DM::FlagByBlock);
    }
    else
    {
        setRed((co >> 16) & kColorMask);
        setGreen((co >> 8) & kColorMask);
        setBlue(co & kColorMask);
    }
}

/// @brief Color distance
/// @brief Calculate distance between two RGB colors using low-cost approximation of human eye response by Thiadmer Riemersma. Formula explanation found here:
/// @param c Color to perform comparison against
/// @return Distance between colors in percent, value ranging from 0 (identical) to 100 (maximum difference)
int DmColor::colorDistance(const DmColor& c) const
{
    int myRed = red();
    int otherRed = c.red();
    int redMean = (myRed + otherRed) / 2;

    // Convert difference value to percentage using maximum color difference (764.834 / 100)
    constexpr double kMaxColorDiff = 7.64834;
    return std::lround(std::sqrt(std::pow(otherRed - myRed, 2) * (512 + redMean) / 256 + std::pow(c.green() - green(), 2) * 4
        + std::pow(c.blue() - blue(), 2) * (767 - redMean) / 256) / kMaxColorDiff);
}

DmColor& DmColor::operator=(const DmColor& c)
{
    setRgb(c.red(), c.green(), c.blue());
    setAlpha(c.alpha());
    setFlags(c.getFlags());

    return *this;
}

void DmColor::saveStream(OutputStream &wrt) const {
    DmFlags::saveStream(wrt);
    wrt << (int)r << (int)g << (int)b << (int)a;
}

void DmColor::restoreStream(InputStream &reader, const std::vector<PAIR> &revs) {
    int fileRev = getRevisionId("DmColor", revs);
    if (revId > fileRev)
    {
        DmFlags::restoreStream(reader, revs);
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        restoreStream(reader);
    }
}

void DmColor::restoreStreamWithRev(InputStream &rdr, int rev) {
    if (rev == 0)
    {
        // legacy rev 0 format
    }
    else //big change, e.g. change supper class of DmFlags
    {
        //step1.
        // read all legacy data one by one
    }
}

void DmColor::restoreStream(InputStream &rdr) {
    DmFlags::restoreStream(rdr);

    rdr >> (int&)r >> (int&)g >> (int&)b >> (int&)a;
}
