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

/// @file FindClosedRegion.cpp
/// @brief 闭合区域查找类实现，通过左转算法从实体集合中查找闭合区域

#include "FindClosedRegion.h"
#include "Information.h"
#include "GeometryMethods.h"
#include "ConstrainedDelaunayTriangulation.h"
#include "DmCircle.h"
#include "Math2d.h"

/// 圆弧与椭圆相切的的情况，半径长轴为100，Information::getIntersection获得的交点在1E-8级别，且算出4个，这里将精度缩小
constexpr double TOL = 1e-3;

namespace FindClosedRegion
{
    bool Edge::isLine() const
    {
        if (entity->getEntityType() == DM::EntityLine)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    double Edge::getStartCurvature()
    {
        if (!m_hasCalculateCurvature)
        {
            calculateCurvature();
        }
        return m_startCurvature;
    }

    double Edge::getEndCurvature()
    {
        if (!m_hasCalculateCurvature)
        {
            calculateCurvature();
        }
        return m_endCurvature;
    }

    void Edge::reverseByNeed()
    {
        if (needReverse)
        {
            switch (entity->getEntityType())
            {
                case DM::EntityLine:
                {
                    DmLine* l = static_cast<DmLine*>(entity.get());
                    DmVector s = l->getStartpoint();
                    DmVector e = l->getEndpoint();
                    l->setStartpoint(e);
                    l->setEndpoint(s);
                }
                break;
                case DM::EntityArc:
                {
                    DmArc* arc = static_cast<DmArc*>(entity.get());
                    arc->setClockwise(!arc->isClockwise());
                }
                break;
                case DM::EntityEllipse:
                {
                    DmEllipse* ell = static_cast<DmEllipse*>(entity.get());
                    ell->setClockwise(!ell->isClockwise());
                }
                break;
                case DM::EntitySpline:
                {
                    DmSpline* spline = static_cast<DmSpline*>(entity.get());
                    spline->reverse();
                }
                break;
                default:
                    break;
            }

            needReverse = false;
        }
    }

    void Edge::getPoints(DmEntity* e, bool reverse, std::vector<DmVector>& pts)
    {
        switch (e->getEntityType())
        {
            case DM::EntityLine:
            {
                DmLine* l = static_cast<DmLine*>(e);
                if (reverse)
                {
                    pts.emplace_back(l->getEndpoint());
                }
                else
                {
                    pts.emplace_back(l->getStartpoint());
                }
            }
            break;
            case DM::EntityArc:
            {
                DmArc* arc = static_cast<DmArc*>(e);
                arc->getPoints(pts, reverse);
                pts.pop_back();
            }
            break;
            case DM::EntityEllipse:
            {
                DmEllipse* ell = static_cast<DmEllipse*>(e);
                ell->getPoints(pts, reverse);
                pts.pop_back();
            }
            break;
            case DM::EntitySpline:
            {
                DmSpline* spline = static_cast<DmSpline*>(e);
                spline->getPoints(pts, reverse);
                pts.pop_back();
            }
            break;
            default:
                break;
        }
    }

    void Edge::calculateCurvature()
    {
        // 关于求参数化曲线的曲率，参考：https://chat.deepseek.com/share/2h8djip9kpiei2mj7x
        switch (entity->getEntityType())
        {
            case DM::EntityArc:
            {
                DmArcPtr arc = std::dynamic_pointer_cast<DmArc>(entity);
                calculateCurvatureForArc(arc);
            }
            break;
            case DM::EntityEllipse:
            {
                DmEllipsePtr ell = std::dynamic_pointer_cast<DmEllipse>(entity);
                calculateCurvatureForEllipse(ell);
            }
            break;
            case DM::EntitySpline:
            {
                DmSplinePtr spline = std::dynamic_pointer_cast<DmSpline>(entity);
                calculateCurvatureForSpline(spline);
            }
            break;
            default:
                break;
        }
    }

    void Edge::calculateCurvatureForArc(DmArcPtr arc)
    {
        if (m_hasCalculateCurvature)
        {
            return;
        }

        // 逆时针圆弧，各点曲率为1/R，顺时针圆弧，各点曲率为-1/R，可根据参数方程验证。
        double r = arc->getRadius();
        bool startValueNegative = false;
        bool isClockwise = arc->isClockwise();
        startValueNegative = isClockwise;
        if (needReverse)
        {
            startValueNegative = !startValueNegative;
        }

        if (startValueNegative)
        {
            m_startCurvature = -1.0 / r;
        }
        else
        {
            m_startCurvature = 1.0 / r;
        }
        m_endCurvature = -m_startCurvature;

        m_hasCalculateCurvature = true;
    }

    void Edge::calculateCurvatureForEllipse(DmEllipsePtr ell)
    {
        if (m_hasCalculateCurvature)
        {
            return;
        }

        bool isClockwise = ell->isClockwise();
        bool switchBeginEndOfNormal = isClockwise; // 如果用正向椭圆弧计算，是否需要切换起终点
        if (needReverse)
        {
            switchBeginEndOfNormal = !switchBeginEndOfNormal;
        }

        double startParaNormal = ell->getStartParamNormal();
        double endParaNormal = ell->getEndParamNormal();
        // 旋转不影响计算曲率，取摆正之后的椭圆弧进行曲率计算
        // x=acost,y=bsint
        DmVector majorP = ell->getMajorP();
        double a = majorP.magnitude();
        double b = ell->getRatio() * a;
        auto getK = [&](double t)
        {
            double sint = std::sin(t);
            double cost = std::cos(t);
            double derivX = -a * sint;
            double derivY = b * cost;
            double secDerivX = -a * cost;
            double secDerivY = -b * sint;
            double fac1 = derivX * secDerivY - derivY * secDerivX;
            double fac2 = std::pow(derivX * derivX + derivY * derivY, 3.0 / 2.0);
            double k = fac1 / fac2;
            return k;
        };
        double startK = getK(startParaNormal);
        double endK = getK(endParaNormal);

        if (switchBeginEndOfNormal)
        {
            m_startCurvature = -endK;
            m_endCurvature = startK;
        }
        else
        {
            m_startCurvature = startK;
            m_endCurvature = -endK;
        }

        m_hasCalculateCurvature = true;
    }

    void Edge::calculateCurvatureForSpline(DmSplinePtr spline)
    {
        if (m_hasCalculateCurvature)
        {
            return;
        }

        // 对于参数曲线，曲率计算，参考：https://chat.deepseek.com/share/55072d4uu8h34kt6rc
        double t1 = 0.0;
        double t2 = 0.0;
        spline->getDomainOfDefinition(t1, t2);
        auto getK = [&](double t)
        {
            DmVector derivT = spline->derivative(t);
            DmVector secDerivT = spline->secondDerivative(t);
            double fac1 = derivT.x * secDerivT.y - derivT.y * secDerivT.x;
            double fac2 = std::pow(derivT.x * derivT.x + derivT.y * derivT.y, 3.0 / 2.0);
            double k = fac1 / fac2;
            return k;
        };
        double k1 = getK(t1);
        double k2 = getK(t2);
        if (needReverse)
        {
            m_startCurvature = -k2;
            m_endCurvature = k1;
        }
        else
        {
            m_startCurvature = k1;
            m_endCurvature = -k2;
        }

        m_hasCalculateCurvature = true;
    }

    void Region::add(EdgePtr edge)
    {
        edges.emplace_back(edge);
        DmVector min = edge->entity->getMin();
        DmVector max = edge->entity->getMax();
        box.add(min);
        box.add(max);
    }

    bool Region::contains(const RegionPtr region)
    {
        if (!box.contains(region->box))
        {
            return false;
        }

        // 判断区域的边界点是否在当前区域三角形中
        for (auto edge : region->edges)
        {
            std::vector<DmVector> pts;
            Edge::getPoints(edge->entity.get(), edge->needReverse, pts);
            for (auto pt : pts)
            {
                if (!isPointInside(pt, false))
                {
                    return false;
                }
            }
        }
        return true;
    }

    void Region::triangulate()
    {
        if (!hasTriangulated)
        {
            triangles.clear();
            calculatePoints();
            // CDT三角化
            ConstrainedDelaunayTriangulation::trianglulate<DmTrianglePtr>(points, {}, triangles);

            hasTriangulated = true;
        }
    }

    void Region::triangulateConsiderHoles()
    {
        if (!hasTriangulated_without_holes)
        {
            triangles_without_holes.clear();
            calculatePoints();
            // CDT三角化
            std::vector<std::vector<DmVector>> holePts;
            for (auto hole : holes)
            {
                hole->calculatePoints();
                auto pts = hole->getPoints();
                holePts.emplace_back(pts);
            }
            ConstrainedDelaunayTriangulation::trianglulate<DmTrianglePtr>(points, holePts, triangles_without_holes);
            hasTriangulated_without_holes = true;
        }
    }

    void Region::calculatePoints()
    {
        if (points.empty())
        {
            for (auto const& edge : edges)
            {
                Edge::getPoints(edge->entity.get(), edge->needReverse, points);
            }
            // 去重，否则三角化判断内外三角形时可能有问题
            DmVectorSolutions sol(points);
            sol.distinct(TOL);
            points = sol.getVector();
        }
    }

    void Region::calculateDirection()
    {
        calculatePoints();

        // 提示词：一系列有序的点，如何判断顺时针还是逆时针
        // 采用有向面积法，面积为正则为逆时针
        double A2 = 0.0; // 面积*2
        int size = static_cast<int>(points.size());
        for (int i = 0; i < size - 1; i++)
        {
            DmVector p1 = points.at(i);
            DmVector p2 = points.at(i + 1);
            A2 += (p1.x * p2.y - p2.x * p1.y);
        }
        // 最后一个
        DmVector lastPt = points.back();
        DmVector firstPt = points.front();
        A2 += (lastPt.x * firstPt.y - firstPt.x * lastPt.y);
        isAnticlockwise_ = (A2 >= 0.0);
    }

    std::vector<DmVector> Region::getPoints() const
    {
        return points;
    }

    bool Region::isPointInside(const DmVector& pt, bool considerNotInHole)
    {
        if (!box.isPointInside(pt))
        {
            return false;
        }

        bool inside = false;
        if (considerNotInHole)
        {
            triangulateConsiderHoles();
            for (auto& tri : triangles_without_holes)
            {
                if (tri->isPointInside(pt))
                {
                    inside = true;
                    break;
                }
            }
        }
        else
        {
            triangulate();
            for (auto& tri : triangles)
            {
                if (tri->isPointInside(pt))
                {
                    inside = true;
                    break;
                }
            }
        }
        return inside;
    }

    bool Region::hasEdge(EdgePtr edge) const
    {
        for (auto e : edges)
        {
            if (e == edge)
            {
                return true;
            }
        }
        return false;
    }

    RegionPtr Region::findSubRegionContainPoint(const DmVector& pt)
    {
        for (auto hole : holes)
        {
            if (hole->isPointInside(pt, true))
            {
                return findSubRegionContainPoint(pt);
            }
        }
        return RegionPtr();
    }

    RegionPtr Region::findRegionContainPointRecursive(const DmVector& pt) const
    {
        for (auto& subRegion : subBoundaries)
        {
            if (subRegion->isPointInside(pt, false))
            {
                if (subRegion->isPointInside(pt, true))
                {
                    return subRegion;
                }
                else
                {
                    return subRegion->findRegionContainPointRecursive(pt);
                }
            }
        }
        return RegionPtr();
    }

    DmRegionPtr Region::getDmRegion() const
    {
        DmEntityContainerPtr boundary(new DmEntityContainer());
        for (auto& edge : edges)
        {
            edge->reverseByNeed();
            boundary->addEntity(edge->entity->clone());
        }

        std::vector<DmEntityContainerPtr> holesBoundary;
        if (holes.size() > 0)
        {
            for (auto& hole : holes)
            {
                DmEntityContainerPtr hBoundary(new DmEntityContainer());
                for (auto& edge : *hole)
                {
                    edge->reverseByNeed();
                    hBoundary->addEntity(edge->entity->clone());
                }
                holesBoundary.emplace_back(hBoundary);
            }
        }

        RegionData d(boundary, holesBoundary);
        DmRegionPtr dmRegion(new DmRegion(nullptr, d));
        dmRegion->update();
        return dmRegion;
    }

    void RegionOwnershipMgr::add(RegionPtr region)
    {
        regions.emplace_back(region);
    }

    void RegionOwnershipMgr::group()
    {
        if (regions.size() <= 1)
        {
            return;
        }
        // 先按逆时针/顺时针分为边界/孔洞
        std::list<RegionPtr> boundaries; // 边界（逆时针）
        std::list<RegionPtr> holes; // 孔洞（顺时针）
        for (auto r : regions)
        {
            if (r->isAnticlockwise())
            {
                boundaries.emplace_back(r);
            }
            else
            {
                holes.emplace_back(r);
            }
        }

        // 可先按box大小/box的左下角坐标排一下序
        // ...

        // 构建边界的包含关系（分组）
        std::list<RegionPtr> groupedBoundaries;
        groupBoundaries(boundaries, groupedBoundaries);

        // 将孔洞添加到分组后的边界
        addHolesToBoundaries(groupedBoundaries, holes);

        // 结果放到regions
        regions.clear();
        regions.reserve(groupedBoundaries.size());
        std::move(groupedBoundaries.begin(), groupedBoundaries.end(), std::back_inserter(regions));
    }

    void RegionOwnershipMgr::removeHoles()
    {
        if (regions.empty())
        {
            return;
        }

        auto it = regions.begin();
        while (it != regions.end())
        {
            if (!(*it)->isAnticlockwise())
            {
                it = regions.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    std::vector<RegionPtr> RegionOwnershipMgr::getRegions() const
    {
        return regions;
    }

    bool RegionOwnershipMgr::regionHasEdgeOverlap(RegionPtr region1, RegionPtr region2)
    {
        for (auto edge1 : *region1)
        {
            for (auto edge2 : *region2)
            {
                if (edge1->reverseEdge.lock() == edge2)
                {
                    return true;
                }
            }
        }
        return false;
    }

    RegionPtr RegionOwnershipMgr::findMinInterBoundaryRecursive(RegionPtr region, RegionPtr whereToFind, bool findHole)
    {
        RegionPtr resBoundary;
        auto& subBoundaries = whereToFind->subBoundaries;
        if (findHole)
        {
            subBoundaries = whereToFind->holes;
        }
        for (auto b : subBoundaries)
        {
            if (b->contains(region) && !regionHasEdgeOverlap(b, region))
            {
                RegionPtr tmpB = findMinInterBoundaryRecursive(region, b, findHole);
                if (tmpB)
                {
                    resBoundary = tmpB;
                }
                else
                {
                    resBoundary = b;
                }
            }
        }
        return resBoundary;
    }

    void RegionOwnershipMgr::groupBoundaries(const std::list<RegionPtr>& boundaries, std::list<RegionPtr>& groupedBoundaries)
    {
        groupedBoundaries.emplace_back(boundaries.front()); // 已分组的边界
        auto it = boundaries.begin();
        ++it;
        for (; it != boundaries.end(); ++it)
        {
            RegionPtr region = *it; // 准备处理的边界
            bool found = false; // region是否被包含
            auto addedIt = groupedBoundaries.begin();
            while (addedIt != groupedBoundaries.end())
            {
                RegionPtr r = *addedIt;
                // 包含region
                if (r->contains(region) && !regionHasEdgeOverlap(r, region))
                {
                    // 递归往下找能包住region的最小子区域，将此孔洞指定为region父亲
                    auto deepSubBoundaryContainRegion = findMinInterBoundaryRecursive(region, r, false);
                    RegionPtr theR; // 查找theR之下的子区域，是否被region包含
                    if (deepSubBoundaryContainRegion)
                    {
                        theR = deepSubBoundaryContainRegion;
                    }
                    // 没有包住region的
                    else
                    {
                        theR = r;
                    }
                    // 如果有子区域在region之下，将之下的子区域放入region
                    addSubBoundaries(region, theR);
                    // 将region置为theR的孔洞
                    region->parent = theR;
                    theR->subBoundaries.emplace_back(region);
                    found = true;
                    break;
                }
                // 被region包含
                else if (region->contains(r) && !regionHasEdgeOverlap(r, region))
                {
                    region->subBoundaries.emplace_back(r);
                    addedIt = groupedBoundaries.erase(addedIt);
                }
                // 互不包含
                else
                {
                    ++addedIt;
                }
            }
            if (!found)
            {
                groupedBoundaries.emplace_back(region);
            }
        }
    }

    void RegionOwnershipMgr::addHolesToBoundaries(std::list<RegionPtr>& groupedBoundaries, std::list<RegionPtr>& holes)
    {
        for (auto hole : holes)
        {
            auto it = groupedBoundaries.begin();
            while (it != groupedBoundaries.end())
            {
                auto r = *it;
                if (r->contains(hole) && !regionHasEdgeOverlap(hole, r))
                {
                    // 递归往下找能包住region的最小子区域，将此孔洞指定为region父亲
                    auto deepSubBoundaryContainRegion = findMinInterBoundaryRecursive(hole, r, false);
                    RegionPtr theR; // 查找theR之下的子区域，是否被region包含
                    if (deepSubBoundaryContainRegion)
                    {
                        theR = deepSubBoundaryContainRegion;
                    }
                    // 没有包住region的
                    else
                    {
                        theR = r;
                    }
                    // 将region置为theR的孔洞
                    hole->parent = theR;
                    theR->holes.emplace_back(hole);
                    break;
                }
                else
                {
                    ++it;
                }
            }
            // 没有被包含的孔洞直接抛弃
        }
    }

    bool RegionOwnershipMgr::addSubBoundaries(RegionPtr region, RegionPtr whereToFind)
    {
        if (whereToFind->subBoundaries.empty())
        {
            return false;
        }

        bool found = false;
        auto it = whereToFind->subBoundaries.begin();
        while (it != whereToFind->subBoundaries.end())
        {
            auto subB = *it;
            if (region->contains(subB))
            {
                subB->parent = region;
                region->subBoundaries.emplace_back(subB);
                it = whereToFind->subBoundaries.erase(it);
                found = true;
            }
            else
            {
                ++it;
            }
        }
        return found;
    }

    void EdgeSearchTree::setPoints(const std::vector<DmVector>& pts, const std::vector<EdgePtr>& edges)
    {
        buildTree(pts);
        m_edges = edges;
    }

    void FindClosedRegion::addEntity(DmEntity* e)
    {
        if (!isValidEntity(e))
        {
            return;
        }
        auto it = m_precessedEntityIds.find(e->getId());
        if (it == m_precessedEntityIds.end())
        {
            // 如果是样条线，先判断是否已处理的样条重合
            bool isEqualSpline = false;
            if (e->getEntityType() == DM::EntitySpline)
            {
                DmSpline* spline = static_cast<DmSpline*>(e);
                for (auto addedEnt : m_addedEntities)
                {
                    if (addedEnt->getEntityType() == DM::EntitySpline)
                    {
                        DmSpline* addedSpline = static_cast<DmSpline*>(addedEnt);
                        if (isTwoSplinesEqual(spline, addedSpline))
                        {
                            isEqualSpline = true;
                            break;
                        }
                    }
                }
            }

            // 添加并分割实体
            if (!isEqualSpline)
            {
                addAndSplitEntity(e);
                m_addedEntities.emplace_back(e);
            }

            m_precessedEntityIds.insert(e->getId());
            m_modified = true;
        }
    }

    void FindClosedRegion::calculate()
    {
        if (!m_modified)
        {
            return;
        }

        // 构KD树
        m_regions.clear();
        m_edges.clear();
        m_searchTree.reset(new EdgeSearchTree());
        std::vector<DmVector> pts;
        pts.reserve(m_splitEnts.size() * 2);
        m_edges.reserve(m_splitEnts.size() * 2);
        for (auto& e : m_splitEnts)
        {
            DmVector start = e->getStartpoint();
            DmVector end = e->getEndpoint();
            auto type = e->getEntityType();
            if (type == DM::EntityLine || type == DM::EntityArc || type == DM::EntityEllipse || type == DM::EntitySpline)
            {
                pts.emplace_back(start);
                pts.emplace_back(end);
                EdgePtr edge1 = std::make_shared<Edge>(start, end, e->getDirection1(), e->getDirection2(), false, e);
                EdgePtr edge2 = std::make_shared<Edge>(end, start, e->getDirection2(), e->getDirection1(), true, DmEntityPtr(e->clone()));
                edge1->reverseEdge = edge2;
                edge2->reverseEdge = edge1;
                m_edges.emplace_back(edge1); // 注意m_edges的顺序与pts的顺序一致，保证添加的是edge的起点
                m_edges.emplace_back(edge2);
            }
        }
        m_searchTree->setPoints(pts, m_edges);

        // 左转之前，需要处理孤悬的线，否则会影响查找。查找过程中匹配孤悬的线，那么查找中断，已查找的线被标记，无法进入下一次查找
        clearSuspendEdges();

        // 执行左转算法，计算闭合区域
        leftTurn();

        // 处理区域所属关系
        RegionOwnershipMgr mgr;
        for (auto r : m_regions)
        {
            mgr.add(r);
        }
        mgr.group();

        m_regions = mgr.getRegions();
        m_modified = false;
    }

    RegionPtr FindClosedRegion::findRegionContainPoint(const DmVector& pt) const
    {
        for (auto r : m_regions)
        {
            if (r->isPointInside(pt, false))
            {
                RegionPtr subRegion = r->findRegionContainPointRecursive(pt);
                if (!subRegion)
                {
                    if (r->isPointInside(pt, true))
                    {
                        return r;
                    }
                }
                else
                {
                    return subRegion;
                }
            }
        }
        return RegionPtr();
    }

    void FindClosedRegion::leftTurn()
    {
        // 参考：《插件式GIS应用框架的设计与实现：基于C#和AE+9.2》 P200
        for (auto& edge : m_edges)
        {
            if (edge->isVisited || edge->isSuspend) // 已访问/悬置
            {
                continue;
            }

            RegionPtr region(std::make_shared<Region>());
            EdgePtr startEdge = edge;
            region->add(startEdge);
            EdgePtr curEdge = edge;
            curEdge->isVisited = true;
            bool isClosed = false;

            // 左转核心
            int maxIterations = static_cast<int>(m_edges.size()); // 防止无限循环
            for (int i = 0; i < maxIterations; i++)
            {
                auto indices = m_searchTree->getPointsWithinCube(curEdge->endPos, TOL);
                if (indices.size() == 0) // 一般不会发生
                {
                    break;
                }
                else
                {
                    std::vector<size_t> validIndices; // [未被访问，非悬置，且不是当已使用边的反向]的边索引
                    validIndices.reserve(indices.size());
                    std::copy_if(indices.begin(), indices.end(), std::back_inserter(validIndices),
                                 [&](const auto& idx)
                                 {
                                     return !m_edges.at(idx)->isVisited && !m_edges.at(idx)->isSuspend
                                         && !region->hasEdge(m_edges.at(idx)->reverseEdge.lock());
                                 });
                    if (validIndices.size() == 0)
                    {
                        break;
                    }
                    EdgePtr nextEdge;
                    if (validIndices.size() == 1)
                    {
                        nextEdge = m_edges.at(validIndices.front());
                    }
                    // >=2
                    else
                    {
                        std::vector<EdgePtr> nextEdges;
                        nextEdges.reserve(validIndices.size());
                        // 获得索引下的所有edge
                        std::transform(validIndices.begin(), validIndices.end(), std::back_inserter(nextEdges),
                                       [&](size_t idx){ return m_edges.at(idx); });
                        nextEdge = leftTurn_getNextEdge(nextEdges, curEdge);
                    }
                    curEdge = nextEdge;
                    curEdge->isVisited = true;
                    region->add(nextEdge);
                    if (nextEdge->endPos.distanceTo(startEdge->startPos) < TOL)
                    {
                        isClosed = true;
                        break;
                    }
                }
            }

            // 一次迭代完成，要么找到一个闭合区域，要么没找到
            if (isClosed && region->size() >= 2)
            {
                region->calculateDirection();
                m_regions.emplace_back(region);
            }
        }
    }

    EdgePtr FindClosedRegion::leftTurn_getNextEdge(std::vector<EdgePtr>& nextEdges, EdgePtr curEdge)
    {
        // 仅处理2个及以上有效nextEdge的情况
        if (nextEdges.size() < 2)
        {
            return EdgePtr();
        }
        double curAngle = Math2d::correctAngle2(curEdge->endAngle + M_PI);

        // 先按转角从大到小排序，如果转角相同，再按曲率从大到小排序。
        std::vector<std::tuple<EdgePtr, double, double>> items;
        for (auto edge : nextEdges)
        {
            double deltaA = Math2d::correctAngle2(edge->startAngle - curAngle); // 转向角
            if (std::abs(std::abs(deltaA) - M_PI) < TOL) // -PI的情况都按PI处理
            {
                deltaA = M_PI;
            }
            auto reverseE = edge->reverseEdge.lock();
            double curv = reverseE->getEndCurvature();
            items.emplace_back(edge, deltaA, curv);
        }
        std::sort(items.begin(), items.end(), [&](auto& e1, auto& e2)
        {
            double a1 = std::get<1>(e1);
            double a2 = std::get<1>(e2);
            if (std::abs(a1 - a2) < TOL)
            {
                double curv1 = std::get<2>(e1);
                double curv2 = std::get<2>(e2);
                return curv1 > curv2;
            }
            else
            {
                return a1 > a2;
            }
        });
        // 找到第一个角度次小于当前角度的（转向角最大的），对于入射与出射呈180度的情况，与出射的曲率比较，取曲率次次小于入射曲率的边
        double inCurv = curEdge->reverseEdge.lock()->getStartCurvature();
        for (auto& item : items)
        {
            double deltaA = std::get<1>(item);
            double curv = std::get<2>(item);
            if (std::abs(deltaA - M_PI) < TOL) // 入射边的反向
            {
                if (curv > inCurv)
                {
                    continue;
                }
            }
            EdgePtr edge = std::get<0>(item);
            return edge;
        }
        // 如何没有次小于的，直接取第一个
        auto& firstItem = items.at(0);
        return std::get<0>(firstItem);
    }

    void FindClosedRegion::clearSuspendEdges()
    {
        // 定义函数：从指定起始位置开始查找第一个未孤悬的边
        auto findFirstValidFunc = [&](std::vector<EdgePtr>::iterator startIt)
        {
            for (auto it = startIt; it != m_edges.end(); ++it)
            {
                if (!(*it)->isSuspend)
                {
                    return it;
                }
            }
            return m_edges.end();
        };

        // 清理操作：标记Edge::isSuspend
        bool foundSuspend = false;
        auto firstValidIt = m_edges.begin();
        auto it = m_edges.begin();
        while (it != m_edges.end())
        {
            auto edge = *it;
            if (edge->isSuspend)
            {
                ++it;
                continue;
            }
            auto indices = m_searchTree->getPointsWithinCube(edge->startPos, TOL);
            std::vector<size_t> validIndices; // 未判断为孤悬的边索引
            validIndices.reserve(indices.size());
            std::copy_if(indices.begin(), indices.end(), std::back_inserter(validIndices),
                         [&](const auto& idx){ return !m_edges.at(idx)->isSuspend; });
            // 找到一个条边孤悬，从这条边往下连续查找
            if (validIndices.size() == 1)
            {
                edge->isSuspend = true;
                edge->reverseEdge.lock()->isSuspend = true;
                foundSuspend = true;
                DmVector nextPos = edge->endPos;
                while (nextPos.valid)
                {
                    auto indices_chain_point = m_searchTree->getPointsWithinCube(nextPos, TOL);
                    std::vector<size_t> validIndices_chain_point; // 未判断为孤悬的边索引
                    validIndices_chain_point.reserve(indices_chain_point.size());
                    std::copy_if(indices_chain_point.begin(), indices_chain_point.end(),
                                 std::back_inserter(validIndices_chain_point),
                                 [&](const auto& idx){ return !m_edges.at(idx)->isSuspend; });
                    int size = static_cast<int>(validIndices_chain_point.size());
                    if (size == 1)
                    {
                        auto tmpEdge = m_edges.at(validIndices_chain_point.front());
                        tmpEdge->isSuspend = true;
                        tmpEdge->reverseEdge.lock()->isSuspend = true;
                        nextPos = tmpEdge->endPos;
                    }
                    else
                    {
                        nextPos.valid = false;
                    }
                }
                // 这条边链查找完成
            }

            // 找到并设置了孤悬边链，重新从开始的有效边查找
            if (foundSuspend)
            {
                it = findFirstValidFunc(firstValidIt);
                firstValidIt = it;
                foundSuspend = false;
            }
            else
            {
                ++it;
            }
        }
    }

    void FindClosedRegion::addAndSplitEntity(DmEntity* e)
    {
        // 组合实体特殊处理
        if (e->getEntityType() == DM::EntityPolyline)
        {
            auto subEnts = getSplitOfPolyline(static_cast<DmPolyline*>(e));
            for (auto subEnt : subEnts)
            {
                addAndSplitEntity_singleEnt(subEnt);
            }
        }
        // 圆须分割成弧
        else if (e->getEntityType() == DM::EntityCircle)
        {
            DmCircle* c = static_cast<DmCircle*>(e);
            DmVector center = c->getCenter();
            double r = c->getRadius();
            DmArcPtr arc1 = std::make_shared<DmArc>(nullptr, ArcData(center, DmVector(0.0, 0.0, 1.0), r, 0, M_PI));
            DmArcPtr arc2 = std::make_shared<DmArc>(nullptr, ArcData(center, DmVector(0.0, 0.0, 1.0), r, M_PI, 2 * M_PI));
            addAndSplitEntity_singleEnt(arc1);
            addAndSplitEntity_singleEnt(arc2);
        }
        // 圆弧的角度跟圆一样，分割
        else if (e->getEntityType() == DM::EntityArc && (M_PI * 2.0 - static_cast<DmArc*>(e)->getAngleLength()) < TOL)
        {
            DmArc* a = static_cast<DmArc*>(e);
            DmVector center = a->getCenter();
            double r = a->getRadius();
            DmVector normal = a->getNormal();
            DmArcPtr arc1 = std::make_shared<DmArc>(nullptr, ArcData(center, normal, r, 0, M_PI));
            DmArcPtr arc2 = std::make_shared<DmArc>(nullptr, ArcData(center, normal, r, M_PI, 2 * M_PI));
            addAndSplitEntity_singleEnt(arc1);
            addAndSplitEntity_singleEnt(arc2);
        }
        // 闭合（或接近闭合）椭圆须分割成弧
        else if (e->getEntityType() == DM::EntityEllipse
            && ((static_cast<DmEllipse*>(e)->isClosed() || (M_PI * 2.0 - static_cast<DmEllipse*>(e)->getAngleLength()) < TOL)))
        {
            DmEllipse* ellpise = static_cast<DmEllipse*>(e);
            DmVector center = ellpise->getCenter();
            DmVector major = ellpise->getMajorP();
            DmVector normal = ellpise->getNormal();
            double ratio = ellpise->getRatio();
            DmEllipsePtr e1 = std::make_shared<DmEllipse>(nullptr, EllipseData(center, major, normal, ratio, false, 0, M_PI));
            DmEllipsePtr e2 = std::make_shared<DmEllipse>(nullptr, EllipseData(center, major, normal, ratio, false, M_PI, 2 * M_PI));
            addAndSplitEntity_singleEnt(e1);
            addAndSplitEntity_singleEnt(e2);
        }
        // 样条特殊处理
        else if (e->getEntityType() == DM::EntitySpline)
        {
            DmSpline* spline = static_cast<DmSpline*>(e);
            auto res = DmSpline::getIntersection(spline, spline);
            res.distinct(TOL);
            // 自交
            if (res.size() > 0)
            {
                DmEntityPtr e_sp(e->clone());
                DmSpline* spline_clone = static_cast<DmSpline*>(e_sp.get());
                std::vector<DmEntityPtr> splitEnts = getSplitOfEntity(e_sp, res);
                for (auto splitEnt : splitEnts)
                {
                    auto splitSpline = std::dynamic_pointer_cast<DmSpline>(splitEnt);
                    auto subSplitEnts = getSplitOfClosedSpline(splitSpline);
                    for (auto subSpline : subSplitEnts)
                    {
                        addAndSplitEntity_singleEnt(subSpline);
                    }
                }
            }
            // 不自交
            else
            {
                DmEntityPtr e_sp(e->clone());
                DmSpline* spline_clone = static_cast<DmSpline*>(e_sp.get());
                auto splitSpline = std::dynamic_pointer_cast<DmSpline>(e_sp);
                auto subSplitEnts = getSplitOfClosedSpline(splitSpline);
                for (auto subSpline : subSplitEnts)
                {
                    addAndSplitEntity_singleEnt(subSpline);
                }
            }
        }
        // 非组合实体
        else
        {
            DmEntityPtr e_sp(e->clone());
            addAndSplitEntity_singleEnt(e_sp);
        }
    }

    void FindClosedRegion::addAndSplitEntity_singleEnt(DmEntityPtr entity)
    {
        // 对椭圆/圆法向转为正
        if (entity->getEntityType() == DM::EntityEllipse)
        {
            auto ell = std::dynamic_pointer_cast<DmEllipse>(entity);
            if (ell->isClockwise())
            {
                ell->setClockwise(false);
            }
        }
        else if (entity->getEntityType() == DM::EntityArc)
        {
            auto arc = std::dynamic_pointer_cast<DmArc>(entity);
            if (arc->isClockwise())
            {
                arc->setClockwise(false);
            }
        }

        // 做重叠处理
        auto clippedEnts = getSplitOfEntityForOverlap(entity);
        if (clippedEnts.size() == 0)
        {
            return;
        }

        // 求交并分解实体
        for (auto& e : clippedEnts)
        {
            addAndSplitEntity_singleOverlapClippedEnt(e);
        }
    }

    void FindClosedRegion::addAndSplitEntity_singleOverlapClippedEnt(DmEntityPtr e)
    {
        auto start1 = e->getStartpoint();
        auto end1 = e->getEndpoint();
        auto it = m_splitEnts.begin();
        std::vector<DmEntityPtr> allSplitEnts; // 存储新分解后的的实体
        DmVectorSolutions intersectionsForE; // 用来打断e的交点
        while (it != m_splitEnts.end())
        {
            auto ent = *it; // 在列表中的实体
            auto start2 = ent->getStartpoint();
            auto end2 = ent->getEndpoint();
            bool noNeedE = false;
            DmVectorSolutions res = Information::getIntersection(e.get(), ent.get(), true);
            res.distinct(TOL);
            bool success = false;
            if (res.size() > 0)
            {
                // 只取除端点外的交点
                std::vector<DmVector> validIntersectionsForEnt;
                for (auto p : res)
                {
                    double dist_b1 = p.distanceTo(start1);
                    double dist_e1 = p.distanceTo(end1);
                    double dist_b2 = p.distanceTo(start2);
                    double dist_e2 = p.distanceTo(end2);
                    // 此交点不在e端点上
                    if (dist_b1 > TOL && dist_e1 > TOL)
                    {
                        intersectionsForE.push_back(p);
                    }
                    // 此交点不在ent端点上
                    if (dist_b2 > TOL && dist_e2 > TOL)
                    {
                        validIntersectionsForEnt.push_back(p);
                    }
                }
                if (validIntersectionsForEnt.size() > 0)
                {
                    std::vector<DmEntityPtr> splitEnts = getSplitOfEntity(ent, validIntersectionsForEnt);
                    // 分解成功
                    if (splitEnts.size() > 0)
                    {
                        success = true;
                        allSplitEnts.insert(allSplitEnts.end(), splitEnts.begin(), splitEnts.end());
                    }
                }
            }
            // 分解成功后，删除原来的实体
            if (success)
            {
                it = m_splitEnts.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 将新获得的分解实体保存
        m_splitEnts.insert(m_splitEnts.end(), allSplitEnts.begin(), allSplitEnts.end());

        // 将e分解
        intersectionsForE.distinct(TOL);
        if (intersectionsForE.size() > 0) // 存在交点
        {
            auto splitEntsOfE = getSplitOfEntity(e, intersectionsForE);
            if (splitEntsOfE.size() > 0) // 分解成功
            {
                m_splitEnts.insert(m_splitEnts.begin(), splitEntsOfE.begin(), splitEntsOfE.end());
            }
            else
            {
                m_splitEnts.emplace_back(e);
            }
        }
        // 与其他实体不存在交点
        else
        {
            m_splitEnts.emplace_back(e);
        }
    }

    bool FindClosedRegion::isValidEntity(DmEntity* e)
    {
        switch (e->getEntityType())
        {
            case DM::EntityLine:
            case DM::EntityPolyline:
            case DM::EntityArc:
            case DM::EntityCircle:
            case DM::EntityEllipse:
            case DM::EntitySpline:
                return true;
            default:
                return false;
        }
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfPolyline(DmPolyline* poly)
    {
        std::vector<DmEntityPtr> splitEnts;
        int segmentCount = poly->getSegmentCount();
        double bulge = 0.0;
        DmVector pt1(false);
        DmVector pt2(false);
        DmVector res(false);
        DmVector temp(false);
        DmVector center(false);
        DmVector normal(false);
        double r = 0.0;
        double startAng = 0.0;
        double endAng = 0.0;

        for (int i = 0; i < segmentCount; i++)
        {
            poly->getSegmentInfoAt(i, bulge, pt1, pt2);
            if (bulge == 0.0)
            {
                splitEnts.emplace_back(new DmLine(pt1, pt2));
            }
            else
            {
                GeometryMethods::getArcInfo(pt1, pt2, bulge, center, r, startAng, endAng, normal);
                DmArc* arc = new DmArc(nullptr, ArcData(center, normal, r, startAng, endAng));
                splitEnts.emplace_back(arc);
            }
        }
        return splitEnts;
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfEntity(DmEntityPtr ent, const DmVectorSolutions& splitPts)
    {
        std::vector<DmEntityPtr> splitEnts;
        switch (ent->getEntityType())
        {
            case DM::EntityLine:
                splitEnts = getSplitOfLine(std::dynamic_pointer_cast<DmLine>(ent), splitPts);
                break;
            case DM::EntityArc:
                splitEnts = getSplitOfArc(std::dynamic_pointer_cast<DmArc>(ent), splitPts);
                break;
            case DM::EntityEllipse:
                splitEnts = getSplitOfEllipse(std::dynamic_pointer_cast<DmEllipse>(ent), splitPts);
                break;
            case DM::EntitySpline:
                splitEnts = getSplitOfSpline(std::dynamic_pointer_cast<DmSpline>(ent), splitPts);
                break;
            default:
                break;
        }
        return splitEnts;
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfLine(DmLinePtr line, const DmVectorSolutions& splitPts)
    {
        // 按到起点距离排序
        std::vector<DmEntityPtr> res;
        res.reserve(splitPts.size() + 1);
        DmVector b = line->getStartpoint();
        std::vector<double> dists;
        dists.reserve(splitPts.size() + 1);
        for (auto p : splitPts)
        {
            double dist = p.distanceTo(b);
            dists.emplace_back(dist);
        }
        dists.emplace_back(line->getLength());
        std::sort(dists.begin(), dists.end());

        // 生成分割的直线
        DmVector dir(line->getDirection1());
        bool isFirst = true;
        DmVector lastPt;
        for (auto dist : dists)
        {
            DmVector p = b + dir * dist;
            if (isFirst)
            {
                res.emplace_back(new DmLine(b, p));
                isFirst = false;
            }
            else
            {
                res.emplace_back(new DmLine(lastPt, p));
            }
            lastPt = p;
        }
        return res;
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfArc(DmArcPtr arc, const DmVectorSolutions& splitPts)
    {
        // 按到圆点的向量的方向角排序
        std::vector<DmEntityPtr> res;
        res.reserve(splitPts.size() + 1);
        DmVector center = arc->getCenter();
        DmVector normal = arc->getNormal();
        double r = arc->getRadius();
        std::vector<double> angles;
        angles.reserve(splitPts.size() + 1);
        for (auto p : splitPts)
        {
            double angle = (p - center).angle();
            angles.emplace_back(angle);
        }
        angles.emplace_back(arc->getEndAngle());
        std::sort(angles.begin(), angles.end());

        // 生成分割的圆弧
        bool isFirst = true;
        double lastAngle = 0.0;
        for (auto a : angles)
        {
            if (isFirst)
            {
                DmArc* subArc = new DmArc(nullptr, ArcData(center, normal, r, arc->getStartAngle(), a));
                res.emplace_back(subArc);
                isFirst = false;
            }
            else
            {
                DmArc* subArc = new DmArc(nullptr, ArcData(center, normal, r, lastAngle, a));
                res.emplace_back(subArc);
            }
            lastAngle = a;
        }
        return res;
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfEllipse(DmEllipsePtr ellipse, const DmVectorSolutions& splitPts)
    {
        // 按到中点的向量的方向角排序
        std::vector<DmEntityPtr> res;
        res.reserve(splitPts.size() + 1);
        DmVector center = ellipse->getCenter();
        DmVector normal = ellipse->getNormal();
        DmVector major = ellipse->getMajorP();
        DmVector majorVec = DmVector(major).normalize();
        double majorAngle = majorVec.angle();
        double ratio = ellipse->getRatio();
        std::vector<double> angles; // 相对于主轴的角度
        angles.reserve(splitPts.size() + 1);
        for (auto p : splitPts)
        {
            auto vec = (p - center).rotate(-majorAngle); // 转到以主轴为X轴的坐标系
            angles.emplace_back(vec.angle());
        }
        // 有时候endAngle为0，取比startAngle大的值
        double endAngle = ellipse->getEndAngle();
        if (endAngle < ellipse->getStartAngle())
        {
            endAngle += M_PI * 2;
        }
        angles.emplace_back(endAngle);
        std::sort(angles.begin(), angles.end());

        // 生成分割的椭圆弧
        bool isFirst = true;
        double lastPara = 0.0;
        for (auto a : angles)
        {
            double para = ellipse->angleToParam(a);
            if (std::abs(para) < TOL && std::abs(a - M_PI * 2) < TOL) // 角度2PI对应的参数计算出为0，参数转为2PI
            {
                para = M_PI * 2;
            }
            if (isFirst)
            {
                DmEllipse* subEllipse = new DmEllipse(nullptr, EllipseData(center, major, normal, ratio, false, ellipse->getStartParam(), para));
                res.emplace_back(subEllipse);
                isFirst = false;
            }
            else
            {
                DmEllipse* subEllipse = new DmEllipse(nullptr, EllipseData(center, major, normal, ratio, false, lastPara, para));
                res.emplace_back(subEllipse);
            }
            lastPara = para;
        }
        return res;
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfSpline(DmSplinePtr spline, const DmVectorSolutions& splitPts)
    {
        // 按点对应的参数排序
        std::vector<double> ts;
        ts.reserve(splitPts.size());
        for (auto p : splitPts)
        {
            // 求出离分割点最近的点，并得到相应的t
            auto closetItems = spline->getClosetPts(p);
            for (auto closetItem : closetItems)
            {
                DmVector closetPt = std::get<0>(closetItem);
                if (!closetPt.valid)
                {
                    continue;
                }
                double t = std::get<1>(closetItem);
                ts.emplace_back(t);
            }
        }
        std::sort(ts.begin(), ts.end(),
                  [](const auto& t1, const auto& t2){ return t1 < t2; }
        );
        // 提示词：有std::vector<double>，如何把里面数值接近的元素去掉一些，保留一个
        auto last = std::unique(ts.begin(), ts.end(), [](double t1, double t2)
        {
            return std::abs(t1 - t2) < TOL;
        });
        ts.erase(last, ts.end());

        // 生成分割样条线，逐个点插入
        std::vector<DmEntityPtr> res;
        res.reserve(splitPts.size() + 1);
        bool isFirst = true;
        DmEntity* lastSubEntity = nullptr; // 用一个点打断后剩下的后面那段
        for (const auto& t : ts)
        {
            DmEntity* spline1 = nullptr;
            DmEntity* spline2 = nullptr;
            if (isFirst)
            {
                bool success = DmSpline::cutForSpline(spline.get(), t, spline1, spline2);
                if (success)
                {
                    if (spline1)
                    {
                        res.emplace_back(spline1);
                    }
                    lastSubEntity = spline2;
                    isFirst = false;
                }
            }
            else
            {
                bool success = DmSpline::cutForSpline(static_cast<DmSpline*>(lastSubEntity), t, spline1, spline2);
                if (success)
                {
                    if (spline1)
                    {
                        res.emplace_back(spline1);
                    }
                    delete lastSubEntity;
                    lastSubEntity = spline2;
                }
            }
        }
        if (lastSubEntity)
        {
            res.emplace_back(lastSubEntity);
        }
        return res;
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfClosedSpline(DmSplinePtr spline)
    {
        // 判断是否闭合
        std::vector<DmEntityPtr> res;
        bool isClosed = false;
        if (spline->isClosed())
        {
            spline->changeCloseToNormal();
            isClosed = true;
        }
        else
        {
            auto startPt = spline->getStartpoint();
            auto endPt = spline->getEndpoint();
            if (startPt.distanceTo(endPt) < TOL)
            {
                isClosed = true;
            }
        }

        if (!isClosed)
        {
            res.emplace_back(spline);
            return res;
        }

        // 拆分
        double t1 = 0.0;
        double t2 = 0.0;
        spline->getDomainOfDefinition(t1, t2);
        double midT = (t1 + t2) / 2.0;
        DmEntity* spline1 = nullptr;
        DmEntity* spline2 = nullptr;
        bool success = DmSpline::cutForSpline(spline.get(), midT, spline1, spline2);
        if (success)
        {
            res.emplace_back(DmEntityPtr(spline1));
            res.emplace_back(DmEntityPtr(spline2));
        }
        return res;
    }

    std::vector<DmEntityPtr> FindClosedRegion::getSplitOfEntityForOverlap(DmEntityPtr e)
    {
        std::vector<DmEntityPtr> es{e}; // 准备按重叠处理的实体，及处理后剩下的实体
        DM::EntityType type = e->getEntityType();
        auto it = m_splitEnts.begin();
        while (it != m_splitEnts.end())
        {
            auto ent = *it; // 在列表中的实体
            if (ent->getEntityType() != type || ent->getEntityType() == DM::EntitySpline) // 类型不一致或样条线不处理
            {
                ++it;
                continue;
            }

            // 仅支持直线、圆弧、椭圆弧
            int i = 0;  // 这里不再使用迭代器，因为过程中es会发生改变，迭代器会失效
            while (i < static_cast<int>(es.size()))
            {
                auto ent2 = es.at(i); // 准备被重叠修剪的实体
                bool noNeed = false;
                if (type == DM::EntityLine)
                {
                    DmLinePtr l = std::dynamic_pointer_cast<DmLine>(ent);
                    DmLinePtr l2 = std::dynamic_pointer_cast<DmLine>(e);
                    getSplitOfLineForOverlap(l, l2, noNeed, es);
                }
                else if (type == DM::EntityArc)
                {
                    DmArcPtr a = std::dynamic_pointer_cast<DmArc>(ent);
                    DmArcPtr a2 = std::dynamic_pointer_cast<DmArc>(e);
                    getSplitOfArcForOverlap(a, a2, noNeed, es);
                }
                else if (type == DM::EntityEllipse)
                {
                    DmEllipsePtr a = std::dynamic_pointer_cast<DmEllipse>(ent);
                    DmEllipsePtr a2 = std::dynamic_pointer_cast<DmEllipse>(e);
                    getSplitOfEllipseForOverlap(a, a2, noNeed, es);
                }
                if (noNeed)
                {
                    es.erase(es.begin() + i);
                    if (es.empty()) // 没有剩余实体
                    {
                        break;
                    }
                    i--;
                }
                else
                {
                    ++i;
                }
            }
            if (es.size() == 0) // 此实体被覆盖，可丢弃
            {
                break;
            }
            ++it;
        }
        return es;
    }

    void FindClosedRegion::getSplitOfLineForOverlap(DmLinePtr l, DmLinePtr l2, bool& noNeedE2, std::vector<DmEntityPtr>& leftEnts)
    {
        noNeedE2 = false;
        double dir = l->getDirection1();
        double dir2 = l2->getDirection1();
        double angBetween = Math2d::correctAngle(dir - dir2);
        bool parallel = angBetween < TOL || M_PI * 2 - angBetween < TOL || std::abs(angBetween - M_PI) < TOL; // 同向/反向
        if (!parallel)
        {
            return;
        }
        DmVector a1 = l2->getStartpoint() - l->getStartpoint();
        DmVector dirVec(dir);
        double dist = DmVector::crossP(a1, dirVec).z; // e2到e的距离，有符号
        if (std::abs(dist) > TOL) // 距离不为0
        {
            return;
        }

        DmVector a2 = l2->getEndpoint() - l->getStartpoint();
        double d1 = dirVec.dotP(a1); // 有向距离
        double d2 = dirVec.dotP(a2);
        double len = l->getLength();
        bool isBetween1 = isBetween(0.0, len, d1);
        bool isBetween2 = isBetween(0.0, len, d2);
        // 分割l2，删除l包含部分，剩下的放到leftEnts
        // 重叠，l2有端点在l内（包括重合于端点）
        if (isBetween1 && isBetween2)
        {
            noNeedE2 = true;
        }
        // l2起点在l内, l2终点在外面
        else if (isBetween1)
        {
            if (d2 > len)
            {
                bool overlapByPoint = std::abs(d1 - len) < TOL; // 仅重叠于一个端点，不处理
                if (!overlapByPoint)
                {
                    leftEnts.emplace_back(std::make_shared<DmLine>(l->getEndpoint(), l2->getEndpoint()));
                    noNeedE2 = true;
                }
            }
            // d2<0
            else
            {
                bool overlapByPoint = std::abs(d1) < TOL; // 仅重叠于一个端点，不处理
                if (!overlapByPoint)
                {
                    leftEnts.emplace_back(std::make_shared<DmLine>(l->getStartpoint(), l2->getEndpoint()));
                    noNeedE2 = true;
                }
            }
        }
        // l2终点在l内, l2起点在外面
        else if (isBetween2)
        {
            if (d1 > len)
            {
                bool overlapByPoint = std::abs(d2 - len) < TOL; // 仅重叠于一个端点，不处理
                if (!overlapByPoint)
                {
                    leftEnts.emplace_back(std::make_shared<DmLine>(l2->getStartpoint(), l->getEndpoint()));
                    noNeedE2 = true;
                }
            }
            // d1<0
            else
            {
                bool overlapByPoint = std::abs(d2) < TOL; // 仅重叠于一个端点，不处理
                if (!overlapByPoint)
                {
                    leftEnts.emplace_back(std::make_shared<DmLine>(l2->getStartpoint(), l->getStartpoint()));
                    noNeedE2 = true;
                }
            }
        }
        else
        {
            // l2包含l，分割l2为两份
            if (d1 > len && d2 < 0.0)
            {
                leftEnts.emplace_back(std::make_shared<DmLine>(l2->getStartpoint(), l->getEndpoint()));
                leftEnts.emplace_back(std::make_shared<DmLine>(l->getStartpoint(), l2->getEndpoint()));
                noNeedE2 = true;
            }
            else if (d2 > len && d1 < 0.0)
            {
                leftEnts.emplace_back(std::make_shared<DmLine>(l2->getStartpoint(), l->getStartpoint()));
                leftEnts.emplace_back(std::make_shared<DmLine>(l->getEndpoint(), l2->getEndpoint()));
                noNeedE2 = true;
            }
            // 其他情况为l,l2分离，不处理
        }
    }

    void FindClosedRegion::getSplitOfArcForOverlap(DmArcPtr e, DmArcPtr e2, bool& noNeedE2, std::vector<DmEntityPtr>& leftEnts)
    {
        if (e->getCenter().distanceTo(e2->getCenter()) > TOL)
        {
            return;
        }
        if (std::abs(e->getRadius() - e2->getRadius()) > TOL)
        {
            return;
        }

        bool changed = false; // 法向是否转变
        if (e->isClockwise() != e2->isClockwise())
        {
            e2->setClockwise(e->isClockwise()); // 设置法向相同
            changed = true;
        }

        auto changeToOriginE2 = [&]()
        {
            if (changed) // 转向变回来
            {
                e2->setClockwise(!e2->isClockwise());
            }
        };

        DmVector center = e->getCenter();
        double r = e->getRadius();
        DmVector normal = e->getNormal();
        double start = e->getStartAngle();
        double end = e->getEndAngle();
        double start2 = e2->getStartAngle();
        double end2 = e2->getEndAngle();
        bool isBetween1 = Math2d::isAngleBetween(start2, start, end);
        bool isBetween2 = Math2d::isAngleBetween(end2, start, end);
        bool isBetween_mid = Math2d::isAngleBetween((start2 + end2) / 2.0, start, end); // 要判断圆弧中点角度也在范围外，避免[0,PI],[PI,2PI]判断为重合
        // 分割e2，删除e包含部分，剩下的放到leftEnts
        // 重叠，e2有端点在e内（包括重合于端点）
        if (isBetween1 && isBetween2)
        {
            if (isBetween_mid) // 重叠
            {
                noNeedE2 = true;
            }
            else    // 互补，仅在两端点重合，啥也不干
            {
                changeToOriginE2();
            }
        }
        // e2起点在e内, e2终点在外面
        else if (isBetween1)
        {
            bool overlapByPoint = Math2d::correctAngle(start2 - end) < TOL; // 仅重叠于一个端点，不处理
            if (overlapByPoint)
            {
                changeToOriginE2();
            }
            else
            {
                auto arc = std::make_shared<DmArc>(nullptr, ArcData(center, normal, r, e->getEndAngle(), e2->getEndAngle()));
                if (changed) // 转向变回来
                {
                    arc->setClockwise(!e2->isClockwise());
                }
                leftEnts.emplace_back(arc);
                noNeedE2 = true;
            }
        }
        // e2终点在e内, e2起点在外面
        else if (isBetween2)
        {
            bool overlapByPoint = Math2d::correctAngle(end2 - start) < TOL; // 仅重叠于一个端点，不处理
            if (overlapByPoint)
            {
                changeToOriginE2();
            }
            else
            {
                auto arc = std::make_shared<DmArc>(nullptr, ArcData(center, normal, r, e2->getStartAngle(), e->getStartAngle()));
                if (changed)
                {
                    arc->setClockwise(!e2->isClockwise());
                }
                leftEnts.emplace_back(arc);
                noNeedE2 = true;
            }
        }
        else
        {
            // e2包含e，分割e2为两份
            if (start2 < start && end2 > end)
            {
                auto arc1 = std::make_shared<DmArc>(nullptr, ArcData(center, normal, r, e2->getStartAngle(), e->getStartAngle()));
                auto arc2 = std::make_shared<DmArc>(nullptr, ArcData(center, normal, r, e->getEndAngle(), e2->getEndAngle()));
                if (changed)
                {
                    arc1->setClockwise(!e2->isClockwise());
                    arc2->setClockwise(!e2->isClockwise());
                }
                leftEnts.emplace_back(arc1);
                leftEnts.emplace_back(arc2);
                noNeedE2 = true;
            }
            // 其他情况为e,e2分离，不处理
            else
            {
                changeToOriginE2();
            }
        }
    }

    void FindClosedRegion::getSplitOfEllipseForOverlap(DmEllipsePtr e, DmEllipsePtr e2, bool& noNeedE2, std::vector<DmEntityPtr>& leftEnts)
    {
        if (e->getCenter().distanceTo(e2->getCenter()) > TOL)
        {
            return;
        }
        if (std::abs(e->getRatio() - e2->getRatio()) > TOL)
        {
            return;
        }
        bool isMajorEqual = e->getMajorP().distanceTo(e2->getMajorP()) < TOL;
        bool isMajorReverse = (e->getMajorP() + e2->getMajorP()).magnitude() < TOL;
        if (!isMajorEqual && !isMajorReverse)
        {
            return;
        }

        // 保证转向及主轴与e一致
        bool changed = false; // 法向是否改变
        bool isClosewise = e->isClockwise();
        if (e2->isClockwise() != isClosewise)
        {
            e2->setClockwise(isClosewise);
            changed = true;
        }
        auto changeToOriginE2 = [&]()
        {
            if (changed) // 转向变回来
            {
                e2->setClockwise(!e2->isClockwise());
            }
        };

        if (isMajorReverse) // 主轴设为一样
        {
            // 角度旋转PI
            double sPara = e2->getStartParam();
            double ePara = e2->getEndParam();
            sPara = Math2d::correctAngle(sPara + M_PI);
            ePara = Math2d::correctAngle(ePara + M_PI);
            e2->setStartParam(sPara);
            e2->setEndParam(ePara);
            e2->setMajorP(e->getMajorP());
        }

        // 分割
        DmVector center = e->getCenter();
        DmVector majorP = e->getMajorP();
        double r = e->getRatio();
        DmVector normal = e->getNormal();
        double start = e->getStartParam();
        double end = e->getEndParam();
        double start2 = e2->getStartParam();
        double end2 = e2->getEndParam();
        bool isBetween1 = Math2d::isAngleBetween(start2, start, end);
        bool isBetween2 = Math2d::isAngleBetween(end2, start, end);
        bool isBetween_mid = Math2d::isAngleBetween((start2 + end2) / 2.0, start, end); // 要判断椭圆弧中点角度也在范围外，避免[0,PI],[PI,2PI]判断为重合
        // 分割e2，删除e包含部分，剩下的放到leftEnts
        // 重叠，e2有端点在e内（包括重合于端点）
        if (isBetween1 && isBetween2)
        {
            if (isBetween_mid) // 重叠
            {
                noNeedE2 = true;
            }
            else    // 互补，仅在两端点重合，啥也不干
            {
                changeToOriginE2();
            }
        }
        // e2起点在e内, e2终点在外面
        else if (isBetween1)
        {
            bool overlapByPoint = Math2d::correctAngle(start2 - end) < TOL; // 仅重叠于一个端点，不处理
            if (overlapByPoint)
            {
                changeToOriginE2();
            }
            else
            {
                auto ell = std::make_shared<DmEllipse>(nullptr, EllipseData(center, majorP, normal, r, false, e->getEndParam(), e2->getEndParam()));
                if (changed) // 转向变回来
                {
                    ell->setClockwise(!e2->isClockwise());
                }
                leftEnts.emplace_back(ell);
                noNeedE2 = true;
            }
        }
        // e2终点在e内, e2起点在外面
        else if (isBetween2)
        {
            bool overlapByPoint = Math2d::correctAngle(end2 - start) < TOL; // 仅重叠于一个端点，不处理
            if (overlapByPoint)
            {
                changeToOriginE2();
            }
            else
            {
                auto ell = std::make_shared<DmEllipse>(nullptr, EllipseData(center, majorP, normal, r, false, e2->getStartParam(), e->getStartParam()));
                if (changed)
                {
                    ell->setClockwise(!e2->isClockwise());
                }
                leftEnts.emplace_back(ell);
                noNeedE2 = true;
            }
        }
        else
        {
            // e2包含e，分割e2为两份
            if (start2 < start && end2 > end)
            {
                auto ell1 = std::make_shared<DmEllipse>(nullptr, EllipseData(center, majorP, normal, r, false, e2->getStartParam(), e->getEndParam()));
                auto ell2 = std::make_shared<DmEllipse>(nullptr, EllipseData(center, majorP, normal, r, false, e->getEndParam(), e2->getEndParam()));
                if (changed)
                {
                    ell1->setClockwise(!e2->isClockwise());
                    ell2->setClockwise(!e2->isClockwise());
                }
                leftEnts.emplace_back(ell1);
                leftEnts.emplace_back(ell2);
                noNeedE2 = true;
            }
            // 其他情况为e,e2分离，不处理
            else
            {
                changeToOriginE2();
            }
        }
    }

    bool FindClosedRegion::isTwoSplinesEqual(DmSpline* e, DmSpline* e2)
    {
        if (e->getDegree() != e2->getDegree())
        {
            return false;
        }
        if (e->isClosed() != e2->isClosed())
        {
            return false;
        }
        if (e->getKnots() != e2->getKnots())
        {
            return false;
        }
        if (e->getControlPoints() != e2->getControlPoints())
        {
            return false;
        }
        return true;
    }

    bool FindClosedRegion::isBetween(double d1, double d2, double d)
    {
        if (d2 < d1)
        {
            std::swap(d1, d2);
        }
        if (d > d1 && d < d2)
        {
            return true;
        }
        else if (std::abs(d - d1) < TOL)
        {
            return true;
        }
        else if (std::abs(d - d2) < TOL)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool FindClosedRegion::isBetweenNoBoundary(double d1, double d2, double d)
    {
        if (d2 < d1)
        {
            std::swap(d1, d2);
        }
        if (std::abs(d - d1) < TOL)
        {
            return false;
        }
        else if (std::abs(d - d2) < TOL)
        {
            return false;
        }
        else if (d > d1 && d < d2)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

}
