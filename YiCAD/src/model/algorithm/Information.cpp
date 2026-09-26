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

/// @file Information.cpp
/// @brief 实体信息查询类的实现，包含交点计算、可修剪判断等功能

#include "Information.h"

#include <vector>

#include "DmEntityContainer.h"
#include "DmVector.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "DmSpline.h"
#include "Quadratic.h"
#include "Math2d.h"
#include "DmRect.h"
#include "GeometryMethods.h"
#include "Debug.h"

/// @brief 交点判断容差
constexpr double INTERSECTION_TOLERANCE = 1.0e-4;

/// @brief 同心圆判定容差
constexpr double CONCENTRIC_TOLERANCE = 1.0e-6;

/// @brief 直线圆弧切点容差因子
constexpr double TANGENT_TOLERANCE_FACTOR = 2.0;

/// @brief 椭圆二次项容差缩放因子
constexpr double ELLIPSE_QUADRATIC_TOLERANCE = 1.0e3;

/// @brief 四边形边数
constexpr int QUADRILATERAL_EDGE_COUNT = 4;

/// @brief 完整圆角度（2*PI）
constexpr double FULL_CIRCLE_ANGLE = 2. * M_PI;

/// @brief 四边形中心计算权重
constexpr double QUADRILATERAL_CENTER_WEIGHT = 0.25;

/// @brief 直线平行判定容差倍数因子
constexpr double PARALLEL_TOLERANCE_FACTOR = 10.;

/// @brief 构造函数，绑定实体容器
/// @param container 实体容器引用（会将地址存入成员指针）
Information::Information(DmEntityContainer& container)
	: container(&container)
{
}

/// @brief 判断实体类型是否为标注类型
/// @param type 实体类型
/// @return true 是标注实体，false 不是
bool Information::isDimension(DM::EntityType type)
{
	switch (type)
	{
	case DM::EntityDimAligned:
	case DM::EntityDimLinear:
	case DM::EntityDimRadial:
	case DM::EntityDimDiametric:
	case DM::EntityDimAngular:
		return true;
	default:
		return false;
	}
}

/// @brief 判断单个实体是否可修剪
/// @details 实体必须在文档、多段线或块中才可修剪
/// @param e 实体指针
/// @return true 可修剪，false 不可修剪
bool Information::isTrimmable(DmEntity* e)
{
	if (e)
	{
		if (e->getParent())
		{
			switch (e->getParent()->getEntityType())
			{
			case DM::EntityPolyline:
			case DM::EntityContainer:
			case DM::EntityBlock:
				return true;
			default:
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

/// @brief 判断两个实体是否可互相修剪
/// @details 两个实体必须在同一文档、同一多段线或同一块中
/// @param e1 实体1指针
/// @param e2 实体2指针
/// @return true 可互相修剪，false 不可
bool Information::isTrimmable(DmEntity* e1, DmEntity* e2)
{
	if (e1 && e2)
	{
		if (e1->getParent() && e2->getParent())
		{
			if (e1->getParent()->getEntityType() == DM::EntityPolyline &&
				e2->getParent()->getEntityType() == DM::EntityPolyline &&
				e1->getParent() == e2->getParent())
			{
				// TODO : polyline不能外部遍历实体
				//// in the same polyline
				//DmPolyline* pl = static_cast<DmPolyline*>(e1->getParent());
				//int idx1 = pl->findEntity(e1);
				//int idx2 = pl->findEntity(e2);
				//if (abs(idx1 - idx2) == 1 || abs(idx1 - idx2) == int(pl->count() - 1))
				//{
				//	return true;
				//}
				//else
				//{
				//	return false;
				//}
				return false;
			}
			else if ((e1->getParent()->getEntityType() == DM::EntityContainer  ||
				e1->getParent()->getEntityType() == DM::EntityBlock) &&
				(e2->getParent()->getEntityType() == DM::EntityContainer ||
					e2->getParent()->getEntityType() == DM::EntityBlock))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return (e1->getParent() == e2->getParent());
		}
	}
	else
	{
		return false;
	}
}

/// @brief 计算两实体的交点
/// @details 支持直线、圆、多段线、圆弧、椭圆、椭圆弧、过点样条
/// @param e1 实体1指针
/// @param e2 实体2指针
/// @param onEntities true仅返回同时在两实体上的交点，false返回曲线方程的交点
/// @return 交点集合
DmVectorSolutions Information::getIntersection(DmEntity const* e1, DmEntity const* e2, bool onEntities)
{
	// 求得可能的交点ret（比如在求圆弧与圆弧的交点时，把对应圆与圆的交点求出）
	DmVectorSolutions ret;
	const double tol = INTERSECTION_TOLERANCE;

	if (!(e1 && e2))
	{
		return ret;
	}

	// 同一实体不求交（样条线，多段线除外）
	if (e1 == e2 && (e1->getEntityType() != DM::EntitySpline) && (e1->getEntityType() != DM::EntityPolyline))
	{
		return ret;
	}

	if (e1->getEntityType() == DM::EntityMText || e2->getEntityType() == DM::EntityMText ||
		e1->getEntityType() == DM::EntityText || e2->getEntityType() == DM::EntityText ||
		isDimension(e1->getEntityType()) || isDimension(e2->getEntityType()))
	{
		return ret;
	}

	// 交点限定在实体上，如果boundingbox不相交，返回空
	if (onEntities)
	{
		DmRect const rect1{ e1->getMin(), e1->getMax() };
		DmRect const rect2{ e2->getMin(), e2->getMax() };

		if (!rect1.intersects(rect2, DM_TOLERANCE))
		{
			return ret;
		}
	}

	if (e1->getEntityType() == DM::EntitySpline || e2->getEntityType() == DM::EntitySpline)
	{
		ret = DmSpline::getIntersection(e1, e2);
	}
	else if (e1->getEntityType() == DM::EntityPolyline || e2->getEntityType() == DM::EntityPolyline)
	{
		ret = getIntersectionPolyline(e1, e2, onEntities);
	}
	else
	{
		auto isArc = [](DmEntity const* e)
		{
			auto type = e->getEntityType();
			return type == DM::EntityCircle || type == DM::EntityArc;
		};

		if (isArc(e1) && isArc(e2))
		{
			ret = getIntersectionArcArc(e1, e2);
		}
		else
		{
			const auto qf1 = e1->getQuadratic();
			const auto qf2 = e2->getQuadratic();
			ret = Quadratic::getIntersection(qf1, qf2);
		}
	}

	// 在可能的交点中过滤出在实体上的
	DmVectorSolutions ret2;
	for (const DmVector& vp : ret)
	{
		if (!vp.valid)
		{
			continue;
		}

		if (onEntities)
		{
			if (!(e1->isPointOnEntity(vp, tol) && e2->isPointOnEntity(vp, tol)))
			{
				continue;
			}
		}

		// 求实体在交点处的切向
		DmVector direction1 = e1->getTangentDirection(vp);
		DmVector direction2 = e2->getTangentDirection(vp);

		// 2个切向平行
		double dotAbs = fabs(direction1.dotP(direction2));
		double magnitudeProduct = sqrt(direction1.squared() * direction2.squared());
		if (direction1.valid && direction2.valid && fabs(dotAbs - magnitudeProduct) < sqrt(tol) * tol)
		{
			ret2.setTangent(true);
		}
		ret2.push_back(vp);
	}
	return ret2;
}

/// @brief 计算多段线与实体的交点
/// @param e1 实体1指针
/// @param e2 实体2指针
/// @param onEntities 是否限定在实体上
/// @return 交点集合
DmVectorSolutions Information::getIntersectionPolyline(DmEntity const* e1, DmEntity const* e2, bool onEntities)
{
	DmVectorSolutions res;
	auto type1 = e1->getEntityType();
	auto type2 = e2->getEntityType();
	if (type1 == DM::EntityPolyline && type2 == DM::EntityPolyline)
	{
		const DmPolyline* poly1 = static_cast<const DmPolyline*>(e1);
		const DmPolyline* poly2 = static_cast<const DmPolyline*>(e2);
		int segCount = poly1->getSegmentCount();
		double bulge;
		DmVector pt1, pt2, center, normal;
		double radius, startAngle, endAngle;
		for (int i = 0; i < segCount; i++)
		{
			poly1->getSegmentInfoAt(i, bulge, pt1, pt2);
			DmVectorSolutions subRes;
			if (bulge == 0.0)
			{
				DmLine line(pt1, pt2);
				subRes = getIntersectionPolyline(poly2, &line, onEntities);
			}
			else
			{
				GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAngle, endAngle, normal);
				DmArc arc(nullptr, ArcData(center, normal, radius, startAngle, endAngle));
				subRes = getIntersectionPolyline(poly2, &arc, onEntities);
			}
			for (auto pt : subRes)
			{
				res.push_back(pt);
			}
		}
	}
	else if (type1 == DM::EntityPolyline)
	{
		const DmPolyline* poly1 = static_cast<const DmPolyline*>(e1);
		int segCount = poly1->getSegmentCount();
		double bulge;
		DmVector pt1, pt2;
		DmVector center, normal;
		double radius, startAngle, endAngle;
		for (int i = 0; i < segCount; i++)
		{
			poly1->getSegmentInfoAt(i, bulge, pt1, pt2);
			DmVectorSolutions subRes;
			if (bulge == 0.0)
			{
				DmLine line(pt1, pt2);
				subRes = getIntersection(e2, &line, onEntities);
			}
			else
			{
				GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAngle, endAngle, normal);
				DmArc arc(nullptr, ArcData(center, normal, radius, startAngle, endAngle));
				subRes = getIntersection(e2, &arc, onEntities);
			}
			for (auto pt : subRes)
			{
				res.push_back(pt);
			}
		}
	}
	else if (type2 == DM::EntityPolyline)
	{
		res = getIntersectionPolyline(e2, e1, onEntities);
	}
	else
	{
		// 非多段线实体，返回空结果
	}

	// 去除重复
	res.distinct();
	return res;
}

/// @brief 计算两直线的交点
/// @param e1 直线1指针
/// @param e2 直线2指针
/// @return 交点集合（平行时为空）
DmVectorSolutions Information::getIntersectionLineLine(DmLine* e1, DmLine* e2)
{
	DmVectorSolutions ret;
	if (!(e1 && e2))
	{
		return ret;
	}

	DmVector p1 = e1->getStartpoint();
	DmVector p2 = e1->getEndpoint();
	DmVector p3 = e2->getStartpoint();
	DmVector p4 = e2->getEndpoint();

	double num = ((p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x));
	double div = ((p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y));

	if (fabs(div) > DM_TOLERANCE && fabs(remainder(e1->getStartAngle() - e2->getStartAngle(), M_PI)) >= DM_TOLERANCE * PARALLEL_TOLERANCE_FACTOR)
	{
		double u = num / div;

		double xs = p1.x + u * (p2.x - p1.x);
		double ys = p1.y + u * (p2.y - p1.y);
		ret = DmVectorSolutions({ DmVector(xs, ys) });
	}
	else
	{
		// lines are parallel
		ret = DmVectorSolutions();
	}

	return ret;
}

/// @brief 计算直线与圆弧的交点
/// @param line 直线指针
/// @param arc 圆弧指针
/// @return 交点集合（相切时含切点标记）
DmVectorSolutions Information::getIntersectionLineArc(DmLine* line, DmArc* arc)
{
	DmVectorSolutions ret;

	if (!(line && arc))
	{
		return ret;
	}

	double dist = 0.0;
	DmVector nearest;
	nearest = line->getNearestPointOnEntity(arc->getCenter(), false, &dist);

	if (nearest.valid && fabs(dist - arc->getRadius()) < INTERSECTION_TOLERANCE)
	{
		ret = DmVectorSolutions({ nearest });
		ret.setTangent(true);
		return ret;
	}

	DmVector p = line->getStartpoint();
	DmVector d = line->getEndpoint() - line->getStartpoint();
	double d2 = d.squared();
	DmVector c = arc->getCenter();
	double r = arc->getRadius();
	DmVector delta = p - c;
	if (d2 < DM_TOLERANCE2)
	{
		if (fabs(delta.squared() - r * r) < TANGENT_TOLERANCE_FACTOR * DM_TOLERANCE * r)
		{
			return DmVectorSolutions({ line->getMiddlePoint() });
		}
		else
		{
			return ret;
		}
	}

	double a1 = DmVector::dotP(delta, d);
	double term1 = a1 * a1 - d2 * (delta.squared() - r * r);
	if (term1 < -DM_TOLERANCE)
	{
		return ret;
	}
	else
	{
		term1 = fabs(term1);
		if (term1 < DM_TOLERANCE * d2)
		{
			ret = DmVectorSolutions({ line->getNearestPointOnEntity(c, false) });
			ret.setTangent(true);
			return ret;
		}
		else
		{
			double t = sqrt(fabs(term1));
			return DmVectorSolutions({ p + d * (t - a1) / d2, p - d * (t + a1) / d2 });
		}
	}
}

/// @brief 计算两圆/圆弧的交点
/// @details 圆弧当作圆处理，所以圆弧得到的交点不一定在圆弧上
/// @param e1 实体1指针（必须是圆或圆弧类型）
/// @param e2 实体2指针（必须是圆或圆弧类型）
/// @return 相切返回1个交点，相交返回2个，不相交返回0个
DmVectorSolutions Information::getIntersectionArcArc(DmEntity const* e1, DmEntity const* e2)
{
	DmVectorSolutions ret;

	if (!(e1 && e2))
	{
		return ret;
	}

	if (e1->getEntityType() != DM::EntityArc && e1->getEntityType() != DM::EntityCircle)
	{
		return ret;
	}
	if (e2->getEntityType() != DM::EntityArc && e2->getEntityType() != DM::EntityCircle)
	{
		return ret;
	}

	DmVector c1 = e1->getCenter();
	DmVector c2 = e2->getCenter();
	double r1 = e1->getRadius();
	double r2 = e2->getRadius();

	// c1到c2的向量
	DmVector u = c2 - c1;

	// 同心圆/圆弧
	if (u.magnitude() < CONCENTRIC_TOLERANCE)
	{
		return ret;
	}

	// 判断圆的交点是否存在，并计算交点
	// （1）先计算特殊情况下的两个圆交点：分别以P1(0,0),P2(d,0)为圆心，半径为r1,r2作圆
	// 令s=1/2*[(r1^2-r2^2)/d^2 + 1.0], term=r1^2/(d^2) - s^2
	// 通过几何计算或方程求解，可得交点坐标x=cos(alpha)*r1 =(r1^2+d^2-r2^2)/(2*d) =s*d
	// 交点y坐标：y^2=r1^2-x^2 =r1^2-(s*d)^2 =d^2*term，=> y=±d*sqrt(term)
	// 如果term<0，则y无解，所以两个圆无交点
	// （2）在一般情况下，以上x表示交点在P1P2方向上的偏移，y表示P1P2垂直方向的偏移
	// 所以一般情况下的交点P = P1 + s*u ± sqrt(term)*v
	DmVector v = DmVector(u.y, -u.x);
	double s, t1, t2, term;
	s = 1.0 / 2.0 * ((r1 * r1 - r2 * r2) / (Math2d::pow(u.magnitude(), 2.0)) + 1.0);
	term = (r1 * r1) / (Math2d::pow(u.magnitude(), 2.0)) - s * s;

	// 没有交点
	if (term < 0.0)
	{
		ret = DmVectorSolutions();
	}
	else
	{
		t1 = sqrt(term);
		t2 = -sqrt(term);
		bool tangent = false;
		DmVector sol1 = c1 + u * s + v * t1;
		DmVector sol2 = c1 + u * s + v * t2;

		// 两个圆相切
		if (sol1.distanceTo(sol2) < INTERSECTION_TOLERANCE)
		{
			sol2 = DmVector(false);
			ret = DmVectorSolutions({ sol1 });
			tangent = true;
		}
		else
		{
			ret = DmVectorSolutions({ sol1, sol2 });
		}

		ret.setTangent(tangent);
	}

	return ret;
}

/// @brief 计算两椭圆的交点
/// @param e1 椭圆1指针
/// @param e2 椭圆2指针
/// @return 交点集合
DmVectorSolutions Information::getIntersectionEllipseEllipse(DmEllipse const* e1, DmEllipse const* e2)
{
	DmVectorSolutions ret;

	if (!(e1 && e2))
	{
		return ret;
	}
	if ((e1->getCenter() - e2->getCenter()).squared() < DM_TOLERANCE2 &&
		(e1->getMajorP() - e2->getMajorP()).squared() < DM_TOLERANCE2 &&
		fabs(e1->getRatio() - e2->getRatio()) < DM_TOLERANCE)
	{
		return ret;
	}
	DmEllipse ellipse01(nullptr, e1->getData());

	DmEllipse* e01 = &ellipse01;
	if (e01->getMajorRadius() < e01->getMinorRadius())
	{
		e01->switchMajorMinor();
	}

	DmEllipse ellipse02(nullptr, e2->getData());
	DmEllipse* e02 = &ellipse02;

	if (e02->getMajorRadius() < e02->getMinorRadius())
	{
		e02->switchMajorMinor();
	}
	DmVector shiftc1 = -e01->getCenter();
	double shifta1 = -e01->getAngle();
	e02->move(shiftc1);
	e02->rotate(shifta1);

	double a1 = e01->getMajorRadius();
	double b1 = e01->getMinorRadius();
	double x2 = e02->getCenter().x;
	double y2 = e02->getCenter().y;
	double a2 = e02->getMajorRadius();
	double b2 = e02->getMinorRadius();

	if (e01->getMinorRadius() < DM_TOLERANCE || e01->getRatio() < DM_TOLERANCE)
	{
		DmLine line(e1->getParent(), {{-a1, 0.}, {a1, 0.}});
		ret = getIntersectionEllipseLine(&line, e02);
		ret.rotate(-shifta1);
		ret.move(-shiftc1);
		return ret;
	}
	if (e02->getMinorRadius() < DM_TOLERANCE || e02->getRatio() < DM_TOLERANCE)
	{
		DmLine line(e1->getParent(), {{-a2, 0.}, {a2, 0.}});
		line.rotateAngle({ 0., 0. }, e02->getAngle());
		line.move(e02->getCenter());
		ret = getIntersectionEllipseLine(&line, e01);
		ret.rotate(-shifta1);
		ret.move(-shiftc1);
		return ret;
	}

	double t2 = -e02->getAngle();

	double cs = cos(t2), si = sin(t2);
	double ucs = x2 * cs, usi = x2 * si;
	double vcs = y2 * cs, vsi = y2 * si;
	double cs2 = cs * cs, si2 = 1 - cs2;
	double tcssi = 2. * cs * si;
	double ia2 = 1. / (a2 * a2), ib2 = 1. / (b2 * b2);
	std::vector<double> m(0, 0.);
	m.push_back(1. / (a1 * a1));                  // ma000
	m.push_back(1. / (b1 * b1));                  // ma011
	m.push_back(cs2 * ia2 + si2 * ib2);           // ma100
	m.push_back(cs * si * (ib2 - ia2));           // ma101
	m.push_back(si2 * ia2 + cs2 * ib2);           // ma111
	m.push_back((y2 * tcssi - 2. * x2 * cs2) * ia2 - (y2 * tcssi + 2 * x2 * si2) * ib2); // mb10
	m.push_back((x2 * tcssi - 2. * y2 * si2) * ia2 - (x2 * tcssi + 2 * y2 * cs2) * ib2); // mb11
	m.push_back((ucs - vsi) * (ucs - vsi) * ia2 + (usi + vcs) * (usi + vcs) * ib2 - 1.); // mc1
	auto vs0 = Math2d::simultaneousQuadraticSolver(m);
	shifta1 = -shifta1;
	shiftc1 = -shiftc1;
	for (DmVector vp : vs0)
	{
		vp.rotate(shifta1);
		vp.move(shiftc1);
		ret.push_back(vp);
	}
	return ret;
}

/// @brief 计算圆与椭圆的交点
/// @details 将圆转换为椭圆后调用椭圆-椭圆交点计算
/// @param c1 圆指针
/// @param e1 椭圆指针
/// @return 交点集合
DmVectorSolutions Information::getIntersectionCircleEllipse(DmCircle* c1, DmEllipse* e1)
{
	DmVectorSolutions ret;
	if (!(c1 && e1))
	{
		return ret;
	}

	DmEllipse const e2(c1->getParent(), {c1->getCenter(), {c1->getRadius(), 0.}, DmVector(0.0, 0.0, 1.0), 1.0, true, 0., FULL_CIRCLE_ANGLE});
	return getIntersectionEllipseEllipse(e1, &e2);
}

/// @brief 计算圆弧与椭圆的交点
/// @details 将圆弧转换为椭圆后调用椭圆-椭圆交点计算
/// @param a1 圆弧指针
/// @param e1 椭圆指针
/// @return 交点集合
DmVectorSolutions Information::getIntersectionArcEllipse(DmArc* a1, DmEllipse* e1)
{
	DmVectorSolutions ret;
	if (!(a1 && e1))
	{
		return ret;
	}
	// TODO : 圆弧reverse去除了，需要改逻辑
	DmEllipse const e2(a1->getParent(), {a1->getCenter(), {a1->getRadius(), 0.}, DmVector(0.0, 0.0, 1.0), 1.0, false, a1->getStartAngle(), a1->getEndAngle()});
	return getIntersectionEllipseEllipse(e1, &e2);
}

/// @brief 计算直线与椭圆的交点
/// @param line 直线指针
/// @param ellipse 椭圆指针
/// @return 交点集合（0~2个交点）
DmVectorSolutions Information::getIntersectionEllipseLine(DmLine* line, DmEllipse* ellipse)
{
	DmVectorSolutions ret;

	if (!(line && ellipse))
	{
		return ret;
	}

	// rotate into normal position:
	double rx = ellipse->getMajorRadius();
	if (rx < DM_TOLERANCE)
	{
		DmVector vp(line->getNearestPointOnEntity(ellipse->getCenter(), true));
		if ((vp - ellipse->getCenter()).squared() < DM_TOLERANCE2)
		{
			ret.push_back(vp);
		}
		return ret;
	}
	DmVector angleVector = ellipse->getMajorP().scale(DmVector(1. / rx, -1. / rx));
	double ry = rx * ellipse->getRatio();
	DmVector center = ellipse->getCenter();
	DmVector a1 = line->getStartpoint().rotate(center, angleVector);
	DmVector a2 = line->getEndpoint().rotate(center, angleVector);
	DmVector dir = a2 - a1;
	DmVector diff = a1 - center;
	DmVector mDir = DmVector(dir.x / (rx * rx), dir.y / (ry * ry));
	DmVector mDiff = DmVector(diff.x / (rx * rx), diff.y / (ry * ry));

	double a = DmVector::dotP(dir, mDir);
	double b = DmVector::dotP(dir, mDiff);
	double c = DmVector::dotP(diff, mDiff) - 1.0;
	double d = b * b - a * c;

	if (d < -ELLIPSE_QUADRATIC_TOLERANCE * DM_TOLERANCE * sqrt(DM_TOLERANCE))
	{
		return ret;
	}
	if (d < 0.)
	{
		d = 0.;
	}
	double root = sqrt(d);
	double t_a = -b / a;
	double t_b = root / a;

	ret.push_back(a1.lerp(a2, t_a + t_b));
	DmVector vp(a1.lerp(a2, t_a - t_b));
	if ((ret.get(0) - vp).squared() > DM_TOLERANCE2)
	{
		ret.push_back(vp);
	}
	angleVector.y *= -1.;
	ret.rotate(center, angleVector);
	return ret;
}

/// @brief 从4条直线构造四边形
/// @param container 包含4条直线的实体容器
/// @return 按角度排序的四边形顶点集合，非4条直线时返回空
DmVectorSolutions Information::createQuadrilateral(const DmEntityContainer& container)
{
	DmVectorSolutions ret;
	if (container.size() != QUADRILATERAL_EDGE_COUNT)
	{
		return ret;
	}
	DmEntityContainer c(container);
	std::vector<DmLine*> lines;
	for (auto e : c)
	{
		if (e->getEntityType() != DM::EntityLine)
		{
			return ret;
		}
		lines.push_back(static_cast<DmLine*>(e));
	}
	if (lines.size() != QUADRILATERAL_EDGE_COUNT)
	{
		return ret;
	}

	// find intersections
	std::vector<DmVector> vertices;
	for (auto it = lines.begin() + 1; it != lines.end(); ++it)
	{
		for (auto jt = lines.begin(); jt != it; ++jt)
		{
			DmVectorSolutions const& sol = Information::getIntersectionLineLine(*it, *jt);
			if (sol.size())
			{
				vertices.push_back(sol.at(0));
			}
		}
	}

	switch (vertices.size())
	{
	default:
		return ret;
	case QUADRILATERAL_EDGE_COUNT:
		break;
	case 5:
	case 6:
		for (DmLine* pl : lines)
		{
			const double a0 = pl->getDirection1();
			std::vector<std::vector<DmVector>::iterator> left;
			std::vector<std::vector<DmVector>::iterator> right;
			for (auto it = vertices.begin(); it != vertices.end(); ++it)
			{
				DmVector const& dir = *it - pl->getNearestPointOnEntity(*it, false);
				if (dir.squared() < DM_TOLERANCE15)
				{
					continue;
				}
				if (remainder(dir.angle() - a0, FULL_CIRCLE_ANGLE) > 0.)
				{
					left.push_back(it);
				}
				else
				{
					right.push_back(it);
				}

				if (left.size() == 2 && right.size() == 1)
				{
					vertices.erase(right[0]);
					break;
				}
				else if (left.size() == 1 && right.size() == 2)
				{
					vertices.erase(left[0]);
					break;
				}
			}
			if (vertices.size() == QUADRILATERAL_EDGE_COUNT)
			{
				break;
			}
		}
		break;
	}

	DmVector center{ 0., 0. };
	for (const DmVector& vp : vertices)
	{
		center += vp;
	}
	center *= QUADRILATERAL_CENTER_WEIGHT;
	std::sort(vertices.begin(), vertices.end(), [&center](const DmVector& a, const DmVector& b)->bool {return center.angleTo(a) < center.angleTo(b); });
	for (const DmVector& vp : vertices)
	{
		ret.push_back(vp);
	}
	return ret;
}
