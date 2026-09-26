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

/// @file BlockEditTool.cpp
/// @brief 块编辑模式的实现；进入与退出的文档操作原样来自 ActionBlocksEdit

#include "BlockEditTool.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QSet>
#include <QStringList>

#include "BlockEditCmd.h"
#include "CmdManager.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "ExclusiveCommandBus.h"
#include "ICommandHost.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "Transaction.h"
#include "UIBlockEditOptions.h"
#include "UIDialogRunner.h"
#include "UINestedBlockSelectDialog.h"

BlockEditTool::BlockEditTool(ICommandHost& host)
    : m_host(host)
    , m_document(host.document())
    , m_view(host.view())
{
}

BlockEditTool::~BlockEditTool() = default;

bool BlockEditTool::prepare(DmBlockReference* blockRef)
{
    m_blockName = blockRef->getName();

    DmBlockTable* blockTable = m_document->getBlockTable();
    if (!blockTable)
    {
        return false;
    }

    DmBlock* block = blockTable->find(m_blockName);
    if (!block)
    {
        GUIDIALOGFACTORY->commandMessage(tr("Block definition not found: %1").arg(m_blockName));
        return false;
    }

    // 检查是否存在嵌套块
    QStringList nestedNames;
    QSet<QString> visited;
    block->collectNestedBlockNames(nestedNames, visited);

    if (nestedNames.size() > 1)
    {
        // 弹出嵌套块选择对话框（无父窗口，与原先一致）
        UINestedBlockSelectDialog dlg(m_document, nestedNames, nullptr);
        if (UIDialogRunner::exec(dlg) != QDialog::Accepted)
        {
            return false;
        }
        const QString selectedName = dlg.selectedBlockName();
        if (selectedName.isEmpty())
        {
            return false;
        }
        if (selectedName != m_blockName)
        {
            m_blockName = selectedName;
            if (!blockTable->find(m_blockName))
            {
                GUIDIALOGFACTORY->commandMessage(tr("Block definition not found: %1").arg(m_blockName));
                return false;
            }
        }
    }
    return true;
}

void BlockEditTool::beginEditing(DmBlockReference* blockRef)
{
    // 进入编辑前取消所有选中状态，避免 undo 退出后块参照仍显示为选中
    blockRef->setSelected(false);

    // 使用 BlockEditEnterCmd 创建事务
    // 使用 addToCurrentCmd()，不要使用 addAndExecuteCmd()，避免重复执行
    Transaction t("Block Edit Begin", m_document);
    t.start();
    auto* enterCmd = new BlockEditEnterCmd(m_document, m_blockName);
    m_document->getCmdManager()->addToCurrentCmd(enterCmd);
    t.commit();

    m_undoCountAtEnter = m_document->getCmdManager()->getUndoCount();

    m_view->zoomAuto();
    GUIDIALOGFACTORY->commandMessage(tr("Editing block: %1").arg(m_blockName));
}

void BlockEditTool::reenter(DmBlock* block)
{
    m_blockName = block->getName();
    m_view->zoomAuto();
    GUIDIALOGFACTORY->commandMessage(tr("Editing block: %1").arg(m_blockName));
}

bool BlockEditTool::hasModifications() const
{
    return m_document->getCmdManager()->getUndoCount() > m_undoCountAtEnter;
}

void BlockEditTool::completeEditing(bool save)
{
    m_exitDecision = save ? ExitDecision::Save : ExitDecision::Discard;
    m_host.commandBus()->requestExitEditMode(this);
}

bool BlockEditTool::askSaveAndExit()
{
    switch (GUIDIALOGFACTORY->requestYesNoCancelDialog(tr("Block Edit"), tr("Finish editing and save changes?")))
    {
    case DialogAnswer::Yes:
        m_exitDecision = ExitDecision::Save;
        return true;
    case DialogAnswer::No:
        m_exitDecision = ExitDecision::Discard;
        return true;
    default:
        // 取消：继续编辑
        return false;
    }
}

bool BlockEditTool::onEndRequested(CommandEndReason reason)
{
    if (reason == CommandEndReason::ViewClosing)
    {
        // 与原先一致：视图关闭时不提问，也不改动文档
        m_exitDecision = ExitDecision::LeaveAsIs;
        return true;
    }
    return askSaveAndExit();
}

void BlockEditTool::onExit()
{
    if (m_exitDecision != ExitDecision::LeaveAsIs && m_document->getEditingBlock())
    {
        // 使用 BlockEditExitCmd 创建事务
        Transaction t("Block Edit End", m_document);
        t.start();
        auto* exitCmd =
            new BlockEditExitCmd(m_document, m_blockName, m_exitDecision == ExitDecision::Save);
        m_document->getCmdManager()->addToCurrentCmd(exitCmd);
        t.commit();
    }

    showOptions(false);
    if (m_exitDecision != ExitDecision::LeaveAsIs)
    {
        m_view->setMouseCursor(DM::CadCursor);
    }
    m_view->redraw();
}

void BlockEditTool::suspendMode()
{
    m_suspended = true;
    showOptions(false);
}

void BlockEditTool::resumeMode()
{
    m_suspended = false;
    updateHints();
    showOptions(true);
}

void BlockEditTool::showOptions(bool on)
{
    GUIDIALOGFACTORY->requestEditModeOptions(
        [this](QWidget* parent) -> QWidget*
        {
            auto* options = new UIBlockEditOptions(parent);
            options->setSession(this);
            return options;
        },
        on);
}

ViewToolResult BlockEditTool::mousePressEvent(QMouseEvent*)
{
    return ViewToolResult::NotHandled;
}

ViewToolResult BlockEditTool::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::RightButton)
    {
        return ViewToolResult::NotHandled;
    }
    if (askSaveAndExit())
    {
        m_host.commandBus()->requestExitEditMode(this);
    }
    return ViewToolResult::Handled;
}

ViewToolResult BlockEditTool::mouseMoveEvent(QMouseEvent*)
{
    return ViewToolResult::NotHandled;
}

ViewToolResult BlockEditTool::mouseDoubleClickEvent(QMouseEvent*)
{
    return ViewToolResult::Handled;
}

ViewToolResult BlockEditTool::keyPressEvent(QKeyEvent*)
{
    return ViewToolResult::NotHandled;
}

ViewToolResult BlockEditTool::keyReleaseEvent(QKeyEvent* e)
{
    e->ignore();
    return ViewToolResult::Handled;
}

ViewToolResult BlockEditTool::coordinateEvent(const DmVector&)
{
    return ViewToolResult::Handled;
}

void BlockEditTool::enterEvent()
{
    if (!m_suspended)
    {
        updateHints();
    }
}

void BlockEditTool::updateHints() const
{
    GUIDIALOGFACTORY->updateMouseWidget(tr("Edit block entities"), tr("Finish / Cancel"));
}
