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


/// @file DmIdManager.h
/// @brief 实体ID管理器，在Document里维护所有实体id的映射

#ifndef DMIDMANAGER_H
#define DMIDMANAGER_H

#include <unordered_map>
#include "DmId.h"

#include "DmObject.h"

typedef std::unordered_map<DmId, DmObject*> ENTITIESIDMAP;

/// @class DmIdManager
/// @brief 在Document里维护所有实体id的类
class DmIdManager
{
public:
    DmIdManager();
    ~DmIdManager() = default;

public:
    /// @brief 创建id并分配给实体
    /// @param [in] pObj 需要分配ID的实体对象
    /// @return 新生成的ID
    DmId assignID(DmObject* pObj);

    /// @brief 直接将指定id赋给实体（特殊情况下调用，如：文件读取）
    /// @param [in] pObj 需要分配ID的实体对象
    /// @param [in] id 指定的ID
    void assignID(DmObject* pObj, const DmId& id);

    /// @brief 查询id是否存在
    /// @param [in] id 要查询的ID
    /// @return 存在返回true
    bool isExistGUID(const DmId& id) const;

    /// @brief 删除id
    /// @param [in] id 要删除的ID
    void removeID(const DmId& id);

    /// @brief 通过id获取实体
    /// @param [in] id 要查询的ID
    /// @return 实体指针，不存在返回 nullptr
    DmObject* getEntity(const DmId& id);

    /// @brief 清空实体集
    void clear();

private:
    ENTITIESIDMAP m_entitiesMap; ///< 实体id和指针组成的键值对
};

#endif // DMIDMANAGER_H
