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

/// @file DrawLineOrthTanCommand.cpp
/// @brief 正交切线命令 ext.draw.line_orth_tan，取代原 ActionDrawLineOrthTan：
///        选一条直线，再选圆、圆弧或椭圆，画与该直线正交的切线

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

/// @brief 正交切线命令；交互由 DrawLineOrthTanTool 驱动
class DrawLineOrthTanCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLineOrthTanCommand)

public:
    /// @brief 预览切线
    void previewTangent(const LineData& data)
    {
        preview().clear();
        auto l = new DmLine(preview().entities().getEntityContainer(), data);
        l->setDocument(document());
        preview().entities().addEntity(l);
        preview().draw();
    }

    /// @brief 提交切线
    void commitTangent(const LineData& data)
    {
        preview().clear();
        Transaction t(tr("Create LineOrthTan").toStdString(), document());
        t.start();
        DmEntity* newEntity = new DmLine(nullptr, data);
        newEntity->setDocument(document());
        document()->getEntityTable()->add(newEntity);
        t.commit();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

namespace
{
/// @brief 正交切线工具：选直线，再选圆、圆弧或椭圆
class DrawLineOrthTanTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetLine,  ///< 选择与切线正交的直线
        SetCircle ///< 选择圆弧/圆/椭圆
    };

    DrawLineOrthTanTool(DrawLineOrthTanCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetLine:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineOrthTanCommand::tr("Select a line"),
                                                DrawLineOrthTanCommand::tr("Cancel"));
            break;
        case SetCircle:
            GUIDIALOGFACTORY->updateMouseWidget(DrawLineOrthTanCommand::tr("Select circle, arc or ellipse"),
                                                DrawLineOrthTanCommand::tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() != SetCircle)
        {
            return;
        }
        DmVector mouse(view()->toGraphX(e->x()), view()->toGraphY(e->y()));
        DmEntity* en = snapper()->catchEntity(e, {DM::EntityArc, DM::EntityCircle, DM::EntityEllipse},
                                              DM::ResolveAll);
        if (!en)
        {
            return;
        }
        if (m_circle)
        {
            m_circle->setHighlighted(false);
        }
        m_circle = en;
        m_circle->setHighlighted(true);
        view()->specifyDocumentModified();
        view()->redraw();
        m_command.preview().clear();
        m_tangent.reset(createTangent(mouse));
        // 原 Action 在求不出切线时解引用空指针，这里跳过预览
        if (m_tangent)
        {
            m_command.previewTangent(m_tangent->getData());
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::RightButton)
        {
            clearLines();
            if (status() == SetLine)
            {
                command().finish();
            }
            else
            {
                restart(status() - 1);
            }
            return;
        }

        switch (status())
        {
        case SetLine:
        {
            DmEntity* en = snapper()->catchEntity(e, DM::EntityLine);
            if (en)
            {
                if (en->getLength() < DM_TOLERANCE)
                {
                    break;
                }
                if (m_normal)
                {
                    m_normal->setHighlighted(false);
                }
                m_normal = static_cast<DmLine*>(en);
                m_normal->setHighlighted(true);
                view()->specifyDocumentModified();
                view()->redraw();
                setStatus(SetCircle);
            }
            break;
        }

        case SetCircle:
            if (m_tangent)
            {
                if (m_circle)
                {
                    m_circle->setHighlighted(false);
                }
                m_circle = nullptr;
                // 与原 Action 一致：提交后不清除切线，不移动鼠标再次单击会再画一条
                m_command.commitTangent(m_tangent->getData());
                setStatus(SetCircle);
            }
            break;

        default:
            break;
        }
    }

    void onFinish() override
    {
        clearLines();
    }

private:
    /// @brief 取消直线与圆的高亮，清除预览
    void clearLines()
    {
        for (DmEntity* p : {static_cast<DmEntity*>(m_normal), m_circle})
        {
            if (p)
            {
                p->setHighlighted(false);
            }
        }
        view()->specifyDocumentModified();
        view()->redraw();
        m_circle = nullptr;
        m_command.preview().clear();
    }

    /// @brief 与所选直线正交、与圆相切的直线；求不出时返回空
    DmLine* createTangent(const DmVector& coord) const
    {
        if (!(m_circle && m_normal))
        {
            return nullptr;
        }
        if (!GeUtility::isEntityArc(m_circle))
        {
            return nullptr;
        }
        DmVector const& t0 = m_circle->getNearestOrthTan(coord, *m_normal, false);
        if (!t0.valid)
        {
            return nullptr;
        }
        DmVector const& vp = m_normal->getNearestPointOnEntity(t0, false);
        DmLine* ret = new DmLine(nullptr, vp, t0);
        ret->setDocument(document());
        return ret;
    }

    DrawLineOrthTanCommand& m_command;
    DmLine* m_normal = nullptr;        ///< 选中的直线
    std::unique_ptr<DmLine> m_tangent; ///< 最近的切线
    DmEntity* m_circle = nullptr;      ///< 用于生成切线的圆弧/圆/椭圆
};
}  // namespace

std::unique_ptr<BasePlaceTool> DrawLineOrthTanCommand::createTool()
{
    return std::make_unique<DrawLineOrthTanTool>(*this, document(), view());
}

ExclusiveCommandFactory DrawCommands::lineOrthTan()
{
    return exclusiveCommandFactory<DrawLineOrthTanCommand>();
}
