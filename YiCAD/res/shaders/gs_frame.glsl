// 图形系统（GS）着色器各阶段共用的声明（doc/RENDER_PLAN.md 第 4.4 节）：每帧常量与常量定义，经 #include 引入。
// 顶点阶段另含 gs_vertex.glsl，线型的片段计算在 gs_dash.glsl。
//
// 数据布局与 C++ 一侧（src/render/gs/GsTypes.h）一一对应：
// - 第 0 组 Frame：每帧常量、各分块原点相对视点的偏移、每视图的状态位图（高亮、临时隐藏）；
// - 第 1 组 Model：对象状态、图层表、线型表、图元记录（每条 3 个 RGBA32UI 纹素）；
// - 第 2 组 Geometry：本管线类的几何（纹素缓冲），按顶点序号取，顶点序号在两个后端都含 firstVertex；
// - 实例属性（位置 0..3，步进为 1 的实例属性，读取遵守 firstInstance）：块参照、字形与顶层几何的实例记录。
// 着色器不用内建实例序号（第 4.7.3 节）。

// ---------------------------------------------------------------------------
// 第 0 组：每帧
// ---------------------------------------------------------------------------

layout(set = 0, binding = 0, std140) uniform Frame
{
    mat4 clipCorrection;   // RhiDevice::clipSpaceCorrection()
    vec4 viewport;         // x: 宽（像素） y: 高（像素） z: 每像素的世界长度 w: 线框外扩的抗锯齿边（像素）
    vec4 eye;              // 视点（画布中心）的世界坐标拆成 double 的高低两部分：xy 高位、zw 低位（无限线用）
    vec4 background;       // 背景色
    vec4 selectedColor;    // 选中色
    vec4 highlightColor;   // 高亮色
    vec4 lineStyle;        // x: 每单位线宽代码的像素（显示线宽时 0.05 × 设备像素比，否则 0） y: 选中、高亮加宽的像素
                           // z: 点的像素 w: 无限线外扩的世界长度
    uvec4 mode;            // x: 通道（0 场景、1 高亮叠加、2 无状态） y: 状态位图是否有效 z: 选中是否生效 w: 目标的采样数
    vec4 strokeStyle;      // x: 全局线型比例 LTSCALE y: 线型周期在屏幕上短于它（设备像素）时画实线
                           // z: 最细线宽（设备像素，即设备像素比） w: 保留
    vec4 grid;             // x: 网格间距 y: 粗网格间距 z: 是否画网格 w: 保留
    vec4 gridOffset;       // xy: 视点对网格间距取模 zw: 视点对粗网格间距取模
    vec4 gridColor;        // 细网格线的颜色
    vec4 metaGridColor;    // 粗网格线的颜色
    vec4 lod;              // LOD 的阈值（设备像素，第 4.3.10 节；为 0 时不起作用）：x 字高、y 填充图案线距、z 对象尺寸、
                           // w 圆弧只画一个四边形的半径
} frame;

const uint kNoSlot = 0xFFFFFFFFu;
const uint kLayerInstance = 0xFFFFu;   // 取实例记录里的图层
const uint kLayerNone = 0xFFFEu;       // 没有图层
const uint kKindValue = 0u;
const uint kKindByLayer = 1u;
const uint kKindByBlock = 2u;

// 图元记录里的对齐方式（GsDashMode）
const uint kDashNone = 0u;     // 不按线型（填充、点、图片）
const uint kDashOpen = 1u;     // 开放曲线：居中（A 型对齐）
const uint kDashClosed = 2u;   // 闭合曲线：整周期
const uint kDashInfinite = 3u; // 射线、构造线：从基点起周期重复
const uint kDashPattern = 4u;  // 填充图案线：相位给定，不做端点对齐

// 片段着色器的虚线画法（顶点着色器按对齐方式算好，gs_dash.glsl）
const uint kStrokeSolid = 0u;     // 实线（连续线、过密、短于一个周期的有划线图案）
const uint kStrokeEndDots = 1u;   // 只在两端画点（短于一个周期的只有点的图案）
const uint kStrokePeriodic = 2u;  // 周期重复：图案位置 = 弧长 + 相位
const uint kStrokeCentered = 3u;  // 居中：头部划线、中间周期重复、尾部划线
const uint kStrokeCapStart = 16u; // 起点是线的端点：起点之外按到端点的距离画圆头
const uint kStrokeCapEnd = 32u;   // 终点是线的端点

const uint kPrimFlagFill = 1u;   // 选中、高亮时半透明叠色，不加宽
const uint kPrimFlagPoint = 2u;
const uint kPrimFlagHatchLine = 4u;    // 填充图案线：dash.z 是线距，太密时不画
const uint kPrimFlagHatchCover = 8u;   // 填充图案过密时的替身：只在太密时画，按覆盖率抖动
const uint kPrimFlagTextBar = 16u;     // 小字的细条：dash.z 是字高，只在字高小于阈值时画

const uint kPassScene = 0u;
const uint kPassHighlight = 1u;
const uint kPassPlain = 2u;

const float kMaxOrderKey = 4194304.0;  // 2^22

