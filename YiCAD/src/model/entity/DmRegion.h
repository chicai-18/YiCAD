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


/// @file DmRegion.h
/// @brief 区域实体类，可包含孔洞，用于DmHatch等

#ifndef DMREGION_H
#define DMREGION_H

#include "DmEntity.h"
#include "DmTriangle.h"
#include "RegionData.h"
#include "ConstrainedDelaunayTriangulation.h"

/// @brief 区域，可包含孔洞。DmHatch中有使用
class DmRegion : public DmEntity
{
    TYPESYSTEM_HEADER();
public:
    DmRegion() = default;
    DmRegion(DmEntity* parent, const RegionData& d);

    DmEntity* clone() const override;
    DM::EntityType getEntityType() const override;
    /// @brief 获得数据，边界未深度复制
    RegionData getData() const;
    void setData(const RegionData& d);
    /// @brief 获得克隆数据，边界也克隆
    RegionData getCloneData() const;
    bool isContainer() const override;
    void calculateBorders() override;
    std::list<DmEntity*> getSubEntities() const override;
    int size() const;
    void update() override;

    /// @brief 判断点是否在区域内，孔洞内的不算
    /// @details 采用射线法
    /// @param  onBoundary 点是否在边界或孔洞边界上
    bool isPointInside(const DmVector& pt, bool* onBoundary = nullptr) const;
    void isPointInside_subroutineForSegment(const DmVector& point, const DmLine* ray, DmEntity* e, bool* onContour, int& counter, bool& sure) const;
    /// @brief 将边界及孔洞放入一个container，这个container没有所有权
    DmEntityContainerPtr getCombineBoundary() const;
    /// @brief 获得三角化之后的三角形
    /// @param considerHoles 考虑孔洞的效果
    template<class T>
    void getTriangles(std::vector<T>& triangles, bool considerHoles);
    /// @brief 获得一个边界的点
    static void getPointsOfOneBoundary(DmEntityContainerPtr boundary, std::vector<DmVector>& pts);

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    DmVector getNearestEndpoint(const DmVector & coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity = true, double* dist = nullptr, DmEntity * *entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector & coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector & coord, double* dist = nullptr, int middlePoints = 1) const override;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    RegionData data;

};

extern template void DmRegion::getTriangles<DmTriangle*>(std::vector<DmTriangle*>&, bool );
extern template void DmRegion::getTriangles<DmTrianglePtr>(std::vector<DmTrianglePtr>&, bool );

using DmRegionPtr = std::shared_ptr<DmRegion>;


#endif //DMREGION_H
