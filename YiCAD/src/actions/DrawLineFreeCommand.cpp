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

/// @file DrawLineFreeCommand.cpp
/// @brief 徒手线命令 draw.line_free，取代原 ActionDrawLineFree：按住左键拖动画多段线

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
/// @brief 多段线的参考点多于该数目才提交
constexpr int MIN_VERTICES_COUNT = 2;
/// @brief 相邻两点的最小屏幕距离平方
constexpr double MIN_SNAP_DISTANCE_SQUARED = 1.0;
}  // namespace

/// @brief 徒手线命令；交互由 DrawLineFreeTool 驱动
class DrawLineFreeCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLineFreeCommand)

public:
    /// @brief 预览里追加一段
    void appendPreviewSegment(const DmVector& from, const DmVector& to)
    {
        DmLine* line = new DmLine(preview().entities().getEntityContainer(), from, to);
        line->setDocument(document());
        preview().entities().appendEntity(line);
        preview().draw();
    }

    /// @brief 提交多段线（参考点足够多时），清除预览
    void commitPolyline(const DmPolyline& polyline)
    {
        preview().clear();
        const DmVectorSolutions sol = polyline.getRefPoints();
        if (sol.getNumber() > MIN_VERTICES_COUNT)
        {
            Transaction t(tr("Create LineFree").toStdString(), document());
            t.start();
            DmEntity* ent = polyline.clone();
            ent->update();
            ent->setDocument(document());
            document()->getEntityTable()->add(ent);
            t.commit();
        }
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

namespace
{
/// @brief 徒手线工具：按下开始，拖动中逐点追加，释放时提交
class DrawLineFreeTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartpoint, ///< 设置起点
        Dragging       ///< 拖拽中
    };

    DrawLineFreeTool(DrawLineFreeCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        // 与原 Action 一致：画的过程中预览容器不持有逐段追加的直线。原先设置后不再
        // 复原，视图共用的预览容器此后一直不持有实体；现在命令结束时复原
        DmEntityContainer* container = m_command.preview().entities().getEntityContainer();
        m_previewOwner = container->isOwner();
        container->setOwner(false);
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetStartpoint:
        case Dragging:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineFreeCommand::tr("Click and drag to draw a line"),
                                                DrawLineFreeCommand::tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector v = snapper()->snapPoint(e);
        snapper()->drawSnapper();
        if (status() == Dragging && m_polyline)
        {
            if ((view()->toGui(v) - view()->toGui(m_vertex)).squared() < MIN_SNAP_DISTANCE_SQUARED)
            {
                return;
            }
            m_polyline->appendVertex(v);
            m_command.appendPreviewSegment(m_vertex, v);
            m_vertex = v;
        }
    }

    void onMousePress(QMouseEvent* e) override
    {
        if (e->button() != Qt::LeftButton)
        {
            return;
        }
        switch (status())
        {
        case SetStartpoint:
            setStatus(Dragging);
            [[fallthrough]];
        case Dragging:
            m_vertex = snapper()->snapPoint(e);
            m_polyline = std::make_unique<DmPolyline>(nullptr, PolylineData());
            m_polyline->setDocument(document());
            m_polyline->appendVertex(m_vertex);
            break;
        default:
            break;
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            if (status() == Dragging)
            {
                m_vertex = {};
                commit();
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            m_polyline.reset();
            stepBack();
        }
    }

    void onFinish() override
    {
        m_command.preview().entities().getEntityContainer()->setOwner(m_previewOwner);
    }

private:
    /// @brief 提交已画的多段线，回到第一步（原 trigger()）
    void commit()
    {
        snapper()->deleteSnapper();
        if (m_polyline)
        {
            m_command.commitPolyline(*m_polyline);
            m_polyline.reset();
        }
        setStatus(SetStartpoint);
    }

    DrawLineFreeCommand& m_command;
    DmVector m_vertex;                      ///< 上一个点
    std::unique_ptr<DmPolyline> m_polyline; ///< 正在画的多段线
    bool m_previewOwner = true;             ///< 预览容器原先是否持有实体
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawLineFreeCommand::createTool()
{
    return std::make_unique<DrawLineFreeTool>(*this, document(), view());
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionDrawLineFree, QStringLiteral("draw.line_free"), exclusiveCommandFactory<DrawLineFreeCommand>());
}  // namespace
