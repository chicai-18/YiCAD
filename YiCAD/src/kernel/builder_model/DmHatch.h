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


/// @file DmHatch.h
/// @brief 填充（Hatch）实体，支持实体填充和图案填充

#ifndef DMHATCH_H
#define DMHATCH_H

#include "HatchData.h"

class DmEntityContainer;

class DmHatch : public DmEntity
{
    TYPESYSTEM_HEADER();
public:
    DmHatch() = default;

    /// @brief 构造填充实体
    /// @param [in] parent 父实体
    /// @param [in] hatchdata 填充数据
    DmHatch(DmEntity* parent, const HatchData& hatchdata);

    /// @brief 从已有填充复制构造
    /// @param [in] parent 父实体
    /// @param [in] hatchdata 源填充实体
    DmHatch(DmEntity* parent, DmHatch& hatchdata);

    /// @brief 拷贝构造函数
    DmHatch(const DmHatch& hatch);

    ~DmHatch();

    DmHatch* clone() const override;

    DM::EntityType getEntityType() const override;

    bool isContainer() const override;

    /// @brief 获取填充数据引用（可修改）
    /// @return 填充数据的引用
    HatchData& getDataRef();

    /// @brief 获取填充数据副本
    HatchData getData() const;

    /// @brief 设置填充数据
    void setData(const HatchData& hdata);

    bool setSelected(bool select) override;

    /// @brief 是否为实体填充
    bool isSolid() const;

    /// @brief 设置是否为实体填充
    void setSolid(bool solid);

    /// @brief 获取填充图案名称
    QString getPattern() const;

    /// @brief 设置填充图案名称
    void setPattern(const QString& pattern);

    /// @brief 获取图案比例
    double getScale() const;

    /// @brief 设置图案比例
    void setScale(double scale);

    /// @brief 获取图案角度
    double getAngle() const;

    /// @brief 设置图案角度
    void setAngle(double angle);

    /// @brief 设置填充边界区域
    void setBoundary(DmRegionPtr boundary);

    /// @brief 获取填充边界区域
    DmRegionPtr getBoundary() const;

    /// @brief 获取填充生成的实体容器
    DmEntityContainerPtr getFilledEntities() const;

    void calculateBorders() override;

    void update() override;

    /// @brief 将轮廓离散化，传入CDT做三角划分，用原始方法判断三角形中心是否在轮廓内
    void fillSolid();

    /// @brief 用一根无限长的pattern线填充轮廓
    /// @param [in] parent 父实体容器
    /// @param [in] pat 图案数据向量
    /// @param [in] minX 轮廓最小X
    /// @param [in] maxX 轮廓最大X
    /// @param [in] minY 轮廓最小Y
    /// @param [in] maxY 轮廓最大Y
    void fillPattern(DmEntityContainerPtr parent, const std::vector<double>& pat,
        double minX, double maxX, double minY, double maxY);

    /// @brief 对于pattern存在虚线的情况，计算实线及点的相对pattern线起始点的"坐标对"
    /// @param [in] dashLineLengths 虚线各段长度
    /// @param [out] dashPosPairs 输出的位置对列表
    void getDashPositionPairs(const std::vector<double>& dashLineLengths,
        std::vector<std::pair<double, double>>& dashPosPairs);

    /// @brief 用一条线段与轮廓求交。不含重复点，且按指定轴排序
    /// @param [in] linePt1 线段端点1
    /// @param [in] linePt2 线段端点2
    /// @param [in] orderByY 是否按Y轴排序
    /// @param [out] intersectPts 交点集合
    void intersectBoundariesWithLine(const DmVector& linePt1,
        const DmVector& linePt2, const bool orderByY,
        std::vector<DmVector>& intersectPts);

    /// @brief 在两个点之间按照指定的pattern线填充
    /// @param [in] parent 父实体容器
    /// @param [in] intersectPt1 交点1
    /// @param [in] intersectPt2 交点2
    /// @param [in] currentPatPt 当前图案线起始点
    /// @param [in] patDir 图案线方向
    /// @param [in] totalDashLen 虚线总长度
    /// @param [in] dashPosPairs 虚线位置对列表
    void fillDashBetweenTwoPoints(DmEntityContainerPtr parent,
        const DmVector& intersectPt1, const DmVector& intersectPt2,
        const DmVector& currentPatPt, const DmVector& patDir,
        const double totalDashLen,
        const std::vector<std::pair<double, double>>& dashPosPairs);

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    std::list<DmEntity*> getSubEntities() const override;

    DmVectorSolutions getRefPoints() const override;
    void moveRef(const DmVector& ref, const DmVector& offset) override;
    DmVector getNearestSelectedRef(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestRef(const DmVector& coord, double* dist = nullptr) const override;

    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true,
        double* dist = nullptr, DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr,
        int middlePoints = 1) const override;

    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    HatchData data;                          ///< 填充数据
    DmEntityContainerPtr m_filledEntities;   ///< 填充生成的实体容器
};

#endif // DMHATCH_H
