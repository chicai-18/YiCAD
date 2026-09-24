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
#include <QImage>
#include <GL/glew.h>

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

void DmCachePainter::cacheAll()
{
    m_cachePainter->removeAllCache();
    regroup();

    cacheEntity(m_groupEntities, opengl::CacheGroupType::Normal);
    cacheEntity(m_selectedEntities, opengl::CacheGroupType::Selected);
    cacheEntity(m_highlightEntities, opengl::CacheGroupType::Highlight);

    cacheSelectedPoints();
}

void DmCachePainter::draw()
{
    //recache();
    if (m_bIsModefied)
    {
        cacheAll();
        m_cachePainter->generateGLData();
        m_bIsModefied = false;
    }
    m_cachePainter->stroke();
}

void DmCachePainter::specifyModified()
{
    m_bIsModefied = true;
}

//void DmCachePainter::specifySelectChanged()
//{
//    m_cachePainter->removeCacheByGroup(opengl::CacheGroupType::Selected);
//    m_cachePainter->removeSelectedPointsCache();
//    regroup();
//    cacheEntity(m_selectedEntities, opengl::CacheGroupType::Selected);
//    cacheSelectedPoints();
//    m_cachePainter->generateGLDataByType(opengl::CacheGroupType::Selected);
//}

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

    regroup();

    for (auto item : m_recacheTypes)
    {
        auto pen = DMPENLIST->request(item.first);
        auto it = m_groupEntities.find(*pen);
        if (it != m_groupEntities.end())
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

void DmCachePainter::regroup()
{
    m_groupEntities.clear();
    m_highlightEntities.clear();
    m_selectedEntities.clear();
    for (auto en : m_containerList)
    {
        for (auto e : *en)
        {
            addGroupEntity(e);
        }
    }
}

void DmCachePainter::addGroupEntity(DmEntity* pEnt)
{
    if (!pEnt->isVisible())
    {
        return;
    }
    addGroupEntity_subRoutine(pEnt, &m_groupEntities);
    if (pEnt->isSelected())
    {
        addGroupEntity_subRoutine(pEnt, &m_selectedEntities);
    }
    else if (pEnt->isHighlighted())
    {
        addGroupEntity_subRoutine(pEnt, &m_highlightEntities);
    }
}

void DmCachePainter::addGroupEntity_subRoutine(DmEntity* pEnt, std::unordered_map<DmPen, std::list<DmEntity*>>* theMap)
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
        auto findEntitise = theMap->find(itemEnt->getPen(true));
        // 分组不存在 则创建
        if (findEntitise == theMap->end())
        {
            std::list<DmEntity*> listEnt = { itemEnt };
            (*theMap)[itemEnt->getPen(true)] = listEnt;
        }
        // 存在直接添加
        else
        {
            findEntitise->second.emplace_back(std::move(itemEnt));
        }
    }
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

        // 创建 OpenGL 纹理
        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        QImage img = image->getData().getPath() != ""
            ? QImage(QString::fromStdString(image->getData().getPath()))
            : QImage(image->getbits(), image->getWidth(), image->getHeight(),
                     image->getBytesPerLine(), QImage::Format_ARGB32_Premultiplied);

        QImage glImg = img.convertToFormat(QImage::Format_RGBA8888).mirrored();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImg.width(), glImg.height(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, glImg.bits());
        glBindTexture(GL_TEXTURE_2D, 0);

        m_cachePainter->addImage(penId, group, vertices, textureId);
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

void DmCachePainter::cacheSelectedPoints()
{
    constexpr size_t kMaxSelectedPoints = 100;
    std::vector<DmVector> selectedPts;
    selectedPts.reserve(kMaxSelectedPoints);
    for (auto en : m_containerList)
    {
        if (selectedPts.size() > kMaxSelectedPoints)
        {
            return;
        }
        for (auto e : *en)
        {
            if (selectedPts.size() > kMaxSelectedPoints)
            {
                return;
            }
            if (e->isSelected())
            {
                for (auto pt : e->getRefPoints())
                {
                    selectedPts.emplace_back(pt);
                }
            }
        }
    }

    for (auto pt : selectedPts)
    {
        m_cachePainter->addSelectedPoints(pt.x, pt.y);
    }
}
