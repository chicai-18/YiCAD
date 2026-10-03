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


/// @file DmId.h
/// @brief 实体唯一标识符类，支持比较和哈希

#ifndef DMID_H
#define DMID_H

#include <string>
#include <ostream>

class DmId
{
public:
    explicit DmId();
    DmId(const DmId& id);
    explicit DmId(const std::string& idStr);

    /// @brief 检查ID是否有效（非默认值"0"）
    bool isValid() const;

    /// @brief 获取ID的字符串表示
    /// @details 返回引用：std::hash<DmId> 每次求哈希都要取它，按值返回时每次都复制一份字符串
    ///          （选择集、实体表按 ID 查找都走这里，全选几十万个实体时这份复制就要上百毫秒）
    const std::string& asString() const;

    DmId& operator=(const DmId& id);
    bool operator==(const DmId& id) const;
    bool operator!=(const DmId& id) const;
    bool operator<(const DmId& id) const;

    friend std::ostream& operator<<(std::ostream& os, const DmId& id);

private:
    std::string m_idStr; ///< ID字符串
};

// 为了默认支持std::unordered_map
// std::unordered_map第3个参数的默认参数是std::hash<Key>，
// 这里对它进行模板定制，参考：https://blog.csdn.net/y109y/article/details/82669620
namespace std
{
    template<>
    class hash<DmId>
    {
    public:
        size_t operator()(const DmId& id) const
        {
            return hash<string>()(id.asString());
        }
    };
}

#endif // DMID_H
