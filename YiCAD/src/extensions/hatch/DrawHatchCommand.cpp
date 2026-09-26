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

/// @file DrawHatchCommand.cpp
/// @brief DrawHatchCommand 与填充工具的实现

#include "DrawHatchCommand.h"

#include <vector>

#include <QMessageBox>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmHatch.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "HatchData.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Preview.h"
#include "Transaction.h"
#include "UIDialogRunner.h"
#include "UIDlgHatch.h"

namespace
{
/// @brief 填充工具：只有一步，单击生成填充，右键结束
class DrawHatchTool : public BasePlaceTool
{
public:
    DrawHatchTool(DrawHatchCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        GUIDIALOGFACTORY->updateMouseWidget(DrawHatchCommand::tr("Specify point to create hatch."),
                                            DrawHatchCommand::tr("Cancel"));
    }

    void onMouseMove(QMouseEvent* e) override { m_command.previewAt(view()->toGraph(e->pos().x(), e->pos().y())); }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            // 原 Action 先捕捉一次（刷新捕捉标记），区域按鼠标所在的点找
            snapper()->snapPoint(e);
            m_command.commitAt(view()->toGraph(e->pos().x(), e->pos().y()));
        }
        else if (e->button() == Qt::RightButton)
        {
            command().finish();
        }
    }

private:
    DrawHatchCommand& m_command;
};
}  // namespace

DrawHatchCommand::DrawHatchCommand()
    : m_data(std::make_unique<HatchData>())
{
}

DrawHatchCommand::~DrawHatchCommand()
{
    QObject::disconnect(m_viewChanged);
}

std::unique_ptr<BasePlaceTool> DrawHatchCommand::createTool()
{
    bool hasSelection = false;
    for (DmEntity* entity : *document()->getEntityTable())
    {
        if (entity->isSelected())
        {
            hasSelection = true;
            break;
        }
    }

    DmHatch tmp(nullptr, *m_data);
    tmp.setDocument(document());
    {
        UIDlgHatch dlg(UIDialogRunner::parentOf(view()));
        dlg.setHatch(tmp, true);
        if (UIDialogRunner::exec(dlg) != QDialog::Accepted)
        {
            return nullptr;
        }
        dlg.updateHatch();
    }
    *m_data = tmp.getData();

    if (hasSelection)
    {
        for (DmEntity* entity : *document()->getEntityTable())
        {
            if (entity->isSelected())
            {
                m_findMethod.addEntity(entity);
            }
        }
    }
    else
    {
        addEntitiesInView();
        // 视图变化后补充视图内的实体（测试用的假视图没有 QObject，跳过）
        if (auto* guiView = qobject_cast<GuiDocumentView*>(view()->asQObject()))
        {
            m_viewChanged = QObject::connect(guiView, &GuiDocumentView::viewChanged, [this]() { addEntitiesInView(); });
        }
    }
    return std::make_unique<DrawHatchTool>(*this, document(), view());
}

void DrawHatchCommand::addEntitiesInView()
{
    const DmRect rect = view()->getViewRect();
    std::vector<DmEntity*> ents;
    document()->searchEntities(rect.minP(), rect.maxP(), ents, true, false);
    for (DmEntity* entity : ents)
    {
        m_findMethod.addEntity(entity);
    }
}

void DrawHatchCommand::previewAt(const DmVector& pos)
{
    preview().clear();
    m_findMethod.calculate();
    if (auto region = m_findMethod.findRegionContainPoint(pos))
    {
        HatchData d(*m_data);
        d.setBoundary(region->getDmRegion());
        auto* hatch = new DmHatch(preview().entities().getEntityContainer(), d);
        hatch->setDocument(document());
        hatch->update();
        preview().entities().addEntity(hatch);
    }
    preview().draw();
}

void DrawHatchCommand::commitAt(const DmVector& pos)
{
    if (!pos.valid)
    {
        return;
    }
    m_findMethod.calculate();
    auto region = m_findMethod.findRegionContainPoint(pos);
    if (!region)
    {
        QMessageBox::information(nullptr, tr("Tips"), tr("Failure to create hatch, can't find the region!"));
        return;
    }
    Transaction t(tr("Hatch").toStdString(), document());
    t.start();
    HatchData d(*m_data);
    d.setBoundary(region->getDmRegion());
    auto* hatch = new DmHatch(nullptr, d);
    hatch->setDocument(document());
    hatch->update();
    document()->getEntityTable()->add(hatch);
    t.commit();
}
