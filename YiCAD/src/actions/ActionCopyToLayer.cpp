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


/// @file ActionCopyToLayer.cpp
/// @brief 复制实体到图层动作类实现文件

#include "ActionCopyToLayer.h"

#include <QMouseEvent>

#include "ActionInterface.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLayer.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param[in] doc 文档指针
/// @param[in] docView 文档视图指针
ActionCopyToLayer::ActionCopyToLayer(
    DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface(
          "Copy Entities To Layer", doc, docView)
    , pPoints(new Points{})
{
    actionType = DM::ActionCopyToLayer;
    pPoints->entityLayer = nullptr;
}

/// @brief 析构函数
ActionCopyToLayer::~ActionCopyToLayer() = default;

/// @brief 初始化动作
/// @param[in] status 初始状态
void ActionCopyToLayer::init(int status)
{
    ActionInterface::init(status);
}

/// @brief 触发动作执行，将选中实体复制到目标图层
void ActionCopyToLayer::trigger()
{
    if (pDocument)
    {
        // 复制选择的实体
        std::vector<DmEntity*> vec;
        auto table = pDocument->getEntityTable();
        for (auto it = table->begin();
             it != table->end(); ++it)
        {
            if ((*it)->isSelected())
            {
                (*it)->setSelected(false);
                DmEntity* ent = (*it)->clone();
                ent->move(pPoints->v2 - pPoints->v1);
                ent->setLayer(
                    pPoints->entityLayer->getName());
                ent->setPen(
                    pPoints->entityLayer->getPen());
                vec.emplace_back(ent);
            }
        }

        // 添加到文档
        if (!vec.empty())
        {
            Transaction t(
                tr("Copy Entities To Layer").toStdString(),
                pDocument);
            t.start();
            for (size_t i = 0; i != vec.size(); ++i)
            {
                DmEntity* ent = vec[i];
                pDocument->getEntityTable()->add(ent);
                docView->redraw();
            }
            t.commit();
        }
    }

    finish();
}

/// @brief 鼠标移动事件处理
/// @param[in] e 鼠标事件指针
void ActionCopyToLayer::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
    case SetLayer:
        snapPoint(e);
        break;
    case SetBasePoint:
        snapPoint(e);
        break;
    case SetEndPoint:
    {
        DmVector pos = snapPoint(e);
        if (preview->isEmpty())
        {
            preparePreview();
        }
        else
        {
            DmVector offset = pos - pPoints->previewPos;
            preview->move(offset);
            pPoints->previewPos.move(offset);
        }
        drawPreview();
        break;
    }
    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
/// @param[in] e 鼠标事件指针
void ActionCopyToLayer::mouseReleaseEvent(QMouseEvent* e)
{
    // 左键
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
        case SetLayer:
        {
            DmEntity* ent = catchEntity(e);
            if (ent)
            {
                pPoints->entityLayer = ent->getLayer();
                deletePreview();
                setStatus(SetBasePoint);
            }
            break;
        }
        case SetBasePoint:
        case SetEndPoint:
        {
            GuiCoordinateEvent ce(snapPoint(e));
            coordinateEvent(&ce);
            break;
        }
        default:
            break;
        }
    }
    // 右键
    else if (e->button() == Qt::RightButton)
    {
        setStatus(getStatus() - 1);
    }
    else
    {
        // 其他按键忽略
    }
}

/// @brief 更新鼠标按钮提示
void ActionCopyToLayer::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetLayer:
        GUIDIALOGFACTORY->commandMessage(
            tr("Select the object on the target layer"));
        break;
    case SetBasePoint:
        GUIDIALOGFACTORY->commandMessage(
            tr("Set base point"));
        break;
    case SetEndPoint:
        GUIDIALOGFACTORY->commandMessage(
            tr("Set end point"));
        break;
    default:
        break;
    }
}

/// @brief 坐标事件处理
/// @param[in] e 坐标事件指针
void ActionCopyToLayer::coordinateEvent(
    GuiCoordinateEvent* e)
{
    DmVector mouse = e->getCoordinate();
    switch (getStatus())
    {
    case SetLayer:
        break;
    case SetBasePoint:
    {
        pPoints->v1 = mouse;
        setStatus(SetEndPoint);
        break;
    }
    case SetEndPoint:
    {
        deletePreview();
        deleteSnapper();
        pPoints->v2 = mouse;
        trigger();
        GUIDIALOGFACTORY->commandMessage(tr("Finish"));
        break;
    }
    default:
        break;
    }
}

/// @brief 准备预览图形
void ActionCopyToLayer::preparePreview()
{
    deletePreview();
    pPoints->previewPos = pPoints->v1;
    auto table = pDocument->getEntityTable();
    for (auto it = table->begin();
         it != table->end(); ++it)
    {
        if ((*it)->isSelected())
        {
            DmEntity* clone = (*it)->clone();
            clone->setLayer(
                pPoints->entityLayer->getName());
            clone->setPen(
                pPoints->entityLayer->getPen());
            clone->setSelected(false);
            clone->setParent(nullptr);
            preview->addEntity(clone);
        }
    }
    preview->setVisible(true);
    drawPreview();
}
