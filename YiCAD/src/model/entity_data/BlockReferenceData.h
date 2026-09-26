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

/// @file BlockReferenceData.h
/// @brief 块参照数据结构类，定义块参照的位置、旋转、缩放等属性

#ifndef BLOCKREFERENCEDATA_H
#define BLOCKREFERENCEDATA_H

#include <list>

#include "EntityData.h"
#include "DmVector.h"

/// @brief 块参照数据结构
class BlockReferenceData : public EntityData
{
public:
    /// @brief 默认构造函数
    BlockReferenceData();

public:
    /// @brief 获取块表记录句柄
    /// @return 块表记录句柄
    int getBlockTableRecHandle() const;

    /// @brief 设置块表记录句柄
    /// @param handle 块表记录句柄
    void setBlockTableRecHHandle(const int handle);

    /// @brief 获取块名称
    /// @return 块名称（宽字符串）
    std::wstring getBlockName() const;

    /// @brief 设置块名称
    /// @param name 块名称（宽字符串）
    void setBlockName(const std::wstring name);

    /// @brief 获取法向量
    /// @return 法向量
    DmVector getNormal() const;

    /// @brief 设置法向量
    /// @param normal 法向量
    void setNormal(const DmVector& normal);

    /// @brief 获取定位点
    /// @return 定位点坐标
    DmVector getPosition() const;

    /// @brief 设置定位点
    /// @param position 定位点坐标
    void setPosition(const DmVector& position);

    /// @brief 获取旋转角度
    /// @return 旋转角度（弧度）
    double getRotationAngle() const;

    /// @brief 设置旋转角度
    /// @param angle 旋转角度（弧度）
    void setRotationAngle(const double angle);

    /// @brief 获取缩放比例
    /// @return 缩放向量
    DmVector getScale() const;

    /// @brief 设置缩放比例
    /// @param scale 缩放向量
    void setScale(const DmVector& scale);

private:
    int             m_BlockTableRecordHandle;   ///< 块表记录句柄
    std::wstring    m_strBlockName;             ///< 块名
    DmVector        m_normal;                   ///< 法向量
    DmVector        m_position;                 ///< 定位点
    double          m_dRotationAngle;           ///< 旋转角度
    DmVector        m_scale;                    ///< 放缩
};

#endif // BLOCKREFERENCEDATA_H
