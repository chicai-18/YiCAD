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


/// @file DmSpline.cpp
/// @brief DmSpline B样条曲线实体类的实现，包括求值、求交、拟合、最近点等

#include "DmSpline.h"

#include<iostream>
#include<cmath>
#include<numeric>

#include "DmLine.h"
#include "DmCircle.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "Information.h"
#include "Quadratic.h"

#include "Debug.h"
#include "GuiDocumentView.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "ConvexHull.h"
#include <Eigen/Dense>

// 迭代容差取为DM_TOLERANCE，与DmEntity::isPointOnEntity()默认容差一致，获取最近点才正常
constexpr double TOL = DM_TOLERANCE;
constexpr int MAX_ITERATIONS = 20;

using DmLinePtr = std::shared_ptr<DmLine>;
using namespace Eigen;

TYPESYSTEM_SOURCE(DmSpline, DmEntity, 0);

DmSpline::DmSpline(DmEntity* parent)
	: DmEntity(parent)
	, pLineStrip(new DmLineStrip(this))
{
}

DmSpline::DmSpline(DmEntity* parent, const SplineData& d)
	: DmEntity(parent)
	, pLineStrip(new DmLineStrip(this))
	, data(d)
{
	calculateBorders();
}

DmSpline::~DmSpline()
{
	if (!pLineStrip)
	{
		delete pLineStrip;
		pLineStrip = nullptr;
	}
}

DmEntity* DmSpline::clone() const
{
	DmSpline* l = new DmSpline(*this);
    l->m_ulID = DmId();
    l->setSelected(false);
    l->setHighlighted(false);
	l->pLineStrip = new DmLineStrip(l);
	l->update();
	return l;
}

DM::EntityType DmSpline::getEntityType() const
{
	return DM::EntitySpline;
}

SplineData DmSpline::getData() const
{
	return data;
}

void DmSpline::setData(const SplineData& d)
{
    data = d;
}

void DmSpline::calculateBorders()
{
	resetBorders();
	pLineStrip->calculateBorders();
	minV = pLineStrip->getMin();
	maxV = pLineStrip->getMax();
}

std::list<DmEntity*> DmSpline::getSubEntities() const
{
	return std::list<DmEntity*>();
}

DmLineStrip* DmSpline::getLineStrip() const
{
	return pLineStrip;
}

DmVectorSolutions DmSpline::getIntersection(const DmEntity* e1, const DmEntity* e2)
{
	DmVectorSolutions ret;
	if (e1->getEntityType() != DM::EntitySpline && e2->getEntityType() != DM::EntitySpline)
		return ret;

	if (e1->getEntityType() == DM::EntitySpline && e2->getEntityType() == DM::EntitySpline)
	{
		ret = getIntersectionSplineSpline(static_cast<const DmSpline*>(e1), static_cast<const DmSpline*>(e2));
	}
	else if(e1->getEntityType() == DM::EntitySpline)
	{
		ret = getIntersectionSplineOther(static_cast<const DmSpline*>(e1), e2);
	}
	else if (e2->getEntityType() == DM::EntitySpline)
	{
		ret = getIntersectionSplineOther(static_cast<const DmSpline*>(e2), e1);
	}

	return ret;
}

DmVectorSolutions DmSpline::getIntersectionSplineOther(const DmSpline* spline1, const DmEntity* e2)
{
    if(e2->getEntityType() == DM::EntityLine)
    {
        return getIntersectionSplineLine(spline1, (const DmLine*)e2);
    }
    else if(e2->getEntityType() == DM::EntityCircle)
    {
        return getIntersectionSplineCircle(spline1, (const DmCircle*)e2);
    }
    else if(e2->getEntityType() == DM::EntityArc)
    {
        return getIntersectionSplineArc(spline1, (const DmArc*)e2, false); //不限制在圆弧上
    }
    else if(e2->getEntityType() == DM::EntityEllipse)
    {
        return getIntersectionSplineEllipse(spline1, (const DmEllipse*)e2, false); //不限制在椭圆弧上
    }

    return DmVectorSolutions();
}

DmVectorSolutions DmSpline::getIntersectionSplineSpline(const DmSpline* spline1, const DmSpline* spline2)
{
    // 自交
    if(spline1 == spline2)
    {
        return getIntersectionSplineSpline_selfIntersection(spline1);
    }
    // 非自交
    else
    {
        return getIntersectionSplineSpline_intersectionOther(spline1, spline2);
    }

    return DmVectorSolutions();
}

DmVectorSolutions DmSpline::getIntersectionSplineSpline_selfIntersection(const DmSpline* spline1)
{
    DmVectorSolutions res;
    // 存在一次的情况特殊处理
    if(spline1->getDegree() == 1)
    {
        // 1阶曲线由直线构成，获得这些直线
        std::vector<DmLinePtr> lines = getLinesOfSplineDegree1(spline1);
        int size = (int)lines.size();
        for(int i=0;i<size-2;i++)
        {
            auto l1 = lines.at(i);
            for(int j=i+2;j<size;j++) //隔一条直线求交，相邻的直线不算交点
            {
                auto l2 = lines.at(j);
                auto tempRes = Information::getIntersectionLineLine(l1.get(),l2.get());
                res.push_back(tempRes);
            }
        }
        return res;
    }

    // 2次及以上的情况
    auto sections = spline1->getSectionsByCloseControlPoints();
    int size = (int)sections.size();
    for(int i=0;i<size-1;i++)
    {
        auto& sec1 = sections.at(i);
        DmVector t1Pt = spline1->evaluate(sec1.first);
        DmVector t2Pt = spline1->evaluate(sec1.second);
        for(int j=i+1;j<size;j++)
        {
            auto& sec2 = sections.at(j);
            DmVectorSolutions tempRes = getIntersectionSplineSpline_bySection(spline1, sec1.first, sec1.second, spline1, sec2.first, sec2.second);
            // 去除区间连接处的点（这不算交点）
            DmVectorSolutions tempRes_NoT1T2;
            for(auto v:tempRes)
            {
                // 连接的两段，区间连接处的点不算
                if(j==i+1 || (j==size-1 && i==0 && spline1->isClosed()))
                {
                    if(v.distanceTo(t1Pt) > TOL && v.distanceTo(t2Pt) > TOL)
                    {
                        tempRes_NoT1T2.push_back(v);
                    }
                }
                // 否则交点都算
                else
                {
                    tempRes_NoT1T2.push_back(v);
                }
            }
            res.push_back(tempRes_NoT1T2);
        }
    }
    res.distinct();
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineSpline_intersectionOther(const DmSpline* spline1, const DmSpline* spline2)
{
    DmVectorSolutions res;
    // 不能用于自交
    if(spline1==spline2)
        return res;

    // 存在一次的情况特殊处理
    if(spline1->getDegree() == 1)
    {
        // 1阶曲线由直线构成，获得这些直线
        std::vector<DmLinePtr> lines = getLinesOfSplineDegree1(spline1);
        for(const auto& l:lines)
        {
            auto tempRes = getIntersectionSplineLine(spline2, l.get());
            res.push_back(tempRes);
        }
        res.distinct();
        return res;

    }
    else if(spline2->getDegree() == 1)
    {
        std::vector<DmLinePtr> lines = getLinesOfSplineDegree1(spline2);
        for(const auto& l:lines)
        {
            auto tempRes = getIntersectionSplineLine(spline1, l.get());
            res.push_back(tempRes);
        }
        return res;
    }

    // 2次及以上的情况
    // 用最靠近控制点的点来分割区间，这种情况下区间与区间最多2个交点（个人推测）。
    // DmSpline::getSections()获得均分的区间，可能包含括号形状的区间，与另外括号形状的区间产生3（或4）个交点，不利于求出所有交点
    auto sections1 = spline1->getSectionsByCloseControlPoints();
    auto sections2 = spline2->getSectionsByCloseControlPoints();
    for(const auto& sec1: sections1)
    {
        for(const auto& sec2:sections2)
        {
            DmVectorSolutions tempRes = getIntersectionSplineSpline_bySection(spline1, sec1.first, sec1.second, spline2, sec2.first, sec2.second);
            res.push_back(tempRes);
        }
    }
    res.distinct();
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineSpline_bySection(const DmSpline* spline1, double t11, double t12, const DmSpline* spline2, double t21, double t22)
{
    // 先用关联控制点围成的凸包判断是否相交
    double midT1 = (t11+t12)/2.0;
    std::vector<DmVector> relativeControlPts1 = spline1->getControlPoints(midT1);
    ConvexHull controlPtsHull1(relativeControlPts1);
    double midT2 = (t21+t22)/2.0;
    std::vector<DmVector> relativeControlPts2 = spline2->getControlPoints(midT2);
    ConvexHull controlPtsHull2(relativeControlPts2);
    bool intersect = controlPtsHull1.intersects(controlPtsHull2);
    if(!intersect)
        return DmVectorSolutions();

    // 用区间端点为初始值进行牛顿迭代（不同的初始值可能得到不同的交点，最多2个（个人推测）。多个初始值可能获得相同的交点，或者获得不到）
    double maxStep1 = std::abs((t12-t11))/3.0;
    double maxStep2 = std::abs((t22-t21))/3.0;
    //auto v0 = newtonRaphsonBSplineSpline(spline1, midT1, spline2, midT2);
    auto v1 = newtonRaphsonBSplineSpline(spline1, t11, maxStep1, spline2, t21, maxStep2);
    auto v2 = newtonRaphsonBSplineSpline(spline1, t12, maxStep1, spline2, t21, maxStep2);
    auto v3 = newtonRaphsonBSplineSpline(spline1, t11, maxStep1, spline2, t22, maxStep2);
    auto v4 = newtonRaphsonBSplineSpline(spline1, t12, maxStep1, spline2, t22, maxStep2);
    auto list = {/*v0,*/v1,v2,v3,v4};
    DmVectorSolutions res;
    for(const auto& v:list)
    {
        DmVector pt = std::get<0>(v);
        double t1 = std::get<1>(v);
        double t2 = std::get<2>(v);
        if(pt.valid)    //有效并t在范围内
        {
            if((t1 >= t11 && t1 <= t12) &&
               (t2 >= t21 && t2 <= t22))
            {
                res.push_back(pt);
            }
        }
    }
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineLine(const DmSpline* spline1, const DmLine* line)
{
    DmVectorSolutions res;
    // 先判断包围框与样条线凸包是否相交
    bool isCross = isCrossBoundingBox(spline1, line->getMin(), line->getMax());
    if(!isCross)
        return res;

    // 一阶样条特殊处理
    if(spline1->getDegree() == 1)
    {
        return getIntersectionSplineEntity_ForDegree1(spline1, line);
    }

    // 获得候选点
    std::vector<double> approxTs; //候选点
    std::vector<std::pair<double, double>> sections_less180 = spline1->getSections();
    DmVector lineDir = DmVector(line->getDirection1());
    DmVector startPt = line->getStartpoint();
    ConvexHull boxHull(line->getMin(), line->getMax()); //直线包围框构成的凸包
    // 处理区间
    for(auto s:sections_less180)
    {
        double t_1=s.first;
        double t_2=s.second;
        double tempT = (t_1+t_2)/2.0;
        // 区间所在凸包与直线包围框不相交，则不继续计算
        std::vector<DmVector> relativeControlPts = spline1->getControlPoints(tempT);
        ConvexHull controlPtsHull(relativeControlPts);
        bool intersect = controlPtsHull.intersects(boxHull);
        if(!intersect)
            continue;

        // 直线可能与样条一个段区间有2个交点，找到切线与直线平行的点（对应的t），需要用这个t做分割
        double splitT;
        bool ret = getSplineTForLine(spline1, lineDir, t_1, t_2, splitT);
        std::vector<std::pair<double,double>> sections; //区间
        if(ret)
        {
            sections.emplace_back(t_1, splitT);
            sections.emplace_back(splitT, t_2);
        }
        else
        {
            sections.emplace_back(t_1, t_2);
        }
        //判断每个区间是否穿过直线两端，穿过则将中间点作为候选点
        for(auto sec:sections)
        {
            DmVector p1 = spline1->evaluate(sec.first);
            DmVector p2 = spline1->evaluate(sec.second);
            DmVector dir1 = (p1-startPt).normalize();
            DmVector dir2 = (p2-startPt).normalize();
            DmVector crossP1 = DmVector::crossP(dir1,lineDir);
            DmVector crossP2 = DmVector::crossP(lineDir,dir2);
            double val = crossP1.z * crossP2.z;
            if(val >= 0.0)
            {
                double t = (sec.first+sec.second)/2.0;
                approxTs.emplace_back(t);
            }
        }
    }

    //牛顿迭代法用候选点计算精确点
    for(auto t:approxTs)
    {
        DmVector pt = newtonRaphsonBSplineLine(spline1, startPt, lineDir, t);
        if(pt.valid)
        {
            res.push_back(pt);
        }
    }
    res.distinct();
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineEntity_ForDegree1(const DmSpline* spline1, const DmEntity* e)
{
    DmVectorSolutions res;
    if(spline1->getDegree()!=1)
        return res;

    // 1阶曲线由直线构成，获得这些直线
    std::vector<DmLinePtr> lines = getLinesOfSplineDegree1(spline1);

    // 用这些直线与指定直线求交
    for(auto l:lines)
    {
        // 参考Information::getIntersection，延长线的交点也算
        const auto qf1 = l->getQuadratic();
        const auto qf2 = e->getQuadratic();
        auto tempRes = Quadratic::getIntersection(qf1, qf2);
        //auto tempRes = Information::getIntersectionLineLine(l.get(), const_cast<DmLine*>(e));
        if(tempRes.size()>0)
        {
            res.push_back(tempRes);
        }
    }
    res.distinct();
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineCircle(const DmSpline* spline1, const DmCircle* circle)
{
    DmVectorSolutions res;
    // 先判断包围框与样条线凸包是否相交
    bool isCross = isCrossBoundingBox(spline1, circle->getMin(), circle->getMax());
    if(!isCross)
        return res;

    // 一阶样条特殊处理
    if(spline1->getDegree() == 1)
    {
        return getIntersectionSplineEntity_ForDegree1(spline1, circle);
    }

    // 获得候选点
    std::vector<double> approxTs; //候选点
    std::vector<std::pair<double, double>> sections_less180 = spline1->getSections();
    ConvexHull boxHull(circle->getMin(), circle->getMax()); //圆包围框构成的凸包
    // 处理区间
    for(auto s:sections_less180)
    {
        double t_1=s.first;
        double t_2=s.second;
        double tempT = (t_1+t_2)/2.0;
        // 区间所在凸包与圆包围框不相交，则不继续计算
        std::vector<DmVector> relativeControlPts = spline1->getControlPoints(tempT);
        ConvexHull controlPtsHull(relativeControlPts);
        bool intersect = controlPtsHull.intersects(boxHull);
        if(!intersect)
            continue;

        DmVector t1Pt = spline1->evaluate(t_1);
        DmVector t2Pt = spline1->evaluate(t_2);
        DmVector center = circle->getCenter();
        double radius = circle->getRadius();
        double distT1Center = t1Pt.distanceTo(center);
        double distT2Center = t2Pt.distanceTo(center);
        bool isT1Inside = distT1Center<=circle->getRadius();
        bool isT2Inside = distT2Center<=circle->getRadius();
        bool isT1T2SameSide = (isT1Inside && isT2Inside) || (!isT1Inside && !isT2Inside); //t1,t2同在圆的内侧或外侧
        std::vector<std::pair<double, double>> splitSections;   //分割后的区间
        if(isT1T2SameSide)// 同侧可能有2交点（需要分割）或无交点
        {
            double tanT;
            // 在样条一个区间(t1,t2)之间获得一个点(tanT)，这个点与圆心的连线与样条在t处切线垂直（或者该区间曲线经过圆心）
            bool getTan = getTangentTOfPoint(spline1, center, t_1, t_2, tanT);
            if(getTan) //[点与圆心连线]和切线垂直，或曲线经过圆心
            {
                DmVector tanPt = spline1->evaluate(tanT);
                double distToCenter = tanPt.distanceTo(center);
                if(distToCenter<TOL)    //曲线经过圆心
                {
                    // t1,t2均在圆内，且经过圆心，不会有交点
                    if(isT1Inside)
                    {
                        continue;
                    }
                    //t1,t2均在圆外，且经过圆心，有2个交点，分割区间
                    else
                    {
                        splitSections.emplace_back(t_1, tanT);
                        splitSections.emplace_back(tanT, t_2);
                    }
                }
                // [点与圆心连线]和切线垂直
                else
                {
                    bool isTanPtInside = distToCenter<radius;
                    bool isDiffSide = (isTanPtInside && !isT1Inside) || (!isTanPtInside && isT1Inside); //t1,t2与tanT分别在圆内外
                    if(isDiffSide) //有2个交点，分割区间
                    {
                        splitSections.emplace_back(t_1, tanT);
                        splitSections.emplace_back(tanT, t_2);
                    }
                    // 否则没有交点
                }
            }
            // 取不到tanT，且t1,t2均在圆内（或圆外），没有交点
        }
        //异侧则有且仅有1个交点
        else
        {
            splitSections.emplace_back(t_1, t_2);
        }

        // 分割后子区间中点作为候选点
        for(auto splitSec:splitSections)
        {
            double t = (splitSec.first+splitSec.second)/2.0;
            approxTs.emplace_back(t);
        }
    }

    //牛顿迭代法用候选点计算精确点
    for(auto t:approxTs)
    {
        DmVector pt = newtonRaphsonBSplineCircle(spline1, circle->getCenter(), circle->getRadius(), t);
        if(pt.valid)
        {
            res.push_back(pt);
        }
    }
    res.distinct();
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineArc(const DmSpline* spline1, const DmArc* arc, bool onArc /*= true*/)
{
    // 求对应圆与样条的交点
    DmVector center = arc->getCenter();
    double r = arc->getRadius();
    DmCircle circle(nullptr, CircleData(center, r));
    DmVectorSolutions res = getIntersectionSplineCircle(spline1, &circle);

    // 如果需要限制在圆弧上，再做筛选
    if(onArc)
    {
        DmVectorSolutions arcRes;
        for (const auto& pt: res) {
            if(arc->isPointOnEntity(pt))
            {
                arcRes.push_back(pt);
            }
        }
        return arcRes;
    }
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineEllipse(const DmSpline* spline1, const DmEllipse* ellipse, bool onEllipse /*= true*/)
{
    // 求对应椭圆(闭合)与样条的交点
    if(ellipse->isClosed())
    {
        DmVectorSolutions res = getIntersectionSplineEllipse_Closed(spline1, ellipse);
        return res;
    }
    DmEllipse ellipse_closed(nullptr, ellipse->getData());
    ellipse_closed.setStartAngle(0);
    ellipse_closed.setEndAngle(M_PI*2);
    ellipse_closed.setClosed(true);
    DmVectorSolutions res = getIntersectionSplineEllipse_Closed(spline1, &ellipse_closed);

    // 如果需要限制在椭圆弧上，再做筛选
    if(onEllipse)
    {
        DmVectorSolutions ellipseRes;
        for (const auto& pt: res) {
            if(ellipse->isPointOnEntity(pt))
            {
                ellipseRes.push_back(pt);
            }
        }
        return ellipseRes;
    }
    return res;
}

DmVectorSolutions DmSpline::getIntersectionSplineEllipse_Closed(const DmSpline* spline1, const DmEllipse* ellipse)
{
    DmVectorSolutions res;
    // 先判断包围框与样条线凸包是否相交
    bool isCross = isCrossBoundingBox(spline1, ellipse->getMin(), ellipse->getMax());
    if(!isCross)
        return res;

    // 一阶样条特殊处理
    if(spline1->getDegree() == 1)
    {
        return getIntersectionSplineEntity_ForDegree1(spline1, ellipse);
    }

    // 获得候选点
    std::vector<double> approxTs; //候选点
    std::vector<std::pair<double, double>> sections_less180 = spline1->getSections();
    ConvexHull boxHull(ellipse->getMin(), ellipse->getMax()); //圆包围框构成的凸包
    // 处理区间
    for(auto s:sections_less180)
    {
        double t_1=s.first;
        double t_2=s.second;
        double tempT = (t_1+t_2)/2.0;
        // 区间所在凸包与圆包围框不相交，则不继续计算
        std::vector<DmVector> relativeControlPts = spline1->getControlPoints(tempT);
        ConvexHull controlPtsHull(relativeControlPts);
        bool intersect = controlPtsHull.intersects(boxHull);
        if(!intersect)
            continue;

        DmVector t1Pt = spline1->evaluate(t_1);
        DmVector t2Pt = spline1->evaluate(t_2);
        DmVector center = ellipse->getCenter();
        bool isT1Inside = ellipse->isPointInside(t1Pt);
        bool isT2Inside = ellipse->isPointInside(t2Pt);
        bool isT1T2SameSide = (isT1Inside && isT2Inside) || (!isT1Inside && !isT2Inside); //t1,t2同在椭圆的内侧或外侧
        std::vector<std::pair<double, double>> splitSections;   //分割后的区间
        if(isT1T2SameSide)// 同侧可能有2交点（需要分割）或无交点
        {
            double tanT;
            // 在样条一个区间(t1,t2)之间获得一个点(tanT)，这个点与椭圆中心的连线与样条在t处切线垂直（或者该区间曲线经过椭圆中心）
            bool getTan = getTangentTOfPoint(spline1, center, t_1, t_2, tanT);
            if(getTan) //[点与椭圆中心连线]和切线垂直，或曲线经过椭圆心
            {
                DmVector tanPt = spline1->evaluate(tanT);
                double distToCenter = tanPt.distanceTo(center);
                if(distToCenter<TOL)    //曲线经过椭圆中心
                {
                    // t1,t2均在椭圆内，且经过椭圆中心，不会有交点
                    if(isT1Inside)
                    {
                        continue;
                    }
                        //t1,t2均在圆外，且经过圆心，有2个交点，分割区间
                    else
                    {
                        splitSections.emplace_back(t_1, tanT);
                        splitSections.emplace_back(tanT, t_2);
                    }
                }
                    // [点与椭圆中心连线]和切线垂直
                else
                {
                    bool isTanPtInside = ellipse->isPointInside(tanPt);
                    bool isDiffSide = (isTanPtInside && !isT1Inside) || (!isTanPtInside && isT1Inside); //t1,t2与tanT分别在椭圆内外
                    if(isDiffSide) //有2个交点，分割区间
                    {
                        splitSections.emplace_back(t_1, tanT);
                        splitSections.emplace_back(tanT, t_2);
                    }
                    // 否则没有交点
                }
            }
            // 取不到tanT，且t1,t2均在圆内（或圆外），没有交点
        }
            //异侧则有且仅有1个交点
        else
        {
            splitSections.emplace_back(t_1, t_2);
        }

        // 分割后子区间中点作为候选点
        for(auto splitSec:splitSections)
        {
            double t = (splitSec.first+splitSec.second)/2.0;
            approxTs.emplace_back(t);
        }
    }

    //牛顿迭代法用候选点计算精确点
    for(auto t:approxTs)
    {
        DmVector pt = newtonRaphsonBSplineEllipse(spline1, ellipse, t);
        if(pt.valid)
        {
            res.push_back(pt);
        }
    }
    res.distinct();
    return res;
}

bool DmSpline::getSplineTForLine(const DmSpline* spline1, const DmVector& lineDir, double t1, double t2, double& foundT)
{
    DmVector dir1 = spline1->derivative(t1).normalize();
    DmVector dir2 = spline1->derivative(t2).normalize();
    DmVector crossP1 = DmVector::crossP(dir1,lineDir);
    DmVector crossP2 = DmVector::crossP(lineDir,dir2);
    double res = crossP1.z * crossP2.z; //如果dir1转到dir2大于180度，绝对能找到切线与直线平行，但是res不一定大于0，所以dir1转到dir2不能大于180度
    // 找到与直线平行的切点
    if(std::abs(res) < TOL)
    {
        foundT = (t1+t2)/2.0;
        return true;
    }
    // 首尾切向穿过直线方向，有可能有2个交点，继续细分
    else if(res > 0)
    {
        double midT = (t1+t2)/2.0;
        bool found = getSplineTForLine(spline1, lineDir, t1, midT, foundT);
        if(found)
            return true;
        found = getSplineTForLine(spline1, lineDir, midT, t2, foundT);
        return found;
    }
    return false;
}

bool DmSpline::getSplineTForLimitAngles(const DmSpline* spline, bool isClockwise, const DmVector& boundryDir1, const DmVector& boundryDir2, double t1, double t2, double& midT)
{
    double t=(t1+t2)/2.0;
    DmVector dir = spline->derivative(t).normalize();
    double angleToBoundary1 = boundryDir1.angleTo(dir);
    if(isClockwise)
    {
        angleToBoundary1 = M_PI*2 - angleToBoundary1;
    }
    double angleToBoundary2=dir.angleTo(boundryDir2);
    if(isClockwise)
    {
        angleToBoundary2 = M_PI*2 - angleToBoundary2;
    }

    if(angleToBoundary1<M_PI && angleToBoundary2<M_PI)
    {
        midT = t;
        return true;
    }
    else if(std::abs(t2-t1)<TOL) //找不到
    {
        return false;
    }
    else if(angleToBoundary1>=M_PI)
    {
        return getSplineTForLimitAngles(spline, isClockwise, boundryDir1, boundryDir2, t1, t, midT);
    }
    else
    {
        return getSplineTForLimitAngles(spline, isClockwise, boundryDir1, boundryDir2, t, t2, midT);
    }
}

DmVector DmSpline::newtonRaphsonBSplineLine(const DmSpline* bspline, const DmVector& linePoint, const DmVector& lineDirection, double initialT)
{
    // 注意牛顿迭代法有时候不收敛，跟初始值选择有关，也跟函数方程有关，这里忽略不收敛的情况
    // TODO:如果出现问题(没求出来)，结合考虑用二分法（二分法需要边界函数值异号）
    DmVector normal(lineDirection.y, -lineDirection.x);
    normal = normal.normalize();
    double t = initialT;
    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        DmVector point = bspline->evaluate(t);
        DmVector deriv = bspline->derivative(t);

        double f = (point - linePoint).dotP(normal);
        double df = deriv.dotP(normal);
        if (std::abs(df) < TOL) {
            break; // 导数为零，无法继续迭代
        }
        double delta = -f / df;
        t += delta;

        // 保证t在定义域内
        t=bspline->getValidT(t);
        if (std::abs(delta) < TOL) {
            return { bspline->evaluate(t)};
        }
    }
    return DmVector(false);
}

DmVector DmSpline::newtonRaphsonBSplineCircle(const DmSpline* bspline, const DmVector& circleCenter, double radius, double initialT)
{
    double t = initialT;
    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        DmVector point = bspline->evaluate(t);
        DmVector deriv = bspline->derivative(t);
        DmVector vec = point - circleCenter;
        double dist = vec.magnitude();

        double f = dist - radius;
        double df = vec.dotP(deriv) / dist;
        if(std::abs(f) < TOL)   //在圆上
        {
            return point;
        }
        if (std::abs(df) < TOL) {
            break; // 导数为零，无法继续迭代
        }
        double delta = -f / df;
        t += delta;

        // 保证t在定义域内
        t=bspline->getValidT(t);
        if (std::abs(delta) < TOL) {
            return { bspline->evaluate(t)};
        }
    }
    return DmVector(false);
}

DmVector DmSpline::newtonRaphsonBSplineEllipse(const DmSpline* bspline, const DmEllipse* ellipse, double initialT)
{
    DmVector center = ellipse->getCenter();
    double a = ellipse->getMajorRadius();
    double b = ellipse->getMinorRadius();
    double t = initialT;
    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        DmVector point = bspline->evaluate(t);
        DmVector deriv = bspline->derivative(t);

        // 变换到椭圆局部坐标系
        DmVector localPt = ellipse->getLocalOfPoint(point);
        DmVector localDeriv = DmVector(deriv).rotate(-ellipse->getAngle());
        // 椭圆方程：F(x,y) = x²/a² + y²/b² - 1 = 0
        double x = localPt.x, y = localPt.y;
        double dx = localDeriv.x, dy = localDeriv.y;
        double f = (x * x) / (a * a) + (y * y) / (b * b) - 1.0;
        double df = (2 * x * dx) / (a * a) + (2 * y * dy) / (b * b);
        if (std::abs(df) < TOL) {
            break;
        }

        double delta = -f / df;
        t += delta;
        // 保证t在定义域内
        t=bspline->getValidT(t);
        if (std::abs(delta) < TOL && std::abs(f) < TOL) {
            return { bspline->evaluate(t)};
        }
    }
    return DmVector(false);
}

std::tuple<DmVector, double, double> DmSpline::newtonRaphsonBSplineSpline(const DmSpline* spline1, double initialT1, double maxStep1, const DmSpline* spline2, double initialT2, double maxStep2)
{
    double t1 = initialT1;
    double t2 = initialT2;
    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        DmVector point1 = spline1->evaluate(t1);
        DmVector point2 = spline2->evaluate(t2);

        DmVector deriv1 = spline1->derivative(t1);
        DmVector deriv2 = spline2->derivative(t2);
        // F(t1, t2) = curve1(t1) - curve2(t2) = 0
        DmVector F = point1 - point2;

        // 雅可比矩阵
        double J[2][2] = {
                {deriv1.x, -deriv2.x},
                {deriv1.y, -deriv2.y}
        };
        // 行列式
        double det = J[0][0] * J[1][1] - J[0][1] * J[1][0];
        if (std::abs(det) < TOL) {
            return {DmVector(false), t1, t2}; // 奇异矩阵
        }
        // 解线性系统
        double inv_det = 1.0 / det;
        double delta_t1 = -inv_det * (J[1][1] * F.x - J[0][1] * F.y);
        double delta_t2 = -inv_det * (-J[1][0] * F.x + J[0][0] * F.y);
        if(std::abs(delta_t1)>maxStep1)
        {
            delta_t1 = std::signbit(delta_t1) ? -maxStep1:maxStep1; //如果delta_t1为负，std::signbit(delta_t1)返回非0
        }
        if(std::abs(delta_t2)>maxStep2)
        {
            delta_t2 = std::signbit(delta_t2) ? -maxStep2:maxStep2; //如果delta_t1为负，std::signbit(delta_t1)返回非0
        }

        t1 += delta_t1;
        t2 += delta_t2;

        // 检查收敛
        if (std::abs(delta_t1) < TOL && std::abs(delta_t2) < TOL) {
            DmVector intersection = spline1->evaluate(t1);
            return {intersection, t1, t2};
        }
    }
    return {DmVector(false), t1, t2};
}

std::pair<DmVector, double> DmSpline::newtonRaphsonBSplinePoint(const DmSpline* bspline, const DmVector& pt, double t0, double maxStep)
{
    double t = t0;
    auto invalidRes = std::make_pair(DmVector(false), t);
    if(bspline->getDegree() <= 1) //样条变直线的情况，无法求(二次导数sec_deriv为0)
        return invalidRes;

    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        DmVector point = bspline->evaluate(t);
        DmVector deriv = bspline->derivative(t);
        DmVector sec_deriv = bspline->secondDerivative(t);

        // 条件1：点是否重合
        double dist = point.distanceTo(pt);
        bool con1 = dist<TOL;

        // 条件2：余弦值是否为0
        bool con2=false;
        DmVector C_minus_P = point - pt;
        double fac1 = deriv.dotP(C_minus_P);
        double fac2 = deriv.magnitude() * dist;
        if(fac2!=0.0)
        {
            con2 = std::abs(fac1/fac2) < TOL;
        }
        if(con1 || con2)
        {
            return { std::make_pair(bspline->evaluate(t), t) };
        }

        double f=deriv.dotP(C_minus_P);
        double deriv_f = sec_deriv.dotP(C_minus_P) + deriv.dotP(deriv);
        double delta = -f/deriv_f;
        if(std::abs(delta) > maxStep)
        {
            delta = std::signbit(delta)? -maxStep:maxStep;
        }
        t += delta;

        // 保证t在定义域内
        t=bspline->getValidT(t);
        // 条件4：参数变化不明显
        if (std::abs(delta) < TOL) {
            return { std::make_pair(bspline->evaluate(t), t)};
        }
    }
    return invalidRes; //无效值
}

bool DmSpline::isCrossBoundingBox(const DmSpline* bspline, const DmVector& min, const DmVector& max)
{
    ConvexHull splineHull(bspline->getControlPoints());
    ConvexHull boxHull(min, max);
    return splineHull.intersects(boxHull);
}

bool DmSpline::getTangentTOfPoint(const DmSpline* bspline, const DmVector& pt, double t1, double t2, double& t)
{
    if(bspline->getDegree() <= 1) //样条变直线的情况，无法求
        return false;
    double midT = (t1+t2)/2.0;
    double maxStep = std::abs(t2-t1)/3.0;
    auto findTanItem = newtonRaphsonBSplinePoint(bspline, pt, midT, maxStep);
    if(findTanItem.first.valid)
    {
        double resultT = findTanItem.second;
        if(t1<=resultT && resultT<=t2)
        {
            t = resultT;
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<DmLine>> DmSpline::getLinesOfSplineDegree1(const DmSpline* spline1)
{
    // 1阶曲线由直线构成，获得这些直线
    std::vector<std::shared_ptr<DmLine>> lines;
    int k = spline1->getDegree();
    if(k!=1)
        return lines;

    if(spline1->isClosed())
    {
        // 闭合的情况控制点补了k个
        int ctrlPtsCount = spline1->getNumberOfControlPoints() - k;
        for(int i=0;i<ctrlPtsCount;i++)
        {
            DmVector pt1(true), pt2(true);
            pt1 = spline1->getControlPointAt(i);
            if(i==ctrlPtsCount-1)
            {
                pt2 = spline1->getControlPointAt(0);
            }
            else
            {
                pt2 = spline1->getControlPointAt(i+1);
            }
            lines.emplace_back(std::make_shared<DmLine>(pt1, pt2));
        }
    }
    else
    {
        int ctrlPtsCount = spline1->getNumberOfControlPoints();
        for(int i=0;i<ctrlPtsCount-1;i++)
        {
            DmVector pt1(true), pt2(true);
            pt1 = spline1->getControlPointAt(i);
            pt2 = spline1->getControlPointAt(i+1);
            lines.emplace_back(std::make_shared<DmLine>(pt1, pt2));
        }
    }
    return lines;
}

bool DmSpline::cutForSpline(const DmVector& cutCoord, DmSpline* spline, DmEntity*& cut1, DmEntity*& cut2)
{
    // 闭合样条不支持单点分割
    if(spline->isClosed())
        return false;

    // 求出离分割点最近的点，并得到相应的t
    DmSplinePtr tempSpline(static_cast<DmSpline*>(spline->clone()));
    auto closetItem = tempSpline->getClosetPt(cutCoord);
    DmVector closetPt = std::get<0>(closetItem);
    if(!closetPt.valid)
    {
        return false;
    }
    double t =std::get<1>(closetItem);
    return cutForSpline(tempSpline.get(), t, cut1, cut2);
}

bool DmSpline::cutForSpline(DmSpline* spline, double t, DmEntity*& cut1, DmEntity*& cut2)
{
    // 闭合不支持打断
    if(spline->isClosed())
        return false;

    // deepseek提示词：如何分割b样条曲线？分割后的曲线与原b样条曲线形状一样，但是变成两条，给出c++代码。
    int k = spline->getDegree();
    // 插入节点直到达到最大重数
    for (int i = 0; i <= k; i++) {
        spline->insertKnot(t);
    }

    // 找到分割位置
    std::vector<double> knots = spline->getKnots();
    auto controlPoints = spline->getControlPoints();
    int splitIndex = -1;    //插入节点中最后那个索引
    for (int i = 0; i < knots.size() - 1; i++) {
        if (knots[i] == t && knots[i + 1] > t) {
            splitIndex = i;
            break;
        }
    }

    // 创建第一条曲线的控制点和节点向量
    std::vector<DmVector> leftControlPoints;
    std::vector<double> leftKnots;
    for (int i = 0; i < splitIndex - k; i++) {
        leftControlPoints.push_back(controlPoints[i]);
    }
    for (int i = 0; i <= splitIndex; i++) {
        leftKnots.push_back(knots[i]);
    }

    // 创建第二条曲线的控制点和节点向量
    std::vector<DmVector> rightControlPoints;
    std::vector<double> rightKnots;
    for (int i = splitIndex - k; i < controlPoints.size(); i++) {
        rightControlPoints.push_back(controlPoints[i]);
    }
    for (int i = splitIndex - k; i < knots.size(); i++) {
        rightKnots.push_back(knots[i]);
    }

    SplineData data1(k, false);
    DmSpline* spline1 = new DmSpline(nullptr, data1);
    spline1->setKnots(leftKnots);
    spline1->setControlPts(leftControlPoints);
    cut1 = spline1;
    spline1->update();
    SplineData data2(k, false);
    DmSpline* spline2 = new DmSpline(nullptr, data2);
    spline2->setKnots(rightKnots);
    spline2->setControlPts(rightControlPoints);
    cut2 = spline2;
    spline2->update();
    return true;
}

bool DmSpline::cutForSpline2P_Closed(const DmSpline* spline, const DmVector& firstCoord, const DmVector& secondCoord, DmEntity*& cut1, DmEntity*& cut2)
{
    if(!spline->isClosed())
        return false;

    // 求出离分割点最近的点，并得到相应的t
    std::shared_ptr<DmSpline> tempSpline(static_cast<DmSpline*>(spline->clone()));
    tempSpline->changeCloseToNormal();// 转为非闭合
    auto closetItem1 = tempSpline->getClosetPt(firstCoord);
    if(!std::get<0>(closetItem1).valid)
    {
        return false;
    }
    auto closetItem2 = tempSpline->getClosetPt(secondCoord);
    if(!std::get<0>(closetItem2).valid)
    {
        return false;
    }
    // 将两最近点的t排序
    DmVector closePt1 = std::get<0>(closetItem1);
    DmVector closePt2 = std::get<0>(closetItem2);
    double t1 = std::get<1>(closetItem1);
    double t2 = std::get<1>(closetItem2);
    if(std::abs(t1-t2)<DM_TOLERANCE)
        return false;
    if(t1>t2) //保证t1<t2
    {
        std::swap(t1, t2);
        std::swap(closePt1, closePt2);
    }
    double defT1, defT2;    //定义域
    tempSpline->getDomainOfDefinition(defT1, defT2);

    // t1与定义域起点/终点重合，t2与【定义域起点/终点】将样条分成2份
    if(std::abs(t1-defT1)<DM_TOLERANCE || std::abs(t1-defT2)<DM_TOLERANCE)
    {
        DmEntity* ent1 = nullptr;
        DmEntity* ent2 = nullptr;
        bool res1 = cutForSpline(closePt2, tempSpline.get(), ent1, ent2);
        if(!res1)
            return false;
        cut1=ent1;
        cut2=ent2;
        return true;
    }
    // t2与定义域起点/终点重合，t1与【定义域起点/终点】将样条分成2份
    else if(std::abs(t2-defT1)<DM_TOLERANCE || std::abs(t2-defT2)<DM_TOLERANCE)
    {
        DmEntity* ent1 = nullptr;
        DmEntity* ent2 = nullptr;
        bool res1 = cutForSpline(closePt1, tempSpline.get(), ent1, ent2);
        if(!res1)
            return false;
        cut1=ent1;
        cut2=ent2;
        return true;
    }
    // 一般情况
    else
    {
        DmEntity* ent1 = nullptr; //第一次分割产生的2个子样条
        DmEntity* ent2 = nullptr; //ent2第二次分割后须释放
        DmEntity* ent3 = nullptr; //第二次分割产生的2个子样条
        DmEntity* ent4 = nullptr;
        bool res1 = cutForSpline(closePt1, tempSpline.get(), ent1, ent2);
        if(!res1)
            return false;
        bool res2 = cutForSpline(closePt2, (DmSpline*)ent2, ent3, ent4);
        if(!res2)
        {
            delete ent1;
            delete ent2;
            return false;
        }
        // 剩下ent3，将ent1，ent4合并
        cut1 = ent3;    //cut1为准备删除的实体
        DmSpline* combineSpline = nullptr;
        DmSpline::combineTwoSplines((DmSpline*)ent4, (DmSpline*)ent1, combineSpline);
        cut2 = combineSpline;
        delete ent1;
        delete ent4;
        delete ent2;
        return true;
    }
}

bool DmSpline::combineTwoSplines(const DmSpline* spline1, const DmSpline* spline2, DmSpline*& resSpline)
{
    // 保证spline1在前面，与spline2连接于spline2起点
    // 拷贝
    int k1 = spline1->getDegree();
    int k2 = spline2->getDegree();
    if(k1!=k2)  //仅同阶进行合并
        return false;
    auto data1 = spline1->getData();
    auto data2 = spline2->getData();
    DmSpline newSpline1(nullptr, data1);
    DmSpline newSpline2(nullptr, data2);
    double t11,t12,t21,t22;
    newSpline1.getDomainOfDefinition(t11,t12);
    newSpline2.getDomainOfDefinition(t21,t22);
    DmVector beginPt1 = newSpline1.evaluate(t11);
    DmVector endPt1 = newSpline1.evaluate(t12);
    DmVector beginPt2 = newSpline2.evaluate(t21);
    DmVector endPt2 = newSpline2.evaluate(t22);
    if(beginPt1.distanceTo(beginPt2)<TOL)
    {
        newSpline1.reverse();
    }
    else if(beginPt1.distanceTo(endPt2)<TOL)
    {
        newSpline1.reverse();
        newSpline2.reverse();
    }
    else if(endPt1.distanceTo(beginPt2)<TOL)
    {
        //正确的连接方式
    }
    else if(endPt1.distanceTo(endPt2)<TOL)
    {
        newSpline2.reverse();
    }
    else
    {
        return false;
    }

    // 移动样条线2的节点向量
    double lastKnot1 = newSpline1.getKnotAt(newSpline1.getNumberOfKnots() - 1);
    double firstKnot2 = newSpline2.getKnotAt(0);
    auto newKnots1 = newSpline1.getKnots();
    auto newKnots2 = newSpline2.getKnots();
    auto newCtrlPoints1 = newSpline1.getControlPoints();
    auto newCtrlPoints2 = newSpline2.getControlPoints();
    if(lastKnot1 != firstKnot2)
    {
        double delta = firstKnot2 - lastKnot1;
        for(auto it = newKnots2.begin();it!=newKnots2.end();++it)
        {
            *it = *it - delta;
        }
        newSpline2.setKnots(newKnots2);
    }

    // 合并
    std::vector<DmVector> newCtrlPoints;
    newCtrlPoints.reserve(newSpline1.getNumberOfControlPoints() + newSpline2.getNumberOfControlPoints());
    newCtrlPoints.insert(newCtrlPoints.end(), newCtrlPoints1.begin(), newCtrlPoints1.end());
    newCtrlPoints.insert(newCtrlPoints.end(), newCtrlPoints2.begin(), newCtrlPoints2.end());
    std::vector<double> newKnots;
    double midKnot = newKnots1.back();
    newKnots.reserve(newKnots1.size() + newKnots2.size());
    newKnots.insert(newKnots.end(), newKnots1.begin(), newKnots1.end());
    newKnots.insert(newKnots.end(), newKnots2.begin() + k1 + 1, newKnots2.end());
    DmSpline conbineSpline(nullptr, SplineData(k1, false));
    conbineSpline.setKnots(newKnots);
    conbineSpline.setControlPts(newCtrlPoints);
    //
    //for(int i=0;i<k1;i++)
    //{
        //conbineSpline.removeKnot(midKnot);
    //}
    resSpline = new DmSpline(nullptr, conbineSpline.getData());
    resSpline->update();
    return true;
}

DmSpline* DmSpline::createSplineWithTwoPoints(const DmVector& p1, const DmVector& p2, int k)
{
    // 样条曲线控制点最少个数为k+1
    int n = k+1;
    int m=n+k+1;
    // 设置控制点
    std::vector<DmVector> controlPts;
    controlPts.resize(n+1);
    std::vector<double> knots;
    knots.resize(m+1, -1);
    int midPtsCount = n-1;
    controlPts.at(0)=p1;
    controlPts.at(n)=p2;
    DmVector deltaVec = p2-p1;
    DmVector stepVec = deltaVec/n;
    for(int i=1;i<n;i++)
    {
        DmVector p = p1+stepVec*i;
        controlPts.at(i)=p;
    }

    // 设置节点向量
    std::fill(knots.begin(),knots.begin()+k,0); //填充前k个
    int i=0;
    for(i=0;i<m+1-k*2;i++) //中间m+1-2k个值
    {
        knots.at(k+i)=i;
    }
    i--;
    std::fill(knots.begin()+n+1,knots.end(),i);

    SplineData d(k, false);
    DmSpline* spline = new DmSpline(nullptr, std::move(d));
    spline->setControlPts(controlPts);
    spline->setKnots(knots);
    return spline;
}

bool DmSpline::getFitKnots(bool close, const std::vector<DmVector>& fitPoints, std::vector<double>& knots, std::vector<double>& u_waves)
{
    // 若数据点数<=4，则采用一般的三次B样条插值曲线(参考：《计算机辅助几何设计与非均匀有理B样条  修订版》P304)，这里暂不处理
    int fitPointsCount = (int)fitPoints.size();
    if(fitPointsCount<4)
        return false;

    int n = fitPointsCount - 1;
    knots.clear();
    u_waves.clear();
    constexpr int k = 3;
    // 闭曲线
    if(close)
    {
        // 闭合情况，第0个数据点与第n个数据点相同。这里仅获得前n个数据点对应的节点（用于后续矩阵计算）
        // 数据点对应的节点取定义域各段端点
        // 据此，设拟合点个数为m+1(此处m与节点矢量“m+1”的m不一样)，对应有n+1个控制点（n=m+2），节点矢量为[u(0),...u(n+k+1)]，曲线定义域为[u(k),u(n+1)]。
        // 参考《计算机辅助几何设计与非均匀有理B样条 -- 施法中2001》P256，关于闭合情况，定义域之外的节点向量取值。
        // 参考《计算机辅助几何设计与非均匀有理B样条 CAGD & NURBS -- 施法中编著1994》P275，关于闭合情况，求解控制点的矩阵，注意这里n+1个数据点，控制点个数为n+3，节点矢量为[u(0),...u(n+6)]，定义域为[u(3),u(n+3)]

        // 求数据点对应的节点（即u波浪线），参考《非均匀有理B样条  第2版》P256,弦长参数化
        u_waves.reserve(n+1);
        u_waves.emplace_back(0.0);
        DmVector lastPt = fitPoints.front();
        double totalDist = 0.0;
        for(auto it=fitPoints.begin()+1;it!=fitPoints.end();++it) //迭代n次，总共n+1个节点
        {
            DmVector pt = *it;
            double dist = pt.distanceTo(lastPt);
            totalDist+=dist;
            u_waves.emplace_back(totalDist);
            lastPt = pt;
        }
        // 获得节点向量
        // 定义域内的一些节点
        double u_3 = u_waves.at(0);//0.0
        double u_4 = u_waves.at(1);
        double u_5 = u_waves.at(2);
        double u_6 = u_waves.at(3);
        double u_n_3 = u_waves.at(n); //u(n+3)
        double u_n_2 = u_waves.at(n-1);
        double u_n_1 = u_waves.at(n-2);
        double u_n = u_waves.at(n-3);
        // 计算定义域之外的点
        double u_0 = u_n - totalDist;
        double u_1 = u_n_1 - totalDist;
        double u_2 = u_n_2 - totalDist;
        double u_n_4 = totalDist + u_4; //u(n+4)
        double u_n_5 = totalDist + u_5;
        double u_n_6 = totalDist + u_6;
        knots.reserve(n+7); // n+7个
        knots.emplace_back(u_0);
        knots.emplace_back(u_1);
        knots.emplace_back(u_2);
        knots.insert(knots.end(), u_waves.begin(), u_waves.end());
        knots.emplace_back(u_n_4);
        knots.emplace_back(u_n_5);
        knots.emplace_back(u_n_6);
        // 偏移
        double offset = -u_0;
        for(auto i = 0;i<knots.size();i++)
        {
            knots.at(i) = knots.at(i) + offset;
        }
    }
    // 开曲线
    else
    {
        // 由弦长参数化计算数据点节点，采用平均法计算节点向量
        // 据此，控制点及数据点个数均为n+1（由定义域[t(k),t(n+1)]知道中间有n+1-k段，n+2-k节点，数据点对应的节点(u(0)...u(n))分布在这个区间（有n+1个数据点），所以至少有1个段是分布有2个数据点），

        // 求数据点对应的节点（即u波浪线），参考《非均匀有理B样条  第2版》P256,弦长参数化
        u_waves.reserve(n+1);
        u_waves.emplace_back(0.0);
        DmVector lastPt = fitPoints.front();
        double totalDist = 0.0;
        for(auto it=fitPoints.begin()+1;it!=fitPoints.end();++it) //迭代n次，总共n+1个节点
        {
            DmVector pt = *it;
            double dist = pt.distanceTo(lastPt);
            totalDist+=dist;
            u_waves.emplace_back(totalDist);
            lastPt = pt;
        }
        // 计算节点向量，按平均法，参考《非均匀有理B样条  第2版》P257
        knots.reserve(n+7); // n+7个
        knots.insert(knots.begin(), k+1, 0.0);
        for(int j=1;j<=n-3;j++)
        {
            int idx = j+k;
            double sum = 0.0;
            for(int i=j;i<=j+k-1;i++)
            {
                sum +=u_waves.at(i);
            }
            double ave = sum / k;
            knots.emplace_back(ave);
        }
        double lastUWave = u_waves.back();
        knots.insert(knots.end(),k+1 ,lastUWave);
    }
    return true;
}

void DmSpline::setControlPointsKnotsByClose(DmSpline* spline, bool isClosed, const std::vector<DmVector>& ctrlPts)
{
    std::vector<DmVector> controlPts = ctrlPts;
    int k = spline->getDegree();
    int n = (int)controlPts.size();
    // 闭合，采用均匀节点。参考《计算机辅助几何设计与非均匀有理B样条  修订版》P251，其中r=1，注意书中最后节点dm=d0，补上控制点直到n+k-1个(补k-1个)。
    // 此处不同dm!=d0，所以书中是补k-1个点，这里k个
    if(isClosed)
    {
        for(int i=0;i<k;i++)
        {
            controlPts.emplace_back(controlPts.at(i%n));
        }
    }
    spline->setControlPts(controlPts);
    spline->setClosed(isClosed);

    // 设置节点矢量
    n = spline->getNumberOfControlPoints() - 1;
    std::vector<double> knots;
    if(spline->isClosed())
    {
        getKnotsOfUniform(n, k, knots);
    }
    else
    {
        getKnotsOfQuasiUniform(n, k, knots);
    }
    spline->setKnots(knots);
    spline->update();
}

void DmSpline::getKnotsOfQuasiUniform(int n, int k, std::vector<double>& knots)
{
    knots.clear();
    knots.resize(k, 0.0);
    int count = n-k+2;
    for(int i=0;i<count;i++)
    {
        knots.emplace_back((double)i);
    }
    knots.insert(knots.end(), k, knots.back());
}

void DmSpline::getKnotsOfUniform(int n, int k, std::vector<double>& knots)
{
    knots.clear();
    int m = n + k + 1; //节点个数m+1
    knots.reserve(m+1);
    for(int i=0;i<m+1;i++)
    {
        knots.emplace_back(i);
    }
}

std::vector<DmLine*> DmSpline::test(DmSpline* spline)
{
    std::vector<DmLine*> lines;
    if(spline->isValid())
    {
        int sCount = spline->getSegmentCount();
        double t1, t2;
        spline->getDomainOfDefinition(t1, t2);
        DmColor c1(255,0,0), c2(0,255,0), c3(0,0,255);
        std::vector<DmPen> pens{DmPen(c1, DM::Width00, DmLineTypeTable::ByLayer), DmPen(c2, DM::Width00, DmLineTypeTable::ByLayer), DmPen(c3, DM::Width00, DmLineTypeTable::ByLayer)};
        int k = spline->getDegree();
        int c = k + 1;
        double step = (t2-t1)/sCount;
        double t=t1;
        double len = 20;
        for(int i=0;i<sCount;i++)
        {
            DmPen p = pens.at(i%pens.size());
            for(int j=0;j<c;j++)
            {
                double tempT = t+ j*step/c;
                DmVector pt = spline->evaluate(tempT);
//                        auto dpt = new DmPoint(preview->getEntityContainer(), PointData(pt));
//                        dpt->setPen(p);
//                        preview->addEntity(dpt);

                DmVector dir = spline->derivative(tempT);
                dir = dir.normalize();
//                        DmVector p1 = pt+dir*len;
//                        DmVector p2 = pt-dir*len;
//                        auto l = new DmLine(preview->getEntityContainer(), p1,p2);
//                        l->setPen(p);
//                        preview->addEntity(l);

                dir.rotate(Math2d::deg2rad(90.0));
                DmVector p3 = pt+dir*len;
                DmVector p4 = pt-dir*len;
                auto l2 = new DmLine(nullptr, p3,p4);
                l2->setDocument(spline->getDocument());
                l2->setPen(p);
                lines.emplace_back(l2);
            }
            t+=step;
        }

        double len2 = 5;
        DmPen pen2(DmColor(255,255,0), DM::Width00, DmLineTypeTable::ByLayer);
        //auto sections=spline->getSections();
        auto sections=spline->getSectionsByCloseControlPoints();
        for(auto sec:sections)
        {
            std::vector<double> ts{sec.first, sec.second};
            for(auto tempT:ts)
            {
                DmVector pt = spline->evaluate(tempT);
                DmVector dir = spline->derivative(tempT);
                dir = dir.normalize();
                dir.rotate(Math2d::deg2rad(90.0));
                DmVector p3 = pt+dir*len2;
                DmVector p4 = pt-dir*len2;
                auto l2 = new DmLine(nullptr, p3,p4);
                l2->setDocument(spline->getDocument());
                l2->setPen(pen2);
                lines.emplace_back(l2);
            }
        }
    }
    return lines;
}

void DmSpline::setDegree(size_t deg)
{
	if (deg >= 1 && deg <= 3)
	{
		data.setDegree(deg);
	}
}

int DmSpline::getDegree() const
{
	return data.getDegree();
}

int DmSpline::getNumberOfKnots()
{
	return data.getKnotsSize();
}

double DmSpline::getKnotAt(int i) const
{
    return data.getKnotAt(i);
}

size_t DmSpline::getNumberOfControlPoints() const
{
	return data.getControlPointsSize();
}

std::vector<double> DmSpline::getKnots() const
{
    return data.getKnots();
}

bool DmSpline::isContainer() const
{
	return false;
}

bool DmSpline::isClosed() const
{
	return data.getIsClosed();
}

void DmSpline::setClosed(bool c)
{
	data.setIsClosed(c);
}

void DmSpline::setKnots(const std::vector<double>& knots)
{
    data.setKnots(knots);
}

DmVectorSolutions DmSpline::getRefPoints() const
{
    if(data.getSplineType() == ESplineType::eControlPoints)
    {
        return DmVectorSolutions(data.getControlPoints());
    }
    else
    {
        return DmVectorSolutions(data.getFitPoints());
    }
}

DmVector DmSpline::getNearestRef(const DmVector& coord, double* dist /*= nullptr*/) const
{
	return DmEntity::getNearestRef(coord, dist);
}

DmVector DmSpline::getNearestSelectedRef(const DmVector& coord, double* dist /*= nullptr*/) const
{
	return DmEntity::getNearestSelectedRef(coord, dist);
}

DmVector DmSpline::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity) const
{
    double minDist = DM_MAXDOUBLE;
    auto closetPtItem = getClosetPt(coord);
    DmVector closetPt = std::get<0>(closetPtItem);
    if(closetPt.valid)
    {
        minDist = closetPt.distanceTo(coord);
    }
    if (entity)
    {
        *entity = const_cast<DmSpline*>(this);
    }
    if(dist)
    {
        *dist = minDist;
    }
    return closetPt;
}

//void DmSpline::setVisible(bool v)
//{
//	DmEntity::setVisible(v);
//	pLineStrip->setVisible(v);
//}
//
//bool DmSpline::setSelected(bool select)
//{
//	if (DmEntity::setSelected(select))
//	{
//		pLineStrip->setSelected(select);
//		return true;
//	}
//	else
//	{
//		return false;
//	}
//}
//
//void DmSpline::setHighlighted(bool highlight)
//{
//	DmEntity::setHighlighted(highlight);
//	pLineStrip->setHighlighted(highlight);
//}


double DmSpline::getDirection1() const
{
    double t1,t2;
    getDomainOfDefinition(t1,t2);
    return derivative(t1).angle();
}

double DmSpline::getDirection2() const
{
    double t1,t2;
    getDomainOfDefinition(t1,t2);
    return Math2d::correctAngle2(derivative(t2).angle() + M_PI);
}

void DmSpline::update()
{
	pLineStrip->clear();

    if(!isValid())
        return;

    std::vector<DmVector> pts;
    getPoints(pts);

	pLineStrip->setPoints(pts);
	pLineStrip->setClosed(isClosed());
	pLineStrip->update();
    calculateBorders();
}

void DmSpline::getPoints(std::vector<DmVector>& pts, bool reverse /*= false*/)
{
    int startIdx = (int)pts.size();
    int n = data.getControlPointsSize() - 1; // n+1个控制点
    int k = data.getDegree();   //k次B样条曲线
    // 包含n+k+1个节点区间，即t的范围为[t0,t1,...,t(m+k+1)]。节点个数(c+1)与n,k应满足:c = n+k+1
    // B样条曲线的定义域为[tk, t(n+1)]
    int segmentCount = getSegmentCount();   //n-k+1段B样条曲线
    for(int i=0;i<segmentCount;i++)
    {
        double t1 = data.getKnotAt(k+i);
        double t2 = data.getKnotAt(k+i+1);
        if(std::abs(t2-t1)<TOL)  //重复的节点
            continue;
        // 1次曲线在连接处不可导，直接取控制点
        if(k==1)
        {
            pts.emplace_back(data.getControlPointAt(i));
        }
            // 2次及以上，递归求点（点的切线夹角变化不大于某个值）
        else
        {
            int count = k*5;    //一段B样条分成多段，计算对应的点坐标，每个控制点之间的段至少分5段，防止S型变直线
            double step = (t2 - t1) / (double)count;
            double t = t1;
            for(int j=0;j<count;j++)
            {
                double tmpT1 = t1+step*j;
                double tmpT2 = t1+step*(j+1);
                getPointsRecursive(tmpT1, tmpT2, (double)count, pts, step/100.0);
            }
        }

        //最后一段连接上
        if(i == segmentCount - 1)
        {
            // 闭合的情况采用均匀B样条，最后一个点通过t计算正确
            if(isClosed())
            {
                DmVector pt = evaluate(t2);   //对于闭合情况上边界获得的点正确
                pts.emplace_back(pt);
            }
                // 非闭合的情况采用准均匀B样条，计算时应算作最后一个定义域区间，且0阶基函数计算值为1.0，但通过basisFunctionValue()获得0.0值，
                // 这里不修改basisFunctionValue，直接采用最后一个控制点
            else
            {
                DmVector lastPt = data.getControlPointAt(data.getControlPointsSize()-1);
                pts.emplace_back(lastPt);
            }
        }
    }

    if(reverse)
    {
        std::reverse(pts.begin()+startIdx, pts.end());
    }
}

void DmSpline::getPointsRecursive(double t1, double t2, double count, std::vector<DmVector>& pts, double maxStep) const
{
    if(std::abs(t2-t1)<maxStep)
    {
        DmVector pt = evaluate(t1);
        pts.emplace_back(pt);
        return;
    }
    DmVector v1 = derivative(t1);
    DmVector v2 = derivative(t2);
    double angle = Math2d::correctAngle2(v1.angleToDir(v2));
    angle = std::abs(angle); // 俩向量夹角(<=PI)，外部分段保证满足此条件
    constexpr double MaxDelta = M_PI / 60.0; //3度
    if(angle>MaxDelta)
    {
        double step = (t2-t1) / count;
        for(int i=0; i < count; i++)
        {
            double tmpT1 = t1+step*i;
            double tmpT2 = t1+step*(i+1);
            getPointsRecursive(tmpT1, tmpT2, count, pts, maxStep);
        }
    }
    else
    {
        DmVector pt = evaluate(t1);
        pts.emplace_back(pt);
    }
}

double DmSpline::basisFunctionValue(double t, int i, int k) const
{
    // 参考《计算几何算法与实现 _ Visual C++版- 孔令德 》 P155
    // 通过de Boor地推定义计算
    double v1, v2, v;
    if(k == 0)
    {
        if(t>= data.getKnotAt(i) && t < data.getKnotAt(i+1))
        {
            return 1.0;
        }
        else
        {
            return 0.0;
        }
    }
    if(k > 0)
    {
        if(t < data.getKnotAt(i) || t > data.getKnotAt(i+k+1))
        {
            return 0.0;
        }
        else
        {
            double c1, c2;
            double d = data.getKnotAt(i+k) - data.getKnotAt(i);
            if(0.0 == d)        //约定0/0
            {
                c1 = 0.0;
            }
            else
            {
                c1 = (t - data.getKnotAt(i)) / d;   //Fi,k-1(t)
            }

            d = data.getKnotAt(i+k+1) - data.getKnotAt(i+1);
            if(0.0 == d)
            {
                c2 = 0.0;
            }
            else
            {
                c2 = (data.getKnotAt(i+k+1) - t) / d;   //Fi+1,k-1(t)
            }
            v1 = c1 * basisFunctionValue(t, i, k-1);
            v2 = c2 * basisFunctionValue(t, i+1, k-1);
            v = v1 + v2;
        }
    }
    return v;
}

double DmSpline::basisDerivative(double t, int i, int k) const
{
    // 参考《非均匀有理B样条  第2版》P42，式2.7
    double N1 = basisFunctionValue(t, i, k-1);
    double N2 = basisFunctionValue(t, i+1, k-1);
    double deno1 = data.getKnotAt(i+k) - data.getKnotAt(i);
    double deno2 = data.getKnotAt(i+k+1) - data.getKnotAt(i+1);
    double a1 = 0.0;
    if(deno1!=0.0)
    {
        a1 = 1.0 / deno1;
    }
    double a2 = 0.0;
    if(deno2!=0.0)
    {
        a2 = 1.0 / deno2;
    }
    double val = (a1*N1 - a2*N2) * (double)k;
    return val;
}

double DmSpline::basisDerivativeQ(double t, int i, int k, int q) const
{
    // 参考《非均匀有理B样条  第2版》P44，式2.9
    if(q==1)
    {
        return basisDerivative(t,i,k);
    }
    else
    {
        double N1 = basisDerivativeQ(t, i, k-1, q-1);
        double N2 = basisDerivativeQ(t, i+1, k-1, q-1);
        double deno1 = data.getKnotAt(i+k) - data.getKnotAt(i);
        double deno2 = data.getKnotAt(i+k+1) - data.getKnotAt(i+1);
        double a1 = 0.0;
        if(deno1!=0.0)
        {
            a1 = 1.0 / deno1;
        }
        double a2 = 0.0;
        if(deno2!=0.0)
        {
            a2 = 1.0 / deno2;
        }
        double val = (a1*N1 - a2*N2) * (double)k;
        return val;
    }
}

DmVector DmSpline::derivativeQth(double t, int q) const
{
    // 参考《非均匀有理B样条  第2版》P66，式3.3
    double t1, t2;
    getDomainOfDefinition(t1, t2);
    if(t<(t1-TOL) || t> (t2+TOL))
        return DmVector(false);
    if(t >= t2 - TOL) //靠近上边界
    {
        t-=TOL; //上边界避免求出为0
    }

    int n = data.getControlPointsSize() - 1;
    int k = data.getDegree();
    DmVector res(true);
    for(int i=0;i<=n;i++)
    {
        DmVector pt = data.getControlPointAt(i);
        double N = basisDerivativeQ(t, i, k, q);
        res+=(pt*N);
    }
    return res;
}

DmVector DmSpline::evaluate(double t) const
{
    double tStart, tEnd;
    getDomainOfDefinition(tStart, tEnd);
    if(t == tEnd && !isClosed())// 上界特殊处理
    {
        return data.getControlPointAt(data.getControlPointsSize()-1);
    }
    // 计算有作用的样条基范围found_i
    int n = data.getControlPointsSize() - 1; // n+1个控制点
    int k = data.getDegree();   //k次B样条曲线
    DmVector res;   //无效值
    int found_i = findSpan(t);   //t所在节点区间的索引
    if(found_i == -1)
        return res;

    // 根据有作用的样条基，累加各项值（至多与[P(i-k),P(i-k+1)...,P(i)]控制点有关，参考《计算机辅助几何设计与非均匀有理B样条  修订版》P222）
    DmVector pt(0.0, 0.0);
    for(int i=found_i-k; i<=found_i;i++)
    {
        double v = basisFunctionValue(t, i, k);
        pt+=data.getControlPointAt(i) * v;
    }
    return pt;
}

DmVector DmSpline::derivative(double t) const
{
    return derivativeQth(t, 1);
}

DmVector DmSpline::secondDerivative(double t) const
{
    return derivativeQth(t, 2);
}

void DmSpline::getDomainOfDefinition(double& t1, double& t2) const
{
    if(!isValid())
        return;
    // 定义域：[t(k), t(n+1))
    int k = data.getDegree();
    int n = data.getControlPointsSize() - 1;
    if(k<n+1)
    {
        t1 = getKnotAt(k);
        t2 = getKnotAt(n+1);
    }
}

int DmSpline::getSegmentCount() const
{
    if(!isValid())
        return 0;
    int k = data.getDegree();
    int n = data.getControlPointsSize() - 1;
    return n-k+1;
}

bool DmSpline::isValid() const
{
    if (data.getDegree() < 1)
    {
        return false;
    }
    if (!data.getIsClosed() && data.getControlPointsSize() < data.getDegree() + 1)
    {
        return false;
    }
    // 闭合的情况会追加k个控制点，这里保证闭合满足绘制条件
    if(data.getIsClosed() && data.getControlPointsSize() - data.getDegree() < data.getDegree() + 1)
    {
        return false;
    }
    if(data.getIsClosed() && data.getControlPointsSize() - data.getDegree() < 3)
    {
        return false;
    }

    // 定义域范围有效
    int k = data.getDegree();
    int n = data.getControlPointsSize() - 1;
    return k<n+1;
}

double DmSpline::getValidT(double t) const
{
    // 参考《非均匀有理B样条  第2版》P165
    double t1, t2;
    getDomainOfDefinition(t1, t2);
    double res;
    if(isClosed())
    {
        if(t<t1)
            res = t2-(t1-t);
        else if(t>t2)
            res = t1+(t-t2);
        else
            res = t;
    }
    else
    {
        if(t<t1)
            res = t1;
        else if(t>t2)
            res = t2;
        else
            res = t;
    }
    return res;
}

std::vector<std::pair<double, double>> DmSpline::getSections() const
{
    std::vector<std::pair<double, double>> sections_less180;
    int segmentCount = getSegmentCount();
    int k = getDegree();
    for(int i=0;i<segmentCount;i++) {
        double t1 = getKnotAt(k + i);
        double t2 = getKnotAt(k + i + 1);
        //再将这个区间分成k段，每一段最多一个凹/凸处（因为仅受k+1个控制点影响，且曲线走向与控制点走向一致，此处是个人猜测，未找到论证）
        double step = (t2 - t1) / k;
        for (int j = 0; j < k; j++) {
            double t11 = t1 + j * step;
            double t12 = t1 + (j + 1) * step;
            double midT = (t11 + t12) / 2.0;
            DmVector a = derivative(t11).normalize();
            DmVector c = derivative(t12).normalize();
            DmVector b = derivative(midT).normalize();
            //判断向量a围绕原点经过b转向c是顺时针还是逆时针，判断依据：(a × b) + (b × c) + (c × a)>0逆时针，<0顺时针，=0共线。此公式a,b,c可以不是单位向量。
            // 它计算的是由原点 O 和点 A, B, C 所形成的闭环（O->A->B->C->O）的有向面积的两倍。该面积的符号直接决定了这三个点围绕原点的环绕顺序是逆时针还是顺时针
            DmVector result = DmVector::crossP(a, b) + DmVector::crossP(b, c) + DmVector::crossP(c, a);
            bool isClockwise = result.z < 0.0;//是否为顺时针
            double angle = a.angleTo(c);
            if (isClockwise) {
                angle = M_PI * 2 - angle;
            }
            // 大于180度要分割成小于180度的两个部分。即使分成k个区间，区间还有可能围成一个闭合，需要将这个区间分割（不闭合）来进一步处理
            if (angle > M_PI) {
                double tempMidT;
                bool ret = getSplineTForLimitAngles(this, isClockwise, a, b, t11, t12, tempMidT);
                if(ret)
                {
                    sections_less180.emplace_back(t11, tempMidT);
                    sections_less180.emplace_back(tempMidT, t12);
                }
                else
                {
                    sections_less180.emplace_back(t11, t12);
                }
            }
            else
            {
                sections_less180.emplace_back(t11, t12);
            }
        }
    }
    return sections_less180;
}

std::vector<std::pair<double, double>> DmSpline::getSectionsByCloseControlPoints() const
{
    std::vector<std::pair<double, double>> sections;
    int segmentCount = getSegmentCount();
    int k = getDegree();

    // 1次特殊处理
    if(k==1)
    {
        for(int i=0;i<segmentCount;i++) {
            double t1 = getKnotAt(k + i);
            double t2 = getKnotAt(k + i + 1);
            sections.emplace_back(t1, t2);
        }
        return sections;
    }

    //2次及以上
    for(int i=0;i<segmentCount;i++) {
        double t1 = getKnotAt(k + i);
        double t2 = getKnotAt(k + i + 1);
        double midT = (t1+t2)/2.0;
        // 一个区间关联k+1个控制点，计算距离各控制点最近的点，以此分割区间
        double deltaT = t2 - t1;
        if(deltaT != 0.0)
        {
            auto ctrlPts = getControlPoints(midT);
            int ctrlPtsSize = (int)ctrlPts.size();
            std::vector<double> secTs{t1, t2};  //子区间的t
            double step = deltaT / k;
            double maxStep = std::abs(step)/3.0;
            for(int j=0;j<ctrlPtsSize;j++)
            {
                double t0 = t1+j*step;  //迭代初始值
                auto secRes = newtonRaphsonBSplinePoint(this, ctrlPts.at(j), t0, maxStep);
                if(secRes.first.valid ) //迭代结果有效，且在这个区间内
                {
                    double t = secRes.second;
                    if(t >=t1 && t <=t2)
                    {
                        if(secTs.end() == std::find(secTs.begin(), secTs.end(), t))
                        {
                            secTs.emplace_back(t);
                        }
                    }
                }
            }
            // 排序并添加子区间
            std::sort(secTs.begin(), secTs.end());
            for(int j =0;j<(int)secTs.size()-1;j++)
            {
                double t_1 = secTs.at(j);
                double t_2 = secTs.at(j+1);
                sections.emplace_back(t_1, t_2);
            }
        }
    }
    return sections;
}

int DmSpline::findSpan(double t) const
{
    // 参考《非均匀有理B样条  第2版》P49
    double t1, t2;
    getDomainOfDefinition(t1, t2);
    if(t<t1 || t>t2) //避免后面的死循环
        return -1;

    int k = data.getDegree();
    int n = data.getControlPointsSize() - 1;
    if(t==data.getKnotAt(n+1)) //特殊情况
    {
        return n;
    }
    int low = k;
    int high = n+1;
    int mid=(low+high)/2;
    //进行二分搜索
    while(t<data.getKnotAt(mid) || t>=data.getKnotAt(mid+1))
    {
        if(t<data.getKnotAt(mid))
        {
            high=mid;
        }
        else
        {
            low=mid;
        }
        mid=(low+high)/2;
    }
    return mid;
}

bool DmSpline::insertKnot(double t)
{
    // 参考《计算几何算法与实现 _ Visual C++版- 孔令德》 P182
    double t1, t2;
    getDomainOfDefinition(t1, t2);
    // 超出定义域的无效节点
    if(t>t2 && t>t1)
        return false;
    if(t<t2 && t<t1)
        return false;

    // deepseek提示词：如何分割b样条曲线？分割后的曲线与原b样条曲线形状一样，但是变成两条，给出c++代码。
    int kCount = getNumberOfKnots();
    int idx = findSpan(t);

    // 参考《计算机辅助几何设计与非均匀有理B样条  修订版》P260，r是原始节点矢量t的重复度。如果插入节点为定义域的边界，多次插入r不变，还是原始值（这是个人想法，因为经测试r变化的化有误）。这里考虑r均为0。
    // 计算t的原始重复度
    int r = 0;
//    for(int i=0;i<kCount;i++)
//    {
//        double knot = data.getKnotAt(i);
//        if(knot == t)
//        {
//            r++;
//        }
//        else if(knot>t)
//        {
//            break;
//        }
//    }

    // 计算插入节点后的控制点
    int n = data.getControlPointsSize() - 1;
    int k = data.getDegree();
    std::vector<DmVector> newCtrlPts;
    newCtrlPts.reserve(n+2);
    for(int j=0;j<=idx-k;j++)
    {
        newCtrlPts.emplace_back(data.getControlPointAt(j));
    }
    for(int j=idx-k+1;j<=idx-r;j++)
    {
        double t_j = data.getKnotAt(j);
        double t_jk = data.getKnotAt(j+k);
        double alpha_j;
        double fac1 = t - t_j;
        double fac2 = t_jk - t_j;
        if(fac2 == 0.0)
            alpha_j = 0.0;
        else
            alpha_j = fac1/fac2;
        DmVector pt = data.getControlPointAt(j)*alpha_j + data.getControlPointAt(j-1)*(1.0-alpha_j);
        newCtrlPts.emplace_back(pt);
    }
    for(int j=idx-r+1;j<=n+1;j++)
    {
        newCtrlPts.emplace_back(data.getControlPointAt(j-1));
    }
    data.setControlPoints(newCtrlPts);

    // 插入节点
    std::vector<double> newKnots = data.getKnots();
    newKnots.insert(newKnots.begin() + idx + 1, t);
    data.setKnots(newKnots);
    return true;
}

//bool DmSpline::removeKnot(double t)
//{
//    double t1, t2;
//    getDomainOfDefinition(t1, t2);
//    // 超出定义域的无效节点
//    if(t>t2 && t>t1)
//        return false;
//    if(t<t2 && t<t1)
//        return false;
//
//    int idx = findSpan(t);
//    int kCount = getNumberOfKnots();
//    int n = data.getControlPointsSize() - 1;
//    int k = getDegree();
//    // 检查节点重复度
//    int multiplicity = 0;
//    bool isFirst = true;
//    int minIdx = idx;
//    for (int i = idx - k -1; i>=0 && i < kCount; i++) {
//        if(t == data.getKnotAt(i))
//        {
//            multiplicity++;
//            if(isFirst)
//            {
//                minIdx = i;
//                isFirst = false;
//            }
//        }
//    }
//    if (multiplicity <= 1) {
//        return false;
//    }
//    if (multiplicity > k + 1) {
//        return false;
//    }
//    idx = minIdx;
//
//    // 参考：《非均匀有理B样条  第2版》P134，仅能删除重节点，而且有异常（3控制点2次曲线，插入2个节点，删除一个都不行），仅作参考
//    int r = idx; //待消去的节点下标
//    int p = getDegree();
//    int m=n+p+1;
//    int num = p; //预设消去次数
//    int ord = p+1;
//    //int s = multiplicity - 1; //重复度
//    int s=1;
//    int fout = (2*r-s-p)/2;
//    int last = r - s;
//    int first = r - p;
//    std::vector<DmVector> Pw = data.getControlPoints();
//    Pw.resize(2*k+1);
//    std::vector<double> U = data.getKnots();
//    std::vector<DmVector> temp;
//    temp.resize(2*k+1);
//    int i, j, ii, jj;
//    double alfi, alfj;
//    int T = 0;  //代替原来循环的t，实际消去次数
//    for(T = 0;T<num;T++)
//    {
//        int off = first - 1;
//        temp[0] = Pw[off];
//        temp[last+1-off] = Pw[last+1];
//        i = first;
//        j = last;
//        ii = 1;
//        jj = last - off;
//        bool remflag = false;
//        while(j-i>T)
//        {
//            alfi = (t-U[i]) / (U[i+ord+T] - U[i]);
//            alfj = (t-U[j-T]) / (U[j+ord] - U[j-T]);
//            temp[ii] = (Pw[i] - temp[ii-1]*(1.0-alfi)) / alfi;
//            temp[jj] = (Pw[j] - temp[jj+1]*alfj) / (1.0-alfj);
//            i++;
//            ii++;
//            j--;
//            jj--;
//        }
//        if(j-i<T)
//        {
//            if(temp[ii-1].distanceTo(temp[jj+1])<TOL)
//                remflag = true;
//        }
//        else
//        {
//            alfi = (t-U[i])/(U[i+ord+T]-U[i]);
//            DmVector pt =temp[ii+T+1]*alfi + temp[ii-1]*(1.0-alfi);
//            if(Pw[i].distanceTo(pt) <= TOL)
//                remflag = true;
//        }
//        if(!remflag)
//            break;
//        else        //节点被成功删除，保存新的控制点
//        {
//            i=first;
//            j=last;
//            while(j-i>T)
//            {
//                Pw[i] = temp[i-off];
//                Pw[j] = temp[j-off];
//                i++;
//                j=j-1;
//            }
//        }
//        first = first -1;
//        last=last+1;
//    }
//    if(T==0)
//        return false;
//    for(int kk=r+1;kk<=m;kk++)
//    {
//        if(std::fmod(kk,2) == 1)
//            i++;
//        else
//            j--;
//    }
//    for(int kk=i+1;kk<n;kk++)
//    {
//        Pw[j]=Pw[kk];
//        j++;
//    }
//
//
//    // 创建新的控制顶点和节点向量
//    //std::vector<DmVector> newControlPoints;
//    std::vector<double> newKnots;
//    // 复制节点向量，跳过要消去的那个节点
//    for (int i = 0; i < kCount; i++) {
//        if (i != idx) {
//            newKnots.push_back(data.getKnotAt(i));
//        }
//    }
//
//
//    data.setControlPoints(Pw);
//    data.setKnots(newKnots);
//    return true;
//}

void DmSpline::reverse()
{
    // deepseek提示词：b样条曲线反向，给出c++代码和详细解释
    // 1. 反转控制点顺序
    std::vector<DmVector> reversedControlPoints = data.getControlPoints();
    std::reverse(reversedControlPoints.begin(), reversedControlPoints.end());

    // 2. 计算反转后的节点向量
    std::vector<double> reversedKnots;
    int kCount = data.getKnotsSize();
    // 获取节点向量的最小值和最大值
    double minKnot = data.getKnotAt(0);
    double maxKnot = data.getKnotAt(kCount-1);

    // 计算反转后的节点值：new_knot = maxKnot + minKnot - old_knot
    for (int i = kCount - 1; i >= 0; --i) {
        double reversedKnot = maxKnot + minKnot - data.getKnotAt(i);
        reversedKnots.push_back(reversedKnot);
    }

    data.setKnots(reversedKnots);
    data.setControlPoints(reversedControlPoints);
}

void DmSpline::fit()
{
    // 清空原始节点矢量，控制点信息
    data.setKnots({});
    data.setControlPoints({});

    constexpr int k = 3;
    std::vector<double> u_waves;
    std::vector<double> knots;
    // 节点向量及数据点的节点
    bool res = getFitKnots(data.getIsClosed(), data.getFitPoints(), knots, u_waves);
    if(!res)
        return;
    // 闭曲线
    if(data.getIsClosed())
    {
        data.setKnots(knots);
        std::vector<DmVector> tmpCtrlPts;
        tmpCtrlPts.resize(knots.size()-1-k); //m-1-k
        data.setControlPoints(tmpCtrlPts);

        // 计算矩阵
        int s = data.getFitPointsSize() - 1; //数据点数为n+1,矩阵为n*n，参考《计算机辅助几何设计与非均匀有理B样条 CAGD & NURBS -- 施法中编著1994》P275
        int n = s;
        MatrixXd m = MatrixXd ::Zero(s, s);
        double u_3 = knots.at(3);
        double u_n_2 = knots.at(n+2);
        m(0,0) = basisFunctionValue(u_3, 1, k); //第一行
        m(0,1) = basisFunctionValue(u_3, 2, k);
        m(0,s-1) = basisFunctionValue(u_3, 0, k);
        m(s-1,0) = basisFunctionValue(u_n_2, n+1, k); //最后一行
        m(s-1,s-2) = basisFunctionValue(u_n_2, n-1, k);
        m(s-1,s-1) = basisFunctionValue(u_n_2, n, k);
        for(int i=1;i<s-1;i++) // 每一行
        {
            double u = knots.at(3+i);
            m(i, i-1) = basisFunctionValue(u,i,k);
            m(i, i) = basisFunctionValue(u,i+1,k);
            m(i, i+1) = basisFunctionValue(u,i+2,k);
        }

        // 分别计算控制点的x,y坐标
        // 求解x
        std::vector<DmVector> fitPoints = data.getFitPoints();
        VectorXd Qx(s);
        for(int i=0;i<n;i++)
        {
            Qx[i] = data.getFitPointAt(i).x;
        }
        Eigen::PartialPivLU<Eigen::MatrixXd> lu(m);
        Eigen::VectorXd x = lu.solve(Qx);
        // 求解y
        VectorXd Qy(s);
        for(int i=0;i<n;i++)
        {
            Qy[i] = data.getFitPointAt(i).y;
        }
        Eigen::VectorXd y = lu.solve(Qy);
        // 设置控制点
        std::vector<DmVector> ctrlPts; // d0~dn
        ctrlPts.resize(n+3);
        for(int i=0;i<n;i++) //求解出来的是d1~dn
        {
            double ptx = x(i);
            double pty = y(i);
            ctrlPts.at(i+1) = DmVector(ptx, pty); //d1~dn
        }
        ctrlPts.at(0) = ctrlPts.at(n); //d(0)=d(n)
        ctrlPts.at(n+1) = ctrlPts.at(1); //d(n+1)=d(1)
        ctrlPts.at(n+2) = ctrlPts.at(2); //d(n+2)=d(2)
        data.setControlPoints(ctrlPts);
    }
    // 开曲线
    else
    {
        // 参考《非均匀有理B样条  第2版》P259例9.1
        data.setKnots(knots);
        std::vector<DmVector> tmpCtrlPts;
        tmpCtrlPts.resize(data.getFitPointsSize()); // 设置控制点数，方便后面计算（findSpan）
        data.setControlPoints(tmpCtrlPts);

        // 计算矩阵
        int s = data.getFitPointsSize();
        MatrixXd m = MatrixXd ::Zero(s, s);
        m(0,0) = 1.0;
        m(s-1,s-1) = 1.0;
        for(int i=1;i<s-1;i++) // 每一行
        {
            double u_wave = u_waves.at(i);
            int idx = findSpan(u_wave);
            for(int j=idx-k; j<=idx;j++)
            {
                double val = basisFunctionValue(u_wave, j, k);
                m(i,j) = val;
            }
        }

        // 分别计算控制点的x,y坐标
        // 求解x
        std::vector<DmVector> fitPoints = data.getFitPoints();
        VectorXd Qx(s);
        for(int i=0;i<data.getFitPointsSize();i++)
        {
            Qx[i] = data.getFitPointAt(i).x;
        }
        Eigen::PartialPivLU<Eigen::MatrixXd> lu(m);
        Eigen::VectorXd x = lu.solve(Qx);
        // 求解y
        VectorXd Qy(s);
        for(int i=0;i<data.getFitPointsSize();i++)
        {
            Qy[i] = data.getFitPointAt(i).y;
        }
        Eigen::VectorXd y = lu.solve(Qy);
        // 设置控制点
        std::vector<DmVector> ctrlPts;
        ctrlPts.reserve(s);
        for(int i=0;i<s;i++)
        {
            double ptx = x(i);
            double pty = y(i);
            ctrlPts.emplace_back(ptx, pty);
        }
        data.setControlPoints(ctrlPts);
    }
}

std::tuple<DmVector, double, double> DmSpline::getClosetPt(const DmVector& coord) const
{
    int k =data.getDegree();
    // 一阶样条特殊处理
    if(k == 1)
    {
        double distance = DM_MAXDOUBLE;
        double minDist = DM_MAXDOUBLE;
        double t = 0.0;
        DmVector nearestPt(false);
        std::vector<DmLinePtr> lines = getLinesOfSplineDegree1(this);
        for(int j=0;j<lines.size();j++)
        {
            // 这段直线对应[t(k+j), t(k+j+1)]，i=k+j,关联的控制点为[P(i-k),P(i)]，其中k=1
            int i=k+j;
            auto l = lines.at(j);
            DmVector tempPt = l->getNearestPointOnEntity(coord, true, &distance, nullptr);
            if(distance<minDist)
            {
                minDist = distance;
                nearestPt = tempPt;
                // 计算t
                t = getTForLine(i, tempPt);
            }
        }
        return {nearestPt, t, minDist};
    }

    // 参考《非均匀有理B样条  第2版》 P164
    // 从每个区间中获得迭代初始值t0
    //std::vector<std::pair<double, double>> sections = getSections();
    std::vector<std::pair<double, double>> sections = getSectionsByCloseControlPoints();
    //获得最近点
    double minDist = DM_MAXDOUBLE;
    DmVector theClosetPt(false);
    double t = 0.0;
    for(auto sec: sections)
    {
        double t0 = (sec.first+sec.second)/2.0;
        double maxStep =std::abs(sec.second - sec.first) / 3.0;
        auto closePtItem = newtonRaphsonBSplinePoint(this, coord, t0, maxStep);
        DmVector closetPt = closePtItem.first;
        if(closetPt.valid)
        {
            double d = closetPt.distanceTo(coord);
            if(d<minDist)
            {
                minDist = d;
                theClosetPt = closetPt;
                t = closePtItem.second;
            }
        }
    }
    // 如果没算出来，采用端点
    if(!isClosed() && !theClosetPt.valid)
    {
        double t1, t2;
        getDomainOfDefinition(t1, t2);
        auto startPt =getStartpoint();
        auto endPt =getEndpoint();
        double dist1 = startPt.distanceTo(coord);
        double dist2 = endPt.distanceTo(coord);
        if(dist1<=dist2)
        {
            theClosetPt = startPt;
            t = t1;
            minDist = dist1;
        }
        else
        {
            theClosetPt = endPt;
            t = t2;
            minDist = dist2;
        }
    }

    return {theClosetPt, t, minDist};
}

std::vector<std::tuple<DmVector, double, double>> DmSpline::getClosetPts(const DmVector& coord) const
{
    // 计算出最近点
    std::vector<std::tuple<DmVector, double, double>> res;
    auto nearestItem = getClosetPt(coord);
    DmVector closetPt = std::get<0>(nearestItem);
    if(!closetPt.valid)
        return res;

    double closetT = std::get<1>(nearestItem);
    double minDist = std::get<2>(nearestItem);
    int k =data.getDegree();
    // 一阶样条特殊处理
    if(k == 1)
    {
        std::vector<DmLinePtr> lines = getLinesOfSplineDegree1(this);
        double distance = DM_MAXDOUBLE;
        // 取接近最近点的所有点
        for(int j=0;j<lines.size();j++)
        {
            int i=k+j;
            auto l = lines.at(j);
            DmVector tempPt = l->getNearestPointOnEntity(coord, true, &distance, nullptr);
            if(std::abs(distance-minDist)<TOL)
            {
                double t = getTForLine(i, tempPt);
                res.emplace_back(tempPt, t, distance);
            }
        }
        return res;
    }

    // 非一阶情况
    std::vector<std::pair<double, double>> sections = getSectionsByCloseControlPoints();
    for(auto sec: sections)
    {
        double t0 = (sec.first+sec.second)/2.0;
        double maxStep =std::abs(sec.second - sec.first) / 3.0;
        auto closePtItem = newtonRaphsonBSplinePoint(this, coord, t0, maxStep);
        DmVector closetPt = closePtItem.first;
        if(closetPt.valid)
        {
            double d = closetPt.distanceTo(coord);
            if(std::abs(d-minDist)<TOL)
            {
                res.emplace_back(closetPt, closePtItem.second, d);
            }
        }
    }

    // 没算出来，取getClosetPt()得到的值
    if(res.empty())
    {
        res.emplace_back(nearestItem);
    }
    return res;
}

double DmSpline::getTForLine(int i, const DmVector& nearestPt) const
{
    int k =data.getDegree();
    if(k!=1) //非一阶无效
        return 0.0;

    // deepseek提示词：一次的b样条曲线，经过控制点，是多段连接的直线，求直线上的点跟参数t的关系
    // 也可通过一次样条基的定义，及B样条的定义式得到：
    // 在[t(i),t(i+1)]上，曲线段为C(t) = (t(i+1)-t)/(t(i+1)-t(i))*P(i-1) + (t-t(i))/(t(i+1)-t(i))*P(i)，
    // 通过推导得到：t = [(t(i+1)-t(i))*C(t) + t(i)P(i) - t(i+1)P(i-1)] / (P(i) - P(i-1))，取x或y计算，保证分母不为0
    double t_i = data.getKnotAt(i);
    double t_i_1 = data.getKnotAt(i+1);
    DmVector P_i=data.getControlPointAt(i);
    DmVector P_i_minus_1=data.getControlPointAt(i-1);
    DmVector delta = P_i - P_i_minus_1;
    double t;
    //使用x计算
    if(std::abs(delta.x)>std::abs(delta.y))
    {
        t = ((t_i_1-t_i)*nearestPt.x + t_i*P_i.x - t_i_1*P_i_minus_1.x) / delta.x;
    }
        //使用y计算
    else
    {
        t = ((t_i_1-t_i)*nearestPt.y + t_i*P_i.y - t_i_1*P_i_minus_1.y) / delta.y;
    }
    return t;
}

bool DmSpline::changeCloseToNormal()
{
    if(!isClosed())
        return false;

    // 参考：《计算机辅助几何设计与非均匀有理B样条  修订版》P262-263
    // 对于闭合的均匀Y样条，在定义域边界插入节点，插入起始节点然后删除第一个节点及控制点，插入终止节点然后删除最后一个节点及控制点，如此重复k次
    int knotsCount = getNumberOfKnots();
    double t1, t2;
    getDomainOfDefinition(t1, t2);
    double k = getDegree();
    for(int i=0;i<k;i++)
    {
        insertKnot(t1);
        // 移除开始的节点及控制点
        data.removeKnotAt(0);
        data.removeControlPointAt(0);
        // 移除结束的节点及控制点
        insertKnot(t2);
        data.removeKnotAt(data.getKnotsSize() - 1);
        data.removeControlPointAt(data.getControlPointsSize() - 1);
    }
    // 不再是闭合
    data.setIsClosed(false);
    return true;
}

DmVector DmSpline::getStartpoint() const
{
	if (data.getIsClosed())
	{
		return DmVector(false);
	}
    double t1, t2;
    getDomainOfDefinition(t1, t2);
	return evaluate(t1);
}

DmVector DmSpline::getEndpoint() const
{
	if (data.getIsClosed())
	{
		return DmVector(false);
	}
    double t1, t2;
    getDomainOfDefinition(t1, t2);
    return evaluate(t2);
}

DmVector DmSpline::getNearestEndpoint(const DmVector& coord, double* dist)const
{
	double minDist = DM_MAXDOUBLE;
	DmVector ret(false);
	if (!data.getIsClosed())
	{ // no endpoint for closed spline
		DmVector vp1(getStartpoint());
		DmVector vp2(getEndpoint());
		double d1((coord - vp1).squared());
		double d2((coord - vp2).squared());
		if (d1 < d2)
		{
			ret = vp1;
			minDist = sqrt(d1);
		}
		else 
		{
			ret = vp2;
			minDist = sqrt(d2);
		}
	}
	if (dist)
	{
		*dist = minDist;
	}
	return ret;
}

DmVector DmSpline::getNearestCenter(const DmVector& /*coord*/, double* dist) const
{
	if (dist)
	{
		*dist = DM_MAXDOUBLE;
	}

	return DmVector(false);
}

DmVector DmSpline::getNearestMiddle(const DmVector& /*coord*/, double* dist, int /*middlePoints*/)const
{
	if (dist)
	{
		*dist = DM_MAXDOUBLE;
	}

	return DmVector(false);
}

void DmSpline::move(const DmVector& offset)
{
	pLineStrip->move(offset);
	std::vector<DmVector> tempVps = std::vector<DmVector>();
	for (DmVector& vp : data.getControlPoints())
	{
		tempVps.emplace_back(vp.move(offset));
	}
	data.setControlPoints(tempVps);

    std::vector<DmVector> tempFitPts = std::vector<DmVector>();
    for (DmVector& vp : data.getFitPoints())
    {
        tempFitPts.emplace_back(vp.move(offset));
    }
    data.setFitPoints(tempFitPts);

	moveBorders(offset);
}

void DmSpline::rotate(const DmVector& center, const DmVector& angleVector)
{
	pLineStrip->rotate(center, angleVector);
	std::vector<DmVector> tempVps = std::vector<DmVector>();
	for (DmVector& vp : data.getControlPoints())
	{
		tempVps.emplace_back(vp.rotate(center, angleVector));
	}
	data.setControlPoints(tempVps);

    std::vector<DmVector> tempFitPts = std::vector<DmVector>();
    for (DmVector& vp : data.getFitPoints())
    {
        tempFitPts.emplace_back(vp.rotate(center, angleVector));
    }
    data.setFitPoints(tempFitPts);

	calculateBorders();
}

void DmSpline::scale(const DmVector& center, const DmVector& factor)
{
	std::vector<DmVector> tempVps = std::vector<DmVector>();
	for (DmVector& vp : data.getControlPoints())
	{
		tempVps.emplace_back(vp.scale(center, factor));
	}
	data.setControlPoints(tempVps);

    std::vector<DmVector> tempFitPts = std::vector<DmVector>();
    for (DmVector& vp : data.getFitPoints())
    {
        tempFitPts.emplace_back(vp.scale(center, factor));
    }
    data.setFitPoints(tempFitPts);

	update();
}

void DmSpline::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	std::vector<DmVector> tempVps = std::vector<DmVector>();
	for (DmVector& vp : data.getControlPoints())
	{
		tempVps.emplace_back(vp.mirror(axisPoint1, axisPoint2));
	}
	data.setControlPoints(tempVps);

    std::vector<DmVector> tempFitPts = std::vector<DmVector>();
    for (DmVector& vp : data.getFitPoints())
    {
        tempFitPts.emplace_back(vp.mirror(axisPoint1, axisPoint2));
    }
    data.setFitPoints(tempFitPts);

	update();
}

void DmSpline::moveRef(const DmVector& ref, const DmVector& offset)
{
	std::vector<DmVector> tempVps = std::vector<DmVector>();
    if(isByFit())
    {
        for (DmVector& vp : data.getFitPoints())
        {
            if (ref.distanceTo(vp) < 1.0e-4)
            {
                tempVps.emplace_back(vp.move(offset));
            }
            else
            {
                tempVps.emplace_back(vp);
            }
        }
        data.setFitPoints(tempVps);
        fit();
    }
    else
    {
        for (DmVector& vp : data.getControlPoints())
        {
            if (ref.distanceTo(vp) < 1.0e-4)
            {
                tempVps.emplace_back(vp.move(offset));
            }
            else
            {
                tempVps.emplace_back(vp);
            }
        }
        data.setControlPoints(tempVps);
    }

	update();
}

std::vector<DmVector> DmSpline::getControlPoints() const
{
	return data.getControlPoints();
}

std::vector<DmVector> DmSpline::getControlPoints(double t) const
{
    int k=getDegree();
    int i = findSpan(t);
    if(i==-1)
        return std::vector<DmVector>();
    // 根据有作用的样条基，至多与[P(i-k),P(i-k+1)...,P(i)]控制点有关
    std::vector<DmVector> pts;
    pts.reserve(k+1);
    for(int j=i-k;j<=i;j++)
    {
        pts.emplace_back(data.getControlPointAt(j));
    }
    return pts;
}

DmVector DmSpline::getControlPointAt(int i) const
{
    return data.getControlPointAt(i);
}

std::vector<DmVector> DmSpline::getFitPoints() const
{
    return data.getFitPoints();
}

void DmSpline::setFitPts(const std::vector<DmVector>& pts)
{
    setByFit(true);
    data.setFitPoints(pts);
}

bool DmSpline::isByFit() const
{
    return data.getSplineType() == ESplineType::eFitPoints;
}

void DmSpline::setByFit(bool fit)
{
    if(fit)
        data.setSplineType(ESplineType::eFitPoints);
    else
        data.setSplineType(ESplineType::eControlPoints);
}

void DmSpline::addControlPoint(const DmVector& v)
{
	std::vector<DmVector> pts = data.getControlPoints();
	pts.emplace_back(v);
	data.setControlPoints(pts);
}

void DmSpline::setControlPts(const std::vector<DmVector>& pts)
{
    data.setControlPoints(pts);
}

void DmSpline::saveStream(OutputStream& wrt) const
{
	DmEntity::saveStream(wrt);

    bool isFit = isByFit(); //曲线类型（控制点/拟合点）
	int iDegree = data.getDegree(); // 阶数
	bool isClosed = data.getIsClosed(); // 是否闭合
	std::vector<DmVector> vecControlPoints = data.getControlPoints(); // 控制点集合
	std::vector<double> vecKnots = data.getKnots(); // 节点数据
    std::vector<DmVector> vecFitPoints = data.getFitPoints(); //拟合点

	wrt << (bool)isFit << (int)iDegree << (bool)isClosed << (uint32_t)vecControlPoints.size() << (uint32_t)vecFitPoints.size();

	for (auto& pt : vecControlPoints)
	{
		wrt << (double)pt.x << (double)pt.y;
	}
    for (auto& pt : vecFitPoints)
    {
        wrt << (double)pt.x << (double)pt.y;
    }

	wrt << (uint32_t)vecKnots.size();
	for (auto& knot : vecKnots)
	{
		wrt << (double)knot;
	}
}

void DmSpline::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmSpline", revs);
	if (revId > fileRev)
	{
        DmEntity::restoreStream(reader, revs);
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
        restoreStream(reader);
	}
}

void DmSpline::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmSpline
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmSpline::restoreStream(InputStream& reader)
{
    DmEntity::restoreStream(reader);

    bool isFit;
    int iDegree;
    bool isClosed;
    auto vecControlPoints = std::vector<DmVector>();
    auto vecFitPoints = std::vector<DmVector>();
    auto vecKnots = std::vector<double>();

    reader >> (bool&)isFit >> (int&)iDegree >> (bool&)isClosed;

    uint32_t numControlPoint;
    uint32_t numFitPoint;
    reader >> (uint32_t&)numControlPoint >> (uint32_t&)numFitPoint;
    for (uint32_t i = 0; i < numControlPoint; i++)
    {
        DmVector pt(true);
        reader >> (double&)pt.x >> (double&)pt.y;
        vecControlPoints.emplace_back(std::move(pt));
    }
    for (uint32_t i = 0; i < numFitPoint; i++)
    {
        DmVector pt(true);
        reader >> (double&)pt.x >> (double&)pt.y;
        vecFitPoints.emplace_back(std::move(pt));
    }

    uint32_t numKnots;
    reader >> (uint32_t&)numKnots;
    for (uint32_t i = 0; i < numKnots; i++)
    {
        double knot;
        reader >> (double&)knot;
        vecKnots.emplace_back(std::move(knot));
    }

    setByFit(isFit);
    data.setDegree(iDegree);
    data.setIsClosed(isClosed);
    data.setControlPoints(vecControlPoints);
    data.setFitPoints(vecFitPoints);
    data.setKnots(vecKnots);

    update();
}

