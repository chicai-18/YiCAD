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

/// @file FindClosedRegion.h
/// @brief 闭合区域查找类，通过左转算法从实体集合中查找闭合区域

#ifndef FINDCLOSEDREGION_H
#define FINDCLOSEDREGION_H

#include "DmEntity.h"
#include "DmLine.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmSpline.h"
#include "DmPolyline.h"
#include "DmTriangle.h"
#include "DmRegion.h"
#include "DmBoundingBox.h"
#include "KDTree.h"

/// @brief 查找闭合区域
namespace FindClosedRegion
{
    class Edge;
    using EdgePtr = std::shared_ptr<Edge>;
    using EdgeWeakPtr = std::weak_ptr<Edge>;

    /// @class Edge
    /// @brief 边界边类，封装实体及其起终点、角度、曲率信息
    class Edge
    {
    public:
        Edge(const DmVector& startPos, const DmVector& endPos, double startAngle,
            double endAngle, bool needReverse, DmEntityPtr entity)
            : startPos(startPos), endPos(endPos), startAngle(startAngle), endAngle(endAngle)
            , needReverse(needReverse), entity(entity)
        {}

        /// @brief 是否为直线
        /// @return 是直线返回true
        bool isLine() const;

        /// @brief 获取起点曲率
        /// @return 起点曲率值
        double getStartCurvature();

        /// @brief 获取终点曲率
        /// @return 终点曲率值
        double getEndCurvature();

        /// @brief 根据需要反转方向
        void reverseByNeed();

        /// @brief 获取实体的采样点
        /// @param [in] entity 实体指针
        /// @param [in] reverse 是否反转
        /// @param [out] pts 输出点列表
        static void getPoints(DmEntity* entity, bool reverse, std::vector<DmVector>& pts);

        DmVector startPos{false};           ///< 起点位置
        DmVector endPos{false};             ///< 终点位置
        double startAngle = 0.0;            ///< 起始方位角
        double endAngle = 0.0;              ///< 结束方位角
        bool needReverse = false;           ///< entity是否需要反转
        DmEntityPtr entity;                 ///< 关联的实体
        EdgeWeakPtr reverseEdge;            ///< 相反的边
        bool isVisited = false;             ///< 左转算法时的标记，代表已访问
        bool isSuspend = false;             ///< 是否为孤悬的边

    private:
        /// @brief 计算曲率
        void calculateCurvature();

        /// @brief 计算圆弧曲率
        /// @param [in] arc 圆弧实体
        void calculateCurvatureForArc(DmArcPtr arc);

        /// @brief 计算椭圆弧曲率
        /// @param [in] ell 椭圆弧实体
        void calculateCurvatureForEllipse(DmEllipsePtr ell);

        /// @brief 计算样条线曲率
        /// @param [in] spline 样条线实体
        void calculateCurvatureForSpline(DmSplinePtr spline);

    private:
        double m_startCurvature = 0.0;          ///< 起点曲率
        double m_endCurvature = 0.0;            ///< 终点曲率
        bool m_hasCalculateCurvature = false;   ///< 是否已计算曲率
    };

    class Region;
    using RegionPtr = std::shared_ptr<Region>;
    using RegionWeakPtr = std::weak_ptr<Region>; // 避免对象相互引用，导致内存不能释放
    class RegionMgr;

    /// @class 区域
    /// @brief 闭合区域类，包含边界边、孔洞、子区域及三角化信息
    class Region
    {
    public:
        Region() = default;

        /// @brief 添加边界边
        /// @param [in] edge 边界边
        void add(EdgePtr edge);

        /// @brief 是否包含区域（从几何上）
        /// @param [in] region 待判断区域
        /// @return 包含返回true
        bool contains(const RegionPtr region);

        /// @brief 三角化（忽略孔洞）
        void triangulate();

        /// @brief 三角化（考虑孔洞）
        void triangulateConsiderHoles();

        /// @brief 计算外边界的点
        void calculatePoints();

        /// @brief 计算区域是顺时针还是逆时针
        void calculateDirection();

        /// @brief 是否为逆时针
        /// @return 逆时针返回true
        bool isAnticlockwise() const
        {
            return isAnticlockwise_;
        }

        /// @brief 获得外边界的点（需要在计算之后才有效）
        /// @return 边界点列表
        std::vector<DmVector> getPoints() const;

        /// @brief 判断点是否在区域内
        /// @details 通过三角化的三角形判断
        /// @param [in] pt 待判断点
        /// @param [in] considerNotInHole 为true表示在孔洞内的不算
        /// @return 在区域内返回true
        bool isPointInside(const DmVector& pt, bool considerNotInHole);

        /// @brief 是否存在边
        /// @param [in] edge 待查找边
        /// @return 存在返回true
        bool hasEdge(EdgePtr edge) const;

        /// @brief 获得外边界的个数
        /// @return 边界边数量
        int size() const
        {
            return static_cast<int>(edges.size());
        }

        /// @brief 从孔洞中查找包含指定点的区域
        /// @param [in] pt 点
        /// @return 包含点的子区域
        RegionPtr findSubRegionContainPoint(const DmVector& pt);

        /// @brief 递归查找包含点的内部区域
        /// @param [in] pt 点
        /// @return 包含点的区域
        RegionPtr findRegionContainPointRecursive(const DmVector& pt) const;

        /// @brief 创建区域对应的DmRegion
        /// @return DmRegion指针
        DmRegionPtr getDmRegion() const;

        std::vector<EdgePtr>::iterator begin()
        {
            return edges.begin();
        }

        std::vector<EdgePtr>::iterator end()
        {
            return edges.end();
        }

        std::vector<EdgePtr>::const_iterator begin() const
        {
            return edges.begin();
        }

        std::vector<EdgePtr>::const_iterator end() const
        {
            return edges.end();
        }

        friend class RegionMgr;

    public:
        std::vector<EdgePtr> edges;                         ///< （外）边界
        std::vector<RegionPtr> holes;                        ///< 包含的孔洞
        std::vector<RegionPtr> subBoundaries;                ///< 内部包含的区域（子区域）
        RegionWeakPtr parent;                                ///< 所属的区域
        std::vector<DmTrianglePtr> triangles;                ///< 三角化之后的三角形，不考虑孔洞情况
        std::vector<DmVector> points;                        ///< 边界的点，不考虑孔洞情况
        std::vector<DmTrianglePtr> triangles_without_holes;  ///< 三角化之后的三角形，去除孔洞内的区域
        bool isAnticlockwise_ = false;                       ///< 是否为逆时针

        DmBoundingBox box;                                   ///< 包围盒
        bool hasTriangulated = false;                        ///< 是否已三角化
        bool hasTriangulated_without_holes = false;          ///< 是否已三角化(考虑孔洞)
    };

    /// @brief 区域关系处理
    class RegionOwnershipMgr
    {
    public:
        /// @brief 添加区域
        /// @param [in] region 待添加区域
        void add(RegionPtr region);

        /// @brief 分组（计算区域所属关系）
        void group();

        /// @brief 移除顺时针的区域
        void removeHoles();

        /// @brief 获取所有区域
        /// @return 区域列表
        std::vector<RegionPtr> getRegions() const;

    private:
        /// @brief 区域存在边重合
        /// @details 这个方法主要是为了避免两个相同的边界（一个顺时针，一个逆时针），一个成为另一个的孔洞
        /// @param [in] region1 第一个区域
        /// @param [in] region2 第二个区域
        /// @return 存在边重合返回true
        bool regionHasEdgeOverlap(RegionPtr region1, RegionPtr region2);

        /// @brief 递归查找内部的孔洞/子区域，判断是否包含region
        /// @param [in] region 待查找区域
        /// @param [in] whereToFind 在哪个区域中查找
        /// @param [in] findHole 是否查找孔洞
        /// @return 找到的最小子区域
        RegionPtr findMinInterBoundaryRecursive(RegionPtr region, RegionPtr whereToFind, bool findHole);

        /// @brief 分组区域，形成层次关系
        /// @param [in] boundaries 输入边界列表
        /// @param [out] groupedBoundaries 分组后的边界列表
        void groupBoundaries(const std::list<RegionPtr>& boundaries, std::list<RegionPtr>& groupedBoundaries);

        /// @brief 添加孔洞到已分组区域
        /// @param [in,out] groupedBoundaries 分组后的边界列表
        /// @param [in,out] holes 孔洞列表
        void addHolesToBoundaries(std::list<RegionPtr>& groupedBoundaries, std::list<RegionPtr>& holes);

        /// @brief 如果有whereToFind的子区域在region之下，将之下的子区域放入region
        /// @param [in] region 目标区域
        /// @param [in] whereToFind 来源区域
        /// @return 有子区域被移动返回true
        bool addSubBoundaries(RegionPtr region, RegionPtr whereToFind);

    public:
        std::vector<RegionPtr> regions;  ///< 区域列表
    };

    /// @brief 边界检索用的树
    class EdgeSearchTree : public KDTree<DmVector>
    {
    public:
        EdgeSearchTree() = default;
        EdgeSearchTree(const std::vector<DmVector>& pts) = delete;

        /// @brief 设置点集和对应的边集
        /// @param [in] pts 点列表
        /// @param [in] edges 边列表
        void setPoints(const std::vector<DmVector>& pts, const std::vector<EdgePtr>& edges);

    private:
        std::vector<EdgePtr> m_edges;  ///< 边列表
    };

    /// @brief 闭合区域查找主类
    class FindClosedRegion
    {
    public:
        FindClosedRegion() = default;

        /// @brief 添加实体
        /// @param [in] e 待添加实体
        void addEntity(DmEntity* e);

        /// @brief 计算出闭合区域，计算闭合区域的归属关系
        void calculate();

        /// @brief 查找包含指定点的区域（孔洞内不算包含）
        /// @param [in] pt 待查找点
        /// @return 包含点的区域
        RegionPtr findRegionContainPoint(const DmVector& pt) const;

    private:
        /// @brief 执行左转算法
        void leftTurn();

        /// @brief 左转算法子程序，获得下一条边，仅处理2个及以上有效下一条边的情况
        /// @param [in,out] nextEdges 候选下一条边列表
        /// @param [in] curEdge 当前边
        /// @return 选中的下一条边
        EdgePtr leftTurn_getNextEdge(std::vector<EdgePtr>& nextEdges, EdgePtr curEdge);

        /// @brief 清理孤悬的边
        void clearSuspendEdges();

        /// @brief 添加一个实体到表中
        /// @param [in] e 实体指针
        void addAndSplitEntity(DmEntity* e);

        /// @brief 对单个实体重叠及求交处理，添加到表中
        /// @param [in] e 实体指针
        void addAndSplitEntity_singleEnt(DmEntityPtr e);

        /// @brief 对已做重叠处理的实体，根据交点分割并添加到表中
        /// @param [in] e 实体指针
        void addAndSplitEntity_singleOverlapClippedEnt(DmEntityPtr e);

        /// @brief 判断实体是否为有效类型
        /// @param [in] e 实体指针
        /// @return 有效返回true
        bool isValidEntity(DmEntity* e);

        /// @brief 分解多段线为基本实体
        /// @param [in] poly 多段线指针
        /// @return 分解后的实体列表
        std::vector<DmEntityPtr> getSplitOfPolyline(DmPolyline* poly);

        /// @brief 通过交点分割实体
        /// @param [in] ent 待分割实体
        /// @param [in] splitPts 分割点集
        /// @return 分割后的实体列表
        std::vector<DmEntityPtr> getSplitOfEntity(DmEntityPtr ent, const DmVectorSolutions& splitPts);

        /// @brief 通过交点分割直线
        /// @param [in] line 直线实体
        /// @param [in] splitPts 分割点集
        /// @return 分割后的直线列表
        std::vector<DmEntityPtr> getSplitOfLine(DmLinePtr line, const DmVectorSolutions& splitPts);

        /// @brief 通过交点分割圆弧
        /// @param [in] arc 圆弧实体
        /// @param [in] splitPts 分割点集
        /// @return 分割后的圆弧列表
        std::vector<DmEntityPtr> getSplitOfArc(DmArcPtr arc, const DmVectorSolutions& splitPts);

        /// @brief 通过交点分割椭圆弧
        /// @param [in] ellipse 椭圆弧实体
        /// @param [in] splitPts 分割点集
        /// @return 分割后的椭圆弧列表
        std::vector<DmEntityPtr> getSplitOfEllipse(DmEllipsePtr ellipse, const DmVectorSolutions& splitPts);

        /// @brief 通过交点分割样条线
        /// @param [in] spline 样条线实体
        /// @param [in] splitPts 分割点集
        /// @return 分割后的样条线列表
        std::vector<DmEntityPtr> getSplitOfSpline(DmSplinePtr spline, const DmVectorSolutions& splitPts);

        /// @brief 如果样条起终点重合，将其按中间t分割，返回分割后的样条
        /// @details 如果起终点不重合，直接返回传入的样条
        /// @param [in] spline 样条线实体
        /// @return 分割后的样条线列表
        std::vector<DmEntityPtr> getSplitOfClosedSpline(DmSplinePtr spline);

        /// @brief 对实体做重叠处理。若与表（m_splitEnts）中实体重叠，用表中实体修剪之，返回修剪后剩下的实体
        /// @param [in] e 实体指针
        /// @return 修剪后的实体列表
        std::vector<DmEntityPtr> getSplitOfEntityForOverlap(DmEntityPtr e);

        /// @brief 重叠处理（直线）
        /// @details 如果e与e2重叠，分割点为e的端点
        /// @param [in] e 直线实体1
        /// @param [in] e2 直线实体2
        /// @param [out] noNeedE2 为true表示e2与e有重叠，不再需要e2（仅须它剩余的部分）
        /// @param [out] leftEnts e2被分割后剩下的实体
        void getSplitOfLineForOverlap(DmLinePtr e, DmLinePtr e2, bool& noNeedE2,
            std::vector<DmEntityPtr>& leftEnts);

        /// @brief 重叠处理（圆弧）
        /// @param [in] e 圆弧实体1
        /// @param [in] e2 圆弧实体2
        /// @param [out] noNeedE2 为true表示e2与e有重叠
        /// @param [out] leftEnts e2被分割后剩下的实体
        void getSplitOfArcForOverlap(DmArcPtr e, DmArcPtr e2, bool& noNeedE2,
            std::vector<DmEntityPtr>& leftEnts);

        /// @brief 重叠处理（椭圆弧）
        /// @param [in] e 椭圆弧实体1
        /// @param [in] e2 椭圆弧实体2
        /// @param [out] noNeedE2 为true表示e2与e有重叠
        /// @param [out] leftEnts e2被分割后剩下的实体
        void getSplitOfEllipseForOverlap(DmEllipsePtr e, DmEllipsePtr e2, bool& noNeedE2,
            std::vector<DmEntityPtr>& leftEnts);

        /// @brief 两条样条线是否完全相等
        /// @param [in] e 样条线1
        /// @param [in] e2 样条线2
        /// @return 相等返回true
        bool isTwoSplinesEqual(DmSpline* e, DmSpline* e2);

        /// @brief 判断d是否在d1,d2之间（含边界）
        /// @param [in] d1 边界1
        /// @param [in] d2 边界2
        /// @param [in] d 待判断值
        /// @return 在范围内返回true
        static bool isBetween(double d1, double d2, double d);

        /// @brief 判断d是否在d1,d2之间（不含边界）
        /// @param [in] d1 边界1
        /// @param [in] d2 边界2
        /// @param [in] d 待判断值
        /// @return 在范围内返回true
        static bool isBetweenNoBoundary(double d1, double d2, double d);

    private:
        std::vector<DmEntityPtr> m_splitEnts;            ///< 添加的实体（添加过程中会按交点分割，这是保存的分割的实体）
        std::set<DmId> m_precessedEntityIds;              ///< 已处理的实体id（对于样条线，包含重合的）
        std::vector<DmEntity*> m_addedEntities;           ///< 已添加的实体（对于样条线，不包含重合的）
        std::vector<RegionPtr> m_regions;                 ///< 经计算后，区域是分层的，有所属关系
        std::vector<EdgePtr> m_edges;                     ///< 生成的边界
        std::shared_ptr<EdgeSearchTree> m_searchTree;     ///< 检索边界用的树
        bool m_modified = false;                          ///< 是否有修改
    };
}

#endif //FINDCLOSEDREGION_H
