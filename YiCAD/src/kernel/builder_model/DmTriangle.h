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


/// @file DmTriangle.h
/// @brief 三角形实体类，支持点判断、最近点计算等

#ifndef DMTRIANGLE_H
#define DMTRIANGLE_H

#include "DmEntity.h"
#include "TriangleData.h"

/// @brief 三角形实体
class DmTriangle : public DmEntity
{
    TYPESYSTEM_HEADER();
public:
    DmTriangle() = default;
    DmTriangle(DmEntity* parent, const TriangleData& d);

    DmEntity* clone() const override;
    DM::EntityType getEntityType() const override;
    TriangleData const& getData() const;
    void setData(const TriangleData& d);
    DmVector getPointAt(int i) const;
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
    double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr, DM::ResolveLevel level = DM::ResolveNone) const override;
    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
    void calculateBorders() override;
    std::list<DmEntity*> getSubEntities() const override;

    bool isInCrossWindow(const DmVector& v1, const DmVector& v2) const;
    bool isPointInside(const DmVector& pt) const;
    bool isContainer() const override;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    TriangleData data;
    bool isModify;
    constexpr static int POINT_SIZE = 3; ///< 三角形顶点个数
};

using DmTrianglePtr = std::shared_ptr<DmTriangle>;

#endif //DMTRIANGLE_H
