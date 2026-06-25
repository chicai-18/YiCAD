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

/// @file EntityDataDef.h
/// @brief 实体类型及各种枚举定义，包括实体类型、文字对齐方式、填充类型等

#ifndef ENTITYDATADEF_H
#define ENTITYDATADEF_H

/// @brief 实体类型枚举
enum class EEntityType
{
    eAttribute,         ///< 属性
    eAttributeDefinition,   ///< 属性定义
    eFace3d,            ///< 3D面
    eArc,               ///< 圆弧
    eBlock,             ///< 块
    eCircle,            ///< 圆
    eDimension,         ///< 标注
    eDimaligned,        ///< 对齐标注
    eDimlinear,         ///< 线性标注
    eDimradial,         ///< 半径标注
    eDimdiametric,      ///< 直径标注
    eDimangular,        ///< 角度标注
    eDimangular3p,      ///< 三点角度标注
    eDimordinate,       ///< 坐标标注
    eEllipse,           ///< 椭圆
    eHatch,             ///< 填充
    eImage,             ///< 图像
    eInsert,            ///< 插入
    eLeader,            ///< 引线
    eMLeader,           ///< 多重引线
    eLine,              ///< 直线
    eLineStrip,         ///< 线段串
    eMText,             ///< 多行文字
    ePoint,             ///< 点
    ePolyline,          ///< 多段线
    eRay,               ///< 射线
    eSolid,             ///< 实体
    eRegion,            ///< 区域
    eSpline,            ///< 样条曲线
    eText,              ///< 单行文字
    eTrace,             ///< 轨迹线
    eUnderlay,          ///< 底图
    eVertex,            ///< 顶点
    eVport,             ///< 视口
    eXLine,             ///< 无限长线
    eArrayRect,         ///< 矩形阵列
    eArrayPolar,        ///< 环形阵列
    eTable,             ///< 表格
    eUnknown            ///< 未知类型
};

/// @brief 样条曲线类型枚举
enum class ESplineType
{
    eFitPoints = 0,     ///< 拟合点
    eControlPoints = 1  ///< 控制点
};

/// @brief 单行文字横向对齐方式
enum class ETextHorzMode
{
    kTextLeft = 0,      ///< 左对齐
    kTextCenter = 1,    ///< 居中对齐
    kTextRight = 2,     ///< 右对齐
    kTextAlign = 3,     ///< 对齐
    kTextMid = 4,       ///< 中间
    kTextFit = 5        ///< 布满
};

/// @brief 单行文字纵向对齐方式
enum class ETextVertMode
{
    kTextBase = 0,      ///< 基线
    kTextBottom = 1,    ///< 底部
    kTextVertMid = 2,   ///< 垂直中间
    kTextTop = 3        ///< 顶部
};

/// @brief 单行文字对齐方式（与ETextHorzMode及ETextVertMode对应）
enum class ETextMode
{
    kTextLeft = 0,          ///< 左对齐
    kTextCenter,            ///< 居中
    kTextRight,             ///< 右对齐
    kTextAligned,           ///< 对齐
    kTextMiddle,            ///< 中间
    kTextFit,               ///< 布满
    kTextTopLeft,           ///< 左上
    kTextTopCenter,         ///< 中上
    kTextTopRight,          ///< 右上
    kTextMiddleLeft,        ///< 左中
    kTextMiddleCenter,      ///< 正中
    kTextMiddleRight,       ///< 右中
    kTextBottomLeft,        ///< 左下
    kTextBottomCenter,      ///< 中下
    kTextBottomRight,       ///< 右下
};

/// @brief 多行文字对齐方式
enum class EMTextMode
{
    kTextTopLeft,           ///< 左上
    kTextTopCenter,         ///< 中上
    kTextTopRight,          ///< 右上
    kTextMiddleLeft,        ///< 左中
    kTextMiddleCenter,      ///< 正中
    kTextMiddleRight,       ///< 右中
    kTextBottomLeft,        ///< 左下
    kTextBottomCenter,      ///< 中下
    kTextBottomRight,       ///< 右下
};

/// @brief 多行文字横向对齐方式
enum class EMTextHorzMode
{
    kTextLeft = 0,      ///< 左对齐
    kTextCenter = 1,    ///< 居中对齐
    kTextRight = 2,     ///< 右对齐
    kTextAlign = 3,     ///< 对齐
    kTextMid = 4,       ///< 中间
    kTextFit = 5        ///< 布满
};

/// @brief 多行文字纵向对齐方式
enum class EMTextVertMode
{
    kTextBase = 0,      ///< 基线
    kTextBottom = 1,    ///< 底部
    kTextVertMid = 2,   ///< 垂直中间
    kTextTop = 3        ///< 顶部
};

/// @brief 标注文字对齐方式
enum class EAttachmentPoint
{
    kTopLeft = 1,       ///< 左上
    kTopCenter = 2,     ///< 中上
    kTopRight = 3,      ///< 右上
    kMiddleLeft = 4,    ///< 左中
    kMiddleCenter = 5,  ///< 正中
    kMiddleRight = 6,   ///< 右中
    kBottomLeft = 7,    ///< 左下
    kBottomCenter = 8,  ///< 中下
    kBottomRight = 9,   ///< 右下
    kBaseLeft = 10,     ///< 基线左对齐 /预留/
    kBaseCenter = 11,   ///< 基线居中 /预留/
    kBaseRight = 12,    ///< 基线右对齐 /预留/
    kBaseAlign = 13,    ///< 基线对齐 /预留/
    kBottomAlign = 14,  ///< 底部对齐 /预留/
    kMiddleAlign = 15,  ///< 中部对齐 /预留/
    kTopAlign = 16,     ///< 顶部对齐 /预留/
    kBaseFit = 17,      ///< 基线布满 /预留/
    kBottomFit = 18,    ///< 底部布满 /预留/
    kMiddleFit = 19,    ///< 中部布满 /预留/
    kTopFit = 20,       ///< 顶部布满 /预留/
    kBaseMid = 21,      ///< 基线中间 /预留/
    kBottomMid = 22,    ///< 底部中间 /预留/
    kMiddleMid = 23,    ///< 中部中间 /预留/
    kTopMid = 24        ///< 顶部中间 /预留/
};

/// @brief 非原子实体的更新模式（如文字、插入等需要更新的实体）
enum class EUpdateMode
{
    NoUpdate,           ///< 不自动更新
    Update,             ///< 修改时始终自动更新
    PreviewUpdate       ///< 仅预览时自动更新（快速更新）
};

/// @brief 填充环类型
enum class EHatchLoopType
{
    kDefault = 0,           ///< 未指定
    kExternal = 1,          ///< 由外部实体定义
    kPolyline = 2,          ///< 由多段线定义
    kDerived = 4,           ///< 从拾取点派生
    kTextbox = 8,           ///< 由文字定义
    kOutermost = 0x10,      ///< 最外层环
    kNotClosed = 0x20,      ///< 非闭合环
    kSelfIntersecting = 0x40,   ///< 自交环
    kTextIsland = 0x80,     ///< 被偶数个环包围的文字环
    kDuplicate = 0x100,     ///< 重复环
    kIsAnnotative = 0x200,  ///< 边界为注释性块
    kDoesNotSupportScale = 0x400,   ///< 边界不支持缩放
    kForceAnnoAllVisible = 0x800,   ///< 强制所有注释性可见
    kOrientToPaper = 0x1000,    ///< 方向适应纸张
    kIsAnnotativeBlock = 0x2000 ///< 标识填充是否为注释性块
};

/// @brief 填充边界边类型
enum class EHatchEdgeType
{
    kNone = 0,      ///< 跳过此段
    kLine = 1,      ///< 直线
    kCirArc = 2,    ///< 圆弧
    kEllArc = 3,    ///< 椭圆弧
    kSpline = 4     ///< 样条曲线
};

/// @brief 填充图案类型
enum class EHatchPatternType
{
    kUserDefined = 0,   ///< 用户定义填充
    kPreDefined = 1,    ///< 在acad.pat和acadiso.pat中定义的填充
    kCustomDefined = 2  ///< 在自有PAT文件中定义的填充
};

/// @brief 填充样式
enum class EHatchStyle
{
    kNormal = 0,    ///< 每层边界切换填充
    kOuter = 1,     ///< 仅最外层填充
    kIgnore = 2     ///< 忽略内部边界
};

#endif // ENTITYDATADEF_H
