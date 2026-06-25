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

/// @file BlockEditCmd.h
/// @brief 块编辑括号命令，用于包裹块编辑会话的进入和退出

#ifndef BLOCKEDITCMD_H
#define BLOCKEDITCMD_H

#include "Cmd.h"
#include <QString>

class DmDocument;
class DmBlock;
class GuiDocumentView;

/// @brief 进入块编辑命令
/// @details 将文档切换到块编辑模式，getEntityTable() 返回块的实体表
class BlockEditEnterCmd : public ICmd
{
public:
    /// @brief 构造进入块编辑命令
    /// @param doc 文档指针
    /// @param blockName 要编辑的块名称
    /// @param docView 文档视图指针
    BlockEditEnterCmd(DmDocument* doc, const QString& blockName,
                      GuiDocumentView* docView);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    CmdType getCmdType() const override { return CmdType::BlockEditEnterCmd; }

private:
    /// @brief 更新文档中引用此块的所有块参照
    void updateBlockRefs();

private:
    DmDocument* m_pDocument;        ///< 文档指针
    QString m_blockName;            ///< 块名称
    GuiDocumentView* m_pDocView;    ///< 文档视图指针
};

/// @brief 退出块编辑命令
/// @details 退出块编辑模式，可选择是否保存（更新块参照）
class BlockEditExitCmd : public ICmd
{
public:
    /// @brief 构造退出块编辑命令
    /// @param doc 文档指针
    /// @param blockName 块名称
    /// @param docView 文档视图指针
    /// @param save 是否保存修改（更新所有引用的块参照）
    BlockEditExitCmd(DmDocument* doc, const QString& blockName,
                     GuiDocumentView* docView, bool save);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;
    CmdType getCmdType() const override { return CmdType::BlockEditExitCmd; }

private:
    /// @brief 更新文档中引用此块的所有块参照
    void updateBlockRefs();

private:
    DmDocument* m_pDocument;        ///< 文档指针
    QString m_blockName;            ///< 块名称
    GuiDocumentView* m_pDocView;    ///< 文档视图指针
    bool m_save;                    ///< 是否保存修改
};

#endif // BLOCKEDITCMD_H
