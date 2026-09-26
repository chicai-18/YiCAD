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


/// @file DmCircle.h
/// @brief 圆实体类，表示CAD中的圆

#ifndef DMCIRCLE_H
#define DMCIRCLE_H

#include <vector>

#include "DmAtomicEntity.h"
#include "CircleData.h"

class Quadratic;

class DmCircle : public DmAtomicEntity
{
    TYPESYSTEM_HEADER();

public:
    DmCircle() = default;

    /// @brief 通过父实体和圆数据构造
    /// @param parent 父实体指针
    /// @param d 圆数据
    DmCircle(DmEntity* parent, const CircleData& d);
    ~DmCircle() = default;

    DmEntity* clone() const override;

    DM::EntityType getEntityType() const override;

    /// @brief 获取圆的定义数据
    /// @return 圆数据的常量引用
    const CircleData& getData() const;

    DmVectorSolutions getRefPoints() const override;

    double getDirection1() const override;
    double getDirection2() const override;

    DmVector getCenter() const override;
    void setCenter(const DmVector& c);

    double getRadius() const override;
    void setRadius(double r);

    /// @brief 获取角度长度（弧度）
    /// @return 角度长度，圆始终返回 2*PI
    double getAngleLength() const;
    double getLength() const override;
    bool isTangent(const CircleData& circleData) const override;

    bool createFrom2P(const DmVector& p1, const DmVector& p2);
    bool createFrom3P(const DmVector& p1, const DmVector& p2, const DmVector& p3);
    std::vector<DmEntity*> offsetTwoSides(const double& distance) const override;
    static DmVectorSolutions createTan2(const std::vector<DmAtomicEntity*>& circles, const double& r);

    static std::vector<DmCircle> solveAppolloniusSingle(const std::vector<DmCircle>& circles);

    std::vector<DmCircle> createTan3(const std::vector<DmAtomicEntity*>& circles);
    bool testTan3(const std::vector<DmAtomicEntity*>& circles);
    DmVector getMiddlePoint(void) const override;
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = NULL, DmEntity** entity = NULL) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = NULL) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
    DmVector getNearestOrthTan(const DmVector& coord, const DmLine& normal, bool onEntity = false) const override;

    bool offset(const DmVector& coord, const double& distance) override;
    DmVectorSolutions getTangentPoint(const DmVector& point) const override;  // find the tangential points seeing from given point
    DmVector getTangentDirection(const DmVector& point) const override;
    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
    void moveRef(const DmVector& ref, const DmVector& offset) override;

    /// @brief 计算获得圆的离散化点
    /// @param center 圆心
    /// @param radius 半径
    /// @return 离散化点坐标
    std::vector<double> calculateVertexs(const DmVector& center, const double radius);

    /// @brief 获得绘制用的顶点数据
    /// @param float_count_per_vertex 输出：每个顶点的float数量
    /// @return 顶点数据引用
    const std::vector<float>& getVerticesRef(int& float_count_per_vertex);

    /// @brief 更新绘制用的顶点数据
    void updateVertices();
    void update() override;

    Quadratic getQuadratic() const override;

    void calculateBorders() override;

    std::list<DmEntity*> getSubEntities() const override;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    CircleData data;

private:
    bool isModify = true; ///< 是否已修改，需要更新顶点缓存
};

#endif
