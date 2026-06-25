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


/// @file ActionModifyExtend.cpp
/// @brief 延伸实体的交互动作类实现

#include "ActionModifyExtend.h"

#include "DmArc.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "DmSpline.h"
#include "GeometryMethods.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "Math2d.h"
#include "Modification.h"
#include "Preview.h"
#include "Tools.h"
#include "Transaction.h"

#include "qevent.h"

/// @brief 构造函数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionModifyExtend::ActionModifyExtend(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Extend Entity", doc, docView)
    , m_entToTrim(nullptr)
    , m_entUnderCursor(nullptr)
    , m_trimPt({false})
    , m_bExtendToSelect(false)
    , m_side(PickEntitySide::Begin)
    , m_extendedEnt(nullptr)
{
    actionType = DM::ActionModifyExtend;

    connect(docView, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()));
}

/// @brief 析构函数
ActionModifyExtend::~ActionModifyExtend() = default;

/// @brief 初始化动作
/// @param [in] status 初始状态
void ActionModifyExtend::init(int status)
{
    PreviewActionInterface::init(status);

    m_entToTrim = nullptr;
    m_entUnderCursor = nullptr;
    m_trimPt = DmVector(false);
    m_bExtendToSelect = false;
    m_side = PickEntitySide::Begin;
    m_extendedEnt.reset(nullptr);
    m_seleltedEnts.clear();

    // 判断是否有选择的实体
    for (auto e : *pDocument->getEntityTable())
    {
        if (e->isSelected())
        {
            m_seleltedEnts.emplace_back(e);
        }
    }
    m_bExtendToSelect = m_seleltedEnts.size() > 0;
    if (!m_bExtendToSelect)
    {
        updateEntitiesInView();
    }
}

/// @brief 触发延伸操作
void ActionModifyExtend::trigger()
{
    if (m_entToTrim)
    {
        m_side = getEntitySide(m_entToTrim, m_trimPt);
        DmEntity* extendEnt = extend(m_entToTrim, m_side);
        if (extendEnt)
        {
            Transaction t(tr("extend entity").toStdString(), pDocument);
            t.start();
            pDocument->getEntityTable()->startModify(m_entToTrim);
            Modification::updateEntityData(m_entToTrim, extendEnt);
            delete extendEnt;
            extendEnt = nullptr;
            t.commit();

            GUIDIALOGFACTORY->commandMessage(tr("Extend success."));
            init();
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(tr("Entity extend failure."));
        }
    }
}

/// @brief 鼠标移动事件处理
/// @param [in] e 鼠标事件指针
void ActionModifyExtend::mouseMoveEvent(QMouseEvent* e)
{
    DmEntity* se = catchEntity(e);
    DmVector mouse = docView->toGraph(e->x(), e->y());
    switch (getStatus())
    {
    case ChooseEntity:
    {
        // 上次光标下的实体与现在不同，还原该实体为可见
        if (m_entUnderCursor && m_entUnderCursor != se)
        {
            m_entUnderCursor->setVisible(true);
        }
        // 拾取的是同一实体且延伸的边不变，不再求交
        bool needRecalculate = true;
        if (se == m_entUnderCursor && m_entUnderCursor != nullptr)
        {
            DmVector ptOnEntity = se->getNearestPointOnEntity(mouse);
            auto side = getEntitySide(se, ptOnEntity);
            if (side == m_side && m_extendedEnt != nullptr)
            {
                needRecalculate = false;
            }
        }

        deletePreview();
        m_entUnderCursor = se;
        if (!needRecalculate)
        {
            preview->getEntityContainer()->addEntity(m_extendedEnt->clone());
        }
        else
        {
            m_extendedEnt.reset(nullptr);
            if (nullptr != m_entUnderCursor)
            {
                if (isExtendableEntity(m_entUnderCursor))
                {
                    DmVector ptOnEntity = m_entUnderCursor->getNearestPointOnEntity(mouse);
                    auto side = getEntitySide(m_entUnderCursor, ptOnEntity);
                    DmEntity* extendEnt = extend(m_entUnderCursor, side);
                    if (extendEnt)
                    {
                        m_entUnderCursor->setVisible(false);
                        preview->addEntity(extendEnt);
                        m_extendedEnt.reset(extendEnt->clone());
                        m_side = side;
                    }
                }
            }
        }
        drawPreview();
    }
    break;
    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
/// @param [in] e 鼠标事件指针
void ActionModifyExtend::mouseReleaseEvent(QMouseEvent* e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
        case ChooseEntity:
        {
            m_entToTrim = catchEntity(e);
            if (m_entToTrim == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
            }
            else
            {
                m_entToTrim->setVisible(true);
                if (!isExtendableEntity(m_entToTrim))
                {
                    GUIDIALOGFACTORY->commandMessage(tr("Entity is not extendable."));
                }
                else
                {
                    DmVector ptOnEntity = m_entToTrim->getNearestPointOnEntity(mouse);
                    m_trimPt = ptOnEntity;
                    trigger();
                    deleteSnapper();
                }
            }
            break;
        }
        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        finish();
    }
}

/// @brief 更新鼠标光标样式
void ActionModifyExtend::updateMouseCursor()
{
    if (getStatus() == ChooseEntity)
    {
        docView->setMouseCursor(DM::SelectCursor);
    }
    else
    {
        docView->setMouseCursor(DM::ArrowCursor);
    }
}

/// @brief 计算点距离哪个端更近
/// @param [in] e 实体指针
/// @param [in] pt 参考点
/// @return 拾取的实体端点侧
ActionModifyExtend::PickEntitySide ActionModifyExtend::getEntitySide(DmEntity* e, const DmVector& pt)
{
    switch (e->getEntityType())
    {
    case DM::EntityLine:
    {
        DmLine* line = static_cast<DmLine*>(e);
        return getEntitySideOfLine(line, pt);
    }
    break;
    case DM::EntityArc:
    {
        DmArc* arc = static_cast<DmArc*>(e);
        return getEntitySideOfArc(arc, pt);
    }
    break;
    case DM::EntityPolyline:
    {
        DmPolyline* poly = static_cast<DmPolyline*>(e);
        return getEntitySideOfPolyline(poly, pt);
    }
    break;
    case DM::EntityEllipse:
    {
        DmEllipse* ellipse = static_cast<DmEllipse*>(e);
        return getEntitySideOfEllipse(ellipse, pt);
    }
    break;
    case DM::EntitySpline:
    {
        DmSpline* spline = static_cast<DmSpline*>(e);
        return getEntitySideOfSpline(spline, pt);
    }
    break;
    default:
        break;
    }
    return PickEntitySide::Begin;
}

/// @brief 计算点在直线上的哪一端更近
/// @param [in] line 直线实体指针
/// @param [in] pt 参考点
/// @return 拾取的实体端点侧
ActionModifyExtend::PickEntitySide ActionModifyExtend::getEntitySideOfLine(DmLine* line, const DmVector& pt)
{
    double d1 = line->getStartpoint().distanceTo(pt);
    double d2 = line->getEndpoint().distanceTo(pt);
    if (d1 <= d2)
    {
        return PickEntitySide::Begin;
    }
    else
    {
        return PickEntitySide::End;
    }
}

/// @brief 计算点在圆弧上的哪一端更近
/// @param [in] arc 圆弧实体指针
/// @param [in] pt 参考点
/// @return 拾取的实体端点侧
ActionModifyExtend::PickEntitySide ActionModifyExtend::getEntitySideOfArc(DmArc* arc, const DmVector& pt)
{
    double sAngle = arc->getStartAngle();
    double eAngle = arc->getEndAngle();
    double angle = arc->getCenter().angleTo(pt);
    if (arc->isClockwise())
    {
        angle = Math2d::correctAngle(M_PI - angle);
    }
    double del1 = Math2d::correctAngle(angle - sAngle);
    double del2 = Math2d::correctAngle(eAngle - angle);
    if (del1 < del2)
    {
        return PickEntitySide::Begin;
    }
    else
    {
        return PickEntitySide::End;
    }
}

/// @brief 计算点在多段线上的哪一端更近
/// @param [in] poly 多段线实体指针
/// @param [in] pt 参考点
/// @return 拾取的实体端点侧
ActionModifyExtend::PickEntitySide ActionModifyExtend::getEntitySideOfPolyline(DmPolyline* poly, const DmVector& pt)
{
    double minDist = DM_MAXDOUBLE;
    int minDistIdx = -1;
    int segCount = poly->getSegmentCount();
    double bulge = 0.0, radius = 0.0, startAng = 0.0, endAng = 0.0;
    DmVector pt1(false), pt2(false), center(false), normal(false);
    // 多段
    if (segCount > 1)
    {
        for (int i = 0; i < segCount; i++)
        {
            poly->getSegmentInfoAt(i, bulge, pt1, pt2);
            double dist = 0.0;
            if (bulge == 0.0)
            {
                DmLine line(pt1, pt2);
                dist = line.getDistanceToPoint(pt);
            }
            else
            {
                GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAng, endAng, normal);
                DmArc arc(nullptr, ArcData(center, normal, radius, startAng, endAng));
                dist = arc.getDistanceToPoint(pt);
            }
            if (dist < minDist)
            {
                minDist = dist;
                minDistIdx = i;
            }
        }
        if (minDistIdx < segCount / 2)
        {
            return PickEntitySide::Begin;
        }
        else
        {
            return PickEntitySide::End;
        }
    }
    // 1个段
    else
    {
        poly->getSegmentInfoAt(0, bulge, pt1, pt2);
        if (bulge == 0.0)
        {
            DmLine line(pt1, pt2);
            return getEntitySideOfLine(&line, pt);
        }
        else
        {
            GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAng, endAng, normal);
            DmArc arc(nullptr, ArcData(center, normal, radius, startAng, endAng));
            return getEntitySideOfArc(&arc, pt);
        }
    }
}

/// @brief 计算点在椭圆弧上的哪一端更近
/// @param [in] ellipse 椭圆弧实体指针
/// @param [in] pt 参考点
/// @return 拾取的实体端点侧
ActionModifyExtend::PickEntitySide ActionModifyExtend::getEntitySideOfEllipse(DmEllipse* ellipse, const DmVector& pt)
{
    // 跟圆弧一样处理
    double sAngle = ellipse->getStartAngle();
    double eAngle = ellipse->getEndAngle();
    double angle = ellipse->getCenter().angleTo(pt);
    if (ellipse->isClockwise())
    {
        angle = Math2d::correctAngle(M_PI - angle);
    }
    double del1 = Math2d::correctAngle(angle - sAngle);
    double del2 = Math2d::correctAngle(eAngle - angle);
    if (del1 < del2)
    {
        return PickEntitySide::Begin;
    }
    else
    {
        return PickEntitySide::End;
    }
}

/// @brief 计算点在样条曲线上的哪一端更近
/// @param [in] e 样条曲线实体指针
/// @param [in] pt 参考点
/// @return 拾取的实体端点侧
ActionModifyExtend::PickEntitySide ActionModifyExtend::getEntitySideOfSpline(DmSpline* e, const DmVector& pt)
{
    double d1 = e->getStartpoint().distanceTo(pt);
    double d2 = e->getEndpoint().distanceTo(pt);
    if (d1 <= d2)
    {
        return PickEntitySide::Begin;
    }
    else
    {
        return PickEntitySide::End;
    }
}

/// @brief 执行实体延伸
/// @param [in] e 待延伸实体指针
/// @param [in] side 延伸端侧
/// @return 延伸后的新实体指针，失败返回nullptr
DmEntity* ActionModifyExtend::extend(DmEntity* e, ActionModifyExtend::PickEntitySide side)
{
    switch (e->getEntityType())
    {
    case DM::EntityLine:
    {
        DmLine* line = static_cast<DmLine*>(e);
        return extendForLine(line, side);
    }
    break;
    case DM::EntityArc:
    {
        DmArc* arc = static_cast<DmArc*>(e);
        return extendForArc(arc, side);
    }
    break;
    case DM::EntityPolyline:
    {
        DmPolyline* poly = static_cast<DmPolyline*>(e);
        return extendForPolyline(poly, side);
    }
    break;
    case DM::EntityEllipse:
    {
        DmEllipse* ellipse = static_cast<DmEllipse*>(e);
        return extendForEllipse(ellipse, side);
    }
    break;
    case DM::EntitySpline:
    {
        DmSpline* spline = static_cast<DmSpline*>(e);
        return extendForSpline(spline, side);
    }
    break;
    default:
        break;
    }

    return nullptr;
}

/// @brief 延伸直线
/// @param [in] line 直线实体指针
/// @param [in] side 延伸端侧
/// @return 延伸后的新直线指针，失败返回nullptr
DmLine* ActionModifyExtend::extendForLine(DmLine* line, ActionModifyExtend::PickEntitySide side)
{
    // 计算直线求交时需要延伸的长度
    double extLength = 0.0;
    DmVector min(false), max(false);
    if (m_bExtendToSelect)
    {
        for (auto e : m_seleltedEnts)
        {
            if (min.valid == false)
            {
                min = e->getMin();
                max = e->getMax();
            }
            else
            {
                min = DmVector::minimum(e->getMin(), min);
                max = DmVector::maximum(e->getMax(), max);
            }
        }
        min = DmVector::minimum(line->getMin(), min);
        max = DmVector::maximum(line->getMax(), max);
        extLength = 2.0 * (std::max(max.x - min.x, max.y - min.y));
    }
    else
    {
        DmRect rect = docView->getViewRect();
        min = rect.minP();
        max = rect.maxP();
        extLength = 2.0 * (std::max(max.x - min.x, max.y - min.y));
    }

    // 求交
    DmVector extendPartStart{false}, extendPartEnd{false}, extendPartDir{false}; // 直线延伸部分的起终点
    if (side == PickEntitySide::Begin)
    {
        extendPartDir = (line->getStartpoint() - line->getEndpoint()).normalize();
        extendPartStart = line->getStartpoint();
    }
    else
    {
        extendPartDir = (line->getEndpoint() - line->getStartpoint()).normalize();
        extendPartStart = line->getEndpoint();
    }
    DmLine extendLine(extendPartStart, extendPartStart + extendPartDir * extLength);
    double minDistSquare = DM_MAXDOUBLE;
    std::vector<DmEntity*>* entsToIntersect = nullptr;
    if (m_bExtendToSelect)
    {
        entsToIntersect = &m_seleltedEnts;
    }
    else
    {
        entsToIntersect = &m_entsInView;
    }
    for (auto e : *entsToIntersect)
    {
        auto sol = Information::getIntersection(&extendLine, e, true);
        for (auto pt : sol)
        {
            double distSquare = pt.squaredTo(extendPartStart);
            if (distSquare < DM_TOLERANCE) // 延伸终点在端点上
            {
                return nullptr;
            }
            if (distSquare < minDistSquare)
            {
                minDistSquare = distSquare;
                extendPartEnd = pt;
            }
        }
    }

    // 没有相交实体
    if (!extendPartEnd.valid)
    {
        return nullptr;
    }
    if (side == PickEntitySide::Begin)
    {
        return new DmLine(line->getParent(), extendPartEnd, line->getEndpoint());
    }
    else
    {
        return new DmLine(line->getParent(), line->getStartpoint(), extendPartEnd);
    }
}

/// @brief 延伸圆弧
/// @param [in] arc 圆弧实体指针
/// @param [in] side 延伸端侧
/// @return 延伸后的新圆弧指针，失败返回nullptr
DmArc* ActionModifyExtend::extendForArc(DmArc* arc, ActionModifyExtend::PickEntitySide side)
{
    // 获得一些参数
    DmVector center = arc->getCenter();
    DmCircle circle(nullptr, CircleData(center, arc->getRadius()));
    std::vector<DmEntity*>* entsToIntersect = nullptr;
    if (m_bExtendToSelect)
    {
        entsToIntersect = &m_seleltedEnts;
    }
    else
    {
        entsToIntersect = &m_entsInView;
    }
    double startAngleNormal = arc->getStartAngleNormal();
    double endAngleNormal = arc->getEndAngleNormal();
    double sideAngle = 0.0;                // 延伸所要接近的角度
    bool isSideStart = true;               // 延伸起始角还是终止角
    double minAngleToSide = DM_MAXDOUBLE;
    DmVector extendPartEndPt(false);
    if (arc->isClockwise())
    {
        if (side == PickEntitySide::Begin)
        {
            sideAngle = endAngleNormal;
            isSideStart = false;
        }
        else
        {
            sideAngle = startAngleNormal;
            isSideStart = true;
        }
    }
    else
    {
        if (side == PickEntitySide::Begin)
        {
            sideAngle = startAngleNormal;
            isSideStart = true;
        }
        else
        {
            sideAngle = endAngleNormal;
            isSideStart = false;
        }
    }

    // 求交，获得最近的延伸点
    for (auto e : *entsToIntersect)
    {
        auto sol = Information::getIntersection(&circle, e, true);
        for (auto pt : sol)
        {
            double angle = center.angleTo(pt);
            // 不考虑圆弧内或端点处的交点
            if (Math2d::isAngleBetween(angle, startAngleNormal, endAngleNormal))
            {
                continue;
            }
            double delta = std::fabs(angle - sideAngle);
            if (delta < minAngleToSide)
            {
                minAngleToSide = delta;
                extendPartEndPt = pt;
            }
        }
    }
    if (!extendPartEndPt.valid)
    {
        return nullptr;
    }

    // 构建新圆弧
    double newAngle = center.angleTo(extendPartEndPt);
    DmArc* newArc = nullptr;
    if (isSideStart)
    {
        newArc = new DmArc(arc->getParent(), ArcData(center, DmVector(0.0, 0.0, 1.0), arc->getRadius(), newAngle, endAngleNormal));
    }
    else
    {
        newArc = new DmArc(arc->getParent(), ArcData(center, DmVector(0.0, 0.0, 1.0), arc->getRadius(), startAngleNormal, newAngle));
    }
    if (arc->isClockwise())
    {
        newArc->setClockwise(true);
    }
    return newArc;
}

/// @brief 延伸多段线
/// @param [in] poly 多段线实体指针
/// @param [in] side 延伸端侧
/// @return 延伸后的新多段线指针，失败返回nullptr
DmPolyline* ActionModifyExtend::extendForPolyline(DmPolyline* poly, ActionModifyExtend::PickEntitySide side)
{
    int segCount = poly->getSegmentCount();
    double bulge = 0.0, radius = 0.0, startAng = 0.0, endAng = 0.0;
    DmVector pt1(false), pt2(false), center(false), normal(false);
    int modifiedSegIndex = 0;
    // 多段
    if (segCount > 1)
    {
        if (side == PickEntitySide::Begin)
        {
            poly->getSegmentInfoAt(0, bulge, pt1, pt2);
            modifiedSegIndex = 0;
        }
        else
        {
            poly->getSegmentInfoAt(segCount - 1, bulge, pt1, pt2);
            modifiedSegIndex = segCount - 1;
        }
    }
    // 1个段
    else
    {
        poly->getSegmentInfoAt(0, bulge, pt1, pt2);
        modifiedSegIndex = 0;
    }

    // 根据不同情况设置data
    auto data = poly->getData();
    // 延伸直线
    if (bulge == 0.0)
    {
        DmLine line(pt1, pt2);
        std::unique_ptr<DmLine> newLine(extendForLine(&line, side));
        if (newLine != nullptr)
        {
            if (modifiedSegIndex == 0)
            {
                if (side == PickEntitySide::Begin)
                {
                    data.setVertexAt(0, newLine->getStartpoint());
                }
                else
                {
                    data.setVertexAt(1, newLine->getEndpoint());
                }
            }
            else
            {
                data.setVertexAt(data.getVertexCount() - 1, newLine->getEndpoint());
            }
        }
        else
        {
            return nullptr;
        }
    }
    // 延伸圆弧
    else
    {
        GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAng, endAng, normal);
        DmArc arc(nullptr, ArcData(center, normal, radius, startAng, endAng));
        std::unique_ptr<DmArc> newArc(extendForArc(&arc, side));
        if (newArc != nullptr)
        {
            if (modifiedSegIndex == 0)
            {
                if (side == PickEntitySide::Begin)
                {
                    data.setVertexAt(0, newArc->getStartpoint());
                }
                else
                {
                    data.setVertexAt(1, newArc->getEndpoint());
                }
                auto bulges = data.getBulges();
                bulges.at(0) = newArc->getBulge();
                data.setBulges(bulges);
            }
            else
            {
                data.setVertexAt(data.getVertexCount() - 1, newArc->getEndpoint());
                auto bulges = data.getBulges();
                bulges.at(bulges.size() - 1) = newArc->getBulge();
                data.setBulges(bulges);
            }
        }
        else
        {
            return nullptr;
        }
    }

    // 构造新多段线
    DmPolyline* newPloy = new DmPolyline(poly->getParent(), data);
    newPloy->update();
    return newPloy;
}

/// @brief 延伸椭圆弧
/// @param [in] ellipse 椭圆弧实体指针
/// @param [in] side 延伸端侧
/// @return 延伸后的新椭圆弧指针，失败返回nullptr
DmEllipse* ActionModifyExtend::extendForEllipse(DmEllipse* ellipse, ActionModifyExtend::PickEntitySide side)
{
    // 与圆弧类似处理
    // 获得一些参数
    DmVector center = ellipse->getCenter();
    DmEllipse fullEllipse(nullptr, EllipseData(center, ellipse->getMajorP(), ellipse->getNormal(), ellipse->getRatio(), true, 0.0, M_PI * 2.0));
    std::vector<DmEntity*>* entsToIntersect = nullptr;
    if (m_bExtendToSelect)
    {
        entsToIntersect = &m_seleltedEnts;
    }
    else
    {
        entsToIntersect = &m_entsInView;
    }
    double startParamNormal = ellipse->getStartParamNormal();
    double endParamNormal = ellipse->getEndParamNormal();
    double sideParam = 0.0;                // 延伸所要接近的参数
    bool isSideStart = true;               // 延伸起始参数还是终止参数
    double minParamToSide = DM_MAXDOUBLE;
    DmVector extendPartEndPt(false);
    if (ellipse->isClockwise())
    {
        if (side == PickEntitySide::Begin)
        {
            sideParam = endParamNormal;
            isSideStart = false;
        }
        else
        {
            sideParam = startParamNormal;
            isSideStart = true;
        }
    }
    else
    {
        if (side == PickEntitySide::Begin)
        {
            sideParam = startParamNormal;
            isSideStart = true;
        }
        else
        {
            sideParam = endParamNormal;
            isSideStart = false;
        }
    }

    // 求交，获得最近的延伸点
    for (auto e : *entsToIntersect)
    {
        auto sol = Information::getIntersection(&fullEllipse, e, true);
        for (auto pt : sol)
        {
            double angle = center.angleTo(pt);
            double param = fullEllipse.getParam(angle);
            // 不考虑圆弧内或端点处的交点
            if (Math2d::isAngleBetween(param, startParamNormal, endParamNormal))
            {
                continue;
            }
            double delta = std::fabs(param - sideParam);
            if (delta < minParamToSide)
            {
                minParamToSide = delta;
                extendPartEndPt = pt;
            }
        }
    }
    if (!extendPartEndPt.valid)
    {
        return nullptr;
    }

    // 构建新椭圆弧
    double newAngle = center.angleTo(extendPartEndPt);
    double newParam = fullEllipse.getParam(newAngle);
    DmEllipse* newEllipse = nullptr;
    if (isSideStart)
    {
        newEllipse = new DmEllipse(ellipse->getParent(), EllipseData(center, ellipse->getMajorP(), DmVector(0.0, 0.0, 1.0), ellipse->getRatio(), false, newParam, endParamNormal));
    }
    else
    {
        newEllipse = new DmEllipse(ellipse->getParent(), EllipseData(center, ellipse->getMajorP(), DmVector(0.0, 0.0, 1.0), ellipse->getRatio(), false, startParamNormal, newParam));
    }
    if (ellipse->isClockwise())
    {
        newEllipse->setClockwise(true);
    }
    return newEllipse;
}

/// @brief 延伸样条曲线
/// @param [in] spline 样条曲线实体指针
/// @param [in] side 延伸端侧
/// @return 延伸后的新样条曲线指针，失败返回nullptr
DmSpline* ActionModifyExtend::extendForSpline(DmSpline* spline, ActionModifyExtend::PickEntitySide side)
{
    // 获得端点及端点方向
    DmVector beginPt(false);
    DmVector dir(false);
    double t1 = 0.0, t2 = 0.0;
    spline->getDomainOfDefinition(t1, t2);
    if (side == ActionModifyExtend::PickEntitySide::Begin)
    {
        beginPt = spline->getStartpoint();
        dir = -spline->derivative(t1).normalize(); // 切向的反向
    }
    else
    {
        beginPt = spline->getEndpoint();
        dir = spline->derivative(t2).normalize();
    }

    // 构造直线并与其他实体求交（与extendForLine相似）
    double extLength = 0.0;
    DmVector min(false), max(false);
    if (m_bExtendToSelect)
    {
        for (auto e : m_seleltedEnts)
        {
            if (min.valid == false)
            {
                min = e->getMin();
                max = e->getMax();
            }
            else
            {
                min = DmVector::minimum(e->getMin(), min);
                max = DmVector::maximum(e->getMax(), max);
            }
        }
        min = DmVector::minimum(beginPt, min);
        max = DmVector::maximum(beginPt, max);
        extLength = 2.0 * (std::max(max.x - min.x, max.y - min.y));
    }
    else
    {
        DmRect rect = docView->getViewRect();
        min = rect.minP();
        max = rect.maxP();
        extLength = 2.0 * (std::max(max.x - min.x, max.y - min.y));
    }

    // 求交
    DmVector extendPartStart = beginPt, extendPartEnd{false}, extendPartDir = dir; // 与extendForLine不同处
    DmLine extendLine(extendPartStart, extendPartStart + extendPartDir * extLength);
    double minDistSquare = DM_MAXDOUBLE;
    std::vector<DmEntity*>* entsToIntersect = nullptr;
    if (m_bExtendToSelect)
    {
        entsToIntersect = &m_seleltedEnts;
    }
    else
    {
        entsToIntersect = &m_entsInView;
    }
    for (auto e : *entsToIntersect)
    {
        auto sol = Information::getIntersection(&extendLine, e, true);
        for (auto pt : sol)
        {
            double distSquare = pt.squaredTo(extendPartStart);
            if (distSquare < DM_TOLERANCE) // 延伸终点在端点上
            {
                return nullptr;
            }
            if (distSquare < minDistSquare)
            {
                minDistSquare = distSquare;
                extendPartEnd = pt;
            }
        }
    }
    // 没有相交实体
    if (!extendPartEnd.valid)
    {
        return nullptr;
    }

    // 由求出的最近交点与起点构造直线，与原样条线合并
    DmSpline* addSpline = DmSpline::createSplineWithTwoPoints(beginPt, extendPartEnd, spline->getDegree());
    std::shared_ptr<DmSpline> addSpline_sptr(addSpline);
    DmSpline* combinedSpline = nullptr;
    DmSpline::combineTwoSplines(spline, addSpline, combinedSpline);

    return combinedSpline;
}

/// @brief 判断实体是否可被延伸
/// @param [in] ent 实体指针
/// @return 可延伸返回true，否则返回false
bool ActionModifyExtend::isExtendableEntity(DmEntity* ent)
{
    if (ent->isLocked() || !ent->isVisible())
    {
        return false;
    }
    switch (ent->getEntityType())
    {
    case DM::EntityArc:
    case DM::EntityLine:
        return true;
    case DM::EntityPolyline:
    {
        DmPolyline* poly = static_cast<DmPolyline*>(ent);
        if (poly->isClosed())
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    case DM::EntityEllipse:
    {
        DmEllipse* ellipse = static_cast<DmEllipse*>(ent);
        if (ellipse->isClosed())
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    case DM::EntitySpline:
    {
        DmSpline* spline = static_cast<DmSpline*>(ent);
        if (spline->isClosed())
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    default:
        return false;
    }
    return false;
}

/// @brief 更新当前视图内的实体列表
void ActionModifyExtend::updateEntitiesInView()
{
    m_entsInView.clear();
    DmRect rect = docView->getViewRect();
    pDocument->searchEntities(rect.minP(), rect.maxP(), m_entsInView);
}

/// @brief 视图变化槽函数
void ActionModifyExtend::slotViewChanged()
{
    if (!m_bExtendToSelect)
    {
        updateEntitiesInView();
    }
}
