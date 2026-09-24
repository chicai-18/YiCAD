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

/// @file GuiCommandEvent.h
/// @brief 命令事件类，封装命令行命令的传递与接受状态

#ifndef GUICOMMANDEVENT_H
#define GUICOMMANDEVENT_H

#include <QString>

/// @brief 命令事件
class GuiCommandEvent
{
public:
    /// @brief 创建一个尚未被接受的命令事件
    /// @param cmd 被触发的命令字符串
    GuiCommandEvent(const QString& cmd)
    {
        this->cmd = cmd;
        accepted = false;
    }

    /// @brief 获取被触发的命令
    /// @return 命令字符串（通常由用户输入触发）
    QString getCommand()
    {
        return cmd;
    }

    /// @brief 将事件状态设置为已接受
    void accept()
    {
        accepted = true;
    }

    /// @brief 检查事件是否已被接受
    /// @return true 表示事件已被接受，false 表示尚未接受
    bool isAccepted()
    {
        return accepted;
    }

protected:
    QString     cmd;
    bool        accepted;
};

#endif
