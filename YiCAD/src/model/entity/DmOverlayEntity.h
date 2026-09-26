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


/// @file DmOverlayEntity.h
/// @brief 前景层覆盖实体：线、圆、点的叠加显示类型

#ifndef DMOVERLAYENTITY_H
#define DMOVERLAYENTITY_H

#include "DmLine.h"
#include "DmCircle.h"
#include "DmPoint.h"

/// @brief 前景层画线
class DmOverlayLine : public DmLine
{
public:
    /// @brief 构造函数
    /// @param parent 父实体指针
    /// @param d 线段几何数据
    DmOverlayLine(DmEntity* parent, const LineData& d);

    /// @brief 获取实体类型
    /// @return 实体类型枚举值
    DM::EntityType getEntityType() const override;

    /// @brief 获取子实体列表
    /// @return 子实体列表
    std::list<DmEntity*> getSubEntities() const override;
};

/// @brief 前景层画圆
class DmOverlayCircle : public DmCircle
{
public:
    /// @brief 构造函数
    /// @param parent 父实体指针
    /// @param d 圆几何数据
    DmOverlayCircle(DmEntity* parent, const CircleData& d);

    /// @brief 获取实体类型
    /// @return 实体类型枚举值
    DM::EntityType getEntityType() const override;

    /// @brief 获取子实体列表
    /// @return 子实体列表
    std::list<DmEntity*> getSubEntities() const override;
};

/// @brief 前景层画点
class DmOverlayPoint : public DmPoint
{
public:
    /// @brief 构造函数
    /// @param parent 父实体指针
    /// @param d 点几何数据
    DmOverlayPoint(DmEntity* parent, const PointData& d);

    /// @brief 获取实体类型
    /// @return 实体类型枚举值
    DM::EntityType getEntityType() const override;

    /// @brief 获取子实体列表
    /// @return 子实体列表
    std::list<DmEntity*> getSubEntities() const override;
};

#endif
