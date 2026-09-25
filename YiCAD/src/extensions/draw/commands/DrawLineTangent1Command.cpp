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

/// @file DrawLineTangent1Command.cpp
/// @brief 过点作切线命令 ext.draw.line_tangent1，取代原 ActionDrawLineTangent1：
///        指定一点，再选圆、圆弧或椭圆，画从该点到切点的直线

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DrawCommands.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GeUtility.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

/// @brief 过点作切线命令；交互由 DrawLineTangent1Tool 驱动
class DrawLineTangent1Command : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLineTangent1Command)

public:
    /// @brief 预览切线
    void previewTangent(const DmLine& tangent)
    {
        preview().clear();
        auto cloneEnt = tangent.clone();
        cloneEnt->setParent(preview().entities().getEntityContainer());
        preview().entities().addEntity(cloneEnt);
        preview().draw();
    }

    /// @brief 提交切线
    void commitTangent(const LineData& data)
    {
        preview().clear();
        DmEntity* newEntity = new DmLine(nullptr, data);
        newEntity->setDocument(document());
        Transaction t(tr("Create line tangent").toStdString(), document());
        t.start();
        document()->getEntityTable()->add(newEntity);
        t.commit();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

namespace
{
/// @brief 过点作切线工具：指定点，再选圆、圆弧或椭圆
class DrawLineTangent1Tool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetPoint, ///< 选择起点
        SetCircle ///< 选择圆或弧
    };

    DrawLineTangent1Tool(DrawLineTangent1Command& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override
    {
        return status() == SetCircle ? DM::SelectCursor : DM::CadCursor;
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineTangent1Command::tr("Specify point"),
                                                DrawLineTangent1Command::tr("Cancel"));
            break;
        case SetCircle:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineTangent1Command::tr("Select circle, arc or ellipse"),
                                                DrawLineTangent1Command::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse(view()->toGraphX(e->x()), view()->toGraphY(e->y()));
        switch (status())
        {
        case SetPoint:
            m_point = snapper()->snapPoint(e);
            break;

        case SetCircle:
        {
            DmEntity* en = snapper()->catchEntity(
                e, EntityTypeList{DM::EntityArc, DM::EntityCircle, DM::EntityEllipse}, DM::ResolveAll);
            if (en)
            {
                if (m_circle)
                {
                    m_circle->setHighlighted(false);
                }
                m_circle = en;
                m_circle->setHighlighted(true);
                view()->specifyDocumentModified();
                view()->redraw();
                m_tangent.reset(createTangent(mouse));
                if (m_tangent)
                {
                    m_command.previewTangent(*m_tangent);
                }
            }
            break;
        }

        default:
            break;
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            if (m_circle)
            {
                m_circle->setHighlighted(false);
                view()->specifyDocumentModified();
                view()->redraw();
            }
            stepBack();
            return;
        }

        switch (status())
        {
        case SetPoint:
            onCoordinate(snapper()->snapPoint(e));
            break;
        case SetCircle:
            if (m_tangent)
            {
                if (m_circle)
                {
                    m_circle->setHighlighted(false);
                    view()->specifyDocumentModified();
                    view()->redraw();
                }
                m_command.commitTangent(m_tangent->getData());
                setStatus(SetPoint);
                m_tangent.reset();
            }
            break;
        default:
            break;
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        if (status() == SetPoint)
        {
            m_point = pos;
            view()->moveRelativeZero(m_point);
            setStatus(SetCircle);
        }
    }

    void onFinish() override
    {
        if (m_circle)
        {
            m_circle->setHighlighted(false);
        }
    }

private:
    /// @brief 从起点到圆上最靠近鼠标的切点的直线；无切点时返回空
    DmLine* createTangent(const DmVector& coord) const
    {
        if (!(m_circle && m_point.valid))
        {
            return nullptr;
        }
        if (!(GeUtility::isEntityArc(m_circle)))
        {
            return nullptr;
        }

        DmVectorSolutions sol = m_circle->getTangentPoint(m_point);
        if (!sol.getNumber())
        {
            return nullptr;
        }
        DmVector const vp2(sol.getClosest(coord));
        LineData d;
        if ((vp2 - m_point).squared() > DM_TOLERANCE2)
        {
            d = LineData(vp2, m_point);
        }
        else
        {
            // 起点在圆上：沿切线方向画
            d = LineData(m_point + m_circle->getTangentDirection(m_point), m_point);
        }
        DmLine* ret = new DmLine(nullptr, d);
        ret->setDocument(document());
        return ret;
    }

    DrawLineTangent1Command& m_command;
    std::unique_ptr<DmLine> m_tangent; ///< 最近的切线
    DmVector m_point;                  ///< 选定的起点坐标
    DmEntity* m_circle = nullptr;      ///< 选定的圆、圆弧或椭圆
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawLineTangent1Command::createTool()
{
    return std::make_unique<DrawLineTangent1Tool>(*this, document(), view());
}

ExclusiveCommandFactory DrawCommands::lineTangent1()
{
    return exclusiveCommandFactory<DrawLineTangent1Command>();
}
