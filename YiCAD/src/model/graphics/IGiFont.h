/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file IGiFont.h
/// @brief GI 的字体句柄：按字符码给出字形

#ifndef IGIFONT_H
#define IGIFONT_H

class IGiDrawable;

/// @brief 字体。glyphRun 经它取每个字形的几何，GS 按 (字体, 字符码) 缓存字形（第 4.3.3 节）
class IGiFont
{
public:
    virtual ~IGiFont() = default;

    /// @brief 字符码对应的字形，在字形坐标系里；字体没有这个字形时返回空
    /// @details 字形的 ByBlock 属性取 glyphRun 当时的属性。首次取某个字形可能要生成它，调用方负责串行化
    virtual const IGiDrawable* glyph(char32_t code) const = 0;
};

#endif // IGIFONT_H
