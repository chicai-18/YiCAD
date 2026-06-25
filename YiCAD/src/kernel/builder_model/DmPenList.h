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


/// @file DmPenList.h
/// @brief 画笔列表，单例模式管理画笔的创建与ID映射

#ifndef DMPENLIST_H
#define DMPENLIST_H

#include <unordered_map>
#include "DmPen.h"

#define DMPENLIST DmPenList::instance()

/// @brief 画笔列表（单例模式），管理画笔实例与ID的双向映射
class DmPenList
{
public:
    /// @brief 获取全局唯一实例
    /// @return 单例指针
    static DmPenList* instance();

    /// @brief 按属性请求画笔，不存在则创建
    /// @param color 颜色
    /// @param lineWidth 线宽
    /// @param lineType 线型指针
    /// @return 画笔指针
    DmPen* request(const DmColor& color, DM::LineWidth lineWidth,
                   DmLineType* lineType);

    /// @brief 按ID请求画笔
    /// @param penId 画笔ID
    /// @return 画笔指针，未找到返回 nullptr
    DmPen* request(int penId) const;

    /// @brief 获取画笔对应的ID
    /// @param pen 画笔引用
    /// @return 画笔ID，未找到返回 -1
    int getPenId(const DmPen& pen);

private:
    DmPenList() = default;

private:
    std::unordered_map<int, DmPen*> m_idPenMap;     ///< ID到画笔的映射
    std::unordered_map<DmPen, int> m_penIdMap;      ///< 画笔到ID的映射
};

#endif // DMPENLIST_H
