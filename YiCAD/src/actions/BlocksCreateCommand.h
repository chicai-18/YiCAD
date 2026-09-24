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

/// @file BlocksCreateCommand.h
/// @brief 创建块命令，取代原 ActionBlocksCreate：指定参考点，输入块名，用选择集创建块并
///        以块参照替换
///
/// 原 ActionBlocksCreate 是排他的（启动时结束全部命令）。命令模型里启动任何命令都
/// 结束当前命令，过渡期也结束全部旧 Action（迁移计划 9.2 节），不再需要这个标志。

#ifndef BLOCKSCREATECOMMAND_H
#define BLOCKSCREATECOMMAND_H

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class DmVector;

/// @brief 创建块命令；交互由 BlocksCreateTool 驱动
class BlocksCreateCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(BlocksCreateCommand)

public:
    /// @brief 弹出块名对话框，以参考点为基点用选择集创建块，并在原处放一个块参照
    /// @param referencePoint 参考点
    /// @return 命令已结束（创建成功或取消了对话框）时返回 true；文档没有块表时
    ///         返回 false，命令不结束（原有行为）
    bool createBlock(const DmVector& referencePoint);

protected:
    /// @brief 激活指定参考点的工具
    bool onSelectionReady() override;
};

#endif // BLOCKSCREATECOMMAND_H
