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

/// @file RegionData.h
/// @brief 面域数据类，包含边界和孔洞信息

#ifndef REGIONDATA_H
#define REGIONDATA_H

#include "EntityData.h"
#include "DmEntityContainer.h"

class RegionData : public EntityData
{
public:
    RegionData();

    /// @brief 带边界和孔洞的构造函数
    /// @param [in] boundary 面域边界
    /// @param [in] holes 面域孔洞列表
    RegionData(DmEntityContainerPtr boundary, const std::vector<DmEntityContainerPtr>& holes);

    /// @brief 获取面域边界
    /// @return 边界实体容器指针
    DmEntityContainerPtr getBoundary() const;

    /// @brief 设置面域边界
    /// @param [in] b 边界实体容器指针
    void setBoundary(DmEntityContainerPtr b);

    /// @brief 获取面域孔洞列表
    /// @return 孔洞实体容器指针向量
    std::vector<DmEntityContainerPtr> getHoles() const;

    /// @brief 设置面域孔洞列表
    /// @param [in] holes 孔洞实体容器指针向量
    void setHoles(const std::vector<DmEntityContainerPtr>& holes);

private:
    std::vector<DmEntityContainerPtr> m_holes; ///< 面域孔洞列表
    DmEntityContainerPtr m_boundary;           ///< 面域边界
};

#endif // REGIONDATA_H
