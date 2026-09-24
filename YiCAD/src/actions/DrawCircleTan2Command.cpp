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

/// @file DrawCircleTan2Command.cpp
/// @brief 两切圆命令与工具的实现

#include "DrawCircleTan2Command.h"

#include <vector>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CircleData.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmAtomicEntity.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmPoint.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"

/// @brief 两切圆工具：选两个实体，再选圆心
class DrawCircleTan2Tool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetCircle1, ///< 选择第一个圆/线
        SetCircle2, ///< 选择第二个圆/线
        SetCenter   ///< 选择最近的相切圆圆心
    };

    DrawCircleTan2Tool(DrawCircleTan2Command& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        // 原 init(0)：选实体不捕捉
        snapper()->suspend();
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

    void setRadius(double r)
    {
        m_data.setRadius(r);
        if (status() == SetCenter)
        {
            m_centers = DmCircle::createTan2(m_circles, m_data.getRadius());
        }
    }

    double getRadius() const { return m_data.getRadius(); }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetCircle1:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleTan2Command::tr("Specify the first line/arc/circle"),
                                                DrawCircleTan2Command::tr("Cancel"));
            break;
        case SetCircle2:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleTan2Command::tr("Specify the second line/arc/circle"),
                                                DrawCircleTan2Command::tr("Back"));
            break;
        case SetCenter:
            GUIDIALOGFACTORY->updateMouseWidget(DrawCircleTan2Command::tr("Select the center of the tangent circle"),
                                                DrawCircleTan2Command::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() != SetCenter)
        {
            return;
        }
        m_coord = view()->toGraph(e->x(), e->y());
        if (prepareCircle())
        {
            CommandPreview& preview = m_command.preview();
            preview.clear();
            auto* circle = new DmCircle(preview.entities().getEntityContainer(), m_data);
            circle->setDocument(document());
            preview.entities().addEntity(circle);
            for (const auto& center : m_centers)
            {
                preview.entities().addEntity(new DmPoint(nullptr, PointData(center)));
            }
            preview.draw();
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            switch (status())
            {
            case SetCircle1:
            case SetCircle2:
            {
                DmEntity* en = catchCircle(e);
                if (!en)
                {
                    return;
                }
                m_circles.resize(status());
                m_circles.push_back(dynamic_cast<DmAtomicEntity*>(en));
                if (status() == SetCircle1 || computeCenters())
                {
                    m_circles.back()->setHighlighted(true);
                    view()->redraw();
                    setStatus(status() + 1);
                }
                break;
            }
            case SetCenter:
                m_coord = view()->toGraph(e->x(), e->y());
                if (prepareCircle())
                {
                    commit();
                }
                break;
            default:
                break;
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            if (status() > 0)
            {
                m_circles[status() - 1]->setHighlighted(false);
                m_circles.pop_back();
                view()->redraw();
                m_command.preview().clear();
            }
            init(status() - 1);
        }
    }

    void onFinish() override
    {
        if (!m_circles.empty())
        {
            for (auto p : m_circles)
            {
                if (p)
                {
                    p->setHighlighted(false);
                }
            }
            view()->redraw();
            m_circles.clear();
        }
    }

private:
    /// @brief 回到某一状态（原 init(status)）：status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        snapper()->suspend();
        if (s == SetCircle1)
        {
            m_circles.clear();
        }
    }

    /// @brief 提交后取消高亮，回到第一步（原 trigger()）
    void commit()
    {
        m_command.commitCircle(m_data);
        for (auto p : m_circles)
        {
            p->setHighlighted(false);
        }
        view()->redraw();
        m_circles.clear();
        setStatus(SetCircle1);
    }

    /// @brief 选完第二个实体时求公切圆的圆心（原 getCenters()）
    bool computeCenters()
    {
        if (status() != SetCircle2)
        {
            return false;
        }
        m_centers = DmCircle::createTan2(m_circles, m_data.getRadius());
        m_valid = (m_centers.size() > 0);
        return m_valid;
    }

    /// @brief 取最靠近鼠标的圆心（原 preparePreview()）
    bool prepareCircle()
    {
        if (m_valid)
        {
            m_data.setCenter(m_centers.getClosest(m_coord));
        }
        return m_valid;
    }

    /// @brief 选可见的直线、圆弧或圆，不重复选同一个
    DmEntity* catchCircle(QMouseEvent* e)
    {
        DmEntity* en = snapper()->catchEntity(e, EntityTypeList{DM::EntityLine, DM::EntityArc, DM::EntityCircle},
                                              DM::ResolveAll);
        if (!en || !en->isVisible())
        {
            return nullptr;
        }
        for (int i = 0; i < status(); i++)
        {
            if (en->getId() == m_circles[i]->getId())
            {
                return nullptr;
            }
        }
        return en;
    }

    DrawCircleTan2Command& m_command;
    CircleData m_data;                    ///< 正在画的圆（半径来自选项条）
    DmVector m_coord;                     ///< 鼠标位置
    bool m_valid = false;                 ///< 是否求出了圆心
    DmVectorSolutions m_centers;          ///< 求出的圆心
    std::vector<DmAtomicEntity*> m_circles; ///< 选中的实体
};

DrawCircleTan2Command::DrawCircleTan2Command() = default;

DrawCircleTan2Command::~DrawCircleTan2Command() = default;

std::unique_ptr<BasePlaceTool> DrawCircleTan2Command::createTool()
{
    return std::make_unique<DrawCircleTan2Tool>(*this, document(), view());
}

DrawCircleTan2Tool* DrawCircleTan2Command::tool() const
{
    return static_cast<DrawCircleTan2Tool*>(placeTool());
}

void DrawCircleTan2Command::setRadius(double r)
{
    if (tool())
    {
        tool()->setRadius(r);
    }
}

double DrawCircleTan2Command::getRadius() const
{
    return tool() ? tool()->getRadius() : 0.0;
}

void DrawCircleTan2Command::commitCircle(const CircleData& data)
{
    preview().clear();
    Transaction t(tr("Create CircleTan2").toStdString(), document());
    t.start();
    DmCircle* circle = new DmCircle(nullptr, data);
    circle->setDocument(document());
    document()->getEntityTable()->add(circle);
    t.commit();
}

void DrawCircleTan2Command::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void DrawCircleTan2Command::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawCircleTan2, QStringLiteral("draw.circle_tan2"), exclusiveCommandFactory<DrawCircleTan2Command>());
}  // namespace
