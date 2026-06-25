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


/// @file ActionEditPaste.cpp
/// @brief 粘贴 Action 类的实现

#include "ActionEditPaste.h"

#include <QAction>
#include <QMouseEvent>
#include <cmath>

#include "DmClipboard.h"
#include "DmLayer.h"
#include "DmUnits.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionEditPaste::ActionEditPaste(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Edit Paste", doc, docView), targetPoint(new DmVector{})
{
}

ActionEditPaste::~ActionEditPaste() = default;

/// @brief 初始化 Action 状态
/// @param status 初始状态
void ActionEditPaste::init(int status)
{
    PreviewActionInterface::init(status);
}

/// @brief 触发粘贴操作
void ActionEditPaste::trigger()
{
    deletePreview();

    if (DMCLIPBOARD->count() == 0)
    {
        finish(false);
        return;
    }

    Transaction t(tr("Paste").toStdString(), pDocument);
    t.start();

    auto entTable = pDocument->getEntityTable();
    auto clipDoc = DMCLIPBOARD->getDocument();

    // 单位换算
    DM::Unit sourceUnit = clipDoc->getUnit();
    DM::Unit targetUnit = pDocument->getUnit();
    double factor = DmUnits::convert(1.0, sourceUnit, targetUnit);

    // 复制图层
    pasteLayers(clipDoc);

    // 遍历剪贴板实体，克隆并添加到文档
    auto table = DMCLIPBOARD->getDocument()->getEntityTable();
    for (auto src : *table)
    {
        if (!src || src->isErased())
        {
            continue;
        }

        DmEntity* clone = src->clone();
        clone->resetId();

        clone->move(*targetPoint);
        if (fabs(factor - 1.0) > DM_TOLERANCE)
        {
            clone->scale(*targetPoint, {factor, factor});
        }
        entTable->add(clone);
    }

    t.commit();

    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
    finish(false);
}

/// @brief 将剪贴板中的图层复制到当前文档（如果不存在）
/// @param source 剪贴板文档
void ActionEditPaste::pasteLayers(DmDocument* source)
{
    if (!source)
    {
        return;
    }

    auto srcLayerTable = source->getLayerTable();
    auto dstLayerTable = pDocument->getLayerTable();
    if (!srcLayerTable || !dstLayerTable)
    {
        return;
    }

    for (auto it = srcLayerTable->begin(); it != srcLayerTable->end(); ++it)
    {
        DmLayer* srcLayer = *it;
        if (!srcLayer)
        {
            continue;
        }

        if (!dstLayerTable->find(srcLayer->getName()))
        {
            DmLayer* newLayer = srcLayer->clone();
            dstLayerTable->add(newLayer);
        }
    }
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件
void ActionEditPaste::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
    case SetTargetPoint:
        *targetPoint = snapPoint(e);

        deletePreview();
        DMCLIPBOARD->getDocument()->getEntityTable()->updateContainer();
        preview->addAllFrom(*DMCLIPBOARD->getDocument()->getEntityTable()->getEntityContainer());
        preview->move(*targetPoint);

        if (pDocument)
        {
            DM::Unit sourceUnit = DMCLIPBOARD->getDocument()->getUnit();
            DM::Unit targetUnit = pDocument->getUnit();
            double const f = DmUnits::convert(1.0, sourceUnit, targetUnit);
            preview->getEntityContainer()->scale(*targetPoint, { f, f });
        }
        drawPreview();
        break;

    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void ActionEditPaste::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        init(getStatus() - 1);
    }
}

/// @brief 坐标事件处理
/// @param e 坐标事件
void ActionEditPaste::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }

    *targetPoint = e->getCoordinate();
    trigger();
}

/// @brief 更新鼠标按钮提示
void ActionEditPaste::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetTargetPoint:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Set reference point"), tr("Cancel"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @brief 更新鼠标光标
void ActionEditPaste::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
