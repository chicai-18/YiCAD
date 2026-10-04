// 线型（虚线）与线宽的片段计算，线段、无限线与圆弧管线共用，经 #include 引入（在 gs_frame.glsl 之后）。
//
// 线型的语义见 doc/RENDER_PLAN.md 第 4.5 节。顶点着色器（gs_vertex.glsl 的 strokeOf）按图元记录的对齐方式与比例链
// （图案 × 实体线型比例 × 块参照的线型比例 × LTSCALE；块的插入比例不在链上，填充图案线随块缩放）定好画法与参数，
// 这里只按它们求一个采样点沿线到墨的距离：
// - 居中（开放曲线，A 型对齐）：头部划线 [0, 头部终点]、尾部划线 [尾部起点, 段长]，中间按"弧长 + 相位"周期重复；
// - 周期重复：闭合曲线（已按整周期拉伸）、射线与构造线（从基点起）、填充图案线（相位给定）；
// - 实线：连续线、周期在屏幕上过密、短于一个周期且图案里有划线；只在两端画点：短于一个周期且图案只有点。
// 划线两端与线端都是圆头；点画成直径等于线宽（至少 1 像素）的圆点。超长的线由编译分成若干段（第 4.5.5 节），
// 段与段相接处不是线端（没有 kStrokeCapStart、kStrokeCapEnd），段外照样按图案延伸，与相邻的段接上。
//
// 覆盖按多重采样的每个采样点判断（insideStroke），片段着色器写采样掩码：边缘的抗锯齿与几何多重采样相同，
// 没有 alpha-to-coverage 的抖动，与绘制顺序也无关（深度只写在覆盖到的采样上）。

// 线型表：每个线型 4 个纹素：(元素数, 周期, 第一段划线起点, 第一段划线终点) 与 12 个元素
layout(set = 1, binding = 2) uniform samplerBuffer lineTypes;

const float kHugeDistance = 1.0e30;

/// @brief 线型的第 i 个元素
float lineTypeElement(uint lineType, uint i)
{
    vec4 t = texelFetch(lineTypes, int(lineType * 4u + 1u + i / 4u));
    uint k = i % 4u;
    return k == 0u ? t.x : (k == 1u ? t.y : (k == 2u ? t.z : t.w));
}

/// @brief 点 x 到区间 [a, b] 的距离
float intervalDistance(float x, float a, float b)
{
    return max(max(a - x, x - b), 0.0);
}

/// @brief 在一个周期（已按 scale 伸缩）里，r 到最近的划线或点的距离；考虑相邻周期
/// @param dotGrow 点向两边放大的长度（点画成直径等于线宽、至少 1 像素的圆点）
float patternDistance(uint lineType, uint count, float period, float scale, float r, float dotGrow)
{
    float best = kHugeDistance;
    float cursor = 0.0;
    float scaledPeriod = period * scale;
    for (uint i = 0u; i < count && i < 12u; ++i)
    {
        float v = lineTypeElement(lineType, i);
        float len = abs(v) * scale;
        if (v >= 0.0)
        {
            // 划线或点（0）：本周期与前后两个周期里的同一段
            float grow = v == 0.0 ? dotGrow : 0.0;
            float a = cursor - grow;
            float b = cursor + len + grow;
            best = min(best, intervalDistance(r, a, b));
            best = min(best, intervalDistance(r, a - scaledPeriod, b - scaledPeriod));
            best = min(best, intervalDistance(r, a + scaledPeriod, b + scaledPeriod));
        }
        cursor += len;
    }
    return best;
}

/// @brief 沿线方向到最近的墨的距离（在墨里为 0）
/// @param s 弧长参数（世界长度），段内从 0 起，段外延伸成负数或超过段长
/// @param len 这一段的长度（世界长度）
/// @param stroke 画法 | 端点标志 | 线型序号 << 16（gs_vertex.glsl 的 Stroke）
/// @param scale 图案长度到弧长参数的比例
/// @param params 相位、头部划线终点、尾部划线起点
/// @param dotGrow 点向两边放大的长度
float dashDistance(float s, float len, uint stroke, float scale, vec3 params, float dotGrow)
{
    uint kind = stroke & 15u;
    bool capStart = (stroke & kStrokeCapStart) != 0u;
    bool capEnd = (stroke & kStrokeCapEnd) != 0u;
    uint lineType = stroke >> 16;
    float before = -s;
    float after = s - len;

    if (kind == kStrokeEndDots)
    {
        return max(min(abs(s), abs(after)) - dotGrow, 0.0);
    }
    if (kind == kStrokeSolid)
    {
        return max(max(capStart ? before : -kHugeDistance, capEnd ? after : -kHugeDistance), 0.0);
    }

    vec4 header = texelFetch(lineTypes, int(lineType * 4u));
    uint count = uint(header.x);
    float period = header.y;
    float scaledPeriod = period * scale;

    if (kind == kStrokePeriodic)
    {
        // 线端之外：到线端的距离加上线端处到墨的距离
        float r = s;
        float outside = 0.0;
        if (capStart && s < 0.0)
        {
            r = 0.0;
            outside = before;
        }
        else if (capEnd && s > len)
        {
            r = len;
            outside = after;
        }
        return outside + patternDistance(lineType, count, period, scale, mod(r + params.x, scaledPeriod), dotGrow);
    }

    // 居中：两端都是划线（或点）
    if (capStart && s < 0.0)
    {
        return before;
    }
    if (capEnd && s > len)
    {
        return after;
    }
    bool dotOnly = header.w <= header.z;   // 没有正长度的划线
    float headEnd = params.y;
    float tailStart = params.z;
    float best = kHugeDistance;
    if (capStart)
    {
        best = min(best, dotOnly ? max(min(abs(s), abs(s - headEnd)) - dotGrow, 0.0) : intervalDistance(s, 0.0, headEnd));
    }
    if (capEnd)
    {
        best = min(best, dotOnly ? max(min(abs(s - tailStart), abs(after)) - dotGrow, 0.0) : intervalDistance(s, tailStart, len));
    }
    if (s > headEnd && s < tailStart)
    {
        best = min(best, patternDistance(lineType, count, period, scale, mod(s + params.x, scaledPeriod), dotGrow));
    }
    return best;
}

/// @brief 一个采样点是否落在线上：沿线到墨的距离与到中心线的距离合成圆头，与半线宽比较
/// @param along 弧长参数（世界长度）
/// @param across 到中心线的距离（世界长度）
bool insideStroke(float along, float across, float len, uint stroke, float scale, vec3 params, float halfWidth)
{
    // 点的直径等于线宽，至少 1 像素
    float dotGrow = max(0.5 * frame.viewport.z - halfWidth, 0.0);
    float d = dashDistance(along, len, stroke, scale, params, dotGrow);
    return length(vec2(d, across)) <= halfWidth;
}
