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

/// @file DimLinearTool.cpp
/// @brief DimLinearCommand 与工具（从原 ActionDimLinear 机械改写）

#include <memory>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringList>
#include <cmath>
#include <utility>
#include "DmConstructionLine.h"
#include "DmDimLinear.h"
#include "DmLine.h"
#include "GeometryMethods.h"
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
/// @brief 线性标注工具：两条延伸线起点，再指定标注线位置（水平或竖直随鼠标）；可连续标注
class DimLinearTool : public DimensionTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetExtPoint1, ///< 设置第一条延伸线起点
        SetExtPoint2, ///< 设置第二条延伸线起点
        SetDefPoint,  ///< 设置标注线位置
        SetText,      ///< 在命令行中设置文本标签
        SetAngle      ///< 在命令行中设置角度
    };

    DimLinearTool(DimLinearCommand& command, DmDocument* doc, IDocumentView* view)
        : DimensionTool(command, doc, view)
        , m_command(command)
    {
        edata = std::make_unique<DmDimLinearData>(DmVector(false), DmVector(false));
    }

    double getAngle() const;
    void setAngle(double a);

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

    DimLinearCommand& m_command;
    std::unique_ptr<DmDimLinearData> edata; ///< 线性标注数据
    Status lastStatus = SetExtPoint1;      ///< 进入文字或角度输入前的状态
};
}  // namespace

void DimLinearTool::reset()
{
    resetDimension();

    edata.reset(new DmDimLinearData(DmVector(false), DmVector(false)));

    m_command.refreshOptions();
}

void DimLinearTool::trigger()
{
    m_command.preview().clear();

    preparePreview();

    DmDimLinear* dim = new DmDimLinear(nullptr, *data, *edata);
    dim->setDocument(document());
    dim->update();

    Transaction t(DimLinearCommand::tr("Add dimension linear").toStdString(), document());
    t.start();
    document()->getEntityTable()->add(dim);
    t.commit();

    DmVector rz = view()->getRelativeZero();
    view()->moveRelativeZero(rz);
}

void DimLinearTool::preparePreview()
{
    DmVector dirV = DmVector::polar(100., data->angle - M_PI_2);
    DmConstructionLine cl(nullptr,
                DmConstructionLineData(edata->extensionPoint2,
                            edata->extensionPoint2 + dirV));
    data->definitionPoint = cl.getNearestPointOnEntity(data->definitionPoint);
}

void DimLinearTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);

    switch (status())
    {
        case SetExtPoint1:
            break;

        case SetExtPoint2:
            if (edata->extensionPoint1.valid)
            {
                m_command.preview().clear();
                m_command.preview().entities().addEntity(new DmLine(nullptr, edata->extensionPoint1, mouse));
                m_command.preview().draw();
            }
            break;

        case SetDefPoint:
            if (edata->extensionPoint1.valid && edata->extensionPoint2.valid)
            {
                m_command.preview().clear();
                data->definitionPoint = mouse;

                // 判断鼠标位置情况，确定标注方向（有2个相互垂直的方向）
                double angle = Math2d::correctAngle(data->angle);
                double angle90 = Math2d::correctAngle(data->angle + M_PI_2);
                DmVector angleDir(angle);
                DmVector angle90Dir(angle90);
                int angleSide1 = GeometryMethods::toLeftTest(
                            edata->extensionPoint1,
                            edata->extensionPoint1 + angleDir,
                            data->definitionPoint);
                int angleSide2 = GeometryMethods::toLeftTest(
                            edata->extensionPoint2,
                            edata->extensionPoint2 + angleDir,
                            data->definitionPoint);

                // 是否在angle方向，2个拾取点之间的无限长条区域内
                bool isInAngleRegion = angleSide1 * angleSide2 <= 0;
                int angle90Side1 = GeometryMethods::toLeftTest(
                            edata->extensionPoint1,
                            edata->extensionPoint1 + angle90Dir,
                            data->definitionPoint);
                int angle90Side2 = GeometryMethods::toLeftTest(
                            edata->extensionPoint2,
                            edata->extensionPoint2 + angle90Dir,
                            data->definitionPoint);

                // 是否在angle+90度方向，2个拾取点之间的无限长条区域内
                bool isInAngle90Region = angle90Side1 * angle90Side2 <= 0;

                if (!isInAngleRegion && isInAngle90Region)
                {
                    // data->angle代表标注线的方向，与长条角度区域相差PI/2
                    data->angle = Math2d::correctAngle(angle90 - M_PI_2);
                }
                else if (!isInAngle90Region && isInAngleRegion)
                {
                    data->angle = Math2d::correctAngle(angle - M_PI_2);
                }
                else
                {
                    // 其他情况不变
                }

                preparePreview();

                DmDimLinear* dim = new DmDimLinear(
                            nullptr, *data, *edata);
                m_command.preview().entities().addEntity(dim);
                dim->update();
                m_command.preview().draw();
            }
            break;
    }
}

void DimLinearTool::onMouseRelease(QMouseEvent* e)
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

void DimLinearTool::onCoordinate(const DmVector& coord)
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

double DimLinearTool::getAngle() const
{
    return data->angle;
}

void DimLinearTool::setAngle(double a)
{
    data->angle = a;
}

void DimLinearTool::onCommand(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(
                    Commands::msgAvailableCommands()
                    + availableCommands().join(", "));
        return;
    }

    switch (status())
    {
        case SetText:
            setText(c);
            // TODO: GUI 选项请求暂时禁用，待确认后启用
            // m_command.refreshOptions();
            view()->enableCoordinateInput();
            setStatus(lastStatus);
            break;

        case SetAngle:
        {
            bool ok;
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                setAngle(Math2d::deg2rad(a));
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(
                            DimLinearCommand::tr("Not a valid expression"));
            }
            // TODO: GUI 选项请求暂时禁用，待确认后启用
            // m_command.refreshOptions();
            setStatus(lastStatus);
        }
        break;

        default:
            lastStatus = (Status)status();
            m_command.preview().clear();
            if (Commands::checkCommand("text", c))
            {
                view()->disableCoordinateInput();
                setStatus(SetText);
                return;
            }
            else if (Commands::checkCommand("angle", c))
            {
                setStatus(SetAngle);
            }
            break;
    }
}

QStringList DimLinearTool::availableCommands() const
{
    QStringList cmd;

    switch (status())
    {
        case SetExtPoint1:
        case SetExtPoint2:
        case SetDefPoint:
            cmd += Commands::command("text");
            cmd += Commands::command("angle");
            break;

        default:
            break;
    }

    return cmd;
}

void DimLinearTool::updateHints()
{
    switch (status())
    {
        case SetExtPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(DimLinearCommand::tr("Specify first extension line origin"), DimLinearCommand::tr("Cancel"));
            break;
        case SetExtPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(DimLinearCommand::tr("Specify second extension line origin"), DimLinearCommand::tr("Back"));
            break;
        case SetDefPoint:
            GUIDIALOGFACTORY->updateMouseWidget(DimLinearCommand::tr("Specify dimension line location"), DimLinearCommand::tr("Back"));
            break;
        case SetText:
            GUIDIALOGFACTORY->updateMouseWidget(DimLinearCommand::tr("Enter dimension text:"), "");
            break;
        case SetAngle:
            GUIDIALOGFACTORY->updateMouseWidget(DimLinearCommand::tr("Enter dimension line angle:"), "");
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

std::unique_ptr<BasePlaceTool> DimLinearCommand::createTool()
{
    return std::make_unique<DimLinearTool>(*this, document(), view());
}

double DimLinearCommand::angle() const
{
    auto* tool = static_cast<DimLinearTool*>(placeTool());
    return tool ? tool->getAngle() : 0.0;
}

void DimLinearCommand::setAngle(double a)
{
    if (auto* tool = static_cast<DimLinearTool*>(placeTool()))
    {
        tool->setAngle(a);
    }
}

void DimLinearCommand::refreshOptions()
{
    // 工具还在构造时（命令尚未持有它）不刷新，激活时 showOptions 会显示
    if (placeTool())
    {
        GUIDIALOGFACTORY->requestCommandOptions(this, true, true);
    }
}
