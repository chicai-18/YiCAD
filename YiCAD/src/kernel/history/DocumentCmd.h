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

/// @file DocumentCmd.h
/// @brief 文档级操作命令，如修改当前画笔

#ifndef DOCUMENTCMD_H
#define DOCUMENTCMD_H

#include "Cmd.h"
#include "DmPen.h"
#include "DmVariable.h"

#include <QHash>
#include <QSet>
#include <QString>

/// @brief 修改文档当前画笔命令
class ModifyDocPenCmd : public ICmd
{
public:
    /// @brief 构造修改画笔命令
    /// @param doc 目标文档
    /// @param newPen 新画笔
    /// @param originPen 原始画笔
    ModifyDocPenCmd(DmDocument* doc, const DmPen& newPen, const DmPen& originPen);
    void execute() override;
    void undo() override;
    void redo() override;

    void clear() override{}
    virtual CmdType getCmdType() const {return CmdType::ActivePenModify;}
protected:
    DmDocument* m_doc;          ///< 目标文档
    DmPen   m_originPen;        ///< 原始画笔
    DmPen   m_newPen;           ///< 新画笔
};

/// @brief 修改文档变量的可撤销命令
class ModifyDocVariablesCmd : public ICmd
{
public:
    /// @brief 构造文档变量修改命令
    /// @param document 目标文档
    /// @param variables 要新增或覆盖的变量
    ModifyDocVariablesCmd(
        DmDocument* document,
        const QHash<QString, DmVariable>& variables);
    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override {}

private:
    void applyVariables();

    DmDocument* m_document;
    QHash<QString, DmVariable> m_newVariables;
    QHash<QString, DmVariable> m_previousVariables;
    QSet<QString> m_addedVariables;
};

#endif //DOCUMENTCMD_H
