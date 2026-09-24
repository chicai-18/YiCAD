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


/// @file DmCachePainter.h
/// @brief 对GLCachePainter及分组的封装，管理实体缓存和绘制

#ifndef DMCACHEPAINTER_H
#define DMCACHEPAINTER_H

#include <list>
#include <unordered_map>
#include "GLCachePainter.h"
#include "DmEntityContainer.h"

class DmLineStrip;

/// @brief 对GLCachePainter及分组的封装
class DmCachePainter
{
public:
    DmCachePainter();

    /// @brief 移动视图
    /// @param x X方向平移量
    /// @param y Y方向平移量
    void translateView(double x, double y);

    /// @brief 初始化glew及shader
    void create_resources();

    /// @brief 指定视图尺寸
    /// @param width 视图宽度
    /// @param height 视图高度
    void new_device_size(unsigned int width, unsigned int height);

    /// @brief 在指定位置做视图缩放。s<1时，放大视图以查看更小的实体。
    /// @param s 缩放比例
    /// @param x_world 缩放中心世界坐标X
    /// @param y_world 缩放中心世界坐标Y
    void scale(double s, double x_world, double y_world);

    /// @brief 直接指定视图比例，即单位设备坐标对应的世界坐标
    /// @param s 缩放比例
    void setScale(double s);

    /// @brief 直接指定视图的原点，即屏幕中心对应的世界坐标
    /// @param posx 原点X坐标
    /// @param posy 原点Y坐标
    void setViewPosition(double posx, double posy);

    /// @brief 添加绘制的实体集，一般为文档或预览的实体集
    /// @param container 实体容器指针
    void addContainer(DmEntityContainer* container);

    /// @brief 清空所有实体集
    void clearContainers();

    // TODO : 暂时无法获得实体以前的子实体，因此无法部分更新
    void recacheEntities(const std::list<DmEntity*>& oldEnts, const std::list<DmEntity*>& newEnts);

    /// @brief 缓存所有，包括：删除原来的vao，重新分组，缓存所有实体，缓存拖拽点
    void cacheAll();

    /// @brief 绘制。如果已修改，重新缓存
    void draw();

    /// @brief 指示实体集已修改，需要重新缓存
    void specifyModified();

    //void specifySelectChanged();

    /// @brief 指定模型矩阵的偏移量
    /// @param offset 偏移量
    void setModelOffset(const DmVector& offset);

    /// @brief 是否显示线宽
    /// @return 如果显示线宽则返回true
    bool isDisplayLineWidth() const;

    /// @brief 设置是否显示线宽
    /// @param display 是否显示线宽
    void setIsDisplayLineWidth(bool display);

    /// @brief 设置选中实体颜色
    void setSelectedColor(const QColor& c);

    /// @brief 设置高亮实体颜色
    void setHighlightColor(const QColor& c);

private:
    void recache();

    /// @brief 重新分组
    void regroup();

    /// @brief 将实体添加到分组集合中
    /// @param pEnt 实体指针
    void addGroupEntity(DmEntity* pEnt);

    /// @brief addGroupEntity的子程序
    /// @param pEnt 实体指针
    /// @param theMap 分组映射表
    void addGroupEntity_subRoutine(DmEntity* pEnt, std::unordered_map<DmPen, std::list<DmEntity*>>* theMap);

    bool isEntityMatchTypes(const DmEntity* e, const std::list<opengl::CacheType>& types);
    opengl::CacheType getCacheTypeOfEntity(const DmEntity* e);
    void cacheEntity(const std::unordered_map<DmPen, std::list<DmEntity*>>& map, opengl::CacheGroupType group);
    void cacheEntity(const DmEntity* e, int penId, opengl::CacheGroupType group);

    /// @brief 缓存linestrip
    /// @param lineStrip LineStrip指针
    /// @param penId 画笔ID
    /// @param group 缓存分组类型
    void cacheLineStrip(DmLineStrip* lineStrip, int penId, opengl::CacheGroupType group);

    /// @brief 缓存拖拽点
    void cacheSelectedPoints();

private:
    opengl::GLCachePainter* m_cachePainter = nullptr; ///< 画笔

    std::unordered_map<int, std::list<opengl::CacheType>> m_recacheTypes;

    std::list<DmEntityContainer*> m_containerList; ///< 绘制的实体集
    std::unordered_map<DmPen, std::list<DmEntity*>> m_groupEntities; ///< 根据画笔分组的子实体集合
    std::unordered_map<DmPen, std::list<DmEntity*>> m_highlightEntities; ///< 根据画笔分组的高亮实体的子实体集合
    std::unordered_map<DmPen, std::list<DmEntity*>> m_selectedEntities; ///< 根据画笔分组的选中实体的子实体集合
    bool m_bIsModefied = true; ///< 实体集是否已修改
};

#endif //DMCACHEPAINTER_H
