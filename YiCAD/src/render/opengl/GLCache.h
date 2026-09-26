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

/// @file GLCache.h
/// @brief OpenGL缓存数据结构定义，按实体类型和显示状态组织顶点缓存

#ifndef GLCACHE_H
#define GLCACHE_H

#include "GLCacheUnit.h"
#include "GLPenData.h"

namespace opengl
{

/// @brief 缓存类型（根据实体类型分类）
enum CacheType
{
    ALL = -1,
    POINTS,
    LINES,
    ARCS,
    CIRCLES,
    TRIANGLES,
    SOLIDS,
    ELLIPSES,           ///< 椭圆弧
    ELLIPSE_CLOSEDS,    ///< 椭圆
    RAYS,
    XLINES,
    SPLINES,            ///< spline, splinepoints（不闭合）
    SPLINE_CLOSED,      ///< spline, splinepoints（闭合）
    IMAGES,             ///< 光栅图像

    COUNT
};

/// @brief 缓存组类型（按显示状态分类）
enum class CacheGroupType
{
    Normal,     ///< 所有实体
    Highlight,  ///< 高亮实体
    Selected    ///< 选中实体
};

/// @brief 渲染缓存结构，包含三种显示状态下的缓存单元和画笔数据
struct GLCache
{
    GLCacheUnitMap m_cacheUnits[COUNT];             ///< 实体未选中状态未高亮的绘制
    GLCacheUnitMap m_cacheHighlightUnits[COUNT];    ///< 实体高亮部分
    GLCacheUnitMap m_cacheSelectedUnits[COUNT];     ///< 实体选中部分
    GLPenDataMap m_pens;                            ///< 所有实体的画笔
    GLCacheUnit    m_cacheSelectedPoints;           ///< 实体的控制点
    std::unordered_map<int, GLuint> m_imageTextures; ///< 图片纹理ID（key为penId）
};

}

#endif //GLCACHE_H
