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

/// @file GLCachePainter.h
/// @brief 带顶点缓存的OpenGL画笔，按缓存单元批量绘制以提高性能

#ifndef GLCACHEPAINTER_H
#define GLCACHEPAINTER_H

#include <QColor>
#include "Painter.h"
#include "GLCache.h"
#include "GLPainterCommon.h"

namespace opengl
{

/// @brief 缓存vao的画笔 [为解决编译错误，添加此段]
class GLCachePainter : public Painter
{
    GL_PAINTER_COMMON()
public:
    GLCachePainter();

    /// @brief 移除所有缓存
    void removeAllCache();

    /// @brief 按缓存组类型移除缓存
    /// @param [in] groupType 缓存组类型
    void removeCacheByGroup(CacheGroupType groupType);

    /// @brief 移除指定画笔和类型的缓存
    /// @param [in] penId 画笔ID
    /// @param [in] type 缓存类型
    void removeCache(int penId, CacheType type);

    /// @brief 移除选中点缓存
    void removeSelectedPointsCache();

    /// @brief 设置画笔颜色
    /// @param [in] penId 画笔ID
    /// @param [in] r 红色分量(0-255)
    /// @param [in] g 绿色分量(0-255)
    /// @param [in] b 蓝色分量(0-255)
    /// @param [in] a 透明度分量(0-255)
    void setColor(int penId, int r, int g, int b, int a);

    /// @brief 设置线宽
    /// @param [in] penId 画笔ID
    /// @param [in] lineWidth 线宽
    void lineWidth(int penId, double lineWidth);

    /// @brief 设置线型
    /// @param [in] penId 画笔ID
    /// @param [in] dashes 线型数据数组
    /// @param [in] num_dashes 线型数据元素个数
    void setDash(int penId, const double* dashes, const int num_dashes);

    /// @brief 添加直线顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    /// @param [in] float_count_per_vertex 每顶点的浮点数
    void addLine(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex);

    /// @brief 添加三角形顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    void addTriangle(int penId, CacheGroupType groupType, const std::vector<float>& vertices);

    /// @brief 添加点
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] x0 点X坐标
    /// @param [in] y0 点Y坐标
    void addPoint(int penId, CacheGroupType groupType, double x0, double y0);

    /// @brief 添加圆顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    /// @param [in] float_count_per_vertex 每顶点的浮点数
    void addCircle(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex);

    /// @brief 添加椭圆弧顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    /// @param [in] float_count_per_vertex 每顶点的浮点数
    void addEllipse(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex);

    /// @brief 添加椭圆顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    /// @param [in] float_count_per_vertex 每顶点的浮点数
    void addEllipseClosed(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex);

    /// @brief 添加样条曲线顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    /// @param [in] float_count_per_vertex 每顶点的浮点数
    void addSpline(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex);

    /// @brief 添加闭合样条曲线顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    /// @param [in] float_count_per_vertex 每顶点的浮点数
    void addSplineClosed(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex);

    /// @brief 添加圆弧顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据
    /// @param [in] float_count_per_vertex 每顶点的浮点数
    void addArc(int penId, CacheGroupType groupType, const std::vector<float>& vertices, int float_count_per_vertex);

    /// @brief 添加填充区域顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] count 坐标数组长度
    /// @param [in] xy 坐标数组(x,y交替)
    void addSolid(int penId, CacheGroupType groupType, int count, double* xy);

    /// @brief 添加射线
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] x0 原点X坐标
    /// @param [in] y0 原点Y坐标
    /// @param [in] dirx 方向X分量
    /// @param [in] diry 方向Y分量
    void addRay(int penId, CacheGroupType groupType, double x0, double y0, double dirx, double diry);

    /// @brief 添加构造线
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] x0 原点X坐标
    /// @param [in] y0 原点Y坐标
    /// @param [in] dirx 方向X分量
    /// @param [in] diry 方向Y分量
    void addXLine(int penId, CacheGroupType groupType, double x0, double y0, double dirx, double diry);

    /// @brief 添加图片顶点数据
    /// @param [in] penId 画笔ID
    /// @param [in] groupType 缓存组类型
    /// @param [in] vertices 顶点数据（每顶点5float: x,y,z + u,v）
    /// @param [in] textureId OpenGL纹理ID
    void addImage(int penId, CacheGroupType groupType, const std::vector<float>& vertices, GLuint textureId);

    /// @brief 添加选中实体控制点
    /// @param [in] x 控制点X坐标
    /// @param [in] y 控制点Y坐标
    void addSelectedPoints(double x, double y);

    /// @brief 执行绘制提交
    void stroke() override;

    /// @brief 生成OpenGL顶点数据
    void generateGLData();

    /// @brief 按缓存组类型生成OpenGL顶点数据
    /// @param [in] group 缓存组类型
    void generateGLDataByType(CacheGroupType group);

    /// @brief 返回是否显示线宽
    /// @return 当前线宽显示状态
    bool isDisplayLineWidth() const;

    /// @brief 设置是否显示线宽
    /// @param [in] display 是否显示线宽
    void setIsDisplayLineWidth(bool display);

    /// @brief 设置选中实体颜色
    /// @param [in] c 选中颜色
    void setSelectedColor(const QColor& c);

    /// @brief 设置高亮实体颜色
    /// @param [in] c 高亮颜色
    void setHighlightColor(const QColor& c);

private:
    /// @brief 按组类型和缓存类型获取对应的缓存单元映射
    /// @param [in] groupType 缓存组类型
    /// @param [in] cacheType 缓存类型
    /// @return 缓存单元映射指针
    GLCacheUnitMap* getUnitMap(CacheGroupType groupType, CacheType cacheType);

    /// @brief 为指定缓存单元映射生成OpenGL数据
    /// @param [in,out] map 缓存单元映射
    /// @param [in] cacheType 缓存类型
    void generateGLDataOfMap(GLCacheUnitMap& map, CacheType cacheType);

    /// @brief 为选中点生成OpenGL数据
    void generateGLDataOfSelectedPoints();

    /// @brief 发送uniform变量到着色器
    /// @param [in] penData 画笔数据
    /// @param [in] group 缓存组类型
    void sendUniform(const opengl::GLPenData& penData, CacheGroupType group);

    /// @brief 按缓存单元映射和类型进行绘制
    /// @param [in,out] map 缓存单元映射
    /// @param [in] type 缓存类型
    /// @param [in] group 缓存组类型
    void drawByMapAndType(GLCacheUnitMap& map, opengl::CacheType type, CacheGroupType group);

    /// @brief 绘制选中实体控制点
    void drawSelectedPoints();

    /// @brief 根据画笔数据和实体类型选择合适的着色器
    /// @param [in] penData 画笔数据
    /// @param [in] type 缓存类型
    /// @param [in] group 缓存组类型
    void useShader(const opengl::GLPenData& penData, opengl::CacheType type, CacheGroupType group);

private:
    GLCache m_cache;
    bool    m_bDisplayLineWidth = false; ///< 是否显示线宽
    QColor  m_selectedColor;            ///< 选中实体颜色
    QColor  m_highlightColor;           ///< 高亮实体颜色
};

}

#endif //GLCACHEPAINTER_H
