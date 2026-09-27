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

class IDocumentView;
class SelectionSet;

/// @brief 预览
///
/// 预览里是命令的临时实体，没有选中状态：预览画笔不设选择来源（doc/SELECTION_SET_PLAN.md 3.2 节）。
class Preview : public DmFlags
{
public:
    /// @brief 预览到指定视图的预览容器（视图可以是测试替身）
    /// @param selection 文档的选择集，addSelectionFromDocument() 从它取实体
    /// @param view 视图；为空时没有预览容器
    Preview(SelectionSet* selection, IDocumentView* view);
    ~Preview() override = default;

    DM::EntityType getEntityType() const;

    /// @brief 克隆给定实体并添加到预览中
    /// @param entity 要克隆的实体指针
    void addCloneOf(DmEntity* entity);

    /// @brief 将文档中的选中实体克隆到预览
    void addSelectionFromDocument();

    /// @brief 将容器中的所有实体克隆到预览中
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
    SelectionSet*       m_pSelection = nullptr;         ///< 文档的选择集
    IDocumentView*      m_pView = nullptr;              ///< 预览所在的视图
    DmEntityContainer*  m_pPreviewContainer = nullptr;  ///< 预览容器指针
};

#endif
