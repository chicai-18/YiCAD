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


/// @file DmBlockTableListener.h
/// @brief 块表监听器接口，用于更新通知和刷新UI

#ifndef DMBLOCKTABLELISTENER_H
#define DMBLOCKTABLELISTENER_H

#include "DmBlock.h"

/// @brief 块表监听器
/// 用于更新通知、刷新UI等
class DmBlockTableListener
{
public:
    DmBlockTableListener() {}
    virtual ~DmBlockTableListener() {}

    /// @brief 当激活的块发生变化时调用
    virtual void blockActivated(DmBlock*) {}

    /// @brief 当新块添加到列表时调用
    virtual void blockAdded(DmBlock*) {}

    /// @brief 当块从列表中移除时调用
    virtual void blockRemoved(DmBlock*) {}

    /// @brief 当块的属性被修改时调用
    virtual void blockEdited(DmBlock*) {}

    /// @brief 当块的可见性被切换时调用
    virtual void blockToggled(DmBlock*) {}

    /// @brief 当块列表被修改时调用
    virtual void blockListModified(bool) {}
};

#endif
