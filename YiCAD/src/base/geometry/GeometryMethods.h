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

/// @file GeometryMethods.h
/// @brief 计算几何方法工具类

#ifndef GEOMETRYMETHODS_H
#define GEOMETRYMETHODS_H

#include <vector>
#include "DmVector.h"

/// @brief 几何计算方法类
class GeometryMethods
{
public:
    /// @brief 判断系列点是否为顺时针
    /// @param [in] polyPts 系列点，如果闭合不需要包含闭合点（至少3个）
    /// @return 顺时针返回true，否则返回false
    static bool isPtsClockwise(const std::vector<DmVector>& polyPts);

    /// @brief 判断点是否在多边形内
    /// @param [in] polyPts 系列点，如果闭合不需要包含闭合点（至少3个）
    /// @param [in] pt 测试点
    /// @return 在内部返回true
    static bool isPtInside(const std::vector<DmVector>& polyPts, const DmVector& pt);

    /// @brief 判断点是否在直线的左侧
    /// @param [in] lineStart 直线起点
    /// @param [in] lineEnd 直线终点
    /// @param [in] testPt 测试点
    /// @return 在左侧返回1，在右侧返回-1，在直线上返回0
    static int toLeftTest(const DmVector& lineStart, const DmVector& lineEnd, const DmVector& testPt);

    /// @brief 求两线段的交点。平行、重叠有线段、不相交返回无效的点。
    /// @param [in] line1Start 线段1起点
    /// @param [in] line1End 线段1终点
    /// @param [in] line2Start 线段2起点
    /// @param [in] line2End 线段2终点
    /// @return 交点坐标
    static DmVector getIntersectionOfTwoSegment(const DmVector& line1Start, const DmVector& line1End, const DmVector& line2Start, const DmVector& line2End);

    /// @brief 求两直线(无限长)的交点。平行返回无效的点。
    /// @param [in] line1Start 直线1起点
    /// @param [in] line1End 直线1终点
    /// @param [in] line2Start 直线2起点
    /// @param [in] line2End 直线2终点
    /// @return 交点坐标
    static DmVector getIntersectionOfTwoLine(const DmVector& line1Start, const DmVector& line1End, const DmVector& line2Start, const DmVector& line2End);

    /// @brief 判断两向量是否平行
    /// @param [in] vec1 向量1
    /// @param [in] vec2 向量2
    /// @return 平行返回true
    static bool isTwoVectorParallel(const DmVector& vec1, const DmVector& vec2);

    /// @brief 求平面向量的垂直单位向量(有两个值，只取一个)
    /// @param [in] v1 点1
    /// @param [in] v2 点2
    /// @return 垂直单位向量
    static DmVector getPerpendicularNormalizeVector(const DmVector& v1, const DmVector& v2);

    /// @brief 计算点到直线的垂足。如果直线起点终点重合，返回无效点
    /// @param [in] lineStart 直线起点
    /// @param [in] lineEnd 直线终点
    /// @param [in] testPt 测试点
    /// @return 垂足坐标
    static DmVector getPerpendicularFoot(const DmVector& lineStart, const DmVector& lineEnd, const DmVector& testPt);

    /// @brief 根据2点及凸度，计算生成圆弧所需的信息
    /// @param [in] pt1 起点
    /// @param [in] pt2 终点
    /// @param [in] bulge 凸度
    /// @param [out] center 圆心
    /// @param [out] radius 半径
    /// @param [out] startAngle 起始角度
    /// @param [out] endAngle 终止角度
    /// @param [out] normal 法向量
    static void getArcInfo(const DmVector& pt1, const DmVector& pt2, const double bulge, DmVector& center, double& radius, double& startAngle, double& endAngle, DmVector& normal);

    /// @brief 计算切向圆弧信息。通过切点、切向、鼠标点计算圆弧信息
    /// @param [in] tangentPt 切点
    /// @param [in] tangentDir 切向
    /// @param [in] mousePt 鼠标点
    /// @param [out] arcCenter 圆弧圆心
    /// @param [out] arcNormal 圆弧法向
    /// @param [out] arcRadius 圆弧半径
    /// @param [out] arcStartAngle 圆弧起始角
    /// @param [out] arcEndAngle 圆弧终止角
    /// @return 成功返回true
    static bool createArcInfoTangentialFree(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle);

    /// @brief 计算切向圆弧信息（锁定角度）。
    /// @param [in] lockAngle 锁定角度
    static bool createArcInfoTangentialLockAngle(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, const double& lockAngle, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle);

    /// @brief 计算切向圆弧信息（锁定半径）。
    /// @param [in] lockRadius 锁定半径
    static bool createArcInfoTangentialLockRadius(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, const double& lockRadius, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle);

    /// @brief 计算切向圆弧信息（锁定角度及半径）。
    /// @param [in] lockRadius 锁定半径
    /// @param [in] lockAngle 锁定角度
    static bool createArcInfoTangentialLockRadiusAngle(const DmVector& tangentPt, const DmVector& tangentDir, const DmVector& mousePt, const double& lockRadius, const double& lockAngle, DmVector& arcCenter, DmVector& arcNormal, double& arcRadius, double& arcStartAngle, double& arcEndAngle);

private:
    /// @brief 判断X方向射线是否与直线有交点。交点在直线起点上算相交，交点在直线终点上不算相交，直线与射线重合不算相交
    /// @param [in] lineStartPt 直线起点
    /// @param [in] lineEndPt 直线终点
    /// @param [in] rayStartPt 射线起点
    /// @return 有交点返回true
    static bool isXRayCrossLine(const DmVector& lineStartPt, const DmVector& lineEndPt, const DmVector& rayStartPt);

    /// @brief 求两直线的交点。必须保证存在交点（不平行）
    /// @param [in] line1Start 直线1起点
    /// @param [in] line1End 直线1终点
    /// @param [in] line2Start 直线2起点
    /// @param [in] line2End 直线2终点
    /// @return 交点坐标
    static DmVector getIntersectionOfTwoLinePrivate(const DmVector& line1Start, const DmVector& line1End, const DmVector& line2Start, const DmVector& line2End);
};

#endif
