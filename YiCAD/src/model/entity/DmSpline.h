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


/// @file DmSpline.h
/// @brief B样条曲线实体类，支持控制点/拟合点两种模式，求交、最近点等运算

#ifndef DMSPLINE_H
#define DMSPLINE_H

#include <vector>

#include "DmLineStrip.h"
#include "SplineData.h"

class DmEllipse;

/// @class DmSpline
/// @brief B样条
class DmSpline : public DmEntity
{
	TYPESYSTEM_HEADER();

public:
	DmSpline(DmEntity* parent = nullptr);
	DmSpline(DmEntity* parent, const SplineData& d);
	~DmSpline();

	DmEntity* clone() const override;

	DM::EntityType getEntityType() const override;
	SplineData getData() const;
    void setData(const SplineData& d);
	void setDegree(size_t deg);
	int getDegree() const;
	int getNumberOfKnots();
    double getKnotAt(int i) const;
	size_t getNumberOfControlPoints() const;
    std::vector<double> getKnots() const;
	bool isClosed() const;
	void setClosed(bool c);
    void setKnots(const std::vector<double>& knots);
    void addControlPoint(const DmVector& v);
    void setControlPts(const std::vector<DmVector>& pts);
    std::vector<DmVector> getControlPoints() const;
    /// @brief 根据t获得关联的控制点
    std::vector<DmVector> getControlPoints(double t) const;
    DmVector getControlPointAt(int i) const;
    std::vector<DmVector> getFitPoints() const;
    void setFitPts(const std::vector<DmVector>& pts);
    /// @brief 是否为拟合曲线
    /// @return true为拟合曲线，false为控制点曲线
    bool isByFit() const;
    /// @brief 设置为曲线类型为控制点，或拟合点
    void setByFit(bool fit);

    bool isContainer() const override;

	void update() override;
    /// @brief 获得曲线上的点
    /// @param reverse 是否反向，反向是从终点到起点
    void getPoints(std::vector<DmVector>& pts, bool reverse = false);
    /// @brief 在t1，t2之间递归获得点，两点之间的夹角小于3度
    /// @param count 在一个递归中分段个数，可取为k
    void getPointsRecursive(double t1, double t2, double count, std::vector<DmVector>& pts, double maxStep) const;
    /// @brief 获得样条基函数Fi,k(t)在t处值。i为第i个k次B样条支撑区间左端节点的下标.定义之外获得0，比如定义域[t1, t2)，用t2就获得0
    double basisFunctionValue(double t, int i, int k) const;
    /// @brief 求样条基1次导数
    double basisDerivative(double t, int i, int k) const;
    /// @brief 求样条基Q次导数
    double basisDerivativeQ(double t, int i, int k, int q) const;
    /// @brief 获得Q阶导数
    DmVector derivativeQth(double t, int q) const;
    /// @brief 获得指定参数t的点，定义域之外获得无效点。对于闭合情况，上边界获得的点正确，对于不闭合情况，上边界获得的0点
    DmVector evaluate(double t) const;
    /// @brief 获得切向量
    DmVector derivative(double t) const;
    /// @brief 获得二阶导数
    DmVector secondDerivative(double t) const;
    /// @brief 获得定义域，[t1, t2)
    void getDomainOfDefinition(double& t1, double& t2) const;
    /// @brief 获得样条段数
    int getSegmentCount() const;
    /// @brief 是否有效
    bool isValid() const;
    /// @brief 保证t在定义域内
    double getValidT(double t) const;
    /// @brief 获得样条线的区间段。这是些离散的区间段，每个区间不会闭合，起终点切线夹角最多PI
    std::vector<std::pair<double, double>> getSections() const;
    /// @brief 获得样条线的区间段。通过离控制点最近的点来分割
    std::vector<std::pair<double, double>> getSectionsByCloseControlPoints() const;
    /// @brief 确定参数t所在节点区间的下标，未找到返回-1。如果为重节点，返回可能是重节点的任何一个索引
    int findSpan(double t) const;
    /// @brief 插入节点。同时会添加控制点，但不会更新实体
    bool insertKnot(double t);
//    /// @brief 去重。有异常，仅作记录
//    bool removeKnot(double t);
    /// @brief 反向。起终点切换，几何保持不变
    void reverse();
    /// @brief 根据数据点拟合计算出控制点及节点向量
    void fit();

    /// @brief 获得样条线距离指定点最近的点
    /// @return 返回第一个坐标为最近点，该点有效时，第二个值为对应的t，第三个为对应的距离
    std::tuple<DmVector, double, double> getClosetPt(const DmVector& coord) const;
    /// @brief 获得样条线距离指定点最近的【所有】点
    /// @details 有些最近点对应多个t（比如交点）
    std::vector<std::tuple<DmVector, double, double>> getClosetPts(const DmVector& coord) const;
    /// @brief 对于1阶的情况，计算指定段上的点对应的t。用于getClosetPt
    /// @param i 为该段直线对应的起始节点索引。对于第j段直线，i=k+j;
    /// @param nearestPt 该段直线上的点
    double getTForLine(int i, const DmVector& nearestPt) const;
    /// @brief 将闭合曲线转为非闭合曲线。
    /// @details 曲线形状不变，节点变为准均匀，节点控制点个数不变，但节点及控制点发生变化
    bool changeCloseToNormal();

    DmVectorSolutions getRefPoints() const override;
    DmVector getNearestRef(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestSelectedRef(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getStartpoint() const override;
    DmVector getEndpoint() const override;
	DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
	DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
	//void setVisible(bool v) override;
	//bool setSelected(bool select = true) override;
	//void setHighlighted(bool highlight = true) override;
    /// @brief 在端点处计算方向，如果实体类型支持该方向，则派生实体必须实现该方向
    virtual double getDirection1() const;
    virtual double getDirection2() const;

	void move(const DmVector& offset) override;
	void rotate(const DmVector& center, const DmVector& angleVector) override;
	void scale(const DmVector& center, const DmVector& factor) override;
	void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
	void moveRef(const DmVector& ref, const DmVector& offset) override;
	void calculateBorders() override;

	std::list<DmEntity*> getSubEntities() const override;
	DmLineStrip* getLineStrip() const;

	static DmVectorSolutions getIntersection(const DmEntity* e1, const DmEntity* e2);
	static DmVectorSolutions getIntersectionSplineOther(const DmSpline* spline1, const DmEntity* e2);
	static DmVectorSolutions getIntersectionSplineSpline(const DmSpline* spline1, const DmSpline* spline2);
    /// @brief 样条自交
    static DmVectorSolutions getIntersectionSplineSpline_selfIntersection(const DmSpline* spline1);
    /// @brief 样条与样条的交点（不能用于自交的情况）
    static DmVectorSolutions getIntersectionSplineSpline_intersectionOther(const DmSpline* spline1, const DmSpline* spline2);
    /// @brief 样条子区间与另一样条子区间求交（仅适用2次及以上曲线，假设至多2个交点，通过不同的初始值迭代求不同的交点）
    static DmVectorSolutions getIntersectionSplineSpline_bySection(const DmSpline* spline1, double t11, double t12, const DmSpline* spline2, double t21, double t22);
    /// @brief 样条线与直线求交
    static DmVectorSolutions getIntersectionSplineLine(const DmSpline* spline1, const DmLine* line);
    /// @brief 1阶样条线与其他实体（可以是直线、圆弧、圆、椭圆、椭圆弧）。直线按无限长处理，圆弧按圆处理，椭圆弧按椭圆处理
    /// @param spline1 必须为1阶样条
    static DmVectorSolutions getIntersectionSplineEntity_ForDegree1(const DmSpline* spline1, const DmEntity* e);
    /// @brief 样条线与圆求交
    static DmVectorSolutions getIntersectionSplineCircle(const DmSpline* spline1, const DmCircle* circle);
    /// @brief 样条线与圆弧求交（默认限制在圆弧上）
    /// @param onArc 是否限制在圆弧上
    static DmVectorSolutions getIntersectionSplineArc(const DmSpline* spline1, const DmArc* arc, bool onArc = true);
    /// @brief 样条线与椭圆（弧）求交（默认限制在椭圆弧上）
    /// @param onEllipse 是否限制在椭圆弧上
    static DmVectorSolutions getIntersectionSplineEllipse(const DmSpline* spline1, const DmEllipse* ellipse, bool onEllipse = true);
    /// @brief 样条线与椭圆(闭合)求交
    static DmVectorSolutions getIntersectionSplineEllipse_Closed(const DmSpline* spline1, const DmEllipse* ellipse);
    /// @brief 直线可能与样条一个段区间有2个交点，找到切线与直线平行的点（对应的t）。没有则返回false
    static bool getSplineTForLine(const DmSpline* spline1, const DmVector& lineDir, double t1, double t2, double& foundT);
    /// @brief 获得在(t1,t2)之间，且与两个边界夹角小于PI对应的t
    /// @details (t1,t2)之间切向连续且单调
    /// @param boundryDir1 边界1
    /// @param boundryDir2 边界2
    /// @param t1 用于迭代的，较边界1夹角较近的t
    /// @param t2 用于迭代的，较边界2夹角较近的t
    /// @param midT 结果
    /// @return 获得成功返回true
    static bool getSplineTForLimitAngles(const DmSpline* spline, bool isClockwise, const DmVector& boundryDir1, const DmVector& boundryDir2, double t1, double t2, double& midT);
    /// @brief 通过牛顿迭代法计算直线与样条线交点
    static DmVector newtonRaphsonBSplineLine(const DmSpline* bspline, const DmVector& linePoint, const DmVector& lineDirection, double initialT);
    /// @brief 通过牛顿迭代法计算圆与样条线交点
    static DmVector newtonRaphsonBSplineCircle(const DmSpline* bspline, const DmVector& circleCenter, double radius, double initialT);
    /// @brief 通过牛顿迭代法计算椭圆(闭合)与样条线交点
    static DmVector newtonRaphsonBSplineEllipse(const DmSpline* bspline, const DmEllipse* ellipse, double initialT);
    /// @brief 通过牛顿迭代法计算样条与样条的交点
    /// @details 不同的初始值可能得到不同的交点
    /// @param maxStep1 最大步长。用于在迭代中限制迭代步长，防止发散，可假设在区间长度的1/3，必须为正
    /// @return 返回的交点有效时算计算出交点（第一个值valid==true），第二个值为交点对应的曲线1的t，第三个值为交点对应的曲线2的t，
    static std::tuple<DmVector, double, double> newtonRaphsonBSplineSpline(const DmSpline* spline1, double initialT1, double maxStep1, const DmSpline* spline2, double initialT2,double maxStep2);
    ///  @brief 通过牛顿迭代法计算样条线上到指定点的最近点（或者点到样条线的投影）
    /// @param maxStep 最大步长。用于在迭代中限制迭代步长，防止发散，可假设在区间长度的1/3，必须为正
    /// @return 成功获得则DmVector.valid为true，否则为false
    static std::pair<DmVector, double> newtonRaphsonBSplinePoint(const DmSpline* bspline, const DmVector& pt, double t0, double maxStep);
    /// @brief 样条线控制点的凸包是否与一个包围框相交
    static bool isCrossBoundingBox(const DmSpline* bspline, const DmVector& min, const DmVector& max);
    /// @brief 在样条一个区间(t1,t2)之间获得一个点(t)，这个点与指定点的连线与样条在t处切线垂直（或者该区间曲线经过指定点），这个点可能是该区间距离指定点最近（或最远）的点。
    /// @details 仅适用于至多一个这样t的区间。采用的是牛顿迭代法
    /// @return 获得返回true，没有返回false
    static bool getTangentTOfPoint(const DmSpline* bspline, const DmVector& pt, double t1, double t2, double& t);
    /// @brief 获得1阶样条构成的直线
    static std::vector<std::shared_ptr<DmLine>> getLinesOfSplineDegree1(const DmSpline* bspline);
    /// @brief 对样条线单点打断
    static bool cutForSpline(const DmVector& cutCoord, DmSpline* spline, DmEntity*& cut1, DmEntity*& cut2);
    /// @brief 通过t打断样条
    static bool cutForSpline(DmSpline* spline, double t, DmEntity*& cut1, DmEntity*& cut2);
    /// @brief 对样条线2点打断
    /// @param cut1 准备删除的实体
    /// @param cut2 准备留下的实体
    static bool cutForSpline2P_Closed(const DmSpline* spline, const DmVector& firstCoord, const DmVector& secondCoord, DmEntity*& cut1, DmEntity*& cut2);
    /// @brief 将两条样条曲线合并为一条。须阶数相同且端点连接。连接后中间会产生重节点（没做去重）
    /// @param resSpline 合并之后的样条
    static bool combineTwoSplines(const DmSpline* spline1, const DmSpline* spline2, DmSpline*& resSpline);
    /// @brief 根据首尾控制点及阶数创建直线型样条。利用了局部凸包性，中间的控制点在首尾节点连成的直线上
    static DmSpline* createSplineWithTwoPoints(const DmVector& p1, const DmVector& p2, int k);
    /// @brief 根据数据点及是否闭合，获得数据点的节点及节点向量
    /// @param knots 节点向量
    /// @param u_waves 数据点的节点
    static bool getFitKnots(bool close, const std::vector<DmVector>& fitPoints, std::vector<double>& knots, std::vector<double>& u_waves);
    /// @brief 根据是否闭合，设置样条线控制点，节点向量，并更新曲线。仅限控制点样条
    /// @param ctrlPts 控制点。如果是闭合，不需要环绕，由该函数添加
    static void setControlPointsKnotsByClose(DmSpline* spline, bool isClosed, const std::vector<DmVector>& ctrlPts);
    /// @brief 根据n,k获得准均匀B样条的节点向量
    static void getKnotsOfQuasiUniform(int n, int k, std::vector<double>& knots);
    /// @brief 根据n,k获得均匀B样条的节点向量
    static void getKnotsOfUniform(int n, int k, std::vector<double>& knots);
    /// @brief 测试用。在距离控制点最近点生成黄色直线，在按次数均匀分段处生成红色直线
    static std::vector<DmLine*> test(DmSpline* spline);

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
	SplineData			data;
	DmLineStrip*	pLineStrip;
};

using DmSplinePtr = std::shared_ptr<DmSpline>;

#endif
