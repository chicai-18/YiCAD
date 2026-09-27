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

/// @file ModifyDeleteCommand.h
/// @brief 删除命令，取代原 ActionModifyDelete 与它的两个工厂
///
/// 两个入口（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步第 5 项）：
///   - ext.modify.delete：交互命令，总是先进入选择阶段，不看是否已有选择集
///     （原 switch 从不检查 hasSelect()，主计划 7.7 节），回车后删除选择集；
///   - ext.modify.delete_no_select：即时命令，直接删除选择集（Delete 键、手写板
///     橡皮擦），不打断当前命令。

#ifndef MODIFYDELETECOMMAND_H
#define MODIFYDELETECOMMAND_H

#include "SelectFirstCommand.h"

class DmDocument;
class IDocumentView;
class SelectionSet;

/// @brief 删除命令
class ModifyDeleteCommand : public SelectFirstCommand
{
public:
    ModifyDeleteCommand();

    /// @brief 删除文档的选择集并刷新选择计数
    /// @param doc 文档；为空时什么也不做
    /// @param selection 文档的选择集；为空时什么也不做
    /// @param view 视图；为空时什么也不做
    static void deleteSelection(DmDocument* doc, SelectionSet* selection, IDocumentView* view);

protected:
    /// @brief 删除选择集后结束
    bool onSelectionReady() override;
};

#endif // MODIFYDELETECOMMAND_H
