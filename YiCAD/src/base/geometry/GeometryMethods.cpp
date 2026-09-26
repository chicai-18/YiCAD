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

/// @file GeometryMethods.cpp
/// @brief 计算几何方法工具类实现

#include "GeometryMethods.h"
#include "Math2d.h"
#include <cassert>

constexpr double COORD_TOLERANCE = 1e-8; ///< 坐标比较容差
constexpr int MIN_POINTS_FOR_POLYGON = 3; ///< 多边形最少点数
constexpr double MIN_X_INIT = 1e10;       ///< 最小X初始值

bool GeometryMethods::isPtsClockwise(const std::vector<DmVector>& polyPts)
{
    // 取出X最小角点，和其前后2点。用组成的向量叉乘
    // 参考：https://blog.csdn.net/D_bel/article/details/94005841
    if (polyPts.size() < MIN_POINTS_FOR_POLYGON)
    {
        return false;
    }
    int minIdx = -1;
    double minX = MIN_X_INIT;
    DmVector curPt;
    for (size_t i = 0; i < polyPts.size(); i++)
    {
        curPt = polyPts.at(i);
        if (curPt.x < minX)
        {
            minX = curPt.x;
            minIdx = static_cast<int>(i);
        }
    }

    int preIdx = -1;
    int postIdx = -1;
    if (minIdx == static_cast<int>(polyPts.size()) - 1)
    {
        preIdx = minIdx - 1;
        postIdx = 0;
    }
    else if (minIdx == 0)
    {
        preIdx = static_cast<int>(polyPts.size()) - 1;
        postIdx = minIdx + 1;
    }
    else
    {
        preIdx = minIdx - 1;
        postIdx = minIdx + 1;
    }

    DmVector preVec = polyPts.at(minIdx) - polyPts.at(preIdx);
    DmVector postVec = polyPts.at(postIdx) - polyPts.at(minIdx);
    DmVector crossProduct = DmVector::crossP(preVec, postVec);
    if (crossProduct.z > 0)
    {
        return false; // 逆时针
    }
    else
    {
        return true; // 顺时针
    }
}

bool GeometryMethods::isPtInside(const std::vector<DmVector>& polyPts, const DmVector& pt)
{
    int intersectNum = 0;
    if (polyPts.empty())
    {
        return false;
    }

    for (int i = 0; i < static_cast<int>(polyPts.size()) - 1; i++)
    {
        const DmVector& pt1 = polyPts.at(i);
        const DmVector& pt2 = polyPts.at(i + 1);
        bool isCross = isXRayCrossLine(pt1, pt2, pt);
        if (isCross)
        {
            intersectNum++;
        }
    }
    if (isXRayCrossLine(polyPts.back(), polyPts.front(), pt))
    {
        intersectNum++;
    }

    // 交点为奇数个，是在内部
    if (intersectNum % 2 == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int GeometryMethods::toLeftTest(const DmVector& lineStart, const DmVector& lineEnd, const DmVector& testPt)
{
    DmVector vecLine = lineEnd - lineStart;
    DmVector testVec = testPt - lineStart;
    double crossZ = vecLine.x * testVec.y - vecLine.y * testVec.x;
    if (abs(crossZ) < COORD_TOLERANCE)
    {
        return 0;
    }
    else if (crossZ > 0)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

DmVector GeometryMethods::getIntersectionOfTwoSegment(const DmVector& line1Start, const DmVector& line1End, const DmVector& line2Start, const DmVector& line2End)
{
    // 判断是否平行
    int start1Side = toLeftTest(line2Start, line2End, line1Start);
    int end1Side = toLeftTest(line2Start, line2End, line1End);
    int start2Side = toLeftTest(line1Start, line1End, line2Start);
    int end2Side = toLeftTest(line1Start, line1End, line2End);
    bool isOnExt = (start2Side == 0 && end2Side == 0); // 在line1所在直线上
    double dotProduct = 0.0;

    // 判断端点是否重合
    bool ssCoincide = abs(line1Start.x - line2Start.x) < COORD_TOLERANCE && abs(line1Start.y - line2Start.y) < COORD_TOLERANCE;
    bool seCoincide = abs(line1Start.x - line2End.x) < COORD_TOLERANCE && abs(line1Start.y - line2End.y) < COORD_TOLERANCE;
    bool esCoincide = abs(line1End.x - line2Start.x) < COORD_TOLERANCE && abs(line1End.y - line2Start.y) < COORD_TOLERANCE;
    bool eeCoincide = abs(line1End.x - line2End.x) < COORD_TOLERANCE && abs(line1End.y - line2End.y) < COORD_TOLERANCE;

    // 在同一延长线上
    if (isOnExt)
    {
        // 两线重合
        if ((ssCoincide || seCoincide) && (esCoincide || eeCoincide))
        {
            return DmVector(false);
        }
        // 两线重合于line1起点
        else if (ssCoincide || seCoincide)
        {
            if (ssCoincide)
            {
                dotProduct = (line1End - line1Start).dotP(line2End - line2Start);
            }
            else
            {
                dotProduct = (line1End - line1Start).dotP(line2Start - line2End);
            }
            // 平行且同向
            if (isOnExt && dotProduct > 0)
            {
                return DmVector(false);
            }
            else
            {
                return line1Start;
            }
        }
        // 两线重合于line1终点
        else if (esCoincide || eeCoincide)
        {
            if (esCoincide)
            {
                dotProduct = (line1Start - line1End).dotP(line2End - line2Start);
            }
            else
            {
                dotProduct = (line1Start - line1End).dotP(line2Start - line2End);
            }
            // 平行且同向
            if (isOnExt && dotProduct > 0)
            {
                return DmVector(false);
            }
            else
            {
                return line1End;
            }
        }
        else
        {
            return DmVector(false);
        }
    }

    // 不在同一延长线上，判断是否相交
    // 直线2个端点在另一条直线的不同端，相交
    if (start1Side * end1Side <= 0 && start2Side * end2Side <= 0)
    {
        // 相交
        return getIntersectionOfTwoLinePrivate(line1Start, line1End, line2Start, line2End);
    }
    else
    {
        return DmVector(false);
    }
}

DmVector GeometryMethods::getIntersectionOfTwoLine(const DmVector& line1Start, const DmVector& line1End, const DmVector& line2Start, const DmVector& line2End)
{
    if (isTwoVectorParallel(line1End - line1Start, line2End - line2Start))
    {
        return DmVector(false);
    }
    else
    {
        return getIntersectionOfTwoLinePrivate(line1Start, line1End, line2Start, line2End);
    }
}

bool GeometryMethods::isTwoVectorParallel(const DmVector& vec1, const DmVector& vec2)
{
    DmVector crossP = DmVector::crossP(vec1, vec2);
    if (abs(crossP.z) < COORD_TOLERANCE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

DmVector GeometryMethods::getPerpendicularNormalizeVector(const DmVector& v1, const DmVector& v2)
{
    if (abs(v1.x - v2.x) < COORD_TOLERANCE && abs(v1.y - v2.y) < COORD_TOLERANCE)
    {
        return DmVector(false);
    }
    else if (abs(v1.x - v2.x) < COORD_TOLERANCE)
    {
        return DmVector(1.0, 0.0, 0.0);
    }
    else if (abs(v1.y - v2.y) < COORD_TOLERANCE)
    {
        return DmVector(0.0, 1.0, 0.0);
    }

    auto vec = v2 - v1;
    double angle = atan(abs(vec.y) / abs(vec.x));
    double dx = tan(angle) * abs(vec.y);
    DmVector trangetVec(false);
    double x = 0.0;
    double y = 0.0;
    if (v2.y > v1.y)
    {
        y = -abs(vec.y);
    }
    else
    {
        y = abs(vec.y);
    }

    if (v2.x > v1.x)
    {
        x = dx;
    }
    else
    {
        x = -dx;
    }

    trangetVec = DmVector(v2.x + x, v2.y + y);
    DmVector v = trangetVec - v2;
    return v.normalize();
}

DmVector GeometryMethods::getPerpendicularFoot(const DmVector& lineStart, const DmVector& lineEnd, const DmVector& testPt)
{
    /*
     * 参考：https://zhuanlan.zhihu.com/p/524027353
     */
    double x1 = lineStart.x;
    double y1 = lineStart.y;
    double x2 = lineEnd.x;
    double y2 = lineEnd.y;
    double x0 = testPt.x;
    double y0 = testPt.y;

    // 直线起点终点重合，返回无效点
    if (abs(x1 - x2) < COORD_TOLERANCE && abs(y1 - y2) < COORD_TOLERANCE)
    {
        return DmVector(false);
    }

    double k_part1 = (x1 - x0) * (x2 - x1) + (y1 - y0) * (y2 - y1);
    double k_part2 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
    double k = -k_part1 / k_part2;
    double xf = k * (x2 - x1) + x1;
    double yf = k * (y2 - y1) + y1;
    return DmVector(xf, yf);
}

void GeometryMethods::getArcInfo(const DmVector& pt1, const DmVector& pt2, const double bulge, DmVector& center, double& radius, double& startAngle, double& endAngle, DmVector& normal)
{
    assert(bulge != 0.0);
    double alpha = atan(std::abs(bulge)) * 4.0;     // 圆弧对应的角度，为正，[0，2pi)
    DmVector middle = (pt2 + pt1) / 2.0;
    double dist = pt2.distanceTo(pt1) / 2.0;
    double angle = pt1.angleTo(pt2);    // 用于计算圆心位置
    radius = dist / sin(alpha / 2.0);
    double temp = fabs(radius * radius - dist * dist);
    double h = sqrt(temp);
    if (bulge < 0.0)
    {
        angle -= M_PI_2;
        normal = DmVector(0.0, 0.0, -1.0);
    }
    else
    {
        angle += M_PI_2;
        normal = DmVector(0.0, 0.0, 1.0);
    }
    if (fabs(alpha) > M_PI)
    {
        h *= -1.0;
    }
    center = DmVector::polar(h, angle);
    center += middle;
    startAngle = center.angleTo(pt1);
    endAngle = center.angleTo(pt2);
    if (bulge < 0.0)
    {
        startAngle = Math2d::correctAngle(M_PI - startAngle);
        endAngle = Math2d::correctAngle(M_PI - endAngle);
    }
}

bool GeometryMethods::createArcInfoTangentialFree(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle)
{
    DmVector chordVec = mousePt - tangentPt;
    double chordLength = chordVec.magnitude(); // 弦长
    // 弦长太短
    if (fabs(chordLength) < DM_MINDOUBLE)
    {
        return false;
    }
    DmVector chordDir = chordVec.normalize();
    // 弦方向不能与切线方向相同
    if (fabs(chordDir.dotP(tangentDir) - 1.0) < DM_MINDOUBLE)
    {
        return false;
    }

    // 计算圆弧信息
    DmVector crossP = DmVector::crossP(tangentDir, chordDir);
    DmVector normal(true);  // 圆弧在切点处的法向量（指向圆心）
    if (crossP.z > 0.0)
    {
        normal = DmVector(tangentDir).rotate(M_PI_2);
    }
    else
    {
        normal = DmVector(tangentDir).rotate(-M_PI_2);
    }
    double cosa = chordDir.dotP(normal); // a为弦与法向的夹角
    double radius = chordLength / (2.0 * cosa);
    DmVector center = tangentPt + normal * radius;
    double startAngle = center.angleTo(tangentPt);
    double endAngle = center.angleTo(mousePt);

    // 逆时针
    if (crossP.z > 0.0)
    {
        arcCenter = center;
        arcRadius = radius;
        arcStartAngle = startAngle;
        arcEndAngle = endAngle;
        arcNormal = DmVector(0.0, 0.0, 1.0);
    }
    // 顺时针
    else
    {
        double newStartAngle = Math2d::correctAngle(M_PI - startAngle);
        double newEndAngle = Math2d::correctAngle(M_PI - endAngle);
        arcCenter = center;
        arcRadius = radius;
        arcStartAngle = newStartAngle;
        arcEndAngle = newEndAngle;
        arcNormal = DmVector(0.0, 0.0, -1.0);
    }
    return true;
}

bool GeometryMethods::createArcInfoTangentialLockAngle(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, const double& lockAngle, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle)
{
    DmVector mouseVec = mousePt - tangentPt;
    double chordLength = mouseVec.magnitude(); // 弦长
    // 弦长太短
    if (fabs(chordLength) < DM_MINDOUBLE)
    {
        return false;
    }
    // 圆
    if (fabs(lockAngle - M_PI * 2) < DM_MINDOUBLE)
    {
        arcCenter = tangentPt;
        arcNormal = DmVector(0.0, 0.0, 1.0);
        arcRadius = chordLength / 2.0;
        arcStartAngle = 0.0;
        arcEndAngle = M_PI * 2.0;
        return true;
    }
    DmVector mouseDir = mouseVec.normalize();
    bool isClockwise = false;
    // 鼠标在切线延长线上
    if (fabs(mouseDir.dotP(tangentDir) - 1.0) < DM_MINDOUBLE)
    {
        isClockwise = false;
    }
    else
    {
        DmVector crossP = DmVector::crossP(tangentDir, mouseDir);
        // 逆时针
        if (crossP.z > 0.0)
        {
            isClockwise = false;
        }
        // 顺时针
        else
        {
            isClockwise = true;
        }
    }

    // 计算参数
    double theta = lockAngle;   // 圆弧所对应的向心角
    if (theta > M_PI)
    {
        theta = M_PI * 2.0 - theta;
    }
    double theta_2 = theta / 2.0;
    double radius = chordLength / (2.0 * sin(theta_2));
    DmVector center(true);
    if (isClockwise)
    {
        arcNormal = DmVector(0.0, 0.0, -1.0);
        arcCenter = DmVector(tangentDir).rotate(-M_PI_2) * radius + tangentPt;
        arcStartAngle = Math2d::correctAngle(M_PI - center.angleTo(tangentPt));
        arcEndAngle = Math2d::correctAngle(arcStartAngle + lockAngle);
    }
    else
    {
        arcNormal = DmVector(0.0, 0.0, 1.0);
        arcCenter = DmVector(tangentDir).rotate(M_PI_2) * radius + tangentPt;
        arcStartAngle = center.angleTo(tangentPt);
        arcEndAngle = Math2d::correctAngle(arcStartAngle + lockAngle);
    }
    return true;
}

bool GeometryMethods::createArcInfoTangentialLockRadius(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, const double& lockRadius, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle)
{
    DmVector mouseVec = mousePt - tangentPt;
    double chordLength = mouseVec.magnitude(); // 弦长
    DmVector normal(true);
    // 弦长太短
    if (fabs(chordLength) < DM_MINDOUBLE)
    {
        arcStartAngle = 0.0;
        arcEndAngle = M_PI * 2.0;
        normal = DmVector(tangentDir).rotate(M_PI / 2.0);
        arcCenter = tangentPt + normal * lockRadius;
        arcNormal = DmVector(0.0, 0.0, 1.0);
    }
    else
    {
        DmVector mouseDir = mouseVec.normalize();
        bool isClockwise = false;
        // 鼠标在切线延长线上
        if (fabs(mouseDir.dotP(tangentDir) - 1.0) < DM_MINDOUBLE)
        {
            arcStartAngle = 0.0;
            arcEndAngle = M_PI * 2.0;
            normal = DmVector(tangentDir).rotate(M_PI / 2.0);
            arcCenter = tangentPt + normal * lockRadius;
            arcNormal = DmVector(0.0, 0.0, 1.0);
        }
        else
        {
            DmVector crossP = DmVector::crossP(tangentDir, mouseDir);
            // 逆时针
            if (crossP.z > 0.0)
            {
                normal = DmVector(tangentDir).rotate(M_PI / 2.0);
                arcCenter = tangentPt + normal * lockRadius;
                arcStartAngle = (tangentPt - arcCenter).angle();
                arcEndAngle = (mousePt - arcCenter).angle();
                arcNormal = DmVector(0.0, 0.0, 1.0);
            }
            // 顺时针
            else
            {
                normal = DmVector(tangentDir).rotate(-M_PI / 2.0);
                arcCenter = tangentPt + normal * lockRadius;
                arcStartAngle = Math2d::correctAngle(M_PI - (tangentPt - arcCenter).angle());
                arcEndAngle = Math2d::correctAngle(M_PI - (mousePt - arcCenter).angle());
                arcNormal = DmVector(0.0, 0.0, -1.0);
            }
        }
    }
    arcRadius = lockRadius;
    return true;
}

bool GeometryMethods::createArcInfoTangentialLockRadiusAngle(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, const double& lockRadius, const double& lockAngle, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle)
{
    DmVector mouseVec = mousePt - tangentPt;
    double chordLength = mouseVec.magnitude(); // 弦长
    DmVector normal(true);
    // 弦长太短
    if (fabs(chordLength) < DM_MINDOUBLE)
    {
        arcNormal = DmVector(0.0, 0.0, 1.0);
        normal = DmVector(tangentDir).rotate(M_PI / 2.0);
    }
    else
    {
        DmVector mouseDir = mouseVec.normalize();
        // 鼠标在切线延长线上
        if (fabs(mouseDir.dotP(tangentDir) - 1.0) < DM_MINDOUBLE)
        {
            arcNormal = DmVector(0.0, 0.0, 1.0);
            normal = DmVector(tangentDir).rotate(M_PI / 2.0);
        }
        else
        {
            DmVector crossP = DmVector::crossP(tangentDir, mouseDir);
            // 逆时针
            if (crossP.z > 0.0)
            {
                normal = DmVector(tangentDir).rotate(M_PI / 2.0);
                arcNormal = DmVector(0.0, 0.0, 1.0);
            }
            // 顺时针
            else
            {
                normal = DmVector(tangentDir).rotate(-M_PI / 2.0);
                arcNormal = DmVector(0.0, 0.0, -1.0);
            }
        }
    }

    arcCenter = tangentPt + normal * lockRadius;
    if (arcNormal.z > 0.0)
    {
        arcStartAngle = (tangentPt - arcCenter).angle();
    }
    else
    {
        arcStartAngle = Math2d::correctAngle(M_PI - (tangentPt - arcCenter).angle());
    }
    arcEndAngle = Math2d::correctAngle(arcStartAngle + lockAngle);
    arcRadius = lockRadius;
    return true;
}

bool GeometryMethods::isXRayCrossLine(const DmVector& lineStartPt, const DmVector& lineEndPt, const DmVector& rayStartPt)
{
    if (lineStartPt.x + COORD_TOLERANCE < rayStartPt.x && lineEndPt.x + COORD_TOLERANCE < rayStartPt.x)
    {
        return false;
    }
    if (lineStartPt.y > rayStartPt.y + COORD_TOLERANCE && lineEndPt.y > rayStartPt.y + COORD_TOLERANCE)
    {
        return false;
    }
    if (lineStartPt.y < rayStartPt.y - COORD_TOLERANCE && lineEndPt.y < rayStartPt.y - COORD_TOLERANCE)
    {
        return false;
    }

    // 平行
    DmVector dir = lineEndPt - lineStartPt;
    // dir = dir / dir.magnitude();
    if (abs(dir.y) < COORD_TOLERANCE)
    {
        return false;
    }

    // 交点与终点重合，不算相交
    double deltaY = lineEndPt.y - lineStartPt.y;
    double dy = rayStartPt.y - lineStartPt.y;
    double t = dy / deltaY;
    DmVector intersectPt = lineStartPt.lerp(lineEndPt, t);
    if (intersectPt.squaredTo(lineEndPt) < COORD_TOLERANCE)
    {
        return false;
    }
    else
    {
        return true;
    }
}

DmVector GeometryMethods::getIntersectionOfTwoLinePrivate(const DmVector& line1Start, const DmVector& line1End, const DmVector& line2Start, const DmVector& line2End)
{
    /* 求交点，参考：https://blog.csdn.net/yangtrees/article/details/7965983
         线的一般方程为：F(x) = ax + by + c = 0，既然我们已经知道直线的两个点，假设为(x0,y0), (x1, y1)，那么可以得到a = y0 - y1, b = x1 - x0, c = x0y1 - x1y0。
         对于2条直线设为：F0(x) = a0*x + b0*y + c0 = 0, F1(x) = a1*x + b1*y + c1 = 0
         求解方程组，得到：
         x = (b0 * c1 - b1 * c0) / D
         y = (a1 * c0 - a0 * c1) / D
         D = a0 * b1 - a1 * b0，(D为0时，表示两直线重合)
    */
    double a0 = line1Start.y - line1End.y;
    double b0 = line1End.x - line1Start.x;
    double c0 = line1Start.x * line1End.y - line1End.x * line1Start.y;
    double a1 = line2Start.y - line2End.y;
    double b1 = line2End.x - line2Start.x;
    double c1 = line2Start.x * line2End.y - line2End.x * line2Start.y;
    double D = a0 * b1 - a1 * b0;
    double x = (b0 * c1 - b1 * c0) / D;
    double y = (a1 * c0 - a0 * c1) / D;
    return DmVector(x, y);
}
