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

ModifyDocVariablesCmd::ModifyDocVariablesCmd(
    DmDocument* document,
    const QHash<QString, DmVariable>& variables)
    : m_document(document)
    , m_newVariables(variables)
{
    if (m_document == nullptr)
    {
        return;
    }

    const auto& variableDict = m_document->getVariableDict();
    for (auto it = m_newVariables.cbegin(); it != m_newVariables.cend(); ++it)
    {
        const auto previous = variableDict.constFind(it.key());
        if (previous == variableDict.cend())
        {
            m_addedVariables.insert(it.key());
        }
        else
        {
            m_previousVariables.insert(it.key(), previous.value());
        }
    }
}

void ModifyDocVariablesCmd::applyVariables()
{
    auto& variables = m_document->getVariableDict();
    variables.reserve(variables.size() + m_newVariables.size());
    for (auto it = m_newVariables.cbegin(); it != m_newVariables.cend(); ++it)
    {
        variables.insert(it.key(), it.value());
    }
}

void ModifyDocVariablesCmd::execute()
{
    if (m_isExecuted || m_document == nullptr)
    {
        return;
    }
    applyVariables();
    ICmd::execute();
}

void ModifyDocVariablesCmd::undo()
{
    if (!m_isExecuted || m_document == nullptr)
    {
        return;
    }
    auto& variables = m_document->getVariableDict();
    variables.reserve(variables.size() + m_previousVariables.size());
    for (auto it = m_previousVariables.cbegin();
         it != m_previousVariables.cend(); ++it)
    {
        variables.insert(it.key(), it.value());
    }
    for (const auto& key : m_addedVariables)
    {
        variables.remove(key);
    }
    ICmd::undo();
}

void ModifyDocVariablesCmd::redo()
{
    if (m_isExecuted || m_document == nullptr)
    {
        return;
    }
    applyVariables();
    ICmd::redo();
}
