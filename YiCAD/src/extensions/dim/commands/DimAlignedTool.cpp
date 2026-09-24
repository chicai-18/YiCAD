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

/// @file DimAlignedTool.cpp
/// @brief DimAlignedCommand 与工具（从原 ActionDimAligned 机械改写）

#include <memory>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringList>
#include <cmath>
#include "DmConstructionLine.h"
#include "DmDimAligned.h"
#include "DmLine.h"

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
/// @brief 对齐标注工具：两条延伸线起点，再指定标注线位置；可连续标注
class DimAlignedTool : public DimensionTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetExtPoint1, ///< 设置第一条延伸线起点
        SetExtPoint2, ///< 设置第二条延伸线起点
        SetDefPoint,  ///< 设置标注线位置
        SetText       ///< 在命令行中设置文本标签
    };

    DimAlignedTool(DimAlignedCommand& command, DmDocument* doc, IDocumentView* view)
        : DimensionTool(command, doc, view)
        , m_command(command)
    {
        edata = std::make_unique<DmDimAlignedData>(DmVector(false), DmVector(false));
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
    void preparePreview();
    QStringList availableCommands() const;

    DimAlignedCommand& m_command;
    std::unique_ptr<DmDimAlignedData> edata; ///< 对齐标注数据
    Status lastStatus = SetExtPoint1;       ///< 进入文本输入前的上一状态
};
}  // namespace

void DimAlignedTool::reset()
{
    resetDimension();

    edata.reset(new DmDimAlignedData(DmVector(false), DmVector(false)));
    lastStatus = SetExtPoint1;
    GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
}

/// @brief 触发创建对齐标注
void DimAlignedTool::trigger()
{
    m_command.preview().clear();

    preparePreview();
    view()->moveRelativeZero(data->definitionPoint);

    DmDimAligned* dim = new DmDimAligned(nullptr, *data, *edata);
    dim->setDocument(document());
    dim->update();

    Transaction t(DimAlignedCommand::tr("Add dimension aligned").toStdString(), document());
    t.start();
    document()->getEntityTable()->add(dim);
    t.commit();

    DmVector rz = view()->getRelativeZero();
    view()->moveRelativeZero(rz);
}

/// @brief 准备预览图形，计算标注线位置
void DimAlignedTool::preparePreview()
{
    constexpr double PREVIEW_LINE_LENGTH = 100.0;

    DmVector dirV = DmVector::polar(
        PREVIEW_LINE_LENGTH,
        edata->extensionPoint1.angleTo(edata->extensionPoint2) + M_PI_2);
    DmConstructionLine cl(
        nullptr,
        DmConstructionLineData(edata->extensionPoint2, edata->extensionPoint2 + dirV));

    data->definitionPoint = cl.getNearestPointOnEntity(data->definitionPoint);
}

/// @brief 鼠标移动事件处理
void DimAlignedTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);

    switch (status())
    {
    case SetExtPoint1:
        break;

    case SetExtPoint2:
    {
        if (edata->extensionPoint1.valid)
        {
            m_command.preview().clear();

            auto l = new DmLine{
                m_command.preview().entities().getEntityContainer(),
                edata->extensionPoint1, mouse};
            l->setDocument(document());
            m_command.preview().entities().addEntity(l);
            m_command.preview().draw();
        }
    }
    break;

    case SetDefPoint:
    {
        if (edata->extensionPoint1.valid && edata->extensionPoint2.valid)
        {
            m_command.preview().clear();
            data->definitionPoint = mouse;

            preparePreview();

            DmDimAligned* dim =
                new DmDimAligned(m_command.preview().entities().getEntityContainer(), *data, *edata);
            dim->setDocument(document());
            m_command.preview().entities().addEntity(dim);
            dim->update();
            m_command.preview().draw();
        }
    }
    break;

    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
void DimAlignedTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        onCoordinate(snapper()->snapPoint(e));
    }
    else if (e->button() == Qt::RightButton)
    {
        m_command.preview().clear();
        init(status() - 1);
    }
}

/// @brief 坐标事件处理
void DimAlignedTool::onCoordinate(const DmVector& coord)
{
    DmVector pos = coord;

    switch (status())
    {
    case SetExtPoint1:
        edata->extensionPoint1 = pos;
        view()->moveRelativeZero(pos);
        setStatus(SetExtPoint2);
        break;

    case SetExtPoint2:
        edata->extensionPoint2 = pos;
        view()->moveRelativeZero(pos);
        setStatus(SetDefPoint);
        break;

    case SetDefPoint:
        data->definitionPoint = pos;
        trigger();
        finishIfOrthogonal();
        reset();
        setStatus(SetExtPoint1);
        break;

    default:
        break;
    }
}

/// @brief 命令事件处理
void DimAlignedTool::onCommand(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(
            Commands::msgAvailableCommands() + availableCommands().join(", "));
        return;
    }

    switch (status())
    {
    case SetText:
    {
        // TODO: 待修正
        // setText(c);
        GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
        setStatus(lastStatus);
        view()->enableCoordinateInput();
    }
    break;

    default:
    {
        if (Commands::checkCommand("text", c))
        {
            lastStatus = static_cast<Status>(status());
            view()->disableCoordinateInput();
            setStatus(SetText);
        }
    }
    break;
    }
}

/// @brief 获取可用命令列表
QStringList DimAlignedTool::availableCommands() const
{
    QStringList cmd;

    switch (status())
    {
    case SetExtPoint1:
    case SetExtPoint2:
    case SetDefPoint:
        cmd += Commands::command("text");
        break;

    default:
        break;
    }

    return cmd;
}

/// @brief 更新鼠标按钮提示
void DimAlignedTool::updateHints()
{
    switch (status())
    {
    case SetExtPoint1:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAlignedCommand::tr("Specify first extension line origin"), DimAlignedCommand::tr("Cancel"));
        break;

    case SetExtPoint2:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAlignedCommand::tr("Specify second extension line origin"), DimAlignedCommand::tr("Back"));
        break;

    case SetDefPoint:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAlignedCommand::tr("Specify dimension line location"), DimAlignedCommand::tr("Back"));
        break;

    case SetText:
        GUIDIALOGFACTORY->updateMouseWidget(
            DimAlignedCommand::tr("Enter dimension text:"), "");
        break;

    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

std::unique_ptr<BasePlaceTool> DimAlignedCommand::createTool()
{
    return std::make_unique<DimAlignedTool>(*this, document(), view());
}
