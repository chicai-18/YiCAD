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

/// @file IGiSubEntityTraits.h
/// @brief GI 的图元属性接口

#ifndef IGISUBENTITYTRAITS_H
#define IGISUBENTITYTRAITS_H

#include <cstdint>

#include "Datamodel.h"

class DmColor;
class DmLayer;
class DmLineType;
class DmVector;
struct GiLinePattern;

/// @brief 图元属性。设置后对之后的图元生效
/// @details 每个可绘制对象开始时，属性初值为全 ByBlock、图层为空（取外层的图层），
///          接着由 IGiDrawable::setAttributes() 按对象自身的属性设置；worldDraw 里可以逐图元覆盖。
///          ByBlock 取外层（嵌套绘制的外层对象，或 drawShared 的 GiByBlockTraits），ByLayer 取所在图层
class IGiSubEntityTraits
{
public:
    virtual ~IGiSubEntityTraits() = default;

    /// @brief 颜色，含 ByLayer、ByBlock、真彩色
    virtual void setColor(const DmColor& color) = 0;

    /// @brief 图层；为空表示取外层的图层
    virtual void setLayer(const DmLayer* layer) = 0;

    /// @brief 线型，含 DmLineTypeTable::ByLayer、ByBlock；为空视为 ByBlock
    virtual void setLineType(const DmLineType* lineType) = 0;

    /// @brief 实体线型比例
    virtual void setLineTypeScale(double scale) = 0;

    /// @brief 内联图案与相位，填充图案线用，不做端点对齐；长度在实体自身坐标系里，随块缩放
    virtual void setLinePattern(const GiLinePattern& pattern) = 0;

    /// @brief 线宽，含 ByLayer、ByBlock、默认
    virtual void setLineWeight(DM::LineWidth weight) = 0;

    /// @brief 透明度，255 为不透明
    virtual void setTransparency(std::uint8_t alpha) = 0;

    /// @brief 子实体标记，供子实体高亮与夹点
    virtual void setSelectionMarker(std::int32_t marker) = 0;

    /// @brief 非空：之后的图元以像素为单位、锚定在该世界点；为空：恢复世界单位
    virtual void setScreenSpace(const DmVector* anchor) = 0;
};

#endif // IGISUBENTITYTRAITS_H
