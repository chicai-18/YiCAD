// 图形系统（GS）顶点阶段共用的声明与函数：状态表、图元记录、实例记录、属性解析，经 #include 引入（在 gs_frame.glsl 之后）。

// ---------------------------------------------------------------------------
// 各分块原点相对视点的偏移（世界长度），按实例记录里的分块序号取；double 在 CPU 上减好再转 float（R6）
layout(set = 0, binding = 1) uniform samplerBuffer cellOffsets;
// 每视图的状态位图：每个对象槽位 2 位，低位为高亮（高亮集超过阈值时），高位为临时隐藏
layout(set = 0, binding = 2) uniform usamplerBuffer viewBits;

// ---------------------------------------------------------------------------
// 第 1 组：模型
// ---------------------------------------------------------------------------

// 对象状态：x 绘图次序 y 标志（1 选中、2 隐藏） z 顶层对象所在图层 w 保留
layout(set = 1, binding = 0) uniform usamplerBuffer objectStates;
// 图层表：x 颜色 RGBA y 线型序号 | 线宽代码 << 16 z 标志（1 冻结） w 保留
layout(set = 1, binding = 1) uniform usamplerBuffer layers;
// 图元记录：每条 2 个纹素，见 GsPrimRecord
layout(set = 1, binding = 3) uniform usamplerBuffer prims;

// ---------------------------------------------------------------------------
// 实例记录（步进为 1 的实例属性）
// ---------------------------------------------------------------------------

layout(location = 0) in vec4 iLinear;     // 线性部分 a b c d：x' = a·x + c·y
layout(location = 1) in vec4 iTranslate;  // xy 平移（相对分块原点） z 线型长度比例 w 保留
layout(location = 2) in uvec4 iInfo0;     // x 槽位 y ByBlock 颜色 z ByBlock 种类 w 实例图层 | ByBlock 图层 << 16
layout(location = 3) in uvec4 iInfo1;     // x ByBlock 线型 | ByBlock 线宽 << 16 y 分块序号 z 保留 w 保留

/// @brief 图元记录
struct Prim
{
    uint slot;
    vec4 color;
    uint primLayer;
    uint colorLayer;
    uint lineTypeLayer;
    uint lineWeightLayer;
    uint lineType;
    int lineWeight;
    uint colorKind;
    uint lineTypeKind;
    uint lineWeightKind;
    uint dashMode;
    uint flags;
    float runLength;
    float lineTypeScale;
};

vec4 unpackColor(uint c)
{
    return vec4(float(c & 0xFFu), float((c >> 8) & 0xFFu), float((c >> 16) & 0xFFu), float(c >> 24)) / 255.0;
}

Prim loadPrim(uint index)
{
    uvec4 a = texelFetch(prims, int(index * 2u));
    uvec4 b = texelFetch(prims, int(index * 2u + 1u));
    Prim p;
    p.slot = a.x;
    p.color = unpackColor(a.y);
    p.primLayer = a.z & 0xFFFFu;
    p.colorLayer = a.z >> 16;
    p.lineTypeLayer = a.w & 0xFFFFu;
    p.lineWeightLayer = a.w >> 16;
    p.lineType = b.x & 0xFFFFu;
    p.lineWeight = int(b.x) >> 16;
    p.colorKind = b.y & 3u;
    p.lineTypeKind = (b.y >> 2) & 3u;
    p.lineWeightKind = (b.y >> 4) & 3u;
    p.dashMode = (b.y >> 6) & 3u;
    p.flags = (b.y >> 8) & 0xFFu;
    p.runLength = uintBitsToFloat(b.z);
    p.lineTypeScale = uintBitsToFloat(b.w);
    return p;
}

uint instanceLayer()
{
    return iInfo0.w & 0xFFFFu;
}

uint resolveLayer(uint layer)
{
    return layer == kLayerInstance ? instanceLayer() : layer;
}

uvec4 loadLayer(uint layer)
{
    return texelFetch(layers, int(layer));
}

bool layerFrozen(uint layer)
{
    return layer < kLayerNone && (loadLayer(layer).z & 1u) != 0u;
}

/// @brief 颜色：值、随层、随块（取实例记录）；RGB 全 0 是 ACI 7，按背景亮度画白或黑（第 4.6 节）
vec4 resolveColor(Prim p)
{
    vec4 c = vec4(1.0);
    if (p.colorKind == kKindValue)
    {
        c = p.color;
    }
    else if (p.colorKind == kKindByLayer)
    {
        uint layer = resolveLayer(p.colorLayer);
        c = layer < kLayerNone ? unpackColor(loadLayer(layer).x) : vec4(0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        uint kind = iInfo0.z & 3u;
        uint layer = iInfo0.w >> 16;
        if (kind == kKindByLayer && layer < kLayerNone)
        {
            c = unpackColor(loadLayer(layer).x);
        }
        else
        {
            c = unpackColor(iInfo0.y);
        }
    }
    if (c.r == 0.0 && c.g == 0.0 && c.b == 0.0)
    {
        float luminance = dot(frame.background.rgb, vec3(0.299, 0.587, 0.114));
        c.rgb = luminance < 0.5 ? vec3(1.0) : vec3(0.0);
    }
    return c;
}

/// @brief 线型序号：0 为连续线
uint resolveLineType(Prim p)
{
    if (p.lineTypeKind == kKindValue)
    {
        return p.lineType;
    }
    if (p.lineTypeKind == kKindByLayer)
    {
        uint layer = resolveLayer(p.lineTypeLayer);
        return layer < kLayerNone ? (loadLayer(layer).y & 0xFFFFu) : 0u;
    }
    uint kind = (iInfo0.z >> 2) & 3u;
    uint layer = iInfo0.w >> 16;
    if (kind == kKindByLayer)
    {
        return layer < kLayerNone ? (loadLayer(layer).y & 0xFFFFu) : 0u;
    }
    return iInfo1.x & 0xFFFFu;
}

/// @brief 线宽代码（DM::LineWidth，毫米 × 100；负数为随层、随块、默认）
int resolveLineWeight(Prim p)
{
    if (p.lineWeightKind == kKindValue)
    {
        return p.lineWeight;
    }
    if (p.lineWeightKind == kKindByLayer)
    {
        uint layer = resolveLayer(p.lineWeightLayer);
        return layer < kLayerNone ? (int(loadLayer(layer).y) >> 16) : -1;
    }
    uint kind = (iInfo0.z >> 4) & 3u;
    uint layer = iInfo0.w >> 16;
    if (kind == kKindByLayer)
    {
        return layer < kLayerNone ? (int(loadLayer(layer).y) >> 16) : -1;
    }
    return int(iInfo1.x) >> 16;
}

/// @brief 线宽的像素：显示线宽时为 代码 × 0.05，至少 1 像素（旧渲染器的换算，阶段 5 改为毫米换算）
float lineWidthPixels(int code)
{
    return max(float(code) * frame.lineStyle.x, 1.0);
}

/// @brief 这个图元怎么画
struct Style
{
    bool visible;
    bool emphasized;   // 选中或高亮：加宽、换色
    bool selected;
    vec4 color;
    float depth;       // 裁剪空间的 z
};

/// @brief 按对象状态、图层与通道算出图元的样式
Style resolveStyle(Prim p)
{
    Style s;
    s.visible = true;
    s.emphasized = false;
    s.selected = false;
    s.depth = 0.0;

    uint slot = iInfo0.x != kNoSlot ? iInfo0.x : p.slot;
    uint mode = frame.mode.x;
    bool highlighted = mode == kPassHighlight;
    if (slot != kNoSlot && mode != kPassPlain)
    {
        uvec4 state = texelFetch(objectStates, int(slot));
        if ((state.y & 2u) != 0u || layerFrozen(state.z))
        {
            s.visible = false;
        }
        s.selected = frame.mode.z != 0u && (state.y & 1u) != 0u;
        if (frame.mode.y != 0u)
        {
            uint word = texelFetch(viewBits, int(slot >> 4)).x;
            uint bits = (word >> ((slot & 15u) * 2u)) & 3u;
            if ((bits & 2u) != 0u)
            {
                s.visible = false;
            }
            if ((bits & 1u) != 0u)
            {
                highlighted = true;
            }
        }
        float order = (float(state.x) + 1.0) / kMaxOrderKey;
        s.depth = 1.0 - order - (s.selected && mode == kPassScene ? 1.0 : 0.0);
    }
    if (layerFrozen(resolveLayer(p.primLayer)))
    {
        s.visible = false;
    }

    vec4 c = resolveColor(p);
    bool fill = (p.flags & kPrimFlagFill) != 0u;
    if (s.selected && mode == kPassScene)
    {
        s.emphasized = true;
        c = fill ? vec4(mix(c.rgb, frame.selectedColor.rgb, 0.5), c.a) : vec4(frame.selectedColor.rgb, c.a);
    }
    else if (highlighted)
    {
        s.emphasized = true;
        c = fill ? vec4(mix(c.rgb, frame.highlightColor.rgb, 0.5), c.a) : vec4(frame.highlightColor.rgb, c.a);
    }
    s.color = c;
    return s;
}

/// @brief 实例变换：局部坐标 -> 相对视点的世界坐标
vec2 toEye(vec2 local)
{
    vec2 world = vec2(iLinear.x * local.x + iLinear.z * local.y, iLinear.y * local.x + iLinear.w * local.y)
                 + iTranslate.xy;
    return world + texelFetch(cellOffsets, int(iInfo1.y)).xy;
}

/// @brief 只做线性部分的变换
vec2 linearPart(vec2 v)
{
    return vec2(iLinear.x * v.x + iLinear.z * v.y, iLinear.y * v.x + iLinear.w * v.y);
}

/// @brief 相对视点的世界坐标 -> 裁剪空间
vec4 eyeToClip(vec2 eye, float depth)
{
    vec2 halfSize = frame.viewport.xy * frame.viewport.z * 0.5;
    return frame.clipCorrection * vec4(eye / halfSize, depth, 1.0);
}

/// @brief 不画：落到裁剪空间外
vec4 collapsed()
{
    return vec4(2.0, 2.0, 2.0, 1.0);
}
