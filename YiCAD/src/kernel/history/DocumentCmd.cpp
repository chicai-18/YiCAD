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

/// @file DocumentCmd.cpp
/// @brief 文档级操作命令实现

#include "DocumentCmd.h"
#include "DmDocument.h"

ModifyDocPenCmd::ModifyDocPenCmd(DmDocument *doc, const DmPen &newPen, const DmPen &originPen)
:m_doc(doc)
,m_newPen(newPen)
,m_originPen(originPen)
{

}

/// @brief 执行修改画笔
void ModifyDocPenCmd::execute()
{
    if (m_isExecuted)
    {
        return;
    }
    m_doc->setActivePen(m_newPen);
    ICmd::execute();
}

/// @brief 撤销修改画笔
void ModifyDocPenCmd::undo()
{
    if (!m_isExecuted)
    {
        return;
    }
    m_doc->setActivePen(m_originPen);
    ICmd::undo();
}

/// @brief 重做修改画笔
void ModifyDocPenCmd::redo()
{
    if (m_isExecuted)
    {
        return;
    }
    m_doc->setActivePen(m_newPen);
    ICmd::redo();
}
