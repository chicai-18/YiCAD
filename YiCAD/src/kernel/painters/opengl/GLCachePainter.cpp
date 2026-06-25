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

/// @file GLCachePainter.cpp
/// @brief 带顶点缓存的OpenGL画笔实现，提供批量绘制和高性能渲染

#include "GLCachePainter.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <QColor>
#include "Debug.h"
#include "DmSettings.h"

namespace
{
    constexpr float COLOR_SCALE_DENOM = 255.0f;     ///< 颜色分量归一化除数
    constexpr float DEFAULT_POINT_SIZE = 2.0f;       ///< 默认点大小
    constexpr float SELECTED_POINT_SIZE = 15.0f;     ///< 选中控制点大小
    constexpr float RESET_POINT_SIZE = 1.0f;         ///< 重置点大小
    constexpr float BLUE_R = 0.0f;                   ///< 蓝色分量R
    constexpr float BLUE_G = 0.0f;                   ///< 蓝色分量G
    constexpr float BLUE_B = 1.0f;                   ///< 蓝色分量B
    constexpr float FULL_ALPHA = 1.0f;               ///< 完全不透明分量
    constexpr float ZERO_COORD = 0.0f;               ///< Z坐标零值
    constexpr int POINT_FLOAT_COUNT = 3;             ///< 点数据浮点数(x,y,z)
    constexpr int RAY_FLOAT_COUNT = 6;               ///< 射线数据浮点数(x,y,z,dirx,diry,dirz)
    constexpr int SOLID_FLOAT_COUNT = 3;             ///< 填充区域每顶点浮点数
}

opengl::GLCachePainter::GLCachePainter()
    : m_bDisplayLineWidth(false)
    , m_selectedColor(Colors::SELECT)
    , m_highlightColor(Colors::HIGHLIGHT)
{
}

static void removeUnit(opengl::GLCacheUnitMap& map)
{
    auto it = map.begin();
    while (it != map.end())
    {
        it->second.free();
        it = map.erase(it);
    }
}

void opengl::GLCachePainter::removeAllCache()
{
    m_cache.m_pens.clear();

    for (int i = 0; i < CacheType::COUNT; i++)
    {
        removeUnit(m_cache.m_cacheUnits[i]);
        removeUnit(m_cache.m_cacheHighlightUnits[i]);
        removeUnit(m_cache.m_cacheSelectedUnits[i]);
    }

    for (auto& it : m_cache.m_imageTextures)
    {
        glDeleteTextures(1, &it.second);
    }
    m_cache.m_imageTextures.clear();

    removeSelectedPointsCache();
}

void opengl::GLCachePainter::removeCacheByGroup(CacheGroupType groupType)
{
    opengl::GLCacheUnitMap* map = nullptr;
    switch (groupType)
    {
    default:
    case opengl::CacheGroupType::Normal:
        map = m_cache.m_cacheUnits;
        break;
    case opengl::CacheGroupType::Highlight:
        map = m_cache.m_cacheHighlightUnits;
        break;
    case opengl::CacheGroupType::Selected:
        map = m_cache.m_cacheSelectedUnits;
        break;
    }
    for (int i = 0; i < CacheType::COUNT; i++)
    {
        removeUnit(map[i]);
    }
    for (auto& it : m_cache.m_imageTextures)
    {
        glDeleteTextures(1, &it.second);
    }
    m_cache.m_imageTextures.clear();
}

void opengl::GLCachePainter::removeCache(int penId, CacheType type)
{
    if (type == CacheType::COUNT)
    {
        return;
    }

    auto func = [=](CacheType t) {
        auto& map = m_cache.m_cacheUnits[t];
        auto it = map.find(penId);
        if (it != map.end())
        {
            it->second.free();
        }
        map.erase(it);
    };

    if (type == CacheType::ALL)
    {
        for (int i = 0; i < CacheType::COUNT; i++)
        {
            func((CacheType)i);
        }
        auto texIt = m_cache.m_imageTextures.find(penId);
        if (texIt != m_cache.m_imageTextures.end())
        {
            glDeleteTextures(1, &texIt->second);
            m_cache.m_imageTextures.erase(texIt);
        }
    }
    else
    {
        func(type);
        if (type == CacheType::IMAGES)
        {
            auto texIt = m_cache.m_imageTextures.find(penId);
            if (texIt != m_cache.m_imageTextures.end())
            {
                glDeleteTextures(1, &texIt->second);
                m_cache.m_imageTextures.erase(texIt);
            }
        }
    }
}

void opengl::GLCachePainter::removeSelectedPointsCache()
{
    m_cache.m_cacheSelectedPoints.free();
}

void opengl::GLCachePainter::setColor(int penId, int r, int g, int b, int a)
{
    auto& pens = m_cache.m_pens;
    auto& pen = pens[penId];
    pen.red = r / COLOR_SCALE_DENOM;
    pen.green = g / COLOR_SCALE_DENOM;
    pen.blue = b / COLOR_SCALE_DENOM;
    pen.alpha = a / COLOR_SCALE_DENOM;
}

void opengl::GLCachePainter::lineWidth(int penId, double lineWidth)
{
    auto& pens = m_cache.m_pens;
    auto& pen = pens[penId];
    pen.lineWidth = lineWidth;
}

void opengl::GLCachePainter::setDash(int penId, const double* dashes, const int num_dashes)
{
    auto& pens = m_cache.m_pens;
    auto& it = pens[penId];
    it.setDash(dashes, num_dashes);
}

void opengl::GLCachePainter::addLine(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex)
{
    GLCacheUnitMap* map = getUnitMap(groupType, LINES);
    auto& vertexRef = (*map)[penId].vertexes;
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());

    // 此处不像addCircle()等，没有jumps
}

void opengl::GLCachePainter::addTriangle(int penId, CacheGroupType groupType, const std::vector<float>& vertices)
{
    GLCacheUnitMap* map = getUnitMap(groupType, TRIANGLES);
    auto& vertexRef = (*map)[penId].vertexes;
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());

    // 此处不像addCircle()等，没有jumps
}

void opengl::GLCachePainter::addPoint(int penId, CacheGroupType groupType, double x0, double y0)
{
    GLCacheUnitMap* map = getUnitMap(groupType, POINTS);
    auto& vertexs = (*map)[penId].vertexes;
    auto& unit = (*map)[penId];
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexs.size()) / POINT_FLOAT_COUNT;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexs.emplace_back(x0);
    vertexs.emplace_back(y0);
    vertexs.emplace_back(ZERO_COORD);
    unit.jumps.emplace_back(1);
}

void opengl::GLCachePainter::addCircle(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex)
{
    GLCacheUnitMap* map = getUnitMap(groupType, CIRCLES);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / float_count_per_vertex;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());
    unit.jumps.emplace_back(static_cast<int>(vertices.size()) / float_count_per_vertex);
}

void opengl::GLCachePainter::addEllipse(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex)
{
    GLCacheUnitMap* map = getUnitMap(groupType, ELLIPSES);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / float_count_per_vertex;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());
    unit.jumps.emplace_back(static_cast<int>(vertices.size()) / float_count_per_vertex);
}

void opengl::GLCachePainter::addEllipseClosed(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex)
{
    GLCacheUnitMap* map = getUnitMap(groupType, ELLIPSE_CLOSEDS);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / float_count_per_vertex;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());
    unit.jumps.emplace_back(static_cast<int>(vertices.size()) / float_count_per_vertex);
}

void opengl::GLCachePainter::addSpline(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex)
{
    GLCacheUnitMap* map = getUnitMap(groupType, SPLINES);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / float_count_per_vertex;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());
    unit.jumps.emplace_back(static_cast<int>(vertices.size()) / float_count_per_vertex);
}

void opengl::GLCachePainter::addSplineClosed(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex)
{
    GLCacheUnitMap* map = getUnitMap(groupType, SPLINE_CLOSED);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / float_count_per_vertex;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());
    unit.jumps.emplace_back(static_cast<int>(vertices.size()) / float_count_per_vertex);
}

void opengl::GLCachePainter::addArc(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex)
{
    GLCacheUnitMap* map = getUnitMap(groupType, ARCS);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / float_count_per_vertex;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }

    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());
    unit.jumps.emplace_back(static_cast<int>(vertices.size()) / float_count_per_vertex);
}

void opengl::GLCachePainter::addSolid(int penId, CacheGroupType groupType, int count, double* xy)
{
    std::vector<float> vertexs;
    int pointCount = count / 2;
    vertexs.reserve(pointCount * SOLID_FLOAT_COUNT);
    for (int i = 0; i < pointCount; i++)
    {
        vertexs.emplace_back(xy[i * 2]);
        vertexs.emplace_back(xy[i * 2 + 1]);
        vertexs.emplace_back(ZERO_COORD);
    }

    GLCacheUnitMap* map = getUnitMap(groupType, SOLIDS);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / SOLID_FLOAT_COUNT;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexRef.insert(vertexRef.end(), vertexs.begin(), vertexs.end());
    unit.jumps.emplace_back(static_cast<int>(vertexs.size()) / SOLID_FLOAT_COUNT);
}

void opengl::GLCachePainter::addRay(int penId, CacheGroupType groupType, double x0, double y0, double dirx, double diry)
{
    GLCacheUnitMap* map = getUnitMap(groupType, RAYS);
    auto& vertexs = (*map)[penId].vertexes;
    auto& unit = (*map)[penId];
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexs.size()) / RAY_FLOAT_COUNT;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexs.emplace_back(x0);
    vertexs.emplace_back(y0);
    vertexs.emplace_back(ZERO_COORD);
    vertexs.emplace_back(dirx);
    vertexs.emplace_back(diry);
    vertexs.emplace_back(ZERO_COORD);
    unit.jumps.emplace_back(1);
}

void opengl::GLCachePainter::addXLine(int penId, CacheGroupType groupType, double x0, double y0, double dirx, double diry)
{
    GLCacheUnitMap* map = getUnitMap(groupType, XLINES);
    auto& vertexs = (*map)[penId].vertexes;
    auto& unit = (*map)[penId];
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexs.size()) / RAY_FLOAT_COUNT;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexs.emplace_back(x0);
    vertexs.emplace_back(y0);
    vertexs.emplace_back(ZERO_COORD);
    vertexs.emplace_back(dirx);
    vertexs.emplace_back(diry);
    vertexs.emplace_back(ZERO_COORD);
    unit.jumps.emplace_back(1);
}

void opengl::GLCachePainter::addImage(int penId, CacheGroupType groupType, const std::vector<float>& vertices, GLuint textureId)
{
    GLCacheUnitMap* map = getUnitMap(groupType, IMAGES);
    auto& unit = (*map)[penId];
    auto& vertexRef = unit.vertexes;
    if (unit.jumps.size() > 0)
    {
        int thisIdx = static_cast<int>(vertexRef.size()) / 5;
        unit.startIndices.emplace_back(thisIdx);
    }
    else
    {
        unit.startIndices.emplace_back(0);
    }
    vertexRef.insert(vertexRef.end(), vertices.begin(), vertices.end());
    unit.jumps.emplace_back(static_cast<int>(vertices.size()) / 5);

    m_cache.m_imageTextures[penId] = textureId;
}

void opengl::GLCachePainter::addSelectedPoints(double x, double y)
{
    auto& vertexes = m_cache.m_cacheSelectedPoints.vertexes;
    vertexes.emplace_back(x);
    vertexes.emplace_back(y);
    vertexes.emplace_back(ZERO_COORD);
}

void opengl::GLCachePainter::stroke()
{
    // 实体
    //glDisable(GL_BLEND);	//如果屏幕渲染，禁用BLEND导致虚线有黑色背景
    glEnable(GL_BLEND);	//如果离屏渲染，开启BLEND反而导致黑屏
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    for (int i = 0; i < CacheType::COUNT; i++)
    {
        opengl::CacheType type = opengl::CacheType(i);
        auto& map = m_cache.m_cacheUnits[i];
        drawByMapAndType(map, type, opengl::CacheGroupType::Normal);
    }
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); //这样只让最终的alpha分量被源颜色向量的alpha值所影响到
    m_painterCommon.selectSelectedColor(m_selectedColor.redF(), m_selectedColor.greenF(), m_selectedColor.blueF(), 0.5f);
    for (int i = 0; i < CacheType::COUNT; i++)
    {
        opengl::CacheType type = opengl::CacheType(i);
        auto& map = m_cache.m_cacheSelectedUnits[i];
        drawByMapAndType(map, type, opengl::CacheGroupType::Selected);
    }
    m_painterCommon.selectHighlightColor(m_highlightColor.redF(), m_highlightColor.greenF(), m_highlightColor.blueF(), 0.5f);
    for (int i = 0; i < CacheType::COUNT; i++)
    {
        opengl::CacheType type = opengl::CacheType(i);
        auto& map = m_cache.m_cacheHighlightUnits[i];
        drawByMapAndType(map, type, opengl::CacheGroupType::Highlight);
    }

    // 拖拽点
    glDisable(GL_MULTISAMPLE);
    drawSelectedPoints();
    glEnable(GL_MULTISAMPLE);
}

opengl::GLCacheUnitMap* opengl::GLCachePainter::getUnitMap(CacheGroupType groupType, CacheType cacheType)
{
    GLCacheUnitMap* map = nullptr;
    switch (groupType)
    {
    default:
    case opengl::CacheGroupType::Normal:
        map = &m_cache.m_cacheUnits[cacheType];
        break;
    case opengl::CacheGroupType::Highlight:
        map = &m_cache.m_cacheHighlightUnits[cacheType];
        break;
    case opengl::CacheGroupType::Selected:
        map = &m_cache.m_cacheSelectedUnits[cacheType];
        break;
    }
    return map;
}

void opengl::GLCachePainter::generateGLData()
{
    for (int i = 0; i < CacheType::COUNT; i++)
    {
        CacheType cacheType = (CacheType)i;
        generateGLDataOfMap(m_cache.m_cacheUnits[i], cacheType);
        generateGLDataOfMap(m_cache.m_cacheHighlightUnits[i], cacheType);
        generateGLDataOfMap(m_cache.m_cacheSelectedUnits[i], cacheType);
    }

    // 选择实体的控制点
    generateGLDataOfSelectedPoints();
}

void opengl::GLCachePainter::generateGLDataByType(CacheGroupType group)
{
    switch (group)
    {
    case opengl::CacheGroupType::Normal:
    {
        for (int i = 0; i < CacheType::COUNT; i++)
        {
            CacheType cacheType = (CacheType)i;
            generateGLDataOfMap(m_cache.m_cacheUnits[i], cacheType);
        }
    }
    break;
    case opengl::CacheGroupType::Highlight:
    {
        for (int i = 0; i < CacheType::COUNT; i++)
        {
            CacheType cacheType = (CacheType)i;
            generateGLDataOfMap(m_cache.m_cacheHighlightUnits[i], cacheType);
        }
    }
    break;
    case opengl::CacheGroupType::Selected:
    {
        for (int i = 0; i < CacheType::COUNT; i++)
        {
            CacheType cacheType = (CacheType)i;
            generateGLDataOfMap(m_cache.m_cacheSelectedUnits[i], cacheType);
        }
        // 选择实体的控制点
        generateGLDataOfSelectedPoints();
    }
    break;
    default:
        break;
    }
}

bool opengl::GLCachePainter::isDisplayLineWidth() const
{
    return m_bDisplayLineWidth;
}

void opengl::GLCachePainter::setIsDisplayLineWidth(bool display)
{
    m_bDisplayLineWidth = display;
}

void opengl::GLCachePainter::setSelectedColor(const QColor& c)
{
    m_selectedColor = c;
}

void opengl::GLCachePainter::setHighlightColor(const QColor& c)
{
    m_highlightColor = c;
}

void opengl::GLCachePainter::generateGLDataOfMap(GLCacheUnitMap& map, CacheType cacheType)
{
    for (auto it = map.begin(); it != map.end(); ++it)
    {
        int penId = it->first;
        auto& data = it->second;
        if ((!data.vao.isValid() || !data.vbo.isValid()) && data.vertexes.size() > 0)
        {
            data.vao.gen();
            data.vbo.gen(&data.vertexes[0], data.vertexes.size() * sizeof(float));
            GLVertexBufferLayout layout;
            switch (cacheType)
            {
            case POINTS:
                data.drawType = GL_POINTS;
                layout.push<float>(3);      // (x,y,z)
                break;
            case LINES:
                data.drawType = GL_LINES;
                data.jumps.emplace_back(static_cast<int>(data.vertexes.size()) / 5);	//直线为固定2个顶点，drawcall可以压缩
                layout.push<float>(3);      // (x,y,z)
                layout.push<float>(1);      // parameter
                layout.push<float>(1);		// total length
                break;
            case ARCS:
            case ELLIPSES:
            case SPLINES:
                data.drawType = GL_LINE_STRIP_ADJACENCY;
                layout.push<float>(3);      // (x,y,z)
                layout.push<float>(1);      // parameter
                layout.push<float>(1);		// total length
                break;
            case CIRCLES:
            case ELLIPSE_CLOSEDS:
            case SPLINE_CLOSED:
                data.drawType = GL_LINE_STRIP_ADJACENCY;	//如果用GL_LINE_STRIP，绘不出来
                //data.jumps.emplace_back(data.vertexes.size() / 5);
                layout.push<float>(3);      // (x,y,z)
                layout.push<float>(1);      // parameter
                layout.push<float>(1);		// total length
                break;
            case TRIANGLES:
                data.drawType = GL_TRIANGLES;
                data.jumps.emplace_back(static_cast<int>(data.vertexes.size()) / 3);	// 三角形为固定3个顶点，drawcall可以压缩
                layout.push<float>(3);      // (x,y,z)
                break;
            case SOLIDS:
                data.drawType = GL_TRIANGLE_FAN;
                layout.push<float>(3);      // (x,y,z)
                break;
            case RAYS:
                data.drawType = GL_POINTS;
                layout.push<float>(3);      // (x,y,z)
                layout.push<float>(3);      // (dirx,diry,dirz)
                break;
            case XLINES:
                data.drawType = GL_POINTS;
                layout.push<float>(3);      // (x,y,z)
                layout.push<float>(3);      // (dirx,diry,dirz)
                break;
            case IMAGES:
                data.drawType = GL_TRIANGLE_FAN;
                layout.push<float>(3);      // (x,y,z)
                layout.push<float>(2);      // (u,v)
                break;
            default:
                break;
            }
            data.vao.addBuffer(data.vbo, layout);
        }
    }
}

void opengl::GLCachePainter::generateGLDataOfSelectedPoints()
{
    auto& data = m_cache.m_cacheSelectedPoints;
    if ((!data.vao.isValid() || !data.vbo.isValid()) && data.vertexes.size() > 0)
    {
        data.vao.gen();
        data.vbo.gen(&data.vertexes[0], data.vertexes.size() * sizeof(float));
        GLVertexBufferLayout layout;
        data.drawType = GL_POINTS;
        layout.push<float>(3);      // (x,y,z)
        data.vao.addBuffer(data.vbo, layout);
    }
}

void opengl::GLCachePainter::sendUniform(const opengl::GLPenData& penData, CacheGroupType group)
{
    bool isSelected = false;
    bool isHighlight = false;
    if (group == opengl::CacheGroupType::Selected)
    {
        isSelected = true;
    }
    else if (group == opengl::CacheGroupType::Highlight)
    {
        isHighlight = true;
    }
    else
    {
        // Normal组，不高亮也不选中
    }
    m_painterCommon.sendUniform(isSelected, isHighlight, m_bDisplayLineWidth, penData);
}

void opengl::GLCachePainter::drawByMapAndType(GLCacheUnitMap& map, opengl::CacheType type, CacheGroupType group)
{
    for (auto it = map.begin(); it != map.end(); ++it)
    {
        int penId = it->first;
        auto& data = it->second;

        auto& pen = m_cache.m_pens[penId];
        m_painterCommon.selectColor(pen.red, pen.green, pen.blue, pen.alpha);

        //bool isFill = type == SOLIDS;
        bool isFill = true;
        GLenum fillMode = isFill ? GL_FILL : GL_LINE;
        glPolygonMode(GL_FRONT_AND_BACK, fillMode);
        if (type == POINTS)
        {
            glPointSize(DEFAULT_POINT_SIZE);
        }
        useShader(pen, type, group);
        sendUniform(pen, group);

        if (type == IMAGES)
        {
            auto texIt = m_cache.m_imageTextures.find(penId);
            if (texIt != m_cache.m_imageTextures.end())
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texIt->second);
            }
        }

        data.vao.bind();
        data.vbo.bind();
        if (data.drawType == GL_LINES || data.drawType == GL_TRIANGLES)	//同类型直线/三角形共用一个drawcall，不使用data.startIndices
        {
            auto& jumps = data.jumps;
            std::vector<int>::const_iterator jumpIt;
            int l = 0;
            for (jumpIt = jumps.begin(); jumpIt != jumps.end(); ++jumpIt)
            {
                glDrawArrays(data.drawType, l, *(jumpIt));
                l += *(jumpIt);
            }
        }
        else
        {
            glMultiDrawArrays(data.drawType, data.startIndices.data(), data.jumps.data(), static_cast<GLsizei>(data.jumps.size()));	//比循环调用glDrawArrays()节省cpu时间
        }
    }
}

void opengl::GLCachePainter::drawSelectedPoints()
{
    m_painterCommon.selectColor(BLUE_R, BLUE_G, BLUE_B, FULL_ALPHA);
    //glDisable(GL_POINT_SMOOTH);
    glPointSize(SELECTED_POINT_SIZE);
    m_painterCommon.useShader(opengl::ShaderType::BASIC);
    m_painterCommon.sendMVP();
    auto& data = m_cache.m_cacheSelectedPoints;
    data.vao.bind();
    data.vbo.bind();
    glDrawArrays(data.drawType, 0, static_cast<GLsizei>(data.vertexes.size()) / POINT_FLOAT_COUNT);	//GL_POINTS
    glPointSize(RESET_POINT_SIZE);
}

void opengl::GLCachePainter::useShader(const opengl::GLPenData& penData, opengl::CacheType type, CacheGroupType group)
{
    opengl::ShaderType shaderType = ShaderType::LINE;
    //仅文档实体采用no-width着色器（没有几何着色器），选中及高亮按（有几何着色器）处理
    if (group == CacheGroupType::Normal && (penData.lineWidth <= 1.0f || !m_bDisplayLineWidth))
    {
        switch (type)
        {
        default:
        case opengl::ALL:
        case opengl::COUNT:
            break;
        case opengl::POINTS:
            shaderType = ShaderType::BASIC;
            break;
        case opengl::LINES:
            shaderType = ShaderType::LINE_NO_WIDTH;
            break;
        case opengl::ARCS:
        case opengl::ELLIPSES:
        case opengl::SPLINES:
            shaderType = ShaderType::LINESTRIP_NO_WIDTH;
            break;
        case opengl::CIRCLES:
        case opengl::ELLIPSE_CLOSEDS:
        case opengl::SPLINE_CLOSED:
            shaderType = ShaderType::LINETRIPCLOSED_NO_WIDTH;
            break;
        case opengl::TRIANGLES:
            shaderType = ShaderType::TRIANGLE;
            break;
        case opengl::SOLIDS:
            shaderType = ShaderType::SOLID;
            break;
        case opengl::RAYS:
            shaderType = ShaderType::RAY;
            break;
        case opengl::XLINES:
            shaderType = ShaderType::XLINE;
            break;
        case opengl::IMAGES:
            shaderType = ShaderType::IMAGE;
            break;
        }
    }
    else
    {
        switch (type)
        {
        default:
        case opengl::ALL:
        case opengl::COUNT:
            break;
        case opengl::POINTS:
            shaderType = ShaderType::BASIC;
            break;
        case opengl::LINES:
            shaderType = ShaderType::LINE;
            break;
        case opengl::ARCS:
        case opengl::ELLIPSES:
        case opengl::SPLINES:
            shaderType = ShaderType::LINESTRIP;
            break;
        case opengl::CIRCLES:
        case opengl::ELLIPSE_CLOSEDS:
        case opengl::SPLINE_CLOSED:
            shaderType = ShaderType::LINETRIPCLOSED;
            break;
        case opengl::TRIANGLES:
            shaderType = ShaderType::TRIANGLE;
            break;
        case opengl::SOLIDS:
            shaderType = ShaderType::SOLID;
            break;
        case opengl::RAYS:
            shaderType = ShaderType::RAY;
            break;
        case opengl::XLINES:
            shaderType = ShaderType::XLINE;
            break;
        case opengl::IMAGES:
            shaderType = ShaderType::IMAGE;
            break;
        }
    }

    m_painterCommon.useShader(shaderType);
}
