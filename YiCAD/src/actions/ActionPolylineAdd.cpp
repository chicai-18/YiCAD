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


/// @file ActionPolylineAdd.cpp
/// @brief 多段线添加节点操作实现

#include "ActionPolylineAdd.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmLine.h"
#include "DmDocument.h"
#include "DmPolyline.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 构造函数，初始化添加节点操作
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionPolylineAdd::ActionPolylineAdd(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Add node", doc, docView), addPoly(nullptr), addVertextIdx(-1),
    addCoord(new DmVector{})
{
    actionType = DM::ActionPolylineAdd;
}

ActionPolylineAdd::~ActionPolylineAdd() = default;

/// @brief 初始化操作状态，重置内部数据
/// @param [in] status 初始状态值，默认为0
void ActionPolylineAdd::init(int status)
{
    ActionInterface::init(status);
    addPoly = nullptr;
    addVertextIdx = -1;
    *addCoord = {};
}

/// @brief 完成操作，清除高亮状态
/// @param [in] updateTB 是否更新工具栏
void ActionPolylineAdd::finish(bool updateTB)
{
    if (addPoly)
    {
        addPoly->setHighlighted(false);
        docView->specifyDocumentModified();
    }

    PreviewActionInterface::finish(updateTB);
}

/// @brief 获取待移除子实体的凸度和线宽信息
/// @param [out] bulgeIdx 凸度索引
/// @param [out] bulge 凸度值
/// @param [out] startLineWeight 起始线宽
/// @param [out] endLineWeight 结束线宽
void ActionPolylineAdd::getRemovedPartInfo(int& bulgeIdx, double& bulge, double& startLineWeight, double& endLineWeight)
{
    auto& dataRef = addPoly->getDataConstRef();
    bulgeIdx = addVertextIdx;
    if (bulgeIdx >= dataRef.getBulgesCount())
    {
        bulgeIdx = dataRef.getBulgesCount() - 1;
    }

    bulge = dataRef.getBulgeAt(bulgeIdx);
    dataRef.getLineWeightsAt(bulgeIdx, startLineWeight, endLineWeight);
}

/// @brief 执行添加节点操作，修改多段线数据
void ActionPolylineAdd::trigger()
{
    PreviewActionInterface::trigger();

    if (addPoly && addVertextIdx != -1 && addCoord->valid)
    {
        // 获得待移除实体的凸度，线宽
        int bulgeIdx = 0;
        double bulge = 0.0;
        double startLineWeight = 0.0;
        double endLineWeight = 0.0;
        getRemovedPartInfo(bulgeIdx, bulge, startLineWeight, endLineWeight);

        // 修改多段线
        auto& dataRef = addPoly->getDataConstRef();
        std::vector<DmVector> vertexs = dataRef.getVertexs();
        vertexs.insert(vertexs.begin() + bulgeIdx + 1, *addCoord);
        std::vector<double> bulges = dataRef.getBulges();
        bulges.insert(bulges.begin() + bulgeIdx + 1, bulge);
        std::vector<double> weights = dataRef.getLineWeights();
        weights.insert(weights.begin() + (bulgeIdx + 1) * 2, { endLineWeight, endLineWeight });

        Transaction t(tr("Add polyline point").toStdString(), pDocument);
        t.start();
        pDocument->getEntityTable()->startModify(addPoly);
        addPoly->setData(PolylineData(vertexs, bulges, weights, dataRef.getIsClosed()));
        addPoly->update();
        t.commit();

        *addCoord = {};
        GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
    }

    docView->redraw();
}

/// @brief 处理鼠标移动事件，根据当前状态更新预览
/// @param [in] e 鼠标事件指针
void ActionPolylineAdd::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
        case ChooseSegment:
            break;

        case SetAddCoord:
            snapPoint(e);
            break;

        case SetPointPos:
        {
            DmVector mouse = snapPoint(e);
            deletePreview();

            // 获得待移除实体的凸度，线宽
            int bulgeIdx = 0;
            double bulge = 0.0;
            double startLineWeight = 0.0;
            double endLineWeight = 0.0;
            getRemovedPartInfo(bulgeIdx, bulge, startLineWeight, endLineWeight);

            // 获得前后2个点
            auto& dataRef = addPoly->getDataConstRef();
            DmVector startPt(true);
            DmVector endPt(true);
            if (bulgeIdx == dataRef.getBulgesCount() - 1)
            {
                if (dataRef.getIsClosed())
                {
                    startPt = dataRef.getVertexAt(dataRef.getVertexCount() - 1);
                    endPt = dataRef.getVertexAt(0);
                }
                else
                {
                    startPt = dataRef.getVertexAt(bulgeIdx);
                    endPt = dataRef.getVertexAt(bulgeIdx + 1);
                }
            }
            else
            {
                startPt = dataRef.getVertexAt(bulgeIdx);
                endPt = dataRef.getVertexAt(bulgeIdx + 1);
            }

            // 生成预览实体
            std::vector<DmEntity*> ents;
            DmPolyline::getEntitiesByInfo(startPt, mouse, bulge, startLineWeight, endLineWeight, ents);
            for (auto ent : ents)
            {
                ent->setParent(preview->getEntityContainer());
                ent->setDocument(pDocument);
                preview->addEntity(ent);
            }
            ents.clear();
            DmPolyline::getEntitiesByInfo(mouse, endPt, bulge, endLineWeight, endLineWeight, ents);
            for (auto ent : ents)
            {
                ent->setParent(preview->getEntityContainer());
                ent->setDocument(pDocument);
                preview->addEntity(ent);
            }

            drawPreview();
            break;
        }

        default:
            break;
    }
}

/// @brief 处理鼠标释放事件，根据当前状态执行对应的选择或确认逻辑
/// @param [in] e 鼠标事件指针
void ActionPolylineAdd::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
            case ChooseSegment:
            {
                DmEntity* catchEnt = catchEntity(e);
                if (!catchEnt)
                {
                    GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
                }
                else if (catchEnt->getEntityType() != DM::EntityPolyline)
                {
                    GUIDIALOGFACTORY->commandMessage(tr("Entity must be a polyline."));
                }
                else
                {
                    addPoly = static_cast<DmPolyline*>(catchEnt);
                    addPoly->setHighlighted(true);
                    setStatus(SetAddCoord);
                    docView->specifyDocumentModified();
                    docView->redraw();
                }
                break;
            }

            case SetAddCoord:
                *addCoord = snapPoint(e);
                if (!addPoly)
                {
                    GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
                }
                else if (!addCoord->valid)
                {
                    GUIDIALOGFACTORY->commandMessage(tr("Adding point is invalid."));
                }
                else
                {
                    if (!addPoly->isPointOnEntity(*addCoord))
                    {
                        GUIDIALOGFACTORY->commandMessage(tr("Adding point is not on entity."));
                        break;
                    }

                    // 计算添加顶点的索引
                    addVertextIdx = -1;
                    double minDistSquare = DM_MAXDOUBLE * DM_MAXDOUBLE;
                    auto& dataRef = addPoly->getDataConstRef();
                    int count = dataRef.getVertexCount();
                    for (int i = 0; i < count; i++)
                    {
                        DmVector v = dataRef.getVertexAt(i);
                        double ds2 = v.squaredTo(*addCoord);
                        if (ds2 < minDistSquare)
                        {
                            minDistSquare = ds2;
                            addVertextIdx = i;
                        }
                    }

                    if (addVertextIdx == -1)
                    {
                        GUIDIALOGFACTORY->commandMessage(tr("Adding point is not on entity."));
                        break;
                    }

                    setStatus(SetPointPos);
                }
                break;

            case SetPointPos:
                *addCoord = snapPoint(e);
                deleteSnapper();
                trigger();
                finish(true);
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        deleteSnapper();
        if (addPoly)
        {
            addPoly->setHighlighted(false);
            docView->specifyDocumentModified();
            docView->redraw();
        }

        init(getStatus() - 1);
    }
}

/// @brief 更新鼠标按钮提示文本
void ActionPolylineAdd::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case ChooseSegment:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify polyline to add nodes"), tr("Cancel"));
            break;

        case SetAddCoord:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify adding node's point"), tr("Back"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标样式为选择光标
void ActionPolylineAdd::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}
