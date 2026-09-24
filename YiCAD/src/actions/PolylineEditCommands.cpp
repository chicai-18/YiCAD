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

/// @file PolylineEditCommands.cpp
/// @brief 多段线节点编辑命令：添加节点 polyline.add（原 ActionPolylineAdd）、追加节点
///        polyline.append（原 ActionPolylineAppend）、删除节点 polyline.del（原 ActionPolylineDel）
///
/// 三个命令没有选项条，只在本文件里定义；工具的事件处理从原 Action 机械改写而来。

#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmPolyline.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
constexpr int POINT_ON_ENTITY_TOLERANCE = 3; ///< 判断点是否在线上的容差值

/// @brief 添加节点命令；交互由 PolylineAddTool 驱动
class PolylineAddCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(PolylineAddCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 追加节点命令；交互由 PolylineAppendTool 驱动
class PolylineAppendCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(PolylineAppendCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 删除节点命令；交互由 PolylineDelTool 驱动
class PolylineDelCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(PolylineDelCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 添加节点工具：选多段线，指定所在的段，再指定新节点的位置；添加后命令结束
class PolylineAddTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ChooseSegment, ///< 选择现有多段线中要添加节点的段
        SetAddCoord,   ///< 设置添加节点的参考点
        SetPointPos    ///< 设置节点插入位置
    };

    PolylineAddTool(PolylineAddCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;

    void onFinish() override
    {
        if (addPoly)
        {
            addPoly->setHighlighted(false);
            view()->specifyDocumentModified();
        }
    }

private:
    /// @brief 回到某一状态并清空已选的多段线（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        addPoly = nullptr;
        addVertextIdx = -1;
        addCoord = {};
    }

    void trigger();
    void getRemovedPartInfo(int& bulgeIdx, double& bulge, double& startLineWeight, double& endLineWeight);

    PolylineAddCommand& m_command;
    DmPolyline* addPoly = nullptr; ///< 待修改的多段线
    int addVertextIdx = -1;        ///< 选择的顶点索引
    DmVector addCoord;             ///< 新插入点坐标
};

/// @brief 追加节点工具：在多段线靠近的一端选中它，再指定追加的点；追加后命令结束
class PolylineAppendTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetStartpoint, ///< 设置起始点（选择多段线）
        SetNextPoint   ///< 设置下一个追加点
    };

    PolylineAppendTool(PolylineAppendCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onCoordinate(const DmVector& coord) override;

private:
    void trigger();

    PolylineAppendCommand& m_command;
    DmPolyline* originalPolyline = nullptr; ///< 原始多段线
    PolylineData data;                      ///< 修改后的多段线数据
    bool prepend = false;                   ///< 是否在首端追加（false 为末端追加）
};

/// @brief 删除节点工具：选多段线，再指定要删除的节点；可连续删除
class PolylineDelTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ChooseEntity, ///< 选择要删除节点的多段线
        SetDelPoint   ///< 设置要删除的节点位置
    };

    PolylineDelTool(PolylineDelCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;

    void onFinish() override
    {
        if (delEntity)
        {
            delEntity->setHighlighted(false);
            view()->specifyDocumentModified();
            view()->redraw();
        }
    }

private:
    /// @brief 回到某一状态并清空已选的多段线（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
        delEntity = nullptr;
        delPoint = {};
    }

    void trigger();

    PolylineDelCommand& m_command;
    DmEntity* delEntity = nullptr; ///< 要删除节点的多段线
    DmVector delPoint;             ///< 要删除的节点位置
};

/// @brief 获取待移除子实体的凸度和线宽信息
/// @param [out] bulgeIdx 凸度索引
/// @param [out] bulge 凸度值
/// @param [out] startLineWeight 起始线宽
/// @param [out] endLineWeight 结束线宽
void PolylineAddTool::getRemovedPartInfo(int& bulgeIdx, double& bulge, double& startLineWeight, double& endLineWeight)
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
void PolylineAddTool::trigger()
{
    m_command.preview().clear();

    if (addPoly && addVertextIdx != -1 && addCoord.valid)
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
        vertexs.insert(vertexs.begin() + bulgeIdx + 1, addCoord);
        std::vector<double> bulges = dataRef.getBulges();
        bulges.insert(bulges.begin() + bulgeIdx + 1, bulge);
        std::vector<double> weights = dataRef.getLineWeights();
        weights.insert(weights.begin() + (bulgeIdx + 1) * 2, { endLineWeight, endLineWeight });

        Transaction t(PolylineAddCommand::tr("Add polyline point").toStdString(), document());
        t.start();
        document()->getEntityTable()->startModify(addPoly);
        addPoly->setData(PolylineData(vertexs, bulges, weights, dataRef.getIsClosed()));
        addPoly->update();
        t.commit();

        addCoord = {};
        GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    }

    view()->redraw();
}

/// @brief 处理鼠标移动事件，根据当前状态更新预览
/// @param [in] e 鼠标事件指针
void PolylineAddTool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
        case ChooseSegment:
            break;

        case SetAddCoord:
            snapper()->snapPoint(e);
            break;

        case SetPointPos:
        {
            DmVector mouse = snapper()->snapPoint(e);
            m_command.preview().clear();

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
                ent->setParent(m_command.preview().entities().getEntityContainer());
                ent->setDocument(document());
                m_command.preview().entities().addEntity(ent);
            }
            ents.clear();
            DmPolyline::getEntitiesByInfo(mouse, endPt, bulge, endLineWeight, endLineWeight, ents);
            for (auto ent : ents)
            {
                ent->setParent(m_command.preview().entities().getEntityContainer());
                ent->setDocument(document());
                m_command.preview().entities().addEntity(ent);
            }

            m_command.preview().draw();
            break;
        }

        default:
            break;
    }
}

/// @brief 处理鼠标释放事件，根据当前状态执行对应的选择或确认逻辑
/// @param [in] e 鼠标事件指针
void PolylineAddTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
            case ChooseSegment:
            {
                DmEntity* catchEnt = snapper()->catchEntity(e);
                if (!catchEnt)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineAddCommand::tr("No Entity found."));
                }
                else if (catchEnt->getEntityType() != DM::EntityPolyline)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineAddCommand::tr("Entity must be a polyline."));
                }
                else
                {
                    addPoly = static_cast<DmPolyline*>(catchEnt);
                    addPoly->setHighlighted(true);
                    setStatus(SetAddCoord);
                    view()->specifyDocumentModified();
                    view()->redraw();
                }
                break;
            }

            case SetAddCoord:
                addCoord = snapper()->snapPoint(e);
                if (!addPoly)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineAddCommand::tr("No Entity found."));
                }
                else if (!addCoord.valid)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineAddCommand::tr("Adding point is invalid."));
                }
                else
                {
                    if (!addPoly->isPointOnEntity(addCoord))
                    {
                        GUIDIALOGFACTORY->commandMessage(PolylineAddCommand::tr("Adding point is not on entity."));
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
                        double ds2 = v.squaredTo(addCoord);
                        if (ds2 < minDistSquare)
                        {
                            minDistSquare = ds2;
                            addVertextIdx = i;
                        }
                    }

                    if (addVertextIdx == -1)
                    {
                        GUIDIALOGFACTORY->commandMessage(PolylineAddCommand::tr("Adding point is not on entity."));
                        break;
                    }

                    setStatus(SetPointPos);
                }
                break;

            case SetPointPos:
                addCoord = snapper()->snapPoint(e);
                snapper()->deleteSnapper();
                trigger();
                command().finish();
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        snapper()->deleteSnapper();
        if (addPoly)
        {
            addPoly->setHighlighted(false);
            view()->specifyDocumentModified();
            view()->redraw();
        }

        init(status() - 1);
    }
}

/// @brief 更新鼠标按钮提示文本
void PolylineAddTool::updateHints()
{
    switch (status())
    {
        case ChooseSegment:
            GUIDIALOGFACTORY->updateMouseWidget(PolylineAddCommand::tr("Specify polyline to add nodes"), PolylineAddCommand::tr("Cancel"));
            break;

        case SetAddCoord:
            GUIDIALOGFACTORY->updateMouseWidget(PolylineAddCommand::tr("Specify adding node's point"), PolylineAddCommand::tr("Back"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void PolylineAppendTool::trigger()
{
    m_command.preview().clear();

    Transaction t(PolylineAppendCommand::tr("Append polyline point").toStdString(), document());
    t.start();
    document()->getEntityTable()->startModify(originalPolyline);
    originalPolyline->setData(data);
    originalPolyline->update();
    t.commit();

    snapper()->deleteSnapper();
    view()->moveRelativeZero(DmVector(0.0, 0.0));
    snapper()->drawSnapper();
    command().finish();
}

/// @brief 处理鼠标释放事件，选择多段线或设置追加点
/// @param [in] e 鼠标事件指针
void PolylineAppendTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (status() == SetStartpoint)
        {
            DmEntity* catchEnt = snapper()->catchEntity(e);
            if (!catchEnt)
            {
                GUIDIALOGFACTORY->commandMessage(PolylineAppendCommand::tr("No Entity found."));
                return;
            }
            else if (catchEnt->getEntityType() != DM::EntityPolyline)
            {
                GUIDIALOGFACTORY->commandMessage(PolylineAppendCommand::tr("Entity must be a polyline."));
                return;
            }
            else
            {
                DmPolyline* poly = static_cast<DmPolyline*>(catchEnt);
                if (poly->isClosed())
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineAppendCommand::tr("Can not append nodes in a closed polyline."));
                    return;
                }

                originalPolyline = poly;
                prepend = false;

                DmVector start = poly->getStartpoint();
                DmVector end = poly->getEndpoint();
                DmVector mouse = snapper()->snapPoint(e);
                prepend = start.distanceTo(mouse) < end.distanceTo(mouse);

                setStatus(SetNextPoint);
                view()->moveRelativeZero(prepend ? start : end);
            }
        }
        else if (status() == SetNextPoint)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        command().finish();
    }
}

/// @brief 处理鼠标移动事件，预览追加的节点
/// @param [in] e 鼠标事件指针
void PolylineAppendTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = snapper()->snapPoint(e);
    if (status() == SetNextPoint)
    {
        m_command.preview().clear();

        auto& dataRef = originalPolyline->getDataConstRef();
        if (!prepend)
        {
            double lastBulge = dataRef.getBulgeAt(dataRef.getBulgesCount() - 1);
            double lastWeight1 = 0.0;
            double lastWeight2 = 0.0;
            dataRef.getLineWeightsAt(dataRef.getBulgesCount() - 1, lastWeight1, lastWeight2);
            DmVector lastVertex = dataRef.getVertexAt(dataRef.getVertexCount() - 1);

            std::vector<DmEntity*> ents;
            DmPolyline::getEntitiesByInfo(lastVertex, mouse, lastBulge, lastWeight1, lastWeight2, ents);
            for (auto ent : ents)
            {
                ent->setParent(m_command.preview().entities().getEntityContainer());
                ent->setDocument(document());
                m_command.preview().entities().addEntity(ent);
            }
        }
        else
        {
            double firstBulge = dataRef.getBulgeAt(0);
            double firstWeight1 = 0.0;
            double firstWeight2 = 0.0;
            dataRef.getLineWeightsAt(0, firstWeight1, firstWeight2);
            DmVector firstVertex = dataRef.getVertexAt(0);

            std::vector<DmEntity*> ents;
            DmPolyline::getEntitiesByInfo(mouse, firstVertex, firstBulge, firstWeight1, firstWeight2, ents);
            for (auto ent : ents)
            {
                ent->setParent(m_command.preview().entities().getEntityContainer());
                ent->setDocument(document());
                m_command.preview().entities().addEntity(ent);
            }
        }

        m_command.preview().draw();
    }
}

/// @brief 处理坐标输入，在多段线一端追加节点
/// @param [in] coord 坐标
void PolylineAppendTool::onCoordinate(const DmVector& coord)
{
    DmVector mouse = coord;

    switch (status())
    {
        case SetStartpoint:
            // 手动输入时不做任何处理
            updateHints();
            break;

        case SetNextPoint:
        {
            auto data = originalPolyline->getData();
            // 后面追加
            if (!prepend)
            {
                double lastBulge = data.getBulgeAt(data.getBulgesCount() - 1);
                double lastWeight1 = 0.0;
                double lastWeight2 = 0.0;
                data.getLineWeightsAt(data.getBulgesCount() - 1, lastWeight1, lastWeight2);
                data.appendVertex(mouse);
                data.appendBulge(lastBulge);
                data.appendLineWeight(lastWeight1, lastWeight2);
            }
            // 前面追加
            else
            {
                double firstBulge = data.getBulgeAt(0);
                double firstWeight1 = 0.0;
                double firstWeight2 = 0.0;
                data.getLineWeightsAt(0, firstWeight1, firstWeight2);
                data.insertVertex(0, mouse);
                data.insertBulge(0, firstBulge);
                data.insertLineWeight(0, firstWeight1, firstWeight2);
            }

            this->data = data;
            trigger();
        }
            break;

        default:
            break;
    }
}

/// @brief 更新鼠标按钮提示文本
void PolylineAppendTool::updateHints()
{
    switch (status())
    {
        case SetStartpoint:
            GUIDIALOGFACTORY->updateMouseWidget(PolylineAppendCommand::tr("Specify the polyline somewhere near the beginning or end point"),
                                                PolylineAppendCommand::tr("Cancel"));
            break;

        case SetNextPoint:
        {
            GUIDIALOGFACTORY->updateMouseWidget(PolylineAppendCommand::tr("Specify next point"), PolylineAppendCommand::tr("Back"));
        }
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 执行删除节点操作，从多段线中移除指定顶点
void PolylineDelTool::trigger()
{
    if (delEntity && delPoint.valid && delEntity->isPointOnEntity(delPoint, POINT_ON_ENTITY_TOLERANCE))
    {
        delEntity->setHighlighted(false);

        DmPolyline* poly = static_cast<DmPolyline*>(delEntity);
        Transaction t(PolylineDelCommand::tr("Append polyline point").toStdString(), document());
        t.start();
        document()->getEntityTable()->startModify(poly);

        int vCount = poly->getVertexCount();
        if (vCount > 2)
        {
            double minDistSquare = DM_MAXDOUBLE * DM_MAXDOUBLE;
            int minIdx = -1;
            for (int i = 0; i < vCount; i++)
            {
                DmVector v = poly->getVertexAt(i);
                double ds2 = v.squaredTo(delPoint);
                if (ds2 < minDistSquare)
                {
                    minDistSquare = ds2;
                    minIdx = i;
                }
            }

            if (minIdx != -1)
            {
                poly->getDataRef().removeVertex(minIdx);
                poly->update();
            }
        }

        t.commit();

        delPoint = DmVector(false);
        GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    }
}

/// @brief 处理鼠标移动事件，根据当前状态更新捕捉点
/// @param [in] e 鼠标事件指针
void PolylineDelTool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
        case ChooseEntity:
            break;

        case SetDelPoint:
            snapper()->snapPoint(e);
            break;

        default:
            break;
    }
}

/// @brief 处理鼠标释放事件，根据当前状态执行选择或删除操作
/// @param [in] e 鼠标事件指针
void PolylineDelTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
            case ChooseEntity:
                delEntity = snapper()->catchEntity(e);
                if (delEntity == nullptr)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineDelCommand::tr("No Entity found."));
                }
                else if (delEntity->getEntityType() != DM::EntityPolyline)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineDelCommand::tr("Entity must be a polyline."));
                }
                else
                {
                    snapper()->snapPoint(e);
                    delEntity->setHighlighted(true);
                    view()->specifyDocumentModified();
                    setStatus(SetDelPoint);
                    view()->redraw();
                }
                break;

            case SetDelPoint:
            {
                DmVector pt = snapper()->snapPoint(e);
                double dist = DM_MAXDOUBLE;
                DmPolyline* poly = static_cast<DmPolyline*>(delEntity);

                delPoint = poly->getNearestPointOnEntity(pt, true, &dist);
                if (delEntity == nullptr)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineDelCommand::tr("No Entity found."));
                }
                else if (!delPoint.valid)
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineDelCommand::tr("Deleting point is invalid."));
                }
                else if (!delEntity->isPointOnEntity(delPoint, POINT_ON_ENTITY_TOLERANCE))
                {
                    GUIDIALOGFACTORY->commandMessage(PolylineDelCommand::tr("Deleting point is not on entity."));
                }
                else
                {
                    snapper()->deleteSnapper();
                    trigger();
                }
                break;
            }

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        snapper()->deleteSnapper();
        if (delEntity)
        {
            delEntity->setHighlighted(false);
            view()->specifyDocumentModified();
            view()->redraw();
        }

        init(status() - 1);
    }
}

/// @brief 更新鼠标按钮提示文本
void PolylineDelTool::updateHints()
{
    switch (status())
    {
        case ChooseEntity:
            GUIDIALOGFACTORY->updateMouseWidget(PolylineDelCommand::tr("Specify polyline to delete node"), PolylineDelCommand::tr("Cancel"));
            break;

        case SetDelPoint:
            GUIDIALOGFACTORY->updateMouseWidget(PolylineDelCommand::tr("Specify deleting node's point"), PolylineDelCommand::tr("Back"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

std::unique_ptr<BasePlaceTool> PolylineAddCommand::createTool()
{
    return std::make_unique<PolylineAddTool>(*this, document(), view());
}

std::unique_ptr<BasePlaceTool> PolylineAppendCommand::createTool()
{
    return std::make_unique<PolylineAppendTool>(*this, document(), view());
}

std::unique_ptr<BasePlaceTool> PolylineDelCommand::createTool()
{
    return std::make_unique<PolylineDelTool>(*this, document(), view());
}

const bool g_registeredAdd = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionPolylineAdd, QStringLiteral("polyline.add"), exclusiveCommandFactory<PolylineAddCommand>());

const bool g_registeredAppend = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionPolylineAppend, QStringLiteral("polyline.append"), exclusiveCommandFactory<PolylineAppendCommand>());

const bool g_registeredDel = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionPolylineDel, QStringLiteral("polyline.del"), exclusiveCommandFactory<PolylineDelCommand>());
}  // namespace
