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


/// @file DmArc.h
/// @brief 圆弧实体类，支持创建、修改、捕捉等操作

#ifndef DMARC_H
#define DMARC_H

#include "DmAtomicEntity.h"
#include "ArcData.h"

class Quadratic;

class DmArc : public DmAtomicEntity
{
    TYPESYSTEM_HEADER();

public:
    DmArc() = default;
    DmArc(DmEntity* parent, const ArcData& d);

    DmEntity* clone() const override;

    DM::EntityType getEntityType() const override;

    ArcData getData() const;
    ArcData& getDataRef();
    const ArcData& getDataConstRef() const;

    DmVectorSolutions getRefPoints() const override;

    void setData(const ArcData& d);

    DmVector getCenter() const override;
    void setCenter(const DmVector& c);

    double getRadius() const override;
    void setRadius(double r);

    /// @return The start angle of this arc
    double getStartAngle() const;
    void setStartAngle(double a1);
    /// @brief 获得变换为逆时针圆弧的起始角度
    double getStartAngleNormal() const;
    /// @return The end angle of this arc
    double getEndAngle() const;
    void setEndAngle(double a2);
    /// @brief 获得变换为逆时针圆弧的终止角度
    double getEndAngleNormal() const;
    /// @brief 获得指定角度对应[翻转正常]时的角度，几何上点的位置一样
    double getAngleNormal(double a) const;

    // get angle relative arc center
    double getArcAngle(const DmVector& vp);

    /// @brief 获得起始点的切向（朝向圆弧内部）
    double getDirection1() const override;
    /// @brief 获得终止点的切向（朝向圆弧内部）
    double getDirection2() const override;

    bool isClockwise() const;
    /// @brief 设置顺逆时针。如果顺逆时针变化，圆弧法向量，起始终止角度跟随变化，但是显示的效果不变。
    void setClockwise(const bool& clockwise);
    ///@brief 切换起始终止角度。显示效果是圆弧变为原来的"补"
    void switchStartEndAngle();

    DmVector getNormal() const;
    void setNormal(const DmVector& normal);

    /// @return Start point of the entity.
    DmVector getStartpoint() const override;
    /// @return End point of the entity.
    DmVector getEndpoint() const override;
    std::vector<DmEntity*> offsetTwoSides(const double& distance) const override;
    void correctAngles();  // make sure angleLength() is not more than 2*M_PI
    void moveStartpoint(const DmVector& pos) override;
    void moveEndpoint(const DmVector& pos) override;
    bool offset(const DmVector& position, const double& distance) override;

    void trimStartpoint(const DmVector& pos) override;
    void trimEndpoint(const DmVector& pos) override;

    DM::Ending getTrimPoint(const DmVector& coord, const DmVector& trimPoint) override;

    DmVector getMiddlePoint() const override;
    /// @brief 获得圆弧的弧度值（0-2pi）
    double getAngleLength() const;
    double getLength() const override;
    double getBulge() const;

    bool createFrom3P(const DmVector& p1, const DmVector& p2, const DmVector& p3);
    bool createFrom2PBulge(const DmVector& startPoint, const DmVector& endPoint, double bulge);

    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    /// @brief 获得圆弧上距离指定点最近的点
    /// @param coord 指定点
    /// @param onEntity 是否最近点是否限制在圆弧上，为true时为圆弧上的点，为false时可以是对应圆上的点
    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
    DmVector getNearestOrthTan(const DmVector& coord, const DmLine& normal, bool onEntity = false) const override;
    DmVectorSolutions getTangentPoint(const DmVector& point) const override;  // find the tangential points seeing from given point
    DmVector getTangentDirection(const DmVector& point) const override;

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
    void moveRef(const DmVector& ref, const DmVector& offset) override;

    /// @brief 计算获得圆弧的离散化点（TODO 待废弃）
    std::vector<double> calculateVertexs(const DmVector& center, const double radius);
    /// @brief 获得绘制用的顶点数据
    const std::vector<float>& getVerticesRef(int& float_count_per_vertex);
    /// @brief 获得曲线上的点
    /// @param reverse 是否反向，反向是从终点到起点
    void getPoints(std::vector<DmVector>& pts, bool reverse = false);
    /// @brief 更新绘制用的顶点数据
    void updateVertices();
    void update() override;

    virtual void calculateBorders() override;

    virtual Quadratic getQuadratic() const override;

    std::list<DmEntity*> getSubEntities() const override;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    ArcData data;

private:
    bool    isModify = true;  ///< 标记是否需要重新计算顶点
};
using DmArcPtr = std::shared_ptr<DmArc>;

#endif
