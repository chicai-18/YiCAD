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


/// @file DmAtomicEntity.h
/// @brief 原子实体基类，所有不可分解的实体（线、弧、圆等）的抽象基类

#ifndef DMATOMICENTITY_H
#define DMATOMICENTITY_H

#include "DmEntity.h"

class CircleData;

class DmAtomicEntity : public DmEntity
{
    TYPESYSTEM_HEADER();

public:
    DmAtomicEntity(DmEntity* parent = nullptr);

    /// @brief 实体是否为容器
    bool isContainer() const override;

    DmVector getEndpoint() const override;
    DmVector getStartpoint() const override;

    double getDirection1() const override;
    double getDirection2() const override;

    DmVector getCenter() const override;
    double getRadius() const override;

    DmVector getNearestCenter(const DmVector& /*coord*/, double* /*dist*/) const override;

    virtual void setStartpointSelected(bool select);
    virtual void setEndpointSelected(bool select);
    virtual bool isTangent(const CircleData& /* circleData */) const;

    bool isStartpointSelected() const;
    bool isEndpointSelected() const;

    bool offset(const DmVector& /*position*/, const double& /*distance*/) override;

    virtual void moveStartpoint(const DmVector& /*pos*/);
    virtual void moveEndpoint(const DmVector& /*pos*/);

    virtual void trimStartpoint(const DmVector& pos);
    virtual void trimEndpoint(const DmVector& pos);

    virtual DM::Ending getTrimPoint(const DmVector& /*coord*/, const DmVector& /*trimPoint*/);

    void moveSelectedRef(const DmVector& ref, const DmVector& offset) override;

    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& rdr, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;
};

#endif
