// 线型（虚线）与线宽的片段计算，线段、无限线与圆弧管线共用，经 #include 引入（在 gs_frame.glsl 之后）。
//
// 语义照搬旧渲染器（res/shaders/line.shader 等，渲染方案第 4 阶段的决定：线型语义阶段 5 再改）：
// - 开放曲线（kDashOpen）：比一个周期短时，有划线的图案画实线、只有点的图案只在两端画点；否则居中：
//   整数个周期之外的余量平分到两端并入两端的划线，中间从第一段划线的中点起算图案；
// - 闭合曲线（kDashClosed）：比一个周期短时画实线；否则取 ceil(L/P) 个周期（至少 2 个）等比伸缩，从起点起算；
// - 无限线（kDashInfinite）：从基点起周期重复；一个像素比周期还长时画实线。
// 划线两端与线端都是圆头；点画成半径至少 1 像素的圆点（同旧渲染器）。
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
/// @param dotGrow 点向两边放大的长度（点画成比线宽大的圆点）
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
/// @param s 弧长参数（世界长度）
/// @param total 这一段线的总长（世界长度）；无限线为负表示构造线（没有端点），否则是射线
/// @param mode kDashOpen、kDashClosed、kDashInfinite
/// @param lineType 线型序号，0 为连续线
/// @param dotGrow 点向两边放大的长度
float dashDistance(float s, float total, uint mode, uint lineType, float dotGrow)
{
    float outside = max(max(-s, s - total), 0.0);
    if (mode == kDashInfinite)
    {
        outside = total < 0.0 ? 0.0 : max(-s, 0.0);
    }
    vec4 header = texelFetch(lineTypes, int(lineType * 4u));
    uint count = uint(header.x);
    float period = header.y;
    if (lineType == 0u || count == 0u || period <= 0.0)
    {
        return outside;
    }
    bool dotOnly = header.w <= header.z;   // 没有正长度的划线

    if (mode == kDashInfinite)
    {
        if (s < 0.0 && total >= 0.0)
        {
            return outside;
        }
        if (frame.viewport.z > period)
        {
            return 0.0;
        }
        return patternDistance(lineType, count, period, 1.0, mod(s, period), dotGrow);
    }

    if (mode == kDashClosed)
    {
        if (total < period)
        {
            return outside;
        }
        float n = ceil(total / period);
        if (n == 1.0)
        {
            n = 2.0;
        }
        float scale = total / n / period;
        return patternDistance(lineType, count, period, scale, mod(s, period * scale), dotGrow);
    }

    // 开放曲线
    if (total < period)
    {
        if (!dotOnly)
        {
            return outside;
        }
        return max(min(abs(s), abs(s - total)) - dotGrow, 0.0);
    }
    float offset1 = fract(total / period) * 0.5 * period;
    float offset2 = total - offset1;
    if (s < 0.0 || s > total)
    {
        return outside;
    }
    float best = kHugeDistance;
    if (!dotOnly)
    {
        best = min(intervalDistance(s, 0.0, offset1), intervalDistance(s, offset2, total));
    }
    else
    {
        best = max(min(min(abs(s), abs(s - offset1)), min(abs(s - offset2), abs(s - total))) - dotGrow, 0.0);
    }
    if (s > offset1 && s < offset2)
    {
        float shift = dotOnly ? 0.0 : (header.z + header.w) * 0.5;
        float r = mod(s - offset1 + shift, period);
        best = min(best, patternDistance(lineType, count, period, 1.0, r, dotGrow));
    }
    return best;
}

/// @brief 一个采样点是否落在线上：沿线到墨的距离与到中心线的距离合成圆头，与半线宽比较
/// @param along 弧长参数（世界长度）
/// @param across 到中心线的距离（世界长度）
bool insideStroke(float along, float across, float total, uint mode, uint lineType, float halfWidth)
{
    float dotGrow = max(frame.viewport.z - halfWidth, 0.0);
    float d = dashDistance(along, total, mode, lineType, dotGrow);
    return length(vec2(d, across)) <= halfWidth;
}
