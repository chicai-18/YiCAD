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


/// @file DmPen.h
/// @brief 画笔类，封装颜色、线宽、线型等绘制属性

#ifndef DMPEN_H
#define DMPEN_H

#include "Datamodel.h"
#include "DmColor.h"
#include "DmFlags.h"

#include "DmLineType.h"
#include "DmLineTypeTable.h"

/// @brief 画笔类，存储线宽、线型和颜色等绘制属性
class DmPen : public DmFlags
{
public:
    /// @brief 创建默认画笔（黑色、实线、线宽0）
    DmPen();

    /// @brief 创建指定属性的画笔
    /// @param c 颜色
    /// @param w 线宽
    /// @param t 线型指针
    DmPen(const DmColor& c, DM::LineWidth w, DmLineType* t);

    /// @brief 创建带标志位的默认画笔，通常用于创建无效画笔
    /// @param f 标志位
    DmPen(unsigned int f);

    virtual ~DmPen();

    /// @brief 获取线型
    /// @return 线型指针
    DmLineType* getLineType() const;

    /// @brief 设置线型
    /// @param t 线型指针
    void setLineType(DmLineType* t);

    /// @brief 获取线宽
    /// @return 线宽枚举值
    DM::LineWidth getWidth() const;

    /// @brief 设置线宽
    /// @param w 线宽枚举值
    void setWidth(DM::LineWidth w);

    /// @brief 获取颜色
    /// @return 颜色常量引用
    const DmColor& getColor() const;

    /// @brief 设置颜色
    /// @param c 颜色
    void setColor(const DmColor& c);

    /// @brief 计算颜色哈希值
    /// @return 哈希值
    size_t hashColor() const;

    /// @brief 判断画笔是否有效
    /// @return 有效返回 true
    bool isValid();

    bool operator==(const DmPen& p) const;
    bool operator!=(const DmPen& p) const;

protected:
    DM::LineWidth width;    ///< 线宽
    DmColor color;          ///< 颜色

    DmLineType* lineType;   ///< 线型指针
};

inline bool DmPen::operator==(const DmPen& p) const
{
    return (lineType == p.lineType && width == p.width && color == p.color);
}

inline bool DmPen::operator!=(const DmPen& p) const
{
    return !(*this == p);
}

inline DmLineType* DmPen::getLineType() const
{
    return lineType;
}

inline void DmPen::setLineType(DmLineType* t)
{
    lineType = t;
}

inline DM::LineWidth DmPen::getWidth() const
{
    return width;
}

inline void DmPen::setWidth(DM::LineWidth w)
{
    width = w;
}

inline const DmColor& DmPen::getColor() const
{
    return color;
}

inline void DmPen::setColor(const DmColor& c)
{
    color = c;
}

inline bool DmPen::isValid()
{
    return !getFlag(DM::FlagInvalid);
}

inline size_t DmPen::hashColor() const
{
    return std::hash<DmColor>()(color);
}

MAKE_HASHABLE(DmPen, t.getLineType(), t.getWidth(), t.hashColor());

#endif
