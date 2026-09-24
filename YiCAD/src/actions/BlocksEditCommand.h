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

/// @file BlocksEditCommand.h
/// @brief 编辑块命令：先选后建，选择集就绪后以选中的块参照进入块编辑模式（BlockEditTool）
///
/// 命令只负责进入：进入后立即结束，块编辑模式由视图的命令总线持有，模式里还能运行
/// 别的命令（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 5 节"命令并存"）。已在块编辑中时
/// 不构造命令，给出警告（原有行为）。

#ifndef BLOCKSEDITCOMMAND_H
#define BLOCKSEDITCOMMAND_H

#include <QCoreApplication>

#include "SelectFirstCommand.h"

/// @brief 编辑块命令
class BlocksEditCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(BlocksEditCommand)

protected:
    /// @brief 取选择集里第一个块参照进入块编辑模式后结束；没有块参照或用户取消时启动失败
    bool onSelectionReady() override;
};

#endif // BLOCKSEDITCOMMAND_H
