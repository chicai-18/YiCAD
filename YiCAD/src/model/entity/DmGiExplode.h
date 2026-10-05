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

/// @file DmGiExplode.h
/// @brief 把可绘制对象经 worldDraw 输出的图元做成基本实体（RENDER_PLAN.md 第 4.8.4 节）
///
/// 对应 ODA 的 OdDbEntity::explodeGeometry 的默认实现："调用 worldDraw，把产生的几何做成实体"。
/// 自定义实体的默认实现（包围框、拾取、捕捉、交叉选、炸开，见 DmCustomEntity）都基于它的结果，
/// 代理实体（DmProxyEntity）的炸开与 DXF 导出也是。

#ifndef DMGIEXPLODE_H
#define DMGIEXPLODE_H

#include <memory>
#include <vector>

class DmDocument;
class DmEntity;
class IGiDrawable;

/// @brief 把 GI 图元做成基本实体，见文件说明
/// @details 图元与实体的对应：多段线 → DmPolyline（两点的直线段 → DmLine），圆 → DmCircle，圆弧 → DmArc，椭圆弧 → DmEllipse，
///          样条 → DmSpline，点 → DmPoint，射线 → DmRay，构造线 → DmXline，三角形 → 每个一个 SOLID，图片 → DmImage。
///          变换按仿射算：相似变换下圆、圆弧仍是圆、圆弧，非等比缩放或错切下变成椭圆，带凸度的多段线拆成直线与椭圆弧。
///          字形按字形的几何做成笔画（GI 里只有字形，没有文字的语义）。以像素为单位的图元（setScreenSpace）不做成实体。
///
///          属性：可绘制对象自己的属性（setAttributes 与 worldDraw 里逐图元的覆盖）原样给出，ByLayer、ByBlock 保持符号形式；
///          嵌套绘制、块、字形里的 ByBlock 取外层，外层是 ByLayer 而所在图层不同时按外层的图层解析成具体值；
///          图层为空取外层的图层；线型比例逐层相乘
class DmGiExplode
{
public:
    /// @brief 结果的用途，决定块、填充怎么做
    enum class Purpose
    {
        Query,      ///< 拾取、捕捉、包围框：块展开成内容，填充只取边界（闭合多段线）
        Explode,    ///< 炸开、导出：块能表示成块参照（没有错切）时做成块参照；实心填充做成实心填充，图案填充做成切好的划线与点
        Graphics,   ///< 写进别的格式的代理图形（DXF，RENDER_PLAN.md 第 8.4 步）：块展开成内容，填充同 Explode，样条离散成多段线
    };

    /// @brief 结果里的一个实体
    struct Item
    {
        std::unique_ptr<DmEntity> entity;
        bool fromGlyph = false;     ///< 来自字形（文字的笔画）：默认的捕捉不用它，与 AutoCAD 不捕捉文字笔画一致
    };

    /// @brief 记录 drawable 的输出并做成实体
    /// @param document 实体所属文档：做成的实体按它设置（setDocument），随块线型取它的保留记录；可为空
    /// @param purpose 见 Purpose
    static std::vector<Item> run(const IGiDrawable& drawable, DmDocument* document, Purpose purpose);
};

#endif // DMGIEXPLODE_H
