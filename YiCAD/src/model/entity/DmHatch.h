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

#include <cstdint>
#include <vector>

#include "HatchData.h"

class DmEntityContainer;

/// @brief 一条图案线：填充边界内的一整段，与它的图案、起点在图案里的位置（RENDER_PLAN.md 第 4.5.1 节）
/// @details 图形系统按图案与相位画划线（GI 的 setLinePattern），相位锚定在图案原点，不做端点对齐
struct DmHatchPatternRun
{
    DmVector start;
    DmVector end;
    std::uint32_t pattern = 0;   ///< 图案线定义的序号（DmHatch::getPatternDashes() 的下标）
    double phase = 0.0;          ///< 起点在图案里的位置
};

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

    /// @brief 获取填充生成的实体容器（图案的划线逐段切开的线段与点，供选择、捕捉、炸开）
    DmEntityContainerPtr getFilledEntities() const;

    /// @brief 图案线（画图用：每条一整段，划线由图形系统按图案画）
    const std::vector<DmHatchPatternRun>& getPatternRuns() const { return m_patternRuns; }

    /// @brief 各条图案线定义的划线（已按图案比例缩放；空为实线）
    const std::vector<std::vector<double>>& getPatternDashes() const { return m_patternDashes; }

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

    /// @brief 记下一条图案线：边界内 a、b 之间的一整段，相位按它在图案线上相对 origin 的位置（沿 dir）算
    /// @param pattern 图案线定义的序号
    /// @param period 图案的周期；实线为 0
    void addPatternRun(const DmVector& a, const DmVector& b, const DmVector& origin, const DmVector& dir,
                       std::uint32_t pattern, double period);

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

    /// @brief 经 GI 描述自身几何（RENDER_PLAN.md 第 4.2 节）
    void worldDraw(IGiWorldDraw& wd) const override;

    DmVectorSolutions getRefPoints() const override;
    void moveRef(const DmVector& ref, const DmVector& offset) override;
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
    /// @brief 边界随之改归目标文档；填充生成的实体在 update() 时重新生成
    void transferReferences(DmDocumentTransfer& transfer) override;

    HatchData data;                          ///< 填充数据
    DmEntityContainerPtr m_filledEntities;   ///< 填充生成的实体容器
    std::vector<DmHatchPatternRun> m_patternRuns;          ///< 图案线
    std::vector<std::vector<double>> m_patternDashes;      ///< 各条图案线定义的划线
};

#endif // DMHATCH_H
