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


/// @file DmCachePainter.cpp
/// @brief DmCachePainter 实现，管理实体缓存和OpenGL绘制

#include "DmCachePainter.h"
#include "GLCacheWorldDraw.h"
#include "IHighlightSource.h"
#include "ISelectionSource.h"

DmCachePainter::DmCachePainter()
    : m_bIsModefied(true)
{
    m_cachePainter = new opengl::GLCachePainter();
}

void DmCachePainter::translateView(double x, double y)
{
    m_cachePainter->translateView(x, y);
}

void DmCachePainter::create_resources()
{
    m_cachePainter->create_resources();
}

void DmCachePainter::new_device_size(unsigned int width, unsigned int height)
{
    m_cachePainter->new_device_size(width, height);
}

void DmCachePainter::scale(double s, double x_world, double y_world)
{
    m_cachePainter->scale(s, x_world, y_world);
}

void DmCachePainter::setScale(double s)
{
    m_cachePainter->setScale(s);
}

void DmCachePainter::setViewPosition(double posx, double posy)
{
    m_cachePainter->setViewPosition(posx, posy);
}

void DmCachePainter::addContainer(DmEntityContainer* container)
{
    m_containerList.emplace_back(container);
}

void DmCachePainter::clearContainers()
{
    m_containerList.clear();
}

void DmCachePainter::rebuild()
{
    m_cachePainter->removeAllCache();
    m_nurbsSamples.beginSweep();
    cacheVisibleEntities();
    cacheSelected();
    cacheHighlight();
    m_nurbsSamples.endSweep();
    m_cachePainter->generateGLData();
}

void DmCachePainter::rebuildSelected()
{
    m_cachePainter->removeCacheByGroup(opengl::CacheGroupType::Selected);
    m_cachePainter->removeSelectedPointsCache();
    cacheSelected();
    m_cachePainter->generateGLDataByType(opengl::CacheGroupType::Selected);
}

void DmCachePainter::rebuildHighlight()
{
    m_cachePainter->removeCacheByGroup(opengl::CacheGroupType::Highlight);
    cacheHighlight();
    m_cachePainter->generateGLDataByType(opengl::CacheGroupType::Highlight);
}

void DmCachePainter::update()
{
    //recache();
    if (m_bIsModefied)
    {
        rebuild();
    }
    else
    {
        // 普通组不随选择集、高亮集变化（RENDER_PLAN.md 1.1 步）；高亮组不含选中的实体，选择集变了也要重建
        if (m_bSelectChanged)
        {
            rebuildSelected();
        }
        if (m_bSelectChanged || m_bHighlightChanged)
        {
            rebuildHighlight();
        }
    }
    m_bIsModefied = false;
    m_bSelectChanged = false;
    m_bHighlightChanged = false;
}

void DmCachePainter::draw()
{
    update();
    m_cachePainter->stroke();
}

void DmCachePainter::drawHighlight()
{
    update();
    m_cachePainter->strokeHighlight();
}

void DmCachePainter::drawSelectedPoints()
{
    update();
    m_cachePainter->strokeSelectedPoints();
}

void DmCachePainter::specifyModified()
{
    m_bIsModefied = true;
}

void DmCachePainter::specifySelectChanged()
{
    m_bSelectChanged = true;
}

void DmCachePainter::specifyHighlightChanged()
{
    m_bHighlightChanged = true;
}

bool DmCachePainter::isModified() const
{
    return m_bIsModefied;
}

bool DmCachePainter::isSelectChanged() const
{
    return !m_bIsModefied && m_bSelectChanged;
}

bool DmCachePainter::isHighlightChanged() const
{
    return !m_bIsModefied && !m_bSelectChanged && m_bHighlightChanged;
}

void DmCachePainter::setModelOffset(const DmVector& offset)
{
    m_cachePainter->setModelOffset(offset.x, offset.y);
}

bool DmCachePainter::isDisplayLineWidth() const
{
    return m_cachePainter->isDisplayLineWidth();
}

void DmCachePainter::setIsDisplayLineWidth(bool display)
{
    m_cachePainter->setIsDisplayLineWidth(display);
}

void DmCachePainter::setSelectedColor(const QColor& c)
{
    m_cachePainter->setSelectedColor(c);
}

void DmCachePainter::setHighlightColor(const QColor& c)
{
    m_cachePainter->setHighlightColor(c);
}

void DmCachePainter::setSelectionSource(const ISelectionSource* source)
{
    m_selectionSource = source;
    specifySelectChanged();
}

void DmCachePainter::setHighlightSource(const IHighlightSource* source)
{
    m_highlightSource = source;
    specifyHighlightChanged();
}

bool DmCachePainter::isSelected(const DmEntity* e) const
{
    return m_selectionSource && m_selectionSource->isSelected(*e);
}

void DmCachePainter::cacheVisibleEntities()
{
    // 每个实体经 GI 描述自己，适配器把图元写进普通组；不按类型分支，也不展平子实体（RENDER_PLAN.md 2.3 步）
    GLCacheWorldDraw wd(*m_cachePainter, opengl::CacheGroupType::Normal, m_nurbsSamples);
    for (auto en : m_containerList)
    {
        for (auto e : *en)
        {
            if (e->isVisible())
            {
                wd.drawEntity(*e);
            }
        }
    }
}

void DmCachePainter::cacheSelected()
{
    if (!m_selectionSource)
    {
        return;
    }
    const std::vector<DmEntity*> selected = m_selectionSource->selectedEntities();
    GLCacheWorldDraw wd(*m_cachePainter, opengl::CacheGroupType::Selected, m_nurbsSamples);
    for (auto e : selected)
    {
        wd.drawEntity(*e);
    }
    cacheSelectedPoints(selected);
}

void DmCachePainter::cacheHighlight()
{
    if (!m_highlightSource)
    {
        return;
    }
    // 来源给出的都是可见的顶层实体；选中优先：已选中的按选中色画，不进高亮组
    GLCacheWorldDraw wd(*m_cachePainter, opengl::CacheGroupType::Highlight, m_nurbsSamples);
    for (auto e : m_highlightSource->highlightedEntities())
    {
        if (!isSelected(e))
        {
            wd.drawEntity(*e);
        }
    }
}

void DmCachePainter::cacheSelectedPoints(const std::vector<DmEntity*>& selected)
{
    constexpr size_t kMaxSelectedPoints = 100;
    std::vector<DmVector> selectedPts;
    selectedPts.reserve(kMaxSelectedPoints);
    for (auto e : selected)
    {
        for (auto pt : e->getRefPoints())
        {
            selectedPts.emplace_back(pt);
        }
        if (selectedPts.size() > kMaxSelectedPoints)
        {
            return;
        }
    }

    for (auto pt : selectedPts)
    {
        m_cachePainter->addSelectedPoints(pt.x, pt.y);
    }
}
