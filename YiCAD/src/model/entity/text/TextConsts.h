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


/// @file TextConsts.h
/// @brief 文字相关的全局常量定义

#ifndef TEXTCONSTS_H
#define TEXTCONSTS_H

/// @brief 水平文字间隔系数
constexpr double CHARGAPFACTOR = 0.05;

/// @brief 行高与字高的比（在行距系数为1.0时的取值）
constexpr double LINE_HEIGHT_PER_CHAR_HEIGHT = (5.0 / 3.0);

/// @brief 默认标注样式名
constexpr auto DEFAULT_DIMSTYLE_NAME = "ISO-25";

/// @brief 默认文字样式名
constexpr auto DEFAULT_TEXTSTYLE_NAME = "Standard";

//  默认字体
#ifdef WIN32
constexpr auto DEFAULT_FONT_FAMILY_NAME = "Arial";
#elif unix
constexpr auto DEFAULT_FONT_FAMILY_NAME = "Unifont";
#else
constexpr auto DEFAULT_FONT_FAMILY_NAME = "Arial";
#endif

/// @brief 默认字体是否粗体
constexpr bool DEFAULT_FONT_FAMILY_BOLD = false;

/// @brief 默认字体是否斜体
constexpr bool DEFAULT_FONT_FAMILY_ITALIC = false;

/// @brief 文字默认高度
constexpr double DEFAULT_TEXT_HEIGHT = 2.5;

/// @brief 默认倾斜角度
constexpr double DEFAULT_SLASH_ANGLE = 0.0;

/// @brief 默认宽度系数
constexpr double DEFAULT_WIDTH_FACTOR = 1.0;

/// @brief shx字体后缀
constexpr auto SHX_POST = ".shx";

/// @brief ttc字体后缀
constexpr auto TTC_POST = ".ttc";

/// @brief ttf字体后缀
constexpr auto TTF_POST = ".ttf";

/// @brief 距离容差
constexpr double DISTANCE_TOLRANCE = 1e-5;

/// @brief 比例容差
constexpr double SCALE_TOLRANCE = 1e-5;

/// @brief 角度容差
constexpr double ANGLE_TOLRANCE = 1e-5;

/// @brief 多行文字剪切板键
constexpr auto CLIPBOARD_MTEXT_KEY = "YiCAD MText Clipboard Data";

////////////字体相关///////////////

/// @brief 在解析freetype字体前，设置文字的大小。不要设置为1，否则会导致freetype只用6 bit（0-64）来表示坐标（因为freetype用26.6表示坐标），精度太低
constexpr int FTFONTSIZE = 10000;

/// @brief 用来计算字体一般不留白高度的测试文字，不能设为空白符
constexpr auto FTFONTTESTCHAR = "A";

/// @brief 默认宽度高度因子
constexpr double CHAR_DEFAULT_WIDTH_HEIGHT_FACTOR = 0.3;

/// @brief 默认高度
constexpr double CHAR_DEFAULT_HEIGHT = 0.5;

/// @brief 空格默认宽度
constexpr double SPACE_WIDTH = 0.2;

/// @brief Tab默认宽度
constexpr double TAB_WIDTH = 0.8;

/// @brief 换行默认宽度
constexpr double LINEFEED_WIDTH = 0.8;

/// @brief 空白符号默认高度
constexpr double WHITE_SPACE_HEIGHT = 1.0;

#endif //!TEXTCONSTS_H
