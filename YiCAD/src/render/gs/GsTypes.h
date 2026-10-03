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

/// @file GsTypes.h
/// @brief 图形系统（GS）送进 GPU 的记录格式与常量（RENDER_PLAN.md 第 4.3、4.4 节）
/// @details 与着色器（YiCAD/res/shaders/src/gs_*.glsl）一一对应，改一边要改另一边。
///          几何按管线类放在纹素缓冲里，着色器按顶点序号取；实例记录是步进为 1 的实例属性

#ifndef GSTYPES_H
#define GSTYPES_H

#include <array>
#include <cstdint>

/// @brief 管线类（第 4.4 节）。每类一个几何缓冲，一条记录画成固定个数的顶点
enum class GsClass : std::uint8_t
{
    Segment,       ///< 线段：点缓冲，相邻两点一段，每个点 6 个顶点
    Arc,           ///< 圆弧：每条记录 48 个顶点（环带 8 段）
    Fill,          ///< 三角形：每个顶点一条记录
    Point,         ///< 点：每个点 6 个顶点
    Image,         ///< 图片：每张 6 个顶点，各自绑纹理
    InfiniteLine,  ///< 射线与构造线：每条 6 个顶点，不进分块
    Count
};

constexpr std::size_t kGsClassCount = static_cast<std::size_t>(GsClass::Count);

/// @brief 每类记录画成的顶点数
constexpr std::uint32_t gsVerticesPerRecord(GsClass c)
{
    switch (c)
    {
    case GsClass::Segment: return 6;
    case GsClass::Arc: return 48;
    case GsClass::Fill: return 1;
    case GsClass::Point: return 6;
    case GsClass::Image: return 6;
    case GsClass::InfiniteLine: return 6;
    case GsClass::Count: break;
    }
    return 0;
}

/// @brief 每类记录占的纹素数（RGBA32F，16 字节）
constexpr std::uint32_t gsTexelsPerRecord(GsClass c)
{
    switch (c)
    {
    case GsClass::Segment: return 1;
    case GsClass::Arc: return 2;
    case GsClass::Fill: return 1;
    case GsClass::Point: return 1;
    case GsClass::Image: return 2;
    case GsClass::InfiniteLine: return 2;
    case GsClass::Count: break;
    }
    return 0;
}

/// @brief 一个纹素（RGBA32F 或 RGBA32UI，按缓冲而定）
struct GsTexel
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};
static_assert(sizeof(GsTexel) == 16);

/// @brief 线段管线的点：x y 相对原点、z 弧长参数、w 图元记录序号的位模式（最高位为"这一点之后断开"）
constexpr std::uint32_t kGsPointBreak = 0x80000000u;

// ---------------------------------------------------------------------------
// 图元记录（每条 2 个 RGBA32UI 纹素）
// ---------------------------------------------------------------------------

constexpr std::uint32_t kGsNoSlot = 0xFFFFFFFFu;         ///< 槽位取实例记录
constexpr std::uint16_t kGsLayerInstance = 0xFFFFu;     ///< 图层取实例记录
constexpr std::uint16_t kGsLayerNone = 0xFFFEu;         ///< 没有图层

/// @brief 属性的种类
enum class GsKind : std::uint8_t
{
    Value = 0,    ///< 记录里的值
    ByLayer = 1,  ///< 随层：查对应图层
    ByBlock = 2,  ///< 随块：取实例记录
};

/// @brief 线型的对齐方式
enum class GsDashMode : std::uint8_t
{
    None = 0,      ///< 不按线型
    Open = 1,      ///< 开放曲线：居中
    Closed = 2,    ///< 闭合曲线：整周期
    Infinite = 3,  ///< 射线、构造线
};

constexpr std::uint8_t kGsPrimFlagFill = 1;    ///< 选中、高亮时半透明叠色，不加宽
constexpr std::uint8_t kGsPrimFlagPoint = 2;

/// @brief 图元记录：一组属性相同的图元共用（颜色、图层、线型、线宽、所属对象、虚线的段长）
struct GsPrimRecord
{
    std::uint32_t slot = kGsNoSlot;          ///< 顶层对象的槽位；块与字形的几何取实例记录
    std::uint32_t color = 0xFF000000u;       ///< RGBA，种类为值时用
    std::uint32_t layers0 = 0;               ///< 所在图层（冻结判断） | 颜色随层用的图层 << 16
    std::uint32_t layers1 = 0;               ///< 线型随层用的图层 | 线宽随层用的图层 << 16
    std::uint32_t lineTypeAndWeight = 0;     ///< 线型序号 | 线宽代码（int16）<< 16
    std::uint32_t kinds = 0;                 ///< 颜色、线型、线宽的种类各 2 位 | 对齐方式 << 6 | 标志 << 8
    float runLength = 0.0f;                  ///< 这一段线的总长（局部长度）
    float lineTypeScale = 1.0f;              ///< 实体线型比例（阶段 5 起生效）
};
static_assert(sizeof(GsPrimRecord) == 32);

// ---------------------------------------------------------------------------
// 实例记录（步进为 1 的实例属性，64 字节）
// ---------------------------------------------------------------------------

/// @brief 实例：一份几何（顶层几何、块定义、字形）按一个仿射变换画一次
/// @details 顶层几何每个分块一条恒等实例（槽位取图元记录）；块参照、字形每个（展开到叶子的）插入一条
struct GsInstanceRecord
{
    std::array<float, 4> linear = {1.0f, 0.0f, 0.0f, 1.0f};  ///< a b c d：x' = a·x + c·y
    std::array<float, 4> translate = {0.0f, 0.0f, 1.0f, 0.0f}; ///< 平移（相对分块原点）、线型长度比例
    std::uint32_t slot = kGsNoSlot;          ///< 顶层对象的槽位
    std::uint32_t byBlockColor = 0xFF000000u; ///< 随块颜色 RGBA
    std::uint32_t byBlockKinds = 0;          ///< 随块颜色、线型、线宽的种类各 2 位（值或随层）
    std::uint32_t layers = 0;                ///< 实例图层（块里图层为空的图元取它） | 随块属性随层用的图层 << 16
    std::uint32_t byBlockLineTypeAndWeight = 0; ///< 随块线型序号 | 随块线宽代码（int16）<< 16
    std::uint32_t cell = 0;                  ///< 分块序号：着色器取分块原点相对视点的偏移
    std::uint32_t reserved0 = 0;
    std::uint32_t reserved1 = 0;
};
static_assert(sizeof(GsInstanceRecord) == 64);

// ---------------------------------------------------------------------------
// 状态表
// ---------------------------------------------------------------------------

constexpr std::uint32_t kGsStateSelected = 1u;   ///< 对象状态：选中
constexpr std::uint32_t kGsStateHidden = 2u;     ///< 对象状态：实体不可见

/// @brief 对象状态（每个顶层对象一个槽位）
struct GsObjectState
{
    std::uint32_t order = 0;                 ///< 绘图次序（越大越靠上），写进深度
    std::uint32_t flags = 0;                 ///< kGsStateSelected、kGsStateHidden
    std::uint32_t layer = kGsLayerNone;      ///< 顶层对象所在的图层（块参照所在层冻结时整个块不画）
    std::uint32_t reserved = 0;
};
static_assert(sizeof(GsObjectState) == 16);

constexpr std::uint32_t kGsMaxOrder = 1u << 22;  ///< 绘图次序的上限，超过时重新编号

/// @brief 图层表的一项
struct GsLayerRecord
{
    std::uint32_t color = 0xFFFFFFFFu;       ///< RGBA
    std::uint32_t lineTypeAndWeight = 0;     ///< 线型序号 | 线宽代码（int16）<< 16
    std::uint32_t flags = 0;                 ///< 1 冻结
    std::uint32_t reserved = 0;
};
static_assert(sizeof(GsLayerRecord) == 16);

constexpr std::uint32_t kGsLayerFrozen = 1u;

/// @brief 线型表的一项：表头（元素数、周期、第一段划线的起止）与最多 12 个元素（AutoCAD 简单线型的上限）
struct GsLineTypeRecord
{
    float count = 0.0f;
    float period = 0.0f;
    float firstDashStart = 0.0f;
    float firstDashEnd = 0.0f;
    std::array<float, 12> elements{};
};
static_assert(sizeof(GsLineTypeRecord) == 64);

/// @brief 每帧常量，std140 布局与 gs_frame.glsl 的 Frame 相同
struct GsFrameConstants
{
    std::array<float, 16> clipCorrection{};
    std::array<float, 4> viewport{};       ///< 宽、高（像素）、每像素世界长度、抗锯齿外扩（像素）
    std::array<float, 4> eye{};            ///< 视点拆成高低两部分：x 高、y 高、x 低、y 低
    std::array<float, 4> background{};
    std::array<float, 4> selectedColor{};
    std::array<float, 4> highlightColor{};
    std::array<float, 4> lineStyle{};      ///< 每单位线宽代码的像素、选中加宽、点像素、无限线外扩
    std::array<std::uint32_t, 4> mode{};   ///< 通道、状态位图有效、选中生效、目标的采样数（片段着色器逐采样判断覆盖）
    std::array<float, 4> grid{};           ///< 细间距、粗间距、是否画、保留
    std::array<float, 4> gridOffset{};     ///< 视点对细、粗间距取模
    std::array<float, 4> gridColor{};
    std::array<float, 4> metaGridColor{};
};
static_assert(sizeof(GsFrameConstants) == 240);

constexpr std::uint32_t kGsPassScene = 0;      ///< 场景通道：选中按对象状态
constexpr std::uint32_t kGsPassHighlight = 1;  ///< 高亮叠加：全部按高亮画
constexpr std::uint32_t kGsPassPlain = 2;      ///< 无状态（预览、块缩略图）

/// @brief 叠加层动态批次的顶点（gs_overlay.vert）
struct GsOverlayVertex
{
    float x = 0.0f;              ///< 像素，左上角为原点
    float y = 0.0f;
    std::uint32_t color = 0;     ///< RGBA8
    float dash = -1.0f;          ///< 沿线的像素长度；负数为实线
};
static_assert(sizeof(GsOverlayVertex) == 16);

/// @brief 打包 RGBA8（r 在最低字节）
constexpr std::uint32_t gsPackColor(int r, int g, int b, int a)
{
    return static_cast<std::uint32_t>(r & 0xFF) | (static_cast<std::uint32_t>(g & 0xFF) << 8)
         | (static_cast<std::uint32_t>(b & 0xFF) << 16) | (static_cast<std::uint32_t>(a & 0xFF) << 24);
}

/// @brief 线型序号与线宽代码打包
constexpr std::uint32_t gsPackLineTypeAndWeight(std::uint32_t lineType, int lineWeight)
{
    return (lineType & 0xFFFFu) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::int16_t>(lineWeight))) << 16);
}

#endif // GSTYPES_H
