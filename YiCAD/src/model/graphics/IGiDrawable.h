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

/// @file IGiDrawable.h
/// @brief GI：可绘制对象及其绘制上下文（RENDER_PLAN.md 第 4.2 节）
///
/// GI 是实体与图形系统之间的契约：实体只描述"我由哪些几何组成"，不认识图形 API，也不知道谁在接收。
/// 接收方有 GS（第 4 阶段）、旧渲染器的适配器（第 2 阶段起，至 GS 取代它为止）、GI 流记录器
/// （代理图形、测试）等。接口放在 Model 层，与 AutoCAD 的 AcGi 位于 AcDb 之下一致。

#ifndef IGIDRAWABLE_H
#define IGIDRAWABLE_H

#include "GiTypes.h"

class IGiGeometry;
class IGiSubEntityTraits;
class IGiWorldDraw;
class IGiViewportDraw;

/// @brief 可绘制对象。DmEntity、DmBlock（块定义）与字形模板实现它
class IGiDrawable
{
public:
    virtual ~IGiDrawable() = default;

    /// @brief 按自身的属性设置 traits（颜色、图层、线型、线宽），在 worldDraw 之前调用
    /// @details 对应 AutoCAD 的 subSetAttributes。调用时 traits 为全 ByBlock、图层为空；
    ///          默认什么也不设，即全部取外层
    virtual void setAttributes(IGiSubEntityTraits& traits) const {}

    /// @brief 描述自身几何
    /// @details 必须是 const、确定性的（同样的数据输出同样的图元）；除非 drawableFlags() 声明，
    ///          否则必须可以在工作线程上与其他对象并行调用
    virtual void worldDraw(IGiWorldDraw& wd) const = 0;

    /// @brief 随视图变化的部分；仅当 drawableFlags() 含 GiDrawableFlags::ViewDependent 时调用
    virtual void viewportDraw(IGiViewportDraw& vd) const {}

    /// @brief 标志
    virtual GiDrawableFlags drawableFlags() const { return GiDrawableFlags::None; }
};

/// @brief worldDraw 的上下文
class IGiWorldDraw
{
public:
    virtual ~IGiWorldDraw() = default;

    /// @brief 图元词汇
    virtual IGiGeometry& geometry() = 0;

    /// @brief 图元属性
    virtual IGiSubEntityTraits& traits() = 0;

    /// @brief 这次生成的用途：显示、代理图形、范围计算、导出。实体一般不需要区分
    virtual GiRegenType regenType() const = 0;

    /// @brief 实体自行离散曲线时应满足的弦高容差（世界单位）
    virtual double deviation() const = 0;

    /// @brief 是否在拖动预览中；实体可以输出简化图形
    virtual bool isDragging() const = 0;
};

/// @brief viewportDraw 的上下文：在 worldDraw 的上下文之外给出视图信息
class IGiViewportDraw : public IGiWorldDraw
{
public:
    /// @brief 每像素的世界长度
    virtual double worldPerPixel() const = 0;
};

#endif // IGIDRAWABLE_H
