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


/// @file DmEllipse.h
/// @brief 椭圆/椭圆弧实体类

#ifndef DMELLIPSE_H
#define DMELLIPSE_H

#include "DmAtomicEntity.h"
#include "EllipseData.h"

class Quadratic;

// Holds the data that defines an ellipse.
// startAngle=endAngle=0.0 is reserved for whole ellipses
// add 2*M_PI to startAngle or endAngle to make whole range ellipse arcs
class DmEllipse : public DmAtomicEntity
{
    TYPESYSTEM_HEADER();

public:
    DmEllipse() = default;

    /// @brief 通过父实体和椭圆数据构造
    /// @param parent 父实体指针
    /// @param d 椭圆数据
    DmEllipse(DmEntity* parent, const EllipseData& d);

    DmEntity* clone() const override;
    DM::EntityType getEntityType() const override;

    /// @brief 获取椭圆数据
    /// @return 椭圆数据副本
    EllipseData getData() const;

    /// @brief 设置椭圆数据
    /// @param d 椭圆数据
    void setData(const EllipseData& d);

    DmVectorSolutions getRefPoints() const override;

    /// @brief 获得椭圆弧的起始点。对于椭圆得到无效的点（但是坐标默认是0）
    /// @return 起始点坐标
    DmVector getStartpoint() const override;

    /// @brief 获得椭圆的2个焦点，第一个是靠近MajorP的焦点
    /// @return 焦点坐标集合
    DmVectorSolutions getFoci() const;

    /// @brief 获得椭圆弧的终止点。对于椭圆得到无效的点（但是坐标默认是0）
    /// @return 终止点坐标
    DmVector getEndpoint() const override;

    /// @brief 获得指定参数的椭圆上的点
    /// @param param 参数值
    /// @return 椭圆上的点坐标
    DmVector getEllipsePoint(const double& param) const;

    /// @brief 获得椭圆中心到指定点的射线对应的参数，这个点不需要在椭圆上
    /// @param pos 坐标点
    /// @return 参数值
    double getEllipseParam(const DmVector& pos) const;

    /// @brief 获得椭圆法向
    /// @return 法向向量
    DmVector getNormal() const;

    void setNormal(const DmVector& normal);

    /// @brief 判断是否为顺时针
    /// @return 如果顺时针则返回true
    bool isClockwise() const;

    /// @brief 设置顺逆时针。如果顺逆时针变化，椭圆弧法向量，起始终止参数跟随变化，但是显示的效果不变。
    /// @param clockwise 是否为顺时针
    void setClockwise(const bool& clockwise);

    /// @brief 切换起始终止参数。显示效果是椭圆弧变为原来的"补"
    void switchStartEndParam();

    /// @brief 获得主轴的角度
    /// @return 主轴角度（弧度）
    double getAngle() const;

    /// @brief 获得指定角度（相对于世界x正轴）对应的参数
    /// @param angle 角度（弧度）
    /// @return 参数值
    double getParam(double angle) const;

    /// @brief 获得指定参数的角度（相对于世界x正轴）
    /// @param param 参数值
    /// @return 角度（弧度）
    double getAngle(double param) const;

    /// @brief 通过参数计算获得（相对于主轴）角度，范围[0-2pi)。顺时针未考虑
    /// @param param 参数值
    /// @return 角度
    double paramToAngle(double param) const;

    /// @brief 通过（相对于主轴）角度计算获得参数，范围[0-2pi)。顺时针未考虑
    /// @param angle 角度
    /// @return 参数值
    double angleToParam(double angle) const;

    /// @brief 获取起始参数
    /// @return 起始参数值
    double getStartParam() const;

    /// @brief 获得变换为逆时针椭圆弧的起始角度
    /// @return 起始角度
    double getStartParamNormal() const;

    void setStartParam(double a1);

    /// @brief 获取终止参数
    /// @return 终止参数值
    double getEndParam() const;

    /// @brief 获得变换为逆时针椭圆弧的终止角度
    /// @return 终止角度
    double getEndParamNormal() const;

    void setEndParam(double a2);

    /// @brief 获得指定参数对应[翻转正常]时的参数，几何上点的位置一样
    /// @param a 参数值
    /// @return 翻转后的参数值
    double getParaNormal(double a);

    /// @brief 通过计算获得起始角度（相对于主轴），范围[0-2pi)
    /// @return 起始角度
    double getStartAngle() const;

    /// @brief 通过起始角度（相对于主轴）设置起始参数
    /// @param startAngle 起始角度
    void setStartAngle(double startAngle);

    /// @brief 通过计算获得终止角度（相对于主轴），范围[0-2pi)
    /// @return 终止角度
    double getEndAngle() const;

    /// @brief 通过终止角度（相对于主轴）设置终止参数
    /// @param endAngle 终止角度
    void setEndAngle(double endAngle);

    /// @brief 获得椭圆的中心
    /// @return 中心点坐标
    DmVector getCenter() const override;

    // Sets new center.
    void setCenter(const DmVector& c);

    /// @brief 获取长轴端点（相对于中心）
    /// @return 长轴端点坐标
    DmVector getMajorP() const;

    // Sets new major point (relative to center).
    void setMajorP(const DmVector& p);

    /// @brief 获得短轴与长轴的比率
    /// @return 比率值
    double getRatio() const;

    void setRatio(double r);

    /// @brief 是否闭合（闭合则为椭圆，不闭合为椭圆弧）
    /// @return 如果闭合则返回true
    bool isClosed() const;

    void setClosed(bool closed);

    /// @brief 获取角度长度（弧度）
    /// @return 角度长度
    double getAngleLength() const;

    /// @brief 获取长轴半径
    /// @return 长轴半径
    double getMajorRadius() const;

    /// @brief 获得长轴端点（世界坐标）
    /// @return 长轴端点坐标
    DmVector getMajorPoint() const;

    /// @brief 获得短轴端点（世界坐标）
    /// @return 短轴端点坐标
    DmVector getMinorPoint() const;

    /// @brief 获取短轴半径
    /// @return 短轴半径
    double getMinorRadius() const;

    /// @brief 获得起始点的切向（朝向椭圆弧内部）
    /// @return 切向角度
    double getDirection1() const override;

    /// @brief 获得终止点的切向（朝向椭圆弧内部）
    /// @return 切向角度
    double getDirection2() const override;

    double getLength() const override;

    /// @brief return the equation of the entity a quadratic contains coefficients for quadratic:
    /// m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0
    /// for linear:
    /// m0 x + m1 y + m2 =0
    Quadratic getQuadratic() const override;

    void moveStartpoint(const DmVector& pos) override;
    void moveEndpoint(const DmVector& pos) override;

    /// @brief Ellipse must have ratio<1, and not reversed
    /// @param x1, ellipse angle
    /// @param x2, ellipse angle
    /// @return the arc length between ellipse angle x1, x2
    double getEllipseLength(double a1, double a2) const;
    double getEllipseLength(double a2) const;
    DmVectorSolutions getTangentPoint(const DmVector& point) const override;  // find the tangential points seeing from given point
    DmVector getTangentDirection(const DmVector& point) const override;
    DM::Ending getTrimPoint(const DmVector& trimCoord, const DmVector& trimPoint) override;

    /// @brief from quadratic form
    /// @param dn x^2 + dn[1] xy + dn[2] y^2 =1
    bool createFromQuadratic(const std::vector<double>& dn);
    // generic quadratic: A x^2 + C xy + B y^2 + D x + E y + F =0
    bool createFromQuadratic(const Quadratic& q);
    bool createInscribeQuadrilateral(const std::vector<DmLine*>& lines);
    DmVector getMiddlePoint(void) const override;
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;

    /// @brief 获得最近的焦点
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;

    //  TODO 这到底是个什么玩意
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
    DmVector getNearestOrthTan(const DmVector& coord, const DmLine& normal, bool onEntity = false) const override;
    bool isPointOnEntity(const DmVector& coord, double tolerance = DM_TOLERANCE) const override;

    /// @brief 点是否在椭圆内（或椭圆上）
    /// @details 不考虑是否闭合，统一按闭合处理
    /// @param coord 坐标点
    /// @param tolerance 容差
    /// @return 如果在椭圆内则返回true
    bool isPointInside(const DmVector& coord, double tolerance = DM_TOLERANCE) const;

    /// @brief 将指定点转变到椭圆局部坐标系。椭圆局部坐标系以长轴的x轴，椭圆中心在原点
    /// @param coord 世界坐标点
    /// @return 局部坐标点
    DmVector getLocalOfPoint(const DmVector& coord) const;

    void move(const DmVector& offset) override;
    void rotate(const double& angle);
    void rotate(const DmVector& angleVector);
    void rotate(const DmVector& center, const DmVector& angle) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
    void moveRef(const DmVector& ref, const DmVector& offset) override;

    /// @brief 计算获得椭圆的离散化点
    /// @param center 椭圆中心
    /// @param radius 半径
    /// @return 离散化点坐标
    std::vector<double> calculateVertexs(const DmVector& center, const double radius);

    /// @brief 获得绘制用的顶点数据
    /// @param float_count_per_vertex 输出：每个顶点的float数量
    /// @return 顶点数据引用
    const std::vector<float>& getVerticesRef(int& float_count_per_vertex);

    /// @brief 更新绘制用的顶点数据
    void updateVertices();

    /// @brief 获得曲线上的点
    /// @param pts 输出：点集合
    /// @param reverse 是否反向，反向是从终点到起点
    void getPoints(std::vector<DmVector>& pts, bool reverse = false);

    void update() override;
    void calculateBorders() override;
    std::list<DmEntity*> getSubEntities() const override;

    // 准备废弃
    bool switchMajorMinor(void);  // switch major minor axes to keep major the longer ellipse radius
    void correctAngles();         // make sure angleLength() is not more than 2*M_PI

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    EllipseData data; ///< 椭圆数据

private:
    bool isModify = true; ///< 是否已修改
};

using DmEllipsePtr = std::shared_ptr<DmEllipse>;

#endif
// EOF
