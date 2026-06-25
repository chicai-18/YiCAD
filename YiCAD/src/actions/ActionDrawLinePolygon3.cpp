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


/// @file ActionDrawLinePolygon3.cpp
/// @brief 中心-切线法绘制正多边形 Action 类实现

#include "ActionDrawLinePolygon3.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "DmLine.h"
#include "Transaction.h"

namespace
{
    /// 多边形最小边数
    constexpr int MIN_POLYGON_SIDES = 3;

    /// 多边形最大边数
    constexpr int MAX_POLYGON_SIDES = 9999;
}

struct ActionDrawLinePolygonCenTan::Points
{
    DmVector center; ///< 多边形中心点
    DmVector corner; ///< 多边形角点
};

/// @brief 构造函数
/// @param [in] doc 文档对象指针
/// @param [in] docView 文档视图指针
ActionDrawLinePolygonCenTan::ActionDrawLinePolygonCenTan(DmDocument* doc,
                                                         GuiDocumentView* docView) :
    PreviewActionInterface("Draw Polygons (Center,Corner)", doc, docView),
    pPoints(new Points{}),
    number(3),
    lastStatus(SetCenter)
{
    actionType = DM::ActionDrawLinePolygonCenCor;
}

ActionDrawLinePolygonCenTan::~ActionDrawLinePolygonCenTan() = default;

/// @brief 触发动作执行，创建多边形并提交到文档
void ActionDrawLinePolygonCenTan::trigger()
{
    PreviewActionInterface::trigger();

    deletePreview();
    auto ls = createPolygon3(nullptr,
                             pPoints->center,
                             pPoints->corner,
                             number);
    if (!ls.empty())
    {
        Transaction t(tr("Create line polygon center tan").toStdString(),
                      pDocument);
        t.start();
        for (auto l : ls)
        {
            pDocument->getEntityTable()->add(l);
        }
        t.commit();
    }
}

/// @brief 鼠标移动事件处理
///
/// 根据当前状态更新预览：
/// - SetCenter: 无操作（等待用户点击）
/// - SetTangent: 实时更新多边形预览
/// @param [in] e 鼠标事件
void ActionDrawLinePolygonCenTan::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);

    switch (getStatus())
    {
        case SetCenter:
            break;

        case SetTangent:
            if (pPoints->center.valid)
            {
                pPoints->corner = mouse;
                deletePreview();
                createPolygon3(preview->getEntityContainer(),
                               pPoints->center,
                               pPoints->corner,
                               number);
                drawPreview();
            }
            break;

        default:
            break;
    }
}

/// @brief 鼠标释放事件处理
///
/// 左键点击发送坐标事件，右键点击回退到上一步。
/// @param [in] e 鼠标事件
void ActionDrawLinePolygonCenTan::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
    }
}

/// @brief 坐标事件处理
///
/// 根据当前状态处理用户输入的坐标：
/// - SetCenter: 记录中心点，进入 SetTangent 状态
/// - SetTangent: 记录切点，触发多边形创建
/// @param [in] e 坐标事件
void ActionDrawLinePolygonCenTan::coordinateEvent(GuiCoordinateEvent* e)
{
    if (!e)
    {
        return;
    }

    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetCenter:
            pPoints->center = mouse;
            setStatus(SetTangent);
            docView->moveRelativeZero(mouse);
            break;

        case SetTangent:
            pPoints->corner = mouse;
            trigger();
            finishOrthogonal();
            break;

        default:
            break;
    }
}

/// @brief 更新鼠标按钮提示信息
void ActionDrawLinePolygonCenTan::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify center"), "");
            break;

        case SetTangent:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify a tangent"), "");
            break;

        case SetNumber:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter number:"), "");
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 显示选项面板
void ActionDrawLinePolygonCenTan::showOptions()
{
    ActionInterface::showOptions();
    GUIDIALOGFACTORY->requestOptions(this, true);
}

/// @brief 隐藏选项面板
void ActionDrawLinePolygonCenTan::hideOptions()
{
    ActionInterface::hideOptions();
    GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 命令事件处理
///
/// 支持 "help" 和 "number" 命令：
/// - help: 显示可用命令列表
/// - number: 切换到边数输入模式，接受 3~9999 的整数
/// @param [in] e 命令事件
void ActionDrawLinePolygonCenTan::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(
            msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
        case SetCenter:
        case SetTangent:
            if (checkCommand("number", c))
            {
                deletePreview();
                lastStatus = static_cast<Status>(getStatus());
                setStatus(SetNumber);
            }
            break;

        case SetNumber:
        {
            bool ok = false;
            int n = c.toInt(&ok);
            if (ok)
            {
                e->accept();
                if (n > 0 && n <= MAX_POLYGON_SIDES)
                {
                    number = n;
                }
                else
                {
                    GUIDIALOGFACTORY->commandMessage(
                        tr("Not a valid number. Try 1..%1")
                            .arg(MAX_POLYGON_SIDES));
                }
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(
                    tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

        default:
            break;
    }
}

/// @brief 获取当前状态下可用的命令列表
/// @return 可用命令字符串列表
QStringList ActionDrawLinePolygonCenTan::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case SetCenter:
        case SetTangent:
            cmd += command("number");
            break;

        default:
            break;
    }

    return cmd;
}

/// @brief 更新鼠标光标样式为 CAD 十字光标
void ActionDrawLinePolygonCenTan::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

/// @brief 根据中心点、切点和边数创建正多边形线串
///
/// 计算正多边形各个顶点的坐标，并创建对应的线段实体。
/// @param [in] container 实体容器指针（可为 nullptr）
/// @param [in] center 中心点坐标
/// @param [in] tangent 切点坐标
/// @param [in] number 边数
/// @return 创建的线段实体列表
std::vector<DmLine*> ActionDrawLinePolygonCenTan::createPolygon3(
    DmEntityContainer* container,
    const DmVector& center,
    const DmVector& tangent,
    int number)
{
    // 检查给定的坐标和边数是否有效：
    if (!center.valid || !tangent.valid
        || number < MIN_POLYGON_SIDES)
    {
        return {};
    }

    std::vector<DmLine*> ret;

    // 计算正多边形第一个角点的坐标
    DmVector corner(0, 0);
    double angle = (2.0 * M_PI) / number / 2.0;
    corner.x = tangent.x + (center.y - tangent.y) * tan(angle);
    corner.y = tangent.y + (tangent.x - center.x) * tan(angle);

    const double r = center.distanceTo(corner);
    const double angle0 = center.angleTo(corner);
    const double da = (2.0 * M_PI) / number;

    for (int i = 0; i < number; ++i)
    {
        const DmVector& c0 =
            center + DmVector::polar(r, angle0 + i * da);
        const DmVector& c1 =
            center + DmVector::polar(r, angle0 + ((i + 1) % number) * da);

        DmLine* line = new DmLine(container, c0, c1);
        line->setDocument(pDocument);
        ret.emplace_back(line);

        if (container)
        {
            container->addEntity(line);
        }
    }

    return ret;
}
