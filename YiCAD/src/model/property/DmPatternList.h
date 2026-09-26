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


/// @file DmPatternList.h
/// @brief 填充图案表，全局单例管理所有填充图案的加载与查询

#ifndef DMPATTERNLIST_H
#define DMPATTERNLIST_H

#include <map>
#include <memory>

class DmPattern;
class QString;

#define DMPATTERNLIST DmPatternList::instance()

/// @brief 填充图案表（单例模式），维护管理填充图案
class DmPatternList
{
    DmPatternList() = default;

public:
    /// @brief 获取全局唯一实例
    /// @return 图案列表单例指针
    static DmPatternList* instance();

    ~DmPatternList();
    DmPatternList(DmPatternList const&) = delete;
    DmPatternList& operator=(DmPatternList const&) = delete;
    DmPatternList(DmPatternList&&) = delete;
    DmPatternList& operator=(DmPatternList&&) = delete;

    /// @brief 初始化图案列表，从文件加载所有图案
    void init();

    /// @brief 获取图案数量
    /// @return 图案总数
    int countPatterns() const;

    // range based loop support
    std::map<QString, std::unique_ptr<DmPattern>>::iterator begin();
    std::map<QString, std::unique_ptr<DmPattern>>::const_iterator begin() const;
    std::map<QString, std::unique_ptr<DmPattern>>::iterator end();
    std::map<QString, std::unique_ptr<DmPattern>>::const_iterator end() const;

    /// @brief 按名称请求图案，若未加载则自动加载
    /// @param name 图案名称
    /// @return 图案指针，未找到返回 nullptr
    DmPattern* requestPattern(const QString& name);

    /// @brief 检查是否包含指定名称的图案
    /// @param name 图案名称
    /// @return 包含返回 true
    bool contains(const QString& name) const;

private:
    std::map<QString, std::unique_ptr<DmPattern>> m_patterns; ///< 应用中所有图案
};

#endif
