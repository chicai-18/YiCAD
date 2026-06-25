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


/// @file ActionBlocksEdit.cpp
/// @brief 编辑块定义的动作类实现文件

#include "ActionBlocksEdit.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSet>

#include "DmBlock.h"
#include "DmDocument.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "GuiEventHandler.h"
#include "BlockEditCmd.h"
#include "CmdManager.h"
#include "MacroCmd.h"
#include "Transaction.h"
#include "UIDialogFactory.h"
#include "UINestedBlockSelectDialog.h"


/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
/// @param selectedRef 选中的块参照，默认为nullptr
ActionBlocksEdit::ActionBlocksEdit(DmDocument* doc,
    GuiDocumentView* docView, DmBlockReference* selectedRef)
    : ActionInterface("Edit Block", doc, docView)
    , m_blockRefBeingEdited(selectedRef)
{
    actionType = DM::ActionBlocksEdit;
}

/// @brief 析构函数
ActionBlocksEdit::~ActionBlocksEdit() = default;

/// @brief 初始化动作
/// @param status 状态参数，默认为0
void ActionBlocksEdit::init(int status)
{
    ActionInterface::init(status);

    // 检查是否通过撤销/重做重新进入块编辑
    DmBlock* editingBlock = pDocument->getEditingBlock();
    if (editingBlock)
    {
        // 通过撤销重新进入：块已设置好，只需恢复界面状态
        reenterEditing(editingBlock);
        return;
    }

    // 正常进入：需要已有选中的块参照
    if (!m_blockRefBeingEdited)
    {
        GUIDIALOGFACTORY->commandMessage(
            tr("No block reference selected. Command cancelled."));
        finish();
        return;
    }

    enterEditing(m_blockRefBeingEdited);
    setStatus(eEditing);
}

/// @brief 触发动作执行
void ActionBlocksEdit::trigger()
{
    // 新设计中不再使用；初始化逻辑全部放在 init() 中处理
}

/// @brief 重新进入编辑模式（通过Undo/Redo）
/// @param block 块定义指针
void ActionBlocksEdit::reenterEditing(DmBlock* block)
{
    m_isReentry = true;
    m_editingBlock = block;
    m_blockName = block->getName();
    setStatus(eEditing);
    showOptions();
    docView->zoomAuto();
    GUIDIALOGFACTORY->commandMessage(tr("Editing block: %1").arg(m_blockName));
}

/// @brief 正常进入编辑模式
/// @param blockRef 块参照指针
void ActionBlocksEdit::enterEditing(DmBlockReference* blockRef)
{
    if (!blockRef || !pDocument)
    {
        return;
    }

    m_blockRefBeingEdited = blockRef;
    m_blockName = blockRef->getName();

    DmBlockTable* blockTable = pDocument->getBlockTable();
    if (!blockTable)
    {
        return;
    }

    DmBlock* block = blockTable->find(m_blockName);
    if (!block)
    {
        GUIDIALOGFACTORY->commandMessage(
            tr("Block definition not found: %1").arg(m_blockName));
        return;
    }

    // 检查是否存在嵌套块
    QStringList nestedNames;
    QSet<QString> visited;
    block->collectNestedBlockNames(nestedNames, visited);

    if (nestedNames.size() > 1)
    {
        // 弹出嵌套块选择对话框
        UINestedBlockSelectDialog dlg(pDocument, nestedNames, nullptr);
        if (dlg.exec() != QDialog::Accepted)
        {
            finish();
            return;
        }

        QString selectedName = dlg.selectedBlockName();
        if (selectedName.isEmpty())
        {
            finish();
            return;
        }

        if (selectedName != m_blockName)
        {
            m_isNestedEdit = true;
            m_topLevelBlockName = m_blockName;
            m_blockName = selectedName;

            block = blockTable->find(m_blockName);
            if (!block)
            {
                GUIDIALOGFACTORY->commandMessage(
                    tr("Block definition not found: %1").arg(m_blockName));
                finish();
                return;
            }
        }
    }

    m_editingBlock = block;

    // 进入编辑前取消所有选中状态，避免 undo 退出后块参照仍显示为选中
    blockRef->setSelected(false);

    // 使用 BlockEditEnterCmd 创建事务
    // 使用 addToCurrentCmd()，不要使用 addAndExecuteCmd()，避免重复执行
    Transaction t("Block Edit Begin", pDocument);
    t.start();
    auto* enterCmd = new BlockEditEnterCmd(pDocument, m_blockName, docView);
    pDocument->getCmdManager()->addToCurrentCmd(enterCmd);
    t.commit();

    // 保存宏命令，供取消编辑时定位回滚位置
    CmdManager* cmdMgr = pDocument->getCmdManager();
    m_enterMacroCmd = cmdMgr->getLastUndoCmd();
    m_undoCountAtEnter = cmdMgr->getUndoCount();

    setStatus(eEditing);
    showOptions();

    docView->zoomAuto();

    GUIDIALOGFACTORY->commandMessage(tr("Editing block: %1").arg(m_blockName));
}

/// @brief 完成编辑
/// @param save 是否保存修改（更新块参照）
void ActionBlocksEdit::completeEditing(bool save)
{
    if (!pDocument)
    {
        finish();
        return;
    }

    DmBlock* editingBlock = pDocument->getEditingBlock();
    if (editingBlock)
    {
        // 使用 BlockEditExitCmd 创建事务
        Transaction t("Block Edit End", pDocument);
        t.start();
        auto* exitCmd = new BlockEditExitCmd(pDocument, m_blockName, docView, save);
        pDocument->getCmdManager()->addToCurrentCmd(exitCmd);
        t.commit();
    }

    m_enterMacroCmd = nullptr;
    m_editingBlock = nullptr;

    hideOptions();
    docView->setMouseCursor(DM::CadCursor);
    docView->redraw();
    finish();
}

/// @brief 取消编辑，丢弃所有修改
void ActionBlocksEdit::cancelEditing()
{
    if (!pDocument)
    {
        finish();
        return;
    }

    CmdManager* cmdMgr = pDocument->getCmdManager();

    if (m_enterMacroCmd)
    {
        // 找到进入编辑命令，并回滚其后的所有命令
        int index = cmdMgr->indexOfCmd(m_enterMacroCmd);
        if (index != -1)
        {
            // 这里会撤销并删除从进入编辑到当前的所有命令，
            // 包括进入编辑命令本身（其内部会调用 setEditBlock(nullptr)）
            cmdMgr->rollbackAndRemoveAfter(index);
        }
        m_enterMacroCmd = nullptr;
    }
    else
    {
        // 重新进入场景或未记录进入命令时：直接退出编辑模式
        DmBlock* editingBlock = pDocument->getEditingBlock();
        if (editingBlock)
        {
            pDocument->setEditBlock(nullptr);
            pDocument->regenerate();
        }
    }

    m_editingBlock = nullptr;

    hideOptions();
    docView->setMouseCursor(DM::CadCursor);
    docView->redraw();
    finish();
}

/// @brief 检测自进入编辑以来是否有修改
/// @return true表示有修改
bool ActionBlocksEdit::hasModifications() const
{
    if (!pDocument)
    {
        return false;
    }
    CmdManager* cmdMgr = pDocument->getCmdManager();
    return cmdMgr->getUndoCount() > m_undoCountAtEnter;
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件指针
void ActionBlocksEdit::mouseMoveEvent(QMouseEvent* e)
{
    if (getStatus() == eEditing)
    {
        auto* handler = docView->getEventHandler();
        if (handler && handler->getDefaultAction())
        {
            handler->getDefaultAction()->mouseMoveEvent(e);
        }
    }
}

/// @brief 鼠标按下事件处理
/// @param e 鼠标事件指针
void ActionBlocksEdit::mousePressEvent(QMouseEvent* e)
{
    if (getStatus() == eEditing)
    {
        auto* handler = docView->getEventHandler();
        if (handler && handler->getDefaultAction())
        {
            handler->getDefaultAction()->mousePressEvent(e);
        }
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件指针
void ActionBlocksEdit::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (getStatus() == eEditing)
        {
            auto* handler = docView->getEventHandler();
            if (handler && handler->getDefaultAction())
            {
                handler->getDefaultAction()->mouseReleaseEvent(e);
            }
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        if (getStatus() == eEditing)
        {
            int ret = QMessageBox::question(nullptr,
                tr("Block Edit"),
                tr("Finish editing and save changes?"),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (ret == QMessageBox::Yes)
            {
                completeEditing(true);
            }
            else if (ret == QMessageBox::No)
            {
                completeEditing(false);
            }
            // 取消：继续编辑
        }
        else
        {
            finish();
        }
    }
}

/// @brief 键盘按下事件处理
/// @param e 键盘事件指针
void ActionBlocksEdit::keyPressEvent(QKeyEvent* e)
{
    if (getStatus() == eEditing)
    {
        auto* handler = docView->getEventHandler();
        if (handler && handler->getDefaultAction())
        {
            handler->getDefaultAction()->keyPressEvent(e);
        }
    }
    else
    {
        ActionInterface::keyPressEvent(e);
    }
}

/// @brief 检查是否可以中断
/// @return true表示可以中断
bool ActionBlocksEdit::canBeInterrupt()
{
    return true;
}

/// @brief 检查是否为独占动作
/// @return false表示非独占
bool ActionBlocksEdit::isExclusive()
{
    return false;
}

/// @brief 检查是否为子动作
/// @return false表示不是子动作
bool ActionBlocksEdit::isSubAction()
{
    return false;
}

/// @brief 显示选项栏
void ActionBlocksEdit::showOptions()
{
    ActionInterface::showOptions();
    GUIDIALOGFACTORY->requestOptions(this, true);
}

/// @brief 隐藏选项栏
void ActionBlocksEdit::hideOptions()
{
    ActionInterface::hideOptions();
    GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 更新鼠标按钮提示
void ActionBlocksEdit::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case eEditing:
        GUIDIALOGFACTORY->updateMouseWidget(
            tr("Edit block entities"), tr("Finish / Cancel"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @brief 更新鼠标光标
void ActionBlocksEdit::updateMouseCursor()
{
    if (getStatus() == eEditing)
    {
        docView->setMouseCursor(DM::ArrowCursor);
    }
    else
    {
        docView->setMouseCursor(DM::CadCursor);
    }
}

// EOF
