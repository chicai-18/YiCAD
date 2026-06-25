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

/// @file Preview.h
/// @brief 预览功能类，用于管理预览容器中的临时实体

#ifndef PREVIEW_H
#define PREVIEW_H

#include "DmEntityContainer.h"

/// @brief 预览
class Preview : public DmFlags
{
public:
    Preview(DmDocument* pDocument);
    ~Preview() = default;

    DM::EntityType getEntityType() const;

    /// @brief 克隆给定实体并添加到预览中
    /// @param entity 要克隆的实体指针
    void addCloneOf(DmEntity* entity);

    /// @brief 将文档中的选中实体克隆到预览
    void addSelectionFromDocument();

    /// @brief 将容器中的所有实体添加到预览中（不选中）
    /// @param container 实体容器引用
    void addAllFrom(DmEntityContainer& container);

    void clear();
    void setModelOffset(const DmVector& offset);

    void addEntity(DmEntity* pEntity);
    void appendEntity(DmEntity* pEntity);
    void removeEntity(DmEntity* pEntity);

    bool isEmpty();
    void move(DmVector offset);
    void moveRef(DmVector v1, DmVector v2);
    void setVisible(bool isVisble);

    DmEntityContainer* getEntityContainer();

private:
    /// @brief 通知视图预览已修改
    void specifyPreviewModified();

private:
    DmDocument*         m_pDocument = nullptr;          ///< 关联的文档指针
    DmEntityContainer*  m_pPreviewContainer = nullptr;  ///< 预览容器指针
};

#endif
