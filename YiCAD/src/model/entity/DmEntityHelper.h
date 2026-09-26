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


/// @file DmEntityHelper.h
/// @brief 实体工厂辅助类，通过类型名创建实体及获取实体类型名称

#ifndef DMENTITYHELPER_H
#define DMENTITYHELPER_H

#include "DmEntity.h"

class DmEntityHelper
{
public:
    /// @brief 根据实体类型名字符串创建对应的实体对象
    /// @param [in] typeName 实体类型名称，如 "Line", "Circle" 等
    /// @return 创建的实体指针，若类型名未匹配则返回 nullptr
    static DmEntity* createEntityByName(const std::string& typeName);

    /// @brief 根据实体类型枚举获取对应的名称字符串
    /// @param [in] entityType 实体类型枚举值
    /// @return 实体类型名称字符串，若未匹配则返回空字符串
    static std::string getEntityNameByType(DM::EntityType entityType);
};

#endif // DMENTITYHELPER_H
