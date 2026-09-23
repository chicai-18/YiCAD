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

/// @file GeUtility.h
/// @brief 几何工具类，提供实体类型判断等静态工具方法

#ifndef GEUTILITY_H
#define GEUTILITY_H

class DmEntity;

/// @brief 几何工具类
class GeUtility
{
public:
    /// @brief 判断实体是否为圆弧类型（圆弧、圆或椭圆）
    /// @param e 待判断的实体
    /// @return 若为圆弧类型则返回true
    static bool isEntityArc(const DmEntity* e);
};

#endif // GEUTILITY_H
