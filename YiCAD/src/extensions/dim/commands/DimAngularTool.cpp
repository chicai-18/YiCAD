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

/// @file DimAngularTool.cpp
/// @brief DimAngularCommand 与工具（从原 ActionDimAngular 机械改写）

#include <memory>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringList>
#include <cmath>
#include "DmDimAngular.h"
#include "DmLine.h"
#include "Information.h"
#include "Math2d.h"

#include "CommandPreview.h"
#include "Commands.h"
#include "DimCommands.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Preview.h"
#include "Transaction.h"

namespace
{
/// @brief 角度标注工具：选两条直线，再指定标注弧线位置；可连续标注
class DimAngularTool : public DimensionTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetLine1,  ///< 选择第一条线
        SetLine2,  ///< 选择第二条线
        SetPos,    ///< 选择标注弧线位置
        SetText    ///< 在命令行中设置文本标签
    };

    DimAngularTool(DimAngularCommand& command, DmDocument* doc, IDocumentView* view)
        : DimensionTool(command, doc, view)
        , m_command(command)
    {
        edata = std::make_unique<DmDimAngularData>(DmVector(false), DmVector(false), DmVector(false), DmVector(false),
                                                     DmVector(false));
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCoordinate(const DmVector& coord) override;
    void onCommand(GuiCommandEvent* e) override;

private:
    void reset();
    void trigger();
    QStringList availableCommands() const;

    DimAngularCommand& m_command;
    DmLine line1;                            ///< 第一条线
    DmLine line2;                            ///< 第二条线
    DmVector center;                         ///< 标注圆弧中心
    std::unique_ptr<DmDimAngularData> edata; ///< 角度标注数据
    Status lastStatus = SetLine1;            ///< 进入文本输入前的上一状态（原 Action 未初始化）
};
}  // namespace

void DimAngularTool::reset()
{
    resetDimension();

    edata.reset(new DmDimAngularData(
        DmVector(false), DmVector(false),
        DmVector(false), DmVector(false), DmVector(false)));
    GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
}

/// @brief 触发创建角度标注
void DimAngularTool::trigger()
{
    m_command.preview().clear();

    if (line1.getStartpoint().valid && line2.getStartpoint().valid)
    {
        DmDimAngular* newEntity = new DmDimAngular(nullptr, *data, *edata);
        newEntity->setDocument(document());
        newEntity->update();

        Transaction t(DimAngularCommand::tr("Add dimension angular").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(newEntity);
        t.commit();

        DmVector rz{view()->getRelativeZero()};
        setStatus(SetLine1);
        view()->moveRelativeZero(rz);
        snapper()->finish();
    }
}

/// @brief 鼠标移动事件处理
void DimAngularTool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
    case SetPos:
    {
        edata->ptOnArc = view()->toGraph(e->x(), e->y());

        DmDimAngular* d =
            new DmDimAngular(m_command.preview().entities().getEntityContainer(), *data, *edata);
        d->setDocument(document());
        m_command.preview().clear();
        m_command.preview().entities().addEntity(d);
        d->update();
        m_command.preview().draw();
    }
    break;

    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
void DimAngularTool::onMouseRelease(QMouseEvent* e)
{
    if (Qt::LeftButton == e->button())
    {
        switch (status())
        {
        case SetLine1:
        {
            DmEntity* en{snapper()->catchEntity(e, DM::ResolveAll)};

            if (en && DM::EntityLine == en->getEntityType())
            {
                line1 = *dynamic_cast<DmLine*>(en);
                edata->line1StartPt = line1.getStartpoint();
                edata->line1EndPt = line1.getEndpoint();
                setStatus(SetLine2);
            }
        }
        break;

        case SetLine2:
        {
            DmEntity* en{snapper()->catchEntity(e, DM::ResolveAll)};

            if (en && en->getEntityType() == DM::EntityLine)
            {
                line2 = *dynamic_cast<DmLine*>(en);
                edata->line2StartPt = line2.getStartpoint();
                edata->line2EndPt = line2.getEndpoint();
                view()->moveRelativeZero(center);
                setStatus(SetPos);
            }
        }
        break;

        case SetPos:
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        break;

        default:
            break;
        }
    }
    else if (Qt::RightButton == e->button())
    {
        m_command.preview().clear();
        init(status() - 1);
    }
}

/// @brief 坐标事件处理
void DimAngularTool::onCoordinate(const DmVector& coord)
{
    switch (status())
    {
    case SetPos:
    {
        edata->ptOnArc = coord;
        trigger();
        finishIfOrthogonal();
        reset();
        setStatus(SetLine1);
    }
    break;

    default:
        break;
    }
}

/// @brief 命令事件处理
void DimAngularTool::onCommand(GuiCommandEvent* e)
{
    QString c(e->getCommand().toLower());

    if (Commands::checkCommand(QStringLiteral("help"), c))
    {
        GUIDIALOGFACTORY->commandMessage(
            Commands::msgAvailableCommands() + availableCommands().join(", "));
        return;
    }

    // 设置新的文本标签
    if (SetText == status())
    {
        setText(c);
        GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
        view()->enableCoordinateInput();
        setStatus(lastStatus);
        return;
    }

    // 命令: text
    if (Commands::checkCommand(QStringLiteral("text"), c))
    {
        lastStatus = static_cast<Status>(status());
        view()->disableCoordinateInput();
        setStatus(SetText);
    }
}

/// @brief 获取可用命令列表
QStringList DimAngularTool::availableCommands() const
{
    QStringList cmd;

    switch (status())
    {
    case SetLine1:
    case SetLine2:
    case SetPos:
        cmd += Commands::command(QStringLiteral("text"));
        break;

    default:
        break;
    }

    return cmd;
}

/// @brief 更新鼠标按钮提示
void DimAngularTool::updateHints()
{
    switch (status())
    {
    case SetLine1:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAngularCommand::tr("Select first line"), DimAngularCommand::tr("Cancel"));
        break;

    case SetLine2:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAngularCommand::tr("Select second line"), DimAngularCommand::tr("Cancel"));
        break;

    case SetPos:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAngularCommand::tr("Specify dimension arc line location"), DimAngularCommand::tr("Cancel"));
        break;

    case SetText:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAngularCommand::tr("Enter dimension text:"), "");
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

std::unique_ptr<BasePlaceTool> DimAngularCommand::createTool()
{
    return std::make_unique<DimAngularTool>(*this, document(), view());
}
