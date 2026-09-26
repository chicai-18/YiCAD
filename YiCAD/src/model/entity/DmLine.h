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


/// @file DmLine.h
/// @brief 线段实体，支持两点定义、变换、求交和顶点渲染

#ifndef DMLINE_H
#define DMLINE_H

#include "DmAtomicEntity.h"
#include "LineData.h"

class Quadratic;

using DmLinePtr = std::shared_ptr<DmLine>;

/// @class DmLine
/// @brief 线段
class DmLine : public DmAtomicEntity
{
    TYPESYSTEM_HEADER();
public:
    DmLine() = default;

    /// @brief 通过数据和父实体构造线段
    DmLine(DmEntity* parent, const LineData& d);

    /// @brief 通过父实体和两点构造线段
    DmLine(DmEntity* parent, const DmVector& pStart, const DmVector& pEnd);

    /// @brief 通过两点构造线段
    DmLine(const DmVector& pStart, const DmVector& pEnd);

    DmEntity* clone() const override;

    /// @brief 获取实体类型
    DM::EntityType getEntityType() const override;

    /// @brief 获取线段几何数据
    LineData getData() const;

    /// @brief 设置线段几何数据
    void setData(const LineData& d);

    DmVectorSolutions getRefPoints() const override;
    DmVector getStartpoint() const override;
    DmVector getEndpoint() const override;

    /// @brief 设置起点
    void setStartpoint(DmVector s);

    /// @brief 设置终点
    void setEndpoint(DmVector e);

    /// @return Direction 1. The angle at which the line starts at the startpoint.
    double getDirection1() const override;

    /// @return Direction 2. The angle at which the line starts at the endpoint.
    double getDirection2() const override;

    DmVector getTangentDirection(const DmVector& point) const override;
    void moveStartpoint(const DmVector& pos) override;
    void moveEndpoint(const DmVector& pos) override;
    DM::Ending getTrimPoint(const DmVector& trimCoord,
        const DmVector& trimPoint) override;

    bool hasEndpointsWithinWindow(const DmVector& v1,
        const DmVector& v2) override;

    /// @return The length of the line.
    double getLength() const override;

    /// @return The angle of the line (from start to endpoint).
    double getStartAngle() const;

    /// @return The angle of the line (from end to startpoint).
    double getEndAngle() const;

    bool isTangent(const CircleData& circleData) const override;

    /// @return a perpendicular vector
    DmVector getNormalVector() const;

    DmVector getMiddlePoint() const override;
    DmVector getNearestEndpoint(const DmVector& coord,
        double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord,
        bool onEntity = true, double* dist = nullptr,
        DmEntity** entity = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord,
        double* dist = nullptr, int middlePoints = 1) const override;

    std::vector<DmEntity*> offsetTwoSides(const double& distance) const override;
    bool offset(const DmVector& coord, const double& distance) override;
    void move(const DmVector& offset) override;

    /// @brief 以原点为中心旋转
    void rotate(const double& angle);

    void rotate(const DmVector& center, const DmVector& angleVector) override;

    /// @brief 以原点为中心缩放
    void scale(const DmVector& factor) override;

    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1,
        const DmVector& axisPoint2) override;
    void moveRef(const DmVector& ref, const DmVector& offset) override;

    void calculateBorders() override;

    /// @brief 获得绘制用的顶点数据
    const std::vector<float>& getVerticesRef(int& float_count_per_vertex);

    void update() override;

    /// @brief 更新绘制用的顶点数据
    void updateVertices();

    /// @brief getQuadratic() returns the equation of the entity for quadratic,
    /// @return a vector contains: m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0
    /// for linear: m0 x + m1 y + m2 =0
    Quadratic getQuadratic() const override;

    std::list<DmEntity*> getSubEntities() const override;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader,
        const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    LineData data;

private:
    bool isModify = false; ///< 修改标志
};

#endif // DMLINE_H
