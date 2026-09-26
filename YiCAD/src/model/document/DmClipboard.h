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


/// @file DmClipboard.h
/// @brief YiCAD 内部剪贴板（单例模式），不使用系统剪贴板以保证可移植性

#ifndef DMCLIPBOARD_H
#define DMCLIPBOARD_H

#include <memory>

#include "DmDocument.h"

#define DMCLIPBOARD DmClipboard::instance()

class DmEntity;

// YiCAD internal clipboard. We don't use the system clipboard for better portaility.
// Implemented as singleton.
/// @details 剪贴板有自己的文档。放进来的实体改归这份文档，实体引用的图层、线型、样式与块也复制进来，
///          与复制来源的图纸不再有关联，来源图纸关闭后照样能粘贴
class DmClipboard
{
protected:
    DmClipboard();

public:
    /// @brief 获取唯一剪贴板实例（单例模式）
    /// @return 剪贴板实例指针
    static DmClipboard* instance();

    /// @brief 清空剪贴板：换一份新文档，上次放进来的实体与表项随旧文档释放
    void clear();

    /// @brief 添加实体到剪贴板，剪贴板取得所有权
    /// @details 实体改归剪贴板的文档（DmEntity::transferTo）：引用的图层、线型、样式与块按名字取
    ///          剪贴板文档里的，没有的复制一份进来
    /// @param e 实体指针
    void addEntity(DmEntity* e);

    /// @brief 获取剪贴板中实体数量
    /// @return 实体数量
    unsigned count();

    /// @brief 获取剪贴板文档
    /// @return 文档指针；clear() 之后是另一份文档，不要跨 clear() 持有
    DmDocument* getDocument();

protected:
    static DmClipboard* uniqueInstance;         ///< 单例实例
    std::unique_ptr<DmDocument> m_pDocument;    ///< 剪贴板文档
};

#endif
