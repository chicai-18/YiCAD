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

/// @file EditPasteCommand.cpp
/// @brief 粘贴命令 ext.edit.paste，取代原 ActionEditPaste：指定参考点，把剪贴板的实体
///        （连同缺少的图层，按单位换算）放进文档，然后结束

#include <cmath>
#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "EditCommands.h"
#include "DmClipboard.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmUnits.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
/// @brief 粘贴命令；交互由 EditPasteTool 驱动
class EditPasteCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(EditPasteCommand)

public:
    /// @brief 预览剪贴板内容放在参考点处
    void previewPaste(const DmVector& targetPoint)
    {
        preview().clear();
        DmDocument* clipDoc = DMCLIPBOARD->getDocument();
        clipDoc->getEntityTable()->updateContainer();
        Preview& entities = preview().entities();
        entities.addAllFrom(*clipDoc->getEntityTable()->getEntityContainer());
        entities.move(targetPoint);
        if (document())
        {
            double const f = DmUnits::convert(1.0, clipDoc->getUnit(), document()->getUnit());
            entities.getEntityContainer()->scale(targetPoint, {f, f});
        }
        preview().draw();
    }

    /// @brief 把剪贴板内容放在参考点处，然后结束命令（剪贴板为空时直接结束）
    void commitPaste(const DmVector& targetPoint)
    {
        preview().clear();
        if (DMCLIPBOARD->count() == 0)
        {
            finish();
            return;
        }

        Transaction t(tr("Paste").toStdString(), document());
        t.start();
        auto entTable = document()->getEntityTable();
        DmDocument* clipDoc = DMCLIPBOARD->getDocument();
        double factor = DmUnits::convert(1.0, clipDoc->getUnit(), document()->getUnit());
        pasteLayers(clipDoc);
        for (auto src : *clipDoc->getEntityTable())
        {
            if (!src || src->isErased())
            {
                continue;
            }
            DmEntity* clone = src->clone();
            clone->resetId();
            clone->move(targetPoint);
            if (std::fabs(factor - 1.0) > DM_TOLERANCE)
            {
                clone->scale(targetPoint, {factor, factor});
            }
            entTable->add(clone);
        }
        t.commit();
        GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
        finish();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;

private:
    /// @brief 把文档里没有的图层从剪贴板复制过来
    void pasteLayers(DmDocument* source)
    {
        if (!source)
        {
            return;
        }
        auto srcLayerTable = source->getLayerTable();
        auto dstLayerTable = document()->getLayerTable();
        if (!srcLayerTable || !dstLayerTable)
        {
            return;
        }
        for (auto it = srcLayerTable->begin(); it != srcLayerTable->end(); ++it)
        {
            DmLayer* srcLayer = *it;
            if (srcLayer && !dstLayerTable->find(srcLayer->getName()))
            {
                dstLayerTable->add(srcLayer->clone());
            }
        }
    }
};

/// @brief 粘贴工具：只有一步
class EditPasteTool : public BasePlaceTool
{
public:
    EditPasteTool(EditPasteCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        // 原 ActionEditPaste 没有设置 Action 类型，结束时不复位正交零点
        setResetsOrthogonalOnFinish(false);
    }

protected:
    void updateHints() override
    {
        if (status() == 0)
        {
            GUIDIALOGFACTORY->updateMouseWidget(EditPasteCommand::tr("Set reference point"),
                                                EditPasteCommand::tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget();
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() == 0)
        {
            m_command.previewPaste(snapper()->snapPoint(e));
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
            stepBack();
        }
    }

    void onCoordinate(const DmVector& pos) override { m_command.commitPaste(pos); }

private:
    EditPasteCommand& m_command;
};

std::unique_ptr<BasePlaceTool> EditPasteCommand::createTool()
{
    return std::make_unique<EditPasteTool>(*this, document(), view());
}

}  // namespace

ExclusiveCommandFactory EditCommands::paste()
{
    return exclusiveCommandFactory<EditPasteCommand>();
}
