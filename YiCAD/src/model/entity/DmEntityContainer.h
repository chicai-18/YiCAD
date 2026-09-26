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


/// @file DmEntityContainer.h
/// @brief 实体容器类，管理实体集合

#ifndef DMENTITYCONTAINER_H
#define DMENTITYCONTAINER_H

#include <vector>
#include <list>

#include "DmEntity.h"

/// @brief 实体容器
class DmEntityContainer : public DmEntity
{
    TYPESYSTEM_HEADER();

public:
    DmEntityContainer(DmEntity* parent = nullptr, bool owner = true);
    ~DmEntityContainer() override;

    DmEntity* clone() const override;

    /// @brief 深度拷贝所有子实体 克隆实体后调用
    void detach();

    DM::EntityType getEntityType() const override;

    /// @brief 实体是否为容器
    /// @return true（容器类永远返回true）
    bool isContainer() const override;

    double getLength() const override;

    void setVisible(bool v) override;

    bool setSelected(bool select = true) override;
    bool toggleSelected() override; // 切换选中状态
    void setHighlighted(bool highlight = true) override;

    std::list<DmEntity*> selectEntitiesInWindow(DmVector v1, DmVector v2, bool cross = false);

    virtual void addEntity(DmEntity* entity);
    virtual void appendEntity(DmEntity* entity);
    virtual void prependEntity(DmEntity* entity);
    virtual void insertEntity(int index, DmEntity* entity);
    virtual bool removeEntity(DmEntity* entity);
    virtual void removeAt(int index);

    /// @brief 将四条线相加，由对角顶点v0、v1形成一个矩形
    /// @param v0 第一个对角点
    /// @param v1 第二个对角点
    void addRectangle(DmVector const& v0, DmVector const& v1);

    virtual DmEntity* firstEntity(DM::ResolveLevel level = DM::ResolveNone) const;
    virtual DmEntity* lastEntity(DM::ResolveLevel level = DM::ResolveNone) const;
    virtual DmEntity* nextEntity(DM::ResolveLevel level = DM::ResolveNone) const;
    virtual DmEntity* prevEntity(DM::ResolveLevel level = DM::ResolveNone) const;
    virtual DmEntity* entityAt(int index);
    virtual void setEntityAt(int index, DmEntity* en);
    virtual int findEntity(DmEntity const* const entity);
    virtual void clear();

    virtual bool isEmpty() const;
    int size() const;

    /// @brief 用指定实体调整文档边界（最大/最小值），不再计算子实体的边界
    /// @param entity 实体指针
    void adjustBorders(DmEntity* entity);

    /// @brief 用所有实体调整文档边界，不再计算子实体的边界
    void adjustBorders();

    void calculateBorders() override;
    void forcedCalculateBorders();
    void update() override;

    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;

    /// @brief 获取离坐标最近的实体
    DmEntity* getNearestEntity(const DmVector& point, double* dist = nullptr, DM::ResolveLevel level = DM::ResolveAll) const;

    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;

    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;

    DmVector getNearestRef(const DmVector& coord, double* dist = nullptr) const override;

    double getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level = DM::ResolveNone) const override;

    bool hasEndpointsWithinWindow(const DmVector& v1, const DmVector& v2) override;

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2a) override;

    void moveRef(const DmVector& ref, const DmVector& offset) override;
    void moveSelectedRef(const DmVector& ref, const DmVector& offset) override;

    bool isOwner() const;
    void setOwner(bool owner);

    QList<DmEntity*>::const_iterator begin() const;
    QList<DmEntity*>::const_iterator end() const;
    QList<DmEntity*>::iterator begin();
    QList<DmEntity*>::iterator end();

    // first and last without resolving into children, assume the container is not empty
    DmEntity* last() const;
    DmEntity* first() const;

    const QList<DmEntity*>& getEntityList();

    std::list<DmEntity*> getSubEntities() const override;

protected:
    QList<DmEntity*>            entities;               ///< 容器里的实体集

private:
    mutable int                 m_iEntIdx = -1;         ///< 当前迭代实体索引
    bool                        m_bAutoDelete = true;   ///< 是否自动删除实体
    mutable DmEntityContainer*  m_subContainer = nullptr; ///< 临时用于迭代的子容器
};

using DmEntityContainerPtr = std::shared_ptr<DmEntityContainer>;

#endif
