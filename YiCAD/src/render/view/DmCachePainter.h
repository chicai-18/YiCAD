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
#include <vector>
#include "GLCachePainter.h"
#include "DmEntityContainer.h"

class DmLineStrip;
class IHighlightSource;
class ISelectionSource;

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

    /// @brief 指示实体集已修改，下一次 update() 整图重建
    void specifyModified();

    /// @brief 指示选择集已修改，下一次 update() 只重建选中组、夹点与高亮组（高亮组不含选中的实体）
    void specifySelectChanged();

    /// @brief 指示高亮集已修改，下一次 update() 只重建高亮组
    void specifyHighlightChanged();

    /// @brief 下一次 update() 是否整图重建
    bool isModified() const;

    /// @brief 下一次 update() 是否重建选中组（不含整图重建）
    bool isSelectChanged() const;

    /// @brief 下一次 update() 是否重建高亮组（不含整图重建与选择集修改）
    bool isHighlightChanged() const;

    /// @brief 按修改标记更新缓存：实体集修改了就整图重建（删除原来的缓存，重新分组、缓存、上传），
    ///        否则只重建修改了的选中组、夹点与高亮组；没有修改时什么也不做
    void update();

    /// @brief 绘制普通组与选中组（场景底图的内容），先 update()
    void draw();

    /// @brief 绘制高亮组，先 update()
    void drawHighlight();

    /// @brief 绘制选中实体的夹点，先 update()
    void drawSelectedPoints();

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

    /// @brief 设置判断实体是否选中的来源，并标记选择集已修改
    /// @param source 非持有指针，可为空；为空时没有实体按选中绘制
    void setSelectionSource(const ISelectionSource* source);

    /// @brief 设置要高亮的实体的来源，并标记高亮集已修改
    /// @param source 非持有指针，可为空；为空时没有实体按高亮绘制
    void setHighlightSource(const IHighlightSource* source);

private:
    /// @brief 按画笔分组的子实体
    using PenGroups = std::unordered_map<DmPen, std::list<DmEntity*>>;

    void recache();

    /// @brief 整图重建：删除全部缓存，普通组、选中组、夹点、高亮组全部重新缓存并上传
    void rebuild();

    /// @brief 只重建选中组与夹点
    void rebuildSelected();

    /// @brief 只重建高亮组
    void rebuildHighlight();

    /// @brief 普通组：实体集里的全部可见实体，展平成子实体后按画笔分组
    PenGroups groupVisibleEntities() const;

    /// @brief 把实体展平成子实体（没有子实体时是它自己），按画笔加入分组
    /// @param pEnt 实体指针
    /// @param groups 分组
    static void addToGroups(DmEntity* pEnt, PenGroups& groups);

    bool isEntityMatchTypes(const DmEntity* e, const std::list<opengl::CacheType>& types);
    opengl::CacheType getCacheTypeOfEntity(const DmEntity* e);
    void cacheEntity(const PenGroups& map, opengl::CacheGroupType group);
    void cacheEntity(const DmEntity* e, int penId, opengl::CacheGroupType group);

    /// @brief 缓存linestrip
    /// @param lineStrip LineStrip指针
    /// @param penId 画笔ID
    /// @param group 缓存分组类型
    void cacheLineStrip(DmLineStrip* lineStrip, int penId, opengl::CacheGroupType group);

    /// @brief 缓存选中组与夹点：实体从选择来源枚举，不遍历全图（P13）
    void cacheSelected();

    /// @brief 缓存高亮组：高亮来源给出的实体里没有选中的那些
    void cacheHighlight();

    /// @brief 缓存拖拽点；超过 100 个时一个也不缓存
    /// @param selected 选中的实体
    void cacheSelectedPoints(const std::vector<DmEntity*>& selected);

    /// @brief 实体是否按选中绘制，见 setSelectionSource()
    bool isSelected(const DmEntity* e) const;

private:
    opengl::GLCachePainter* m_cachePainter = nullptr; ///< 画笔
    const ISelectionSource* m_selectionSource = nullptr; ///< 判断实体是否选中的来源，为空时没有实体选中
    const IHighlightSource* m_highlightSource = nullptr; ///< 要高亮的实体的来源，为空时没有实体高亮

    std::unordered_map<int, std::list<opengl::CacheType>> m_recacheTypes;

    std::list<DmEntityContainer*> m_containerList; ///< 绘制的实体集
    bool m_bIsModefied = true;          ///< 实体集是否已修改（整图重建）
    bool m_bSelectChanged = false;      ///< 选择集是否已修改（重建选中组、夹点与高亮组）
    bool m_bHighlightChanged = false;   ///< 高亮集是否已修改（重建高亮组）
};

#endif //DMCACHEPAINTER_H
