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

/// @file DrawTextCommand.cpp
/// @brief DrawTextCommand 与单行文字工具的实现

#include "DrawTextCommand.h"

#include <QMouseEvent>
#include <QStringList>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLine.h"
#include "DmText.h"
#include "DmTextStyleTable.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "LineData.h"
#include "Preview.h"
#include "TextData.h"
#include "Transaction.h"
#include "UIDialogRunner.h"
#include "UIDlgText.h"

namespace
{
/// @brief 单行文字工具：插入点，对齐、布满方式再加第二点；命令行可改文字
class DrawTextTool : public BasePlaceTool
{
public:
    /// @brief 交互状态（原 ShowDialog 一步改为在启动前弹出对话框）
    enum Status
    {
        SetPos,    ///< 设置插入点
        SetSecPos, ///< 设置对齐、布满方式的第二点
        SetText    ///< 在命令行输入文字
    };

    DrawTextTool(DrawTextCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetPos:
            GUIDIALOGFACTORY->updateMouseWidget(DrawTextCommand::tr("Specify insertion point"),
                                                DrawTextCommand::tr("Cancel"));
            break;
        case SetSecPos:
            GUIDIALOGFACTORY->updateMouseWidget(DrawTextCommand::tr("Specify second point"),
                                                DrawTextCommand::tr("Cancel"));
            break;
        case SetText:
            GUIDIALOGFACTORY->updateMouseWidget(DrawTextCommand::tr("Enter text:"), DrawTextCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        const DmVector mouse = snapper()->snapPoint(e);
        if (status() == SetPos)
        {
            m_pos = mouse;
            m_command.previewText(m_pos);
        }
        else if (status() == SetSecPos)
        {
            m_command.previewLine(m_pos, mouse);
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            if (status() == SetSecPos)
            {
                restart(SetPos);
            }
            else
            {
                command().finish();
            }
        }
    }

    void onCoordinate(const DmVector& coord) override
    {
        switch (status())
        {
        case SetPos:
            m_pos = coord;
            if (m_command.needsSecondPoint())
            {
                setStatus(SetSecPos);
            }
            else
            {
                m_command.commitOnePoint(m_pos);
                restart(SetPos);
            }
            break;
        case SetSecPos:
            m_command.commitTwoPoints(m_pos, coord);
            restart(SetPos);
            break;
        default:
            break;
        }
    }

    /// @note 与原 Action 一样不接受命令行文本：文本随后还会被当作新命令解析
    void onCommand(GuiCommandEvent* e) override
    {
        const QString c = e->getCommand().toLower();
        if (Commands::checkCommand("help", c))
        {
            QStringList cmd;
            if (status() == SetPos)
            {
                cmd += Commands::command("text");
            }
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands() + cmd.join(", "));
            return;
        }

        switch (status())
        {
        case SetPos:
            // Commands::checkCommand 对 help/close/undo 以外的关键字都返回 true：任何文字都进入
            // "输入文字"（原有行为）
            if (Commands::checkCommand("text", c))
            {
                m_command.preview().clear();
                view()->disableCoordinateInput();
                setStatus(SetText);
            }
            break;
        case SetText:
            m_command.setText(e->getCommand());
            GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
            view()->enableCoordinateInput();
            setStatus(SetPos);
            break;
        default:
            break;
        }
    }

    /// @brief 在输入文字时结束：恢复坐标输入（原 Action 在这一步右键结束时没有恢复）
    void onFinish() override
    {
        if (status() == SetText)
        {
            view()->enableCoordinateInput();
        }
    }

private:
    DrawTextCommand& m_command;
    DmVector m_pos; ///< 插入点（对齐、布满方式的第一点）
};

/// @brief 对齐与布满方式要两点
bool isTwoPointMode(ETextMode mode)
{
    return mode == ETextMode::kTextAligned || mode == ETextMode::kTextFit;
}
}  // namespace

DrawTextCommand::DrawTextCommand() = default;

DrawTextCommand::~DrawTextCommand() = default;

std::unique_ptr<BasePlaceTool> DrawTextCommand::createTool()
{
    // 原 reset()：当前文字样式、左对齐、字高 1
    DmTextStyle* style = document()->getTextStyleTable()->getActive();
    m_data = std::make_unique<TextData>(DmVector(0.0, 0.0), 1.0, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft,
                                        QString(), style, 0.0, EUpdateMode::Update);
    DmText tmp(nullptr, *m_data);
    tmp.setDocument(document());
    UIDlgText dlg(dialogParent());
    dlg.setText(tmp, true);
    if (UIDialogRunner::exec(dlg) != QDialog::Accepted)
    {
        return nullptr;
    }
    dlg.updateText();
    m_data = std::make_unique<TextData>(tmp.getData());
    return std::make_unique<DrawTextTool>(*this, document(), view());
}

void DrawTextCommand::showOptions()
{
    // 原 Action 显示选项条时一律从 Action 读取（update 为 true）
    GUIDIALOGFACTORY->requestCommandOptions(this, true, true);
}

void DrawTextCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

QString DrawTextCommand::text() const
{
    return m_data ? m_data->getTextString() : QString();
}

void DrawTextCommand::setText(const QString& t)
{
    if (m_data)
    {
        m_data->setTextString(t);
    }
}

double DrawTextCommand::angle() const
{
    return m_data ? m_data->getAngle() : 0.0;
}

void DrawTextCommand::setAngle(double a)
{
    if (m_data)
    {
        m_data->setAngle(a);
    }
}

bool DrawTextCommand::needsSecondPoint() const
{
    return isTwoPointMode(m_data->getTextMode());
}

void DrawTextCommand::previewText(const DmVector& pos)
{
    preview().clear();
    if (needsSecondPoint())
    {
        return;
    }
    setDataWithOnePoint(pos);
    auto* text = new DmText(preview().entities().getEntityContainer(), *m_data);
    text->setDocument(document());
    text->update();
    preview().entities().addEntity(text);
    preview().draw();
}

void DrawTextCommand::previewLine(const DmVector& first, const DmVector& second)
{
    preview().clear();
    preview().entities().appendEntity(new DmLine(nullptr, LineData(first, second)));
    preview().draw();
}

void DrawTextCommand::commitOnePoint(const DmVector& pos)
{
    setDataWithOnePoint(pos);
    commit();
}

void DrawTextCommand::commitTwoPoints(const DmVector& first, const DmVector& second)
{
    m_data->setPosition(first);
    m_data->setAlignment(second);
    commit();
}

void DrawTextCommand::setDataWithOnePoint(const DmVector& pos)
{
    if (m_data->getTextMode() == ETextMode::kTextLeft)
    {
        // 左对齐的对齐点为 0，位置随插入点
        m_data->setPosition(pos);
        m_data->setAlignment(DmVector(0.0, 0.0));
    }
    else
    {
        m_data->setAlignment(pos);
    }
}

void DrawTextCommand::commit()
{
    preview().clear();
    Transaction t(tr("Create Text").toStdString(), document());
    t.start();
    auto* text = new DmText(nullptr, *m_data);
    text->setDocument(document());
    text->update();
    document()->getEntityTable()->add(text);
    t.commit();
}
