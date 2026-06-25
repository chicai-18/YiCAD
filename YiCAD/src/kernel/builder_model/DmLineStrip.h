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


/// @file DmLineStrip.h
/// @brief 可带线型的折线段实体（LineStrip），支持多点定义和顶点渲染

#ifndef DMLINESTRIP_H
#define DMLINESTRIP_H

#include "DmEntity.h"
#include "LineStripData.h"

class SpacialSearchTree;

/// @brief 可带线型的linestrip
class DmLineStrip : public DmEntity
{
    TYPESYSTEM_HEADER();

public:
    /// @brief 构造LineStrip实体
    /// @param [in] parent 父实体，默认为 nullptr
    DmLineStrip(DmEntity* parent = nullptr);

    /// @brief 通过数据和父实体构造LineStrip
    /// @param [in] parent 父实体
    /// @param [in] data LineStrip数据
    DmLineStrip(DmEntity* parent, const LineStripData& data);

    virtual bool isContainer() const override;
    virtual DmEntity* clone() const override;
    DM::EntityType getEntityType() const override;

    virtual DmVector getNearestPointOnEntity(const DmVector& coord,
        bool onEntity = true, double* dist = nullptr,
        DmEntity** entity = nullptr) const override;
    virtual DmVector getNearestCenter(const DmVector& coord,
        double* dist = nullptr) const override;
    virtual DmVector getNearestMiddle(const DmVector& coord,
        double* dist = nullptr, int middlePoints = 1) const override;
    DmVector getStartpoint() const override;
    DmVector getEndpoint() const override;

    /// @brief 获得linestrip经过的点
    std::vector<DmVector> getPoints() const;

    /// @brief 设置linestrip经过的点
    void setPoints(const std::vector<DmVector>& pts);

    /// @brief 清空linestrip经过的点
    void clear();

    /// @brief 是否为空
    bool isEmpty() const;

    /// @brief 生成实体（仅用于搜索树做快速求交，不用于绘制）、自交点、搜索树
    void update();

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1,
        const DmVector& axisPoint2) override;

    std::list<DmEntity*> getSubEntities() const override;
    void calculateBorders() override;

    /// @brief 获得绘制用的顶点数据
    const std::vector<float>& getVerticesRef(int& float_count_per_vertex);

    /// @brief 更新绘制用的顶点数据
    void updateVertices();

    /// @brief 是否闭合
    bool isClosed();

    /// @brief 设置闭合状态
    void setClosed(bool isClosed);

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader,
        const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;

protected:
    LineStripData data;

private:
    bool isModify = false; ///< 修改标志
};

#endif // DMLINESTRIP_H
