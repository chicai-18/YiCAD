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

/// @file EditUndoCommand.h
/// @brief 撤销、重做：即时命令 ext.edit.undo、ext.edit.redo，取代原 ActionEditUndo
///
/// 即时命令没有命令对象；这里只声明供其它命令直接调用的实现（画直线的"撤销"
/// 按钮，原先嵌套启动 ActionEditUndo，迁移计划第三步第 3 项）。

#ifndef EDITUNDOCOMMAND_H
#define EDITUNDOCOMMAND_H

class DmDocument;

/// @brief 撤销、重做
namespace EditUndoCommand
{
/// @brief 撤销或重做文档的一步
/// @param doc 文档；为空时只给出警告
/// @param undo true 撤销，false 重做
void run(DmDocument* doc, bool undo);
}  // namespace EditUndoCommand

#endif // EDITUNDOCOMMAND_H
