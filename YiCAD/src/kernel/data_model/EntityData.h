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

/// @file EntityData.h
/// @brief 实体数据基类，所有CAD实体数据结构的基类，包含实体类型标识

#ifndef ENTITYDATA_H
#define ENTITYDATA_H

#include <string>
#include <vector>

#include "EntityDataDef.h"

/// @brief 实体数据基类
class EntityData
{
public:
    /// @brief 默认构造函数
    EntityData();

    /// @brief 虚析构函数
    virtual ~EntityData() = default;

public:
    /// @brief 获取实体类型
    /// @return 实体类型枚举
    EEntityType getEntityType() const;

    /// @brief 设置实体类型
    /// @param entType 实体类型枚举
    void setEntityType(const EEntityType& entType);

private:
    EEntityType     m_eType;    ///< 实体类型
};

#endif // ENTITYDATA_H
