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
#include "DmPenList.h"
#include "DmPoint.h"
#include "DmLine.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmSolid.h"
#include "DmTriangle.h"
#include "DmEllipse.h"
#include "DmRay.h"
#include "DmXline.h"
#include "DmSpline.h"
#include "DmLineStrip.h"
#include "DmImage.h"
#include "IHighlightSource.h"
#include "ISelectionSource.h"
#include <QByteArrayView>
#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QImage>

namespace
{

/// @brief 图片来源，纹理按它缓存（GLImageTextureCache）：有文件时是文件的绝对路径、修改时间与大小，
///        文件在磁盘上改了就是新的来源；没有文件时是内嵌像素的尺寸与内容哈希
QString imageSource(DmImage* image)
{
    const std::string path = image->getData().getPath();
    if (!path.empty())
    {
        const QFileInfo info(QString::fromStdString(path));
        return QStringLiteral("file:%1|%2|%3")
            .arg(info.absoluteFilePath())
            .arg(info.lastModified().toMSecsSinceEpoch())
            .arg(info.size());
    }
    const unsigned char* bits = image->getbits();
    const qsizetype bytes = bits ? static_cast<qsizetype>(image->getBytesPerLine()) * image->getHeight() : 0;
    return QStringLiteral("bits:%1x%2|%3|%4")
        .arg(image->getWidth())
        .arg(image->getHeight())
        .arg(image->getBytesPerLine())
        .arg(qHash(QByteArrayView(reinterpret_cast<const char*>(bits), bytes)));
}

}  // namespace

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

void DmCachePainter::recacheEntities(const std::list<DmEntity*>& oldEnts, const std::list<DmEntity*>& newEnts)
{
    m_recacheTypes.clear();

    std::list<DmEntity*> allChangedEnts;

    // TODO: 实现部分更新逻辑
}

void DmCachePainter::rebuild()
{
    m_cachePainter->removeAllCache();
    cacheEntity(groupVisibleEntities(), opengl::CacheGroupType::Normal);
    cacheSelected();
    cacheHighlight();
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

void DmCachePainter::recache()
{
    if (m_recacheTypes.size() == 0)
    {
        return;
    }
    for (auto item : m_recacheTypes)
    {
        for (auto type : item.second)
        {
            m_cachePainter->removeCache(item.first, type);
        }
    }

    const PenGroups groups = groupVisibleEntities();

    for (auto item : m_recacheTypes)
    {
        auto pen = DMPENLIST->request(item.first);
        auto it = groups.find(*pen);
        if (it != groups.end())
        {
            for (auto e : it->second)
            {
                if (isEntityMatchTypes(e, item.second))
                {
                    cacheEntity(e, item.first, opengl::CacheGroupType::Normal);
                }
            }
        }
    }

    m_recacheTypes.clear();
}

DmCachePainter::PenGroups DmCachePainter::groupVisibleEntities() const
{
    PenGroups groups;
    for (auto en : m_containerList)
    {
        for (auto e : *en)
        {
            if (e->isVisible())
            {
                addToGroups(e, groups);
            }
        }
    }
    return groups;
}

void DmCachePainter::addToGroups(DmEntity* pEnt, PenGroups& groups)
{
    // 获取该实体的所有子实体
    auto subEntities = pEnt->getSubEntities();
    if (subEntities.size() == 0)
    {
        subEntities.emplace_back(std::move(pEnt));
    }

    // 将子实体集合添加到map分组
    for (auto& itemEnt : subEntities)
    {
        auto findEntitise = groups.find(itemEnt->getPen(true));
        // 分组不存在 则创建
        if (findEntitise == groups.end())
        {
            std::list<DmEntity*> listEnt = { itemEnt };
            groups[itemEnt->getPen(true)] = listEnt;
        }
        // 存在直接添加
        else
        {
            findEntitise->second.emplace_back(std::move(itemEnt));
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
    PenGroups groups;
    for (auto e : selected)
    {
        addToGroups(e, groups);
    }
    cacheEntity(groups, opengl::CacheGroupType::Selected);
    cacheSelectedPoints(selected);
}

void DmCachePainter::cacheHighlight()
{
    if (!m_highlightSource)
    {
        return;
    }
    // 来源给出的都是可见的顶层实体；选中优先：已选中的按选中色画，不进高亮组
    PenGroups groups;
    for (auto e : m_highlightSource->highlightedEntities())
    {
        if (!isSelected(e))
        {
            addToGroups(e, groups);
        }
    }
    cacheEntity(groups, opengl::CacheGroupType::Highlight);
}

bool DmCachePainter::isEntityMatchTypes(const DmEntity* e, const std::list<opengl::CacheType>& types)
{
    opengl::CacheType type = getCacheTypeOfEntity(e);
    bool find = std::find(types.begin(), types.end(), type) != types.end();
    if (std::find(types.begin(), types.end(), opengl::CacheType::ALL) != types.end())
    {
        return true;
    }
    return find;
}

opengl::CacheType DmCachePainter::getCacheTypeOfEntity(const DmEntity* e)
{
    switch (e->getEntityType())
    {
    case DM::EntityPoint:
        return opengl::CacheType::POINTS;
    case DM::EntityLine:
        return opengl::CacheType::LINES;
    case DM::EntityArc:
        return opengl::CacheType::ARCS;
    case DM::EntityCircle:
        return opengl::CacheType::CIRCLES;
    case DM::EntityEllipse:
    {
        if (((DmEllipse*)e)->isClosed())
        {
            return opengl::CacheType::ELLIPSE_CLOSEDS;
        }
        else
        {
            return opengl::CacheType::ELLIPSES;
        }
    }
    case DM::EntitySolid:
        return opengl::CacheType::SOLIDS;
    case DM::EntityImage:
        return opengl::CacheType::IMAGES;
    case DM::EntityRay:
        return opengl::CacheType::RAYS;
    case DM::EntityXline:
        return opengl::CacheType::XLINES;
    case DM::EntitySpline:
    {
        if (((DmSpline*)e)->isClosed())
        {
            return opengl::CacheType::SPLINE_CLOSED;
        }
        else
        {
            return opengl::CacheType::SPLINES;
        }
    }
    // TODO: EntitySplinePoint
    default:
        return opengl::CacheType::POINTS;
    }
}

void DmCachePainter::cacheEntity(const std::unordered_map<DmPen, std::list<DmEntity*>>& map, opengl::CacheGroupType group)
{
    for (auto item : map)
    {
        auto& pen = item.first;
        DmPen* thePen = DMPENLIST->request(pen.getColor(), pen.getWidth(), pen.getLineType());
        int penId = DMPENLIST->getPenId(*thePen);
        constexpr double kMinLineWidth = 1.0;
        m_cachePainter->lineWidth(penId, std::max(pen.getWidth() * 0.05, kMinLineWidth));
        DmLineType* lineType = pen.getLineType();
        if (pen.getLineType()->getLineTypeName() != "continuous" && pen.getLineType()->getLineTypeName() != "ByLayer" && pen.getLineType()->getLineTypeName() != "ByBlock")
        {
            m_cachePainter->setDash(penId, lineType->getLineTypeData().data(), lineType->getNum());
        }
        if (pen.getColor().red() + pen.getColor().green() + pen.getColor().blue() == 0)
        {
            m_cachePainter->setColor(penId, 255, 255, 255, 255);
        }
        else
        {
            m_cachePainter->setColor(penId, pen.getColor().red(), pen.getColor().green(), pen.getColor().blue(), pen.getColor().alpha());
        }
        for (auto e : item.second)
        {
            cacheEntity(e, penId, group);
        }
    }
}

void DmCachePainter::cacheEntity(const DmEntity* e, int penId, opengl::CacheGroupType group)
{
    switch (e->getEntityType())
    {
    case DM::EntityPoint:
    {
        DmPoint* ptEnt = (DmPoint*)e;
        DmVector pt = ptEnt->getPos();
        m_cachePainter->addPoint(penId, group, pt.x, pt.y);
    }
    break;
    case DM::EntityLine:
    {
        DmLine* line = (DmLine*)e;
        int float_count_per_vertex = 0;
        const std::vector<float>& vertices = line->getVerticesRef(float_count_per_vertex);
        m_cachePainter->addLine(penId, group, vertices, float_count_per_vertex);
    }
    break;
    case DM::EntityTriangle:
    {
        DmTriangle* triangle = (DmTriangle*)e;
        int float_count_per_vertex = 0;

        std::array<DmVector, 3> corners = triangle->getData().getPoints();
        std::vector<float> vertices;
        vertices.reserve(corners.size() * 3);
        for (auto v : corners)
        {
            vertices.emplace_back(v.x);
            vertices.emplace_back(v.y);
            vertices.emplace_back(0.0);
        }
        m_cachePainter->addTriangle(penId, group, vertices);
    }
    break;
    case DM::EntityArc:
    {
        DmArc* arc = (DmArc*)e;
        int float_count_per_vertex = 0;
        const std::vector<float>& vertices = arc->getVerticesRef(float_count_per_vertex);
        m_cachePainter->addArc(penId, group, vertices, float_count_per_vertex);
    }
    break;
    case DM::EntityCircle:
    {
        DmCircle* circle = (DmCircle*)e;
        int float_count_per_vertex = 0;
        const std::vector<float>& vertices = circle->getVerticesRef(float_count_per_vertex);
        m_cachePainter->addCircle(penId, group, vertices, float_count_per_vertex);
    }
    break;
    case DM::EntityEllipse:
    {
        DmEllipse* ellipse = (DmEllipse*)e;
        int float_count_per_vertex = 0;
        const std::vector<float>& vertices = ellipse->getVerticesRef(float_count_per_vertex);
        if (ellipse->isClosed())
        {
            m_cachePainter->addEllipseClosed(penId, group, vertices, float_count_per_vertex);
        }
        else
        {
            m_cachePainter->addEllipse(penId, group, vertices, float_count_per_vertex);
        }
    }
    break;
    case DM::EntitySolid:
    {
        DmSolid* solid = (DmSolid*)e;
        std::vector<DmVector> corners = solid->getData().getCorners();
        std::vector<double> xy;
        xy.reserve(corners.size() * 2);
        for (auto v : corners)
        {
            xy.emplace_back(v.x);
            xy.emplace_back(v.y);
        }
        m_cachePainter->addSolid(penId, group, corners.size() * 2, &xy[0]);
    }
    break;
    case DM::EntityImage:
    {
        DmImage* image = (DmImage*)e;
        DmVectorSolutions corners = image->getCorners();

        std::vector<float> vertices;
        vertices.reserve(5 * 4);
        // corner 0: bottom-left -> texcoord (0,0)
        vertices.insert(vertices.end(), { (float)corners.get(0).x, (float)corners.get(0).y, 0.0f, 0.0f, 0.0f });
        // corner 1: bottom-right -> texcoord (1,0)
        vertices.insert(vertices.end(), { (float)corners.get(1).x, (float)corners.get(1).y, 0.0f, 1.0f, 0.0f });
        // corner 2: top-right -> texcoord (1,1)
        vertices.insert(vertices.end(), { (float)corners.get(2).x, (float)corners.get(2).y, 0.0f, 1.0f, 1.0f });
        // corner 3: top-left -> texcoord (0,1)
        vertices.insert(vertices.end(), { (float)corners.get(3).x, (float)corners.get(3).y, 0.0f, 0.0f, 1.0f });

        // 纹理按图片来源缓存，缓存重建时复用，只有新的来源才解码、上传（RENDER_PLAN.md 1.2 步）
        m_cachePainter->addImage(penId, group, vertices, imageSource(image), [image]() {
            return image->getData().getPath() != ""
                ? QImage(QString::fromStdString(image->getData().getPath()))
                : QImage(image->getbits(), image->getWidth(), image->getHeight(),
                         image->getBytesPerLine(), QImage::Format_ARGB32_Premultiplied);
        });
    }
    break;
    case DM::EntityRay:
    {
        DmRay* ray = (DmRay*)e;
        m_cachePainter->addRay(penId, group, ray->getBasePoint().x, ray->getBasePoint().y, ray->getDirecion().x, ray->getDirecion().y);
    }
    break;
    case DM::EntityXline:
    {
        DmXline* xline = (DmXline*)e;
        m_cachePainter->addXLine(penId, group, xline->getBasePoint().x, xline->getBasePoint().y, xline->getDirecion().x, xline->getDirecion().y);
    }
    break;
    case DM::EntitySpline:
    {
        DmSpline* spline = (DmSpline*)e;
        cacheLineStrip(spline->getLineStrip(), penId, group);
    }
    break;
    default:
        break;
    }
}

void DmCachePainter::cacheLineStrip(DmLineStrip* lineStrip, int penId, opengl::CacheGroupType group)
{
    int float_count_per_vertex = 0;
    const std::vector<float>& vertices = lineStrip->getVerticesRef(float_count_per_vertex);
    if (lineStrip->isClosed())
    {
        m_cachePainter->addSplineClosed(penId, group, vertices, float_count_per_vertex);
    }
    else
    {
        m_cachePainter->addSpline(penId, group, vertices, float_count_per_vertex);
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
