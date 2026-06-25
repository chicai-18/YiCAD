/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file Snapper.cpp
/// @brief 画布鼠标捕捉功能的实现，包括多种捕捉模式和实体捕获

#include <cmath>
#include <QMouseEvent>
#include "Snapper.h"

#include "DmPoint.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "GuiGrid.h"
#include "DmSettings.h"
#include "DmOverlayEntity.h"
#include "GuiCoordinateEvent.h"
#include "DmDocument.h"
#include "DmPen.h"
#include "Debug.h"
#include "ActionInterface.h"
#include "Information.h"

// 禁用所有捕捉模式
SnapMode const& SnapMode::clear()
{
    snapIntersection = false;
    snapOnEntity = false;
    snapCenter = false;
    snapMiddle = false;
    snapEndpoint = false;
    snapGrid = false;
    snapFree = false;
    snapAngle = false;
    snapSubsection = false;

    restriction = DM::RestrictNothing;

    return *this;
}

bool SnapMode::operator==(SnapMode const& rhs) const
{
    return snapIntersection == rhs.snapIntersection
        && snapOnEntity == rhs.snapOnEntity
        && snapCenter == rhs.snapCenter
        && snapMiddle == rhs.snapMiddle
        && snapSubsection == rhs.snapSubsection
        && snapEndpoint == rhs.snapEndpoint
        && snapGrid == rhs.snapGrid
        && snapFree == rhs.snapFree
        && restriction == rhs.restriction
        && snapAngle == rhs.snapAngle;
}

// 将模式捕捉转换为整数
unsigned int SnapMode::toInt(const SnapMode& s)
{
    uint ret = 0;

    if (s.snapIntersection) { ret |= SnapMode::SnapIntersection; }
    if (s.snapOnEntity)     { ret |= SnapMode::SnapOnEntity; }
    if (s.snapCenter)       { ret |= SnapMode::SnapCenter; }
    if (s.snapMiddle)       { ret |= SnapMode::SnapMiddle; }
    if (s.snapSubsection)   { ret |= SnapMode::SnapSubsection; }
    if (s.snapEndpoint)     { ret |= SnapMode::SnapEndpoint; }
    if (s.snapGrid)         { ret |= SnapMode::SnapGrid; }
    if (s.snapFree)         { ret |= SnapMode::SnapFree; }
    if (s.snapAngle)        { ret |= SnapMode::SnapAngle; }

    switch (s.restriction)
    {
    case DM::RestrictHorizontal:
        ret |= SnapMode::RestrictHorizontal;
        break;
    case DM::RestrictVertical:
        ret |= SnapMode::RestrictVertical;
        break;
    case DM::RestrictOrthogonal:
        ret |= SnapMode::RestrictOrthogonal;
        break;
    default:
        break;
    }

    return ret;
}

// 将整数转换为枚举型
SnapMode SnapMode::fromInt(unsigned int ret)
{
    SnapMode s;

    if (SnapMode::SnapIntersection & ret) { s.snapIntersection = true; }
    if (SnapMode::SnapOnEntity & ret) { s.snapOnEntity = true; }
    if (SnapMode::SnapCenter & ret) { s.snapCenter = true; }
    if (SnapMode::SnapMiddle & ret) { s.snapMiddle = true; }
    if (SnapMode::SnapSubsection & ret) { s.snapSubsection = true; }
    if (SnapMode::SnapEndpoint & ret) { s.snapEndpoint = true; }
    if (SnapMode::SnapGrid & ret) { s.snapGrid = true; }
    if (SnapMode::SnapFree & ret) { s.snapFree = true; }
    if (SnapMode::SnapAngle & ret) { s.snapAngle = true; }

    switch (SnapMode::RestrictOrthogonal & ret)
    {
    case SnapMode::RestrictHorizontal:
        s.restriction = DM::RestrictHorizontal;
        break;
    case SnapMode::RestrictVertical:
        s.restriction = DM::RestrictVertical;
        break;
    case SnapMode::RestrictOrthogonal:
        s.restriction = DM::RestrictOrthogonal;
        break;
    default:
        s.restriction = DM::RestrictNothing;
        break;
    }

    return s;
}

struct Snapper::ImpData
{
    DmVector snapCoord;
    DmVector snapSpot;
    SnapResultType snapResult = SnapResultType::SnapNone;
};


Snapper::Snapper(DmDocument* doc, GuiDocumentView* docView)
    : pDocument(doc)
    , docView(docView)
    , m_pImpData(new ImpData())
{}

Snapper::~Snapper()
{
}

void Snapper::init()
{
    this->snapMode = docView->getDefaultSnapMode();
    keyEntity = nullptr;
    m_pImpData->snapSpot = DmVector{ false };
    m_pImpData->snapCoord = DmVector{ false };
    m_pImpData->snapResult = SnapResultType::SnapNone;
    snapDistance = 1.0;

    snapRange = getSnapRange();
}

void Snapper::finish()
{
    finished = true;
    deleteSnapper();
}

void Snapper::finishOrthogonal()
{
    if (this->snapMode.restriction == DM::RestrictOrthogonal)
    {
        docView->getCurrentAction()->finish();
    }
}

void Snapper::resetOrthogonal()
{
    m_orthogonalPoint = DmVector{ false };
    docView->setOrthogonalZero(DmVector{ false });
}

DmEntity* Snapper::getKeyEntity() const
{
    return keyEntity;
}

void Snapper::setSnapMode(const SnapMode& snapMode)
{
    this->snapMode = snapMode;
    GUIDIALOGFACTORY->requestSnapMiddleOptions(snapSubsectionPts, snapMode.snapSubsection);
}

SnapMode const* Snapper::getSnapMode() const
{
    return &this->snapMode;
}

SnapMode* Snapper::getSnapMode()
{
    return &this->snapMode;
}

SnapResultType Snapper::getSnapResult() const
{
    return m_pImpData->snapResult;
}

DmVector Snapper::getSnapSpot() const
{
    return m_pImpData->snapSpot;
}

DmVector Snapper::getSnapCoord() const
{
    return m_pImpData->snapCoord;
}

void Snapper::setSnapRestriction(DM::SnapRestriction)
{
}

// get current mouse coordinates
DmVector Snapper::snapFree(QMouseEvent* e)
{
    if (!e)
    {
        return DmVector(false);
    }
    m_pImpData->snapSpot = docView->toGraph(e->x(), e->y());
    m_pImpData->snapCoord = m_pImpData->snapSpot;
    return m_pImpData->snapCoord;
}

/// @brief 使用当前捕捉模式捕捉到图形中的坐标
/// @param e 鼠标事件
/// @return 鼠标事件点的坐标或无效向量
DmVector Snapper::snapPoint(QMouseEvent* e)
{
    m_pImpData->snapSpot = DmVector(false);
    m_pImpData->snapResult = SnapResultType::SnapNone;
    DmVector t(false);

    if (!e)
    {
        return m_pImpData->snapSpot;
    }

    snapFree(e);

    DmVector mouseCoord = docView->toGraph(e->x(), e->y());
    double ds2Min = DM_MAXDOUBLE * DM_MAXDOUBLE;

    if (this->snapMode.snapEndpoint)
    {
        t = snapEndpoint(mouseCoord);
        if (t.valid)
        {
            double ds2 = mouseCoord.squaredTo(t);
            if (ds2 < ds2Min)
            {
                ds2Min = ds2;
                m_pImpData->snapSpot = t;
                m_pImpData->snapResult = SnapResultType::SnapEndpoint;
            }
        }
    }
    if (this->snapMode.snapCenter)
    {
        t = snapCenter(mouseCoord);
        if (t.valid)
        {
            double ds2 = mouseCoord.squaredTo(t);
            if (ds2 < ds2Min)
            {
                ds2Min = ds2;
                m_pImpData->snapSpot = t;
                m_pImpData->snapResult = SnapResultType::SnapCenter;
            }
        }
    }
    if (this->snapMode.snapMiddle)
    {
        t = snapMiddle(mouseCoord);
        if (t.valid)
        {
            double ds2 = mouseCoord.squaredTo(t);
            if (ds2 < ds2Min)
            {
                ds2Min = ds2;
                m_pImpData->snapSpot = t;
                m_pImpData->snapResult = SnapResultType::SnapMiddle;
            }
        }
    }
    if (this->snapMode.snapSubsection)
    {
        // this is still brutal force
        // todo: accept value from widget UISnapMiddleOptions
        GUIDIALOGFACTORY->requestSnapMiddleOptions(snapSubsectionPts, this->snapMode.snapSubsection);
        t = snapSubsection(mouseCoord);
        if (t.valid)
        {
            double ds2 = mouseCoord.squaredTo(t);
            if (ds2 < ds2Min)
            {
                ds2Min = ds2;
                m_pImpData->snapSpot = t;
                m_pImpData->snapResult = SnapResultType::SnapSubsection;
            }
        }
    }
    if (this->snapMode.snapIntersection)
    {
        t = snapIntersection(mouseCoord);
        if (t.valid)
        {
            double ds2 = mouseCoord.squaredTo(t);
            if (ds2 < ds2Min)
            {
                ds2Min = ds2;
                m_pImpData->snapSpot = t;
                m_pImpData->snapResult = SnapResultType::SnapIntersection;
            }
        }
    }

    if (this->snapMode.snapOnEntity)
    {
        t = snapOnEntity(mouseCoord);
        if (t.valid)
        {
            double ds2 = mouseCoord.squaredTo(t);
            if (ds2 < ds2Min)
            {
                ds2Min = ds2;
                m_pImpData->snapSpot = t;
                m_pImpData->snapResult = SnapResultType::SnapOnEntity;
            }
        }
    }

    if (this->snapMode.snapGrid)
    {
        t = snapGrid(mouseCoord);
        if (t.valid)
        {
            double ds2 = mouseCoord.squaredTo(t);
            if (ds2 < ds2Min)
            {
                m_pImpData->snapSpot = t;
                m_pImpData->snapResult = SnapResultType::SnapGrid;
            }
        }
    }

    // 根据距离判断是否设置鼠标捕捉点到指定目标点
    DmVector const& ds = mouseCoord - m_pImpData->snapSpot;
    double snapDis = docView->toGraphDX(ADSORPTIONDISTANCE);
    if (fabs(ds.x) > fabs(snapDis) || fabs(ds.y) > fabs(snapDis))
    {
        m_pImpData->snapSpot = mouseCoord;
        m_pImpData->snapResult = SnapResultType::SnapNone;
    }

    // 极轴 0度90度180度270度捕捉模式
    DmVector zeroPoint = docView->getOrthogonalZero();
    DmVector vpv(zeroPoint.x, m_pImpData->snapSpot.y);
    DmVector vph(m_pImpData->snapSpot.x, zeroPoint.y);
    switch (this->snapMode.restriction)
    {
    case DM::RestrictOrthogonal:
        if (m_orthogonalPoint.x > -0.000001 && m_orthogonalPoint.x < 0.000001 && m_orthogonalPoint.y > -0.000001 && m_orthogonalPoint.y < 0.000001)
        {
            if (fabs(ds.x) > fabs(snapDis) || fabs(ds.y) > fabs(snapDis))
            {
                m_pImpData->snapCoord = mouseCoord;
            }
            else
            {
                m_pImpData->snapCoord = m_pImpData->snapSpot;
            }
        }
        else
        {
            m_pImpData->snapCoord = (mouseCoord.distanceTo(vpv) < mouseCoord.distanceTo(vph)) ? vpv : vph;
        }
        m_orthogonalPoint = zeroPoint;
        break;
    case DM::RestrictHorizontal:
        m_pImpData->snapCoord = vph;
        break;
    case DM::RestrictVertical:
        m_pImpData->snapCoord = vpv;
        break;
    default:
        m_pImpData->snapCoord = m_pImpData->snapSpot;
        break;
    }

    snapPoint(m_pImpData->snapSpot, false);

    return m_pImpData->snapCoord;
}

/// @brief 手动设置捕捉点
/// @param coord 坐标
/// @param setSpot 是否设置捕捉点
/// @return 坐标
DmVector Snapper::snapPoint(const DmVector& coord, bool setSpot)
{
    if (coord.valid)
    {
        m_pImpData->snapSpot = coord;
        if (setSpot)
        {
            m_pImpData->snapCoord = coord;
        }
        drawSnapper();
        GUIDIALOGFACTORY->updateCoordinateWidget(m_pImpData->snapCoord, m_pImpData->snapCoord - docView->getRelativeZero());
    }
    return coord;
}

double Snapper::getSnapRange() const
{
    if (docView)
    {
        return (docView->getGrid()->getCellVector() * 0.5).magnitude();
    }

    constexpr double DEFAULT_SNAP_RANGE = 20.;
    return DEFAULT_SNAP_RANGE;
}

void Snapper::getAdsorptionExtentOfPoint(const DmVector& pt, DmVector& min, DmVector& max)
{
    DmVector factor = docView->getFactor();
    double xTol = ADSORPTIONDISTANCE * factor.x;
    double yTol = ADSORPTIONDISTANCE * factor.y;
    min.x = pt.x - xTol;
    min.y = pt.y - yTol;
    max.x = pt.x + xTol;
    max.y = pt.y + yTol;
}

void Snapper::getCatchExtentOfPoint(const DmVector& pt, DmVector& min, DmVector& max)
{
    DmVector factor = docView->getFactor();
    double xTol = CATCH_ENTITY_DISTANCE * factor.x;
    double yTol = CATCH_ENTITY_DISTANCE * factor.y;
    min.x = pt.x - xTol;
    min.y = pt.y - yTol;
    max.x = pt.x + xTol;
    max.y = pt.y + yTol;
}

void Snapper::getSubEntitiesOfContainerRecrusive(DmEntityContainer* container, std::vector<DmEntity*>& subEntities)
{
    for (DmEntity* e : *container)
    {
        DmEntityContainer* subContainer = dynamic_cast<DmEntityContainer*>(e);
        if (subContainer)
        {
            getSubEntitiesOfContainerRecrusive(subContainer, subEntities);
        }
        else
        {
            subEntities.emplace_back(e);
        }
    }
}

/// @brief 捕捉到自由坐标
/// @param coord 鼠标坐标
/// @return 点坐标或无效向量
DmVector Snapper::snapFree(const DmVector& coord)
{
    keyEntity = nullptr;
    return coord;
}

DmVector Snapper::snapEndpoint(const DmVector& coord)
{
    std::function<DmVector(DmEntity*, const DmVector&, double*)> func = [](DmEntity* e, const DmVector& pt, double* dist)
    {
        return e->getNearestEndpoint(pt, dist);
    };
    return snapByFunc(coord, func);
}

/// @brief 捕捉到栅格点
/// @param coord 鼠标坐标
/// @return 点坐标或无效向量
DmVector Snapper::snapGrid(const DmVector& coord)
{
    return docView->getGrid()->snapGrid(coord);
}

DmVector Snapper::snapOnEntity(const DmVector& coord)
{
    std::function<DmVector(DmEntity*, const DmVector&, double*)> func = [=](DmEntity* e, const DmVector& pt, double* dist)
    {
        return e->getNearestPointOnEntity(pt, true, dist, &keyEntity);
    };
    return snapByFunc(coord, func);
}

DmVector Snapper::snapCenter(const DmVector& coord)
{
    std::function<DmVector(DmEntity*, const DmVector&, double*)> func = [](DmEntity* e, const DmVector& pt, double* dist)
    {
        return e->getNearestCenter(pt, dist);
    };
    return snapByFunc(coord, func);
}

DmVector Snapper::snapMiddle(const DmVector& coord)
{
    std::function<DmVector(DmEntity*, const DmVector&, double*)> func = [](DmEntity* e, const DmVector& pt, double* dist)
    {
        return e->getNearestMiddle(pt, dist, 1);
    };
    return snapByFunc(coord, func);
}

DmVector Snapper::snapSubsection(const DmVector& coord)
{
    // 关于传递成员变量，参考：https://www.cnblogs.com/yajiu/p/15693724.html
    std::function<DmVector(DmEntity*, const DmVector&, double*)> func = [=](DmEntity* e, const DmVector& pt, double* dist)
    {
        return e->getNearestMiddle(pt, dist, snapSubsectionPts - 1);
    };
    return snapByFunc(coord, func);
}

DmVector Snapper::snapIntersection(const DmVector& coord)
{
    // 获得鼠标包围框相交的实体
    DmVector res(false);
    DmVector min(true), max(true);
    std::vector<DmEntity*> ents;
    getAdsorptionExtentOfPoint(coord, min, max);
    pDocument->searchEntities(min, max, ents);

    // 从中过滤出最近的（子）实体
    double minDist = DM_MAXDOUBLE;      // minimum measured distance
    double curDist;                     // currently measured distance
    DmEntity* closestEntity = nullptr;    // closest entity found
    DmEntity* subEntity = nullptr;
    for (auto e : ents)
    {
        curDist = e->getDistanceToPoint(coord, &subEntity, DM::ResolveAllButTextImage);
        if (curDist <= minDist)
        {
            closestEntity = subEntity;
            minDist = curDist;
        }
    }
    if (!closestEntity)
    {
        return DmVector(false);
    }

    //用最近的子实体与其他实体求交
    DmEntityContainer ec(nullptr, false);
    for (auto e : ents)
    {
        ec.addEntity(e);
    }
    minDist = DM_MAXDOUBLE;
    DmVector closestPoint(false);    // closest found endpoint
    DmVector point;                  // endpoint found
    DmVectorSolutions sol;
    for (DmEntity* en = ec.firstEntity(DM::ResolveAllButTextImage); en; en = ec.nextEntity(DM::ResolveAllButTextImage))
    {
        if (!en->isVisible())
        {
            continue;
        }
        sol = Information::getIntersection(closestEntity, en, true);

        point = sol.getClosest(coord, &curDist, nullptr);
        if (sol.getNumber() > 0 && curDist < minDist)
        {
            closestPoint = point;
            minDist = curDist;
        }
    }
    return closestPoint;
}

/// @brief 将给定坐标修正到当前相对零点的0/90/180/270度方向
/// @param coord 原始坐标
/// @return 修正后的坐标
DmVector Snapper::restrictOrthogonal(const DmVector& coord)
{
    DmVector rz = docView->getRelativeZero();
    DmVector ret(coord);

    DmVector retx = DmVector(rz.x, ret.y);
    DmVector rety = DmVector(ret.x, rz.y);

    if (retx.distanceTo(ret) > rety.distanceTo(ret))
    {
        ret = rety;
    }
    else
    {
        ret = retx;
    }

    return ret;
}

/// @brief 将给定坐标修正到当前相对零点的0/180度方向
/// @param coord 原始坐标
/// @return 修正后的坐标
DmVector Snapper::restrictHorizontal(const DmVector& coord)
{
    DmVector rz = docView->getRelativeZero();
    DmVector ret = DmVector(coord.x, rz.y);
    return ret;
}

/// @brief 将给定坐标修正到当前相对零点的90/270度方向
/// @param coord 原始坐标
/// @return 修正后的坐标
DmVector Snapper::restrictVertical(const DmVector& coord)
{
    DmVector rz = docView->getRelativeZero();
    DmVector ret = DmVector(rz.x, coord.y);
    return ret;
}

DmEntity* Snapper::catchEntity(const DmVector& pos, DM::ResolveLevel level)
{
    // 获得鼠标包围框相交的实体
    DmVector res(false);
    DmVector min(true), max(true);
    std::vector<DmEntity*> ents;
    getCatchExtentOfPoint(pos, min, max);
    if (level == DM::ResolveNone)
    {
        pDocument->searchEntities(min, max, ents, false, false);
    }
    else
    {
        pDocument->searchEntities(min, max, ents, true);
    }

    // 从中过滤出最近的（子）实体
    double minDist = DM_MAXDOUBLE;      // minimum measured distance
    double curDist = 0.0;                     // currently measured distance
    DmEntity* closestEntity = nullptr;    // closest entity found
    DmEntity* subEntity = nullptr;
    for (auto e : ents)
    {
        curDist = e->getDistanceToPoint(pos, &subEntity, level);
        if (curDist <= minDist)
        {
            if (level == DM::ResolveAll || level == DM::ResolveAllButTextImage)
            {
                closestEntity = subEntity;
            }
            else
            {
                closestEntity = e;
            }
            minDist = curDist;
        }
    }
    if (closestEntity && minDist < max.x - min.x)
    {
        return closestEntity;
    }
    else
    {
        return nullptr;
    }
}

DmEntity* Snapper::catchEntity(const DmVector& pos, DM::EntityType enType, DM::ResolveLevel level)
{
    // 获得鼠标包围框相交的实体
    DmVector res(false);
    DmVector min(true), max(true);
    std::vector<DmEntity*> ents;
    getCatchExtentOfPoint(pos, min, max);
    if (level == DM::ResolveNone)
    {
        pDocument->searchEntities(min, max, ents, false, false);
    }
    else
    {
        pDocument->searchEntities(min, max, ents, true);
    }

    // 从中过滤出指定类型的实体/子实体
    DmEntityContainer ec(nullptr, false);
    std::vector<DmEntity*> subEntities;
    for (auto en : ents)
    {
        DmEntityContainer* subEntContainer = dynamic_cast<DmEntityContainer*>(en);
        // 如果是容器类型，递归获得子实体
        if (en->getEntityType() != enType && subEntContainer != nullptr && (level == DM::ResolveAll || level == DM::ResolveAllButTextImage))
        {
            subEntities.clear();
            getSubEntitiesOfContainerRecrusive(subEntContainer, subEntities);
            for (auto subEnt : subEntities)
            {
                if (subEnt->getEntityType() == enType)
                {
                    ec.addEntity(subEnt);
                }
            }
        }
        if (en->getEntityType() == enType)
        {
            ec.addEntity(en);
        }
    }
    if (ec.size() == 0)
    {
        return nullptr;
    }

    //从中选择最近的
    double dist(0.0);
    DmEntity* entity = ec.getNearestEntity(pos, &dist, DM::ResolveNone);

    if (entity && dist < max.x - min.x)
    {
        return entity;
    }
    else
    {
        return nullptr;
    }
}

DmEntity* Snapper::catchEntity(QMouseEvent* e, DM::ResolveLevel level)
{
    return catchEntity(DmVector(docView->toGraphX(e->x()), docView->toGraphY(e->y())), level);
}

DmEntity* Snapper::catchEntity(QMouseEvent* e, DM::EntityType enType, DM::ResolveLevel level)
{
    return catchEntity({ docView->toGraphX(e->x()), docView->toGraphY(e->y()) }, enType, level);
}

DmEntity* Snapper::catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList, DM::ResolveLevel level)
{
    DmEntity* pten = nullptr;
    DmVector coord = DmVector(docView->toGraphX(e->x()), docView->toGraphY(e->y()));
    switch (enTypeList.size())
    {
    case 0:
        return catchEntity(coord, level);
    default:
    {
        DmEntityContainer ec(nullptr, false);
        for (auto t0 : enTypeList)
        {
            DmEntity* en = catchEntity(coord, t0, level);
            if (en) { ec.addEntity(en); }
        }
        if (ec.size() > 0)
        {
            ec.getDistanceToPoint(coord, &pten, DM::ResolveNone);
            return pten;
        }
    }
    }
    return nullptr;
}

void Snapper::suspend()
{
    m_pImpData->snapSpot = m_pImpData->snapCoord = DmVector{ false };
}

void Snapper::resume()
{
    drawSnapper();
}

// Hides the snapper options. Default implementation does nothing.
void Snapper::hideOptions()
{
    // not used any more, will be removed
}

// Shows the snapper options. Default implementation does nothing.
void Snapper::showOptions()
{
    // not used any more, will be removed
}

// Deletes the snapper from the screen.
void Snapper::deleteSnapper()
{
    if (docView)
    {
        docView->getOverlayContainer(DM::Snapper)->clear();
        docView->redraw(); // redraw will happen in the mouse movement event
    }
}

// 创建捕捉指示器
void Snapper::drawSnapper()
{
    docView->redraw();
}

/// @brief 将当前坐标按角度捕捉到参考点的指定角度增量上，实现极轴追踪功能
/// @param [in] currentCoord 当前鼠标坐标
/// @param [in] referenceCoord 参考点坐标（角度捕捉的基准点）
/// @param [in] angularResolution 角度分辨率（度数），通常为 15°
/// @return 角度捕捉后的坐标点；若同时开启了实体捕捉，返回该角度射线与最近实体的虚拟交点
DmVector Snapper::snapToAngle(const DmVector& currentCoord, const DmVector& referenceCoord, const double angularResolution)
{
    // 已有正交/水平/垂直限制或网格捕捉时，不叠加角度捕捉，直接返回当前坐标
    if (this->snapMode.restriction != DM::RestrictNothing || this->snapMode.snapGrid)
    {
        return currentCoord;
    }

    // 计算参考点到当前坐标的角度，量化到最近的 angularResolution 倍数
    double angle = referenceCoord.angleTo(currentCoord) * 180.0 / M_PI;
    angle -= remainder(angle, angularResolution);
    angle *= M_PI / 180.;
    // 保持原始距离不变，以量化后的角度重建坐标点
    DmVector res = DmVector::polar(referenceCoord.distanceTo(currentCoord), angle);
    res += referenceCoord;

    // 若开启实体捕捉，在角度锁定方向上查找与最近实体的虚拟交点
    if (this->snapMode.snapOnEntity)
    {
        DmVector t = pDocument->getEntityTable()->getNearestVirtualIntersection(res, angle, nullptr);
        m_pImpData->snapSpot = t;
        snapPoint(m_pImpData->snapSpot, true);
        return t;
    }
    else
    {
        snapPoint(res, true);
        return res;
    }
}

DmVector Snapper::snapByFunc(const DmVector& coord, const std::function<DmVector(DmEntity*, const DmVector&, double*)>& func)
{
    DmVector res(false);
    DmVector min(true), max(true);
    std::vector<DmEntity*> ents;
    getAdsorptionExtentOfPoint(coord, min, max);
    pDocument->searchEntities(min, max, ents);
    double minDist = DM_MAXDOUBLE;
    double curDist = DM_MAXDOUBLE;
    DmVector curCatchPt(true);
    DmVector minPt(true);
    for (auto e : ents)
    {
        if (!e->isVisible())
        {
            continue;
        }
        curCatchPt = func(e, coord, &curDist);
        if (!curCatchPt.valid)
        {
            continue;
        }
        if (curDist < minDist)
        {
            minDist = curDist;
            minPt = curCatchPt;
        }
    }
    if (minDist < max.x - min.x)
    {
        res = minPt;
    }
    return res;
}
