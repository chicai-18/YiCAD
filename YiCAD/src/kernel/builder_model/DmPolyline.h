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


/// @file DmPolyline.h
/// @brief 多段线实体类，由直线和圆弧段组成

#ifndef DMPOLYLINE_H
#define DMPOLYLINE_H

#include "DmEntity.h"
#include "DmEntityContainer.h"
#include "PolylineData.h"
#include <list>

/// @brief 多段线实体，包含多个直线/圆弧段和凸度信息
class DmPolyline : public DmEntity
{
    TYPESYSTEM_HEADER();

public:
    DmPolyline(DmEntity* parent = nullptr);
    DmPolyline(DmEntity* parent, const PolylineData& d);
    DmPolyline(const DmPolyline& poly);
    DmPolyline& operator=(const DmPolyline& poly) = delete;
    ~DmPolyline();

    DmEntity* clone() const override;

    /// @brief 获取实体类型
    /// @return 多段线实体类型
    DM::EntityType getEntityType() const override;

    /// @brief 获取多段线总长度
    /// @return 各段直线/圆弧长度之和
    double getLength() const override;

    /// @brief 获取多段线几何数据
    /// @return 多段线数据副本
    PolylineData getData() const;

    /// @brief 获取多段线几何数据引用
    /// @return 多段线数据引用
    PolylineData& getDataRef();

    /// @brief 获取多段线几何数据常量引用
    /// @return 常量引用
    const PolylineData& getDataConstRef() const;

    /// @brief 设置多段线几何数据
    /// @param data 新数据
    void setData(const PolylineData& data);

    /// @brief 获取起点
    /// @return 起点坐标
    DmVector getStartpoint() const override;

    /// @brief 设置图层名称（同时设置子实体图层）
    /// @param name 图层名称
    void setLayer(const QString& name);

    /// @brief 设置图层指针（同时设置子实体图层）
    /// @param l 图层指针
    void setLayer(DmLayer* l);

    bool setSelected(bool select = true) override;
    void setHighlighted(bool highlight = true) override;

    /// @brief 获取终点
    /// @return 终点坐标
    DmVector getEndpoint() const override;

    /// @brief 判断多段线是否闭合
    /// @return 闭合返回 true
    bool isClosed() const;

    /// @brief 设置闭合状态
    /// @param cl 是否闭合
    void setClosed(bool cl);

    /// @brief 获取指定索引的顶点
    /// @param i 顶点索引
    /// @return 顶点坐标
    DmVector getVertexAt(int i) const;

    /// @brief 获取顶点数量
    /// @return 顶点数
    int getVertexCount() const;

    /// @brief 获取指定索引的凸度
    /// @param i 凸度索引
    /// @return 凸度值
    double getBulgeAt(int i) const;

    /// @brief 获取线段段数
    /// @return 段数
    int getSegmentCount() const;

    /// @brief 获得第i段几何信息
    /// @param i 段索引
    /// @param [out] bulge 凸度
    /// @param [out] pt1 起点
    /// @param [out] pt2 终点
    /// @param [out] startWeight 起点线宽（可选）
    /// @param [out] endWeight 终点线宽（可选）
    void getSegmentInfoAt(int i, double& bulge, DmVector& pt1,
                          DmVector& pt2, double* startWeight = nullptr,
                          double* endWeight = nullptr) const;

    bool isContainer() const override;
    void calculateBorders() override;
    DmVectorSolutions getRefPoints() const override;
    DmVector getNearestRef(const DmVector& coord,
                           double* dist = nullptr) const override;
    DmVector getNearestSelectedRef(const DmVector& coord,
                                   double* dist = nullptr) const override;
    DmVector getNearestEndpoint(const DmVector& coord,
                                double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord,
                                     bool onEntity = true,
                                     double* dist = nullptr,
                                     DmEntity** entity = nullptr)
        const override;
    DmVector getNearestCenter(const DmVector& coord,
                              double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord,
                              double* dist = nullptr,
                              int middlePoints = 1) const override;

    /// @brief 在指定位置插入顶点
    /// @param i 插入位置索引
    /// @param v 顶点坐标
    /// @param bulge 凸度（默认0.0）
    /// @param startWeight 起点线宽（默认0.0）
    /// @param endWeight 终点线宽（默认0.0）
    void insertVertex(int i, const DmVector& v, double bulge = 0.0,
                      double startWeight = 0.0, double endWeight = 0.0);

    /// @brief 在末尾追加顶点
    /// @param v 顶点坐标
    /// @param bulge 凸度（默认0.0）
    /// @param startWeight 起点线宽（默认0.0）
    /// @param endWeight 终点线宽（默认0.0）
    void appendVertex(const DmVector& v, double bulge = 0.0,
                      double startWeight = 0.0, double endWeight = 0.0);

    void update() override;
    bool isValid() const;
    bool hasEntity() const;

    bool offset(const DmVector& coord, const double& distance) override;
    void move(const DmVector& offset) override;
    void rotate(const DmVector& center,
                const DmVector& angleVector) override;
    void scale(const DmVector& center,
               const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1,
                const DmVector& axisPoint2) override;

    void moveRef(const DmVector& ref, const DmVector& offset) override;

    std::list<DmEntity*> getSubEntities() const override;

    /// @brief 根据起点、终点、凸度、线宽生成实体（支持直线生成）
    /// @param pt1 起点
    /// @param pt2 终点
    /// @param bulge 凸度
    /// @param startLineWeight 起点线宽
    /// @param endLineWeight 终点线宽
    /// @param [out] ents 生成的实体列表
    static void getEntitiesByInfo(const DmVector& pt1, const DmVector& pt2,
                                  const double bulge,
                                  const double startLineWeight,
                                  const double endLineWeight,
                                  std::vector<DmEntity*>& ents);

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader,
                               const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    /// @brief 清空实体
    void clear();

    /// @brief 添加子实体
    /// @param e 实体指针
    void addEntity(DmEntity* e);

    /// @brief 根据顶点个数及是否闭合，修正凸度及线宽
    /// @return 成功返回 true，失败返回 false
    bool validateData();

private:
    /// @brief 根据几何信息生成并添加实体
    /// @param bulge 凸度
    /// @param startPt 起点
    /// @param endPt 终点
    /// @param startWeight 起点线宽
    /// @param endWeight 终点线宽
    void addEntityByInfo(double bulge, const DmVector& startPt,
                         const DmVector& endPt, double startWeight,
                         double endWeight);

protected:
    std::list<DmEntity*> entities;   ///< 子实体列表
    PolylineData data;                ///< 多段线几何数据
};

using DmPolylinePtr = std::shared_ptr<DmPolyline>;

#endif
