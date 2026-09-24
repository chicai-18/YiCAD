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

/// @file ModifyMTextCommand.cpp
/// @brief ModifyMTextCommand 与属性面板工具的实现

#include "ModifyMTextCommand.h"

#include <QMouseEvent>
#include <QWidget>

#include "BasePlaceTool.h"
#include "DmDocument.h"
#include "DmMText.h"
#include "EntityTable.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"
#include "UIMTextModifyOptions.h"

namespace
{
constexpr int OPTION_PANEL_X = 1;       ///< 属性面板在主窗口里的位置与大小（与原 Action 相同）
constexpr int OPTION_PANEL_Y = 186;
constexpr int OPTION_PANEL_WIDTH = 500;
constexpr int OPTION_PANEL_HEIGHT = 70;

/// @brief 属性面板工具：在画布上单击（或双击）即取消选中并结束
///
/// 双击时第一次松开已结束本命令，第二次按下与双击归选择层，由它进入文字编辑（与原
/// Action 经"让路钩子"得到的效果相同）。
class ModifyMTextTool : public BasePlaceTool
{
public:
    ModifyMTextTool(ModifyMTextCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    /// @brief 不指定光标，沿用选择层的
    std::optional<DM::CursorType> getCursor() const override { return std::nullopt; }

protected:
    /// @brief 原 Action 没有按键提示
    void updateHints() override {}

    void onMouseMove(QMouseEvent*) override
    {
        // 原 Action：不画捕捉标记，免得光标看起来卡顿
        snapper()->deleteSnapper();
    }

    void onMouseRelease(QMouseEvent*) override { m_command.deselectAndFinish(); }

    void onMouseDoubleClick(QMouseEvent*) override { m_command.deselectAndFinish(); }

    void onFinish() override { m_command.closePanel(); }

private:
    ModifyMTextCommand& m_command;
};
}  // namespace

ModifyMTextCommand::ModifyMTextCommand(DmMText* text)
    : m_pMText(text)
{
}

ModifyMTextCommand::~ModifyMTextCommand()
{
    closePanel();
}

std::unique_ptr<BasePlaceTool> ModifyMTextCommand::createTool()
{
    return std::make_unique<ModifyMTextTool>(*this, document(), view());
}

bool ModifyMTextCommand::onStarted()
{
    // 面板挂在主窗口上；测试用的假视图没有窗口，不显示
    QWidget* canvas = view()->asQObject() ? qobject_cast<QWidget*>(view()->asQObject()) : nullptr;
    if (!canvas)
    {
        return true;
    }
    m_optionBack = new QWidget(canvas->window());
    m_optionBack->setObjectName("mTextModifyOptionBackWidget");
    auto* options = new UIMTextModifyOptions(m_optionBack);
    options->setCommand(this);
    m_optionBack->setGeometry(OPTION_PANEL_X, OPTION_PANEL_Y, OPTION_PANEL_WIDTH, OPTION_PANEL_HEIGHT);
    m_optionBack->show();
    return true;
}

void ModifyMTextCommand::deselectAndFinish()
{
    if (m_pMText)
    {
        m_pMText->setSelected(false);
    }
    finish();
    // 结束在分发返回后才生效：选择变化的监听者此时仍看到本命令，不会再启动它
    view()->emitSelectedChanged();
}

void ModifyMTextCommand::closePanel()
{
    if (m_optionBack)
    {
        m_optionBack->close();
        m_optionBack->deleteLater();
        m_optionBack.clear();
    }
}

void ModifyMTextCommand::updateContentIfEmpty(DmMText* text)
{
    if (text->getContent().isEmpty())
    {
        text->updateContent();
    }
}

void ModifyMTextCommand::setHeight(double height)
{
    if (!m_pMText || m_pMText->getCharHeight() == height)
    {
        return;
    }
    Transaction t(tr("Modify MText").toStdString(), document());
    t.start();
    document()->getEntityTable()->startModify(m_pMText);
    m_pMText->setCharHeight(height);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

double ModifyMTextCommand::getHeight() const
{
    return m_pMText->getDataConstPtr()->getCharHeight();
}

void ModifyMTextCommand::setStyle(DmTextStyle* textStyle)
{
    if (!m_pMText || m_pMText->getTextStyle() == textStyle)
    {
        return;
    }
    Transaction t(tr("Modify MText").toStdString(), document());
    t.start();
    document()->getEntityTable()->startModify(m_pMText);
    m_pMText->setTextStyle(textStyle);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

DmTextStyle* ModifyMTextCommand::getStyle() const
{
    return m_pMText->getDataConstPtr()->getTextStyle();
}

void ModifyMTextCommand::setLineSpaceFatctor(double factor)
{
    if (!m_pMText || m_pMText->getLineSpacingFactor() == factor)
    {
        return;
    }
    Transaction t(tr("Modify MText").toStdString(), document());
    t.start();
    document()->getEntityTable()->startModify(m_pMText);
    m_pMText->setLineSpacingFactor(factor);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

double ModifyMTextCommand::getLineSpaceFatctor() const
{
    return m_pMText->getDataConstPtr()->getLineSpacingFactor();
}

void ModifyMTextCommand::setAngle(double angle)
{
    if (!m_pMText || m_pMText->getAngle() == angle)
    {
        return;
    }
    Transaction t(tr("Modify MText").toStdString(), document());
    t.start();
    document()->getEntityTable()->startModify(m_pMText);
    m_pMText->setAngle(angle);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

double ModifyMTextCommand::getAngle() const
{
    return m_pMText->getDataConstPtr()->getAngle();
}

void ModifyMTextCommand::setLineSpace(double lineSpace)
{
    if (!m_pMText || m_pMText->getLineSpace() == lineSpace)
    {
        return;
    }
    Transaction t(tr("Modify MText").toStdString(), document());
    t.start();
    document()->getEntityTable()->startModify(m_pMText);
    m_pMText->setLineSpace(lineSpace);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

double ModifyMTextCommand::getLineSpace() const
{
    return m_pMText->getDataConstPtr()->getLineSpace();
}
