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

/// @file DefineAttributesCommand.cpp
/// @brief DefineAttributesCommand 与定义属性工具的实现

#include "DefineAttributesCommand.h"

#include <QMouseEvent>
#include <QStringList>

#include "AttributeDefinitionData.h"
#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "Commands.h"
#include "DmAttributeDefinition.h"
#include "DmDocument.h"
#include "DmLine.h"
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

namespace
{
/// @brief 定义属性工具：插入点，对齐、布满方式再加第二点
class DefineAttributesTool : public BasePlaceTool
{
public:
    /// @brief 交互状态（原 ShowDialog 一步改为在启动前弹出对话框）
    enum Status
    {
        SetPos,   ///< 设置插入点
        SetSecPos ///< 设置对齐、布满方式的第二点
    };

    DefineAttributesTool(DefineAttributesCommand& command, DmDocument* doc, IDocumentView* view)
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
            GUIDIALOGFACTORY->updateMouseWidget(DefineAttributesCommand::tr("Specify insertion point"),
                                                DefineAttributesCommand::tr("Cancel"));
            break;
        case SetSecPos:
            GUIDIALOGFACTORY->updateMouseWidget(DefineAttributesCommand::tr("Specify second point"),
                                                DefineAttributesCommand::tr("Cancel"));
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
            // 原 Action 右键在任何一步都直接结束
            command().finish();
        }
    }

    void onCoordinate(const DmVector& coord) override
    {
        switch (status())
        {
        case SetPos:
            // 原 Action 取的是最后一次鼠标移动的位置，命令行输入的坐标不起作用；这里用输入的点
            m_pos = coord;
            if (m_command.needsSecondPoint())
            {
                setStatus(SetSecPos);
            }
            else
            {
                m_command.commitOnePoint(m_pos);
            }
            break;
        case SetSecPos:
            m_command.commitTwoPoints(m_pos, coord);
            break;
        default:
            break;
        }
    }

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
        }
    }

private:
    DefineAttributesCommand& m_command;
    DmVector m_pos; ///< 插入点（对齐、布满方式的第一点）
};

/// @brief 对齐与布满方式要两点
bool isTwoPointMode(ETextMode mode)
{
    return mode == ETextMode::kTextAligned || mode == ETextMode::kTextFit;
}
}  // namespace

DefineAttributesCommand::DefineAttributesCommand() = default;

DefineAttributesCommand::~DefineAttributesCommand() = default;

std::unique_ptr<BasePlaceTool> DefineAttributesCommand::createTool()
{
    // 原 reset()：当前文字样式、左对齐、字高 1
    DmTextStyle* style = document()->getTextStyleTable()->getActive();
    m_textData = std::make_unique<TextData>(DmVector(0.0, 0.0), 1.0, ETextVertMode::kTextBase,
                                            ETextHorzMode::kTextLeft, QString(), style, 0.0, EUpdateMode::Update);
    m_attrData = std::make_unique<AttributeDefinitionData>("", "");

    DmAttributeDefinition tmp(nullptr, *m_textData, *m_attrData);
    tmp.setDocument(document());
    if (!GUIDIALOGFACTORY->requestDefineAttributesDialog(&tmp))
    {
        return nullptr;
    }
    m_textData = std::make_unique<TextData>(tmp.getData());
    m_attrData = std::make_unique<AttributeDefinitionData>(tmp.getAttributeData());
    return std::make_unique<DefineAttributesTool>(*this, document(), view());
}

bool DefineAttributesCommand::needsSecondPoint() const
{
    return isTwoPointMode(m_textData->getTextMode());
}

void DefineAttributesCommand::previewText(const DmVector& pos)
{
    preview().clear();
    if (needsSecondPoint())
    {
        return;
    }
    setDataWithOnePoint(pos);
    auto* text = new DmAttributeDefinition(nullptr, *m_textData, *m_attrData);
    text->update();
    preview().entities().addEntity(text);
    preview().draw();
}

void DefineAttributesCommand::previewLine(const DmVector& first, const DmVector& second)
{
    preview().clear();
    preview().entities().appendEntity(new DmLine(nullptr, LineData(first, second)));
    preview().draw();
}

void DefineAttributesCommand::commitOnePoint(const DmVector& pos)
{
    setDataWithOnePoint(pos);
    commit();
}

void DefineAttributesCommand::commitTwoPoints(const DmVector& first, const DmVector& second)
{
    m_textData->setPosition(first);
    m_textData->setAlignment(second);
    commit();
}

void DefineAttributesCommand::setDataWithOnePoint(const DmVector& pos)
{
    if (m_textData->getTextMode() == ETextMode::kTextLeft)
    {
        // 左对齐的对齐点为 0，位置随插入点
        m_textData->setPosition(pos);
        m_textData->setAlignment(DmVector(0.0, 0.0));
    }
    else
    {
        m_textData->setAlignment(pos);
    }
}

void DefineAttributesCommand::commit()
{
    preview().clear();
    Transaction t(tr("Create Attributes").toStdString(), document());
    t.start();
    auto* text = new DmAttributeDefinition(nullptr, *m_textData, *m_attrData);
    text->setDocument(document());
    text->update();
    document()->getEntityTable()->add(text);
    t.commit();
    finish();
}
