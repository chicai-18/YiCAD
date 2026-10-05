/// @file PluginGi.cpp
/// @brief 宿主 GI 表的实现：校验插件给的参数，转成宿主类型交给 IGiGeometry、IGiSubEntityTraits

#include "PluginGi.h"

#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "DmEntity.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmText.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "GiNurbs.h"
#include "GiTypes.h"
#include "IGiDrawable.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"
#include "PluginAbiConvert.h"
#include "YiCadLog.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{

using namespace plugin_abi;

PluginGiContext* contextOf(YiCadGiContextHandle ctx) noexcept
{
    return static_cast<PluginGiContext*>(ctx);
}

/// @brief 在插件边界内调用：上下文为空、参数无效（body 返回 false）或抛异常都返回失败
template<typename Body>
YiCadResult guarded(YiCadGiContextHandle ctx, Body&& body) noexcept
{
    try
    {
        PluginGiContext* context = contextOf(ctx);
        return context != nullptr && body(*context) ? YICAD_SUCCESS : YICAD_FAILURE;
    }
    catch (...)
    {
        return YICAD_FAILURE;
    }
}

bool finiteValue(double value) noexcept
{
    return std::isfinite(value) && std::abs(value) <= 1.0e150;
}

std::vector<DmVector> toPoints(const YiCadPoint2dArrayView& view)
{
    std::vector<DmVector> points;
    points.reserve(view.count);
    for (uint32_t i = 0; i < view.count; ++i)
    {
        points.push_back(toDmVector(view.data[i]));
    }
    return points;
}

/// @brief 资源句柄是否为本文档相应表里的对象（插件交来的句柄不经核对不解引用）
bool ownsResource(DmDocument* document, YiCadReadResourceKind kind, const void* handle)
{
    if (document == nullptr || handle == nullptr)
    {
        return false;
    }
    switch (kind)
    {
    case YICAD_READ_LINE_TYPE:
        for (auto* value : *document->getLineTypeTable())
        {
            if (value == handle)
            {
                return true;
            }
        }
        return handle == document->getLineTypeTable()->getLineTypeByLayer() ||
               handle == document->getLineTypeTable()->getLineTypeByBlock();
    case YICAD_READ_LAYER:
        for (auto* value : *document->getLayerTable())
        {
            if (value == handle)
            {
                return true;
            }
        }
        return false;
    case YICAD_READ_TEXT_STYLE:
        for (auto* value : *document->getTextStyleTable())
        {
            if (value == handle)
            {
                return true;
            }
        }
        return false;
    case YICAD_READ_BLOCK:
        for (auto* value : *document->getBlockTable())
        {
            if (value == handle)
            {
                return true;
            }
        }
        return false;
    default:
        return false;
    }
}

YiCadResult YICAD_PLUGIN_CALL setColor(YiCadGiContextHandle ctx, const YiCadColorData* color) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        DmColor value;
        if (color == nullptr || !toDmColor(*color, value))
        {
            return false;
        }
        c.color = value;
        c.wd().traits().setColor(value);
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setLayer(YiCadGiContextHandle ctx, YiCadReadResourceHandle layer) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!ownsResource(c.document(), YICAD_READ_LAYER, layer))
        {
            return false;
        }
        c.wd().traits().setLayer(static_cast<const DmLayer*>(layer));
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setLineType(YiCadGiContextHandle ctx, YiCadReadResourceHandle lineType) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        // 为空即随块（GI 的约定）
        if (lineType != nullptr && !ownsResource(c.document(), YICAD_READ_LINE_TYPE, lineType))
        {
            return false;
        }
        c.lineType = static_cast<const DmLineType*>(lineType);
        c.wd().traits().setLineType(c.lineType);
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setLineTypeScale(YiCadGiContextHandle ctx, double scale) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!finiteValue(scale) || scale <= 0.0)
        {
            return false;
        }
        c.wd().traits().setLineTypeScale(scale);
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setLineWeight(YiCadGiContextHandle ctx, int32_t weight) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!validLineWidth(weight))
        {
            return false;
        }
        c.lineWeight = static_cast<DM::LineWidth>(weight);
        c.wd().traits().setLineWeight(c.lineWeight);
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setTransparency(YiCadGiContextHandle ctx, uint32_t alpha) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (alpha > 255)
        {
            return false;
        }
        c.wd().traits().setTransparency(static_cast<std::uint8_t>(alpha));
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setSelectionMarker(YiCadGiContextHandle ctx, int32_t marker) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        c.wd().traits().setSelectionMarker(marker);
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setFillPattern(YiCadGiContextHandle ctx, const YiCadHatchPatternLineV4* lines,
                                             uint32_t count) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (count == 0)
        {
            c.wd().traits().setFill(nullptr);
            return true;
        }
        if (lines == nullptr || count > 10000)
        {
            return false;
        }
        GiHatchPattern pattern;
        pattern.lines.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            const YiCadHatchPatternLineV4& in = lines[i];
            if (!finitePoint(in.basePoint) || !finitePoint(in.direction) || !finitePoint(in.offset) ||
                !validDoubleArray(in.dashes))
            {
                return false;
            }
            const DmVector direction = toDmVector(in.direction);
            const DmVector offset = toDmVector(in.offset);
            const double length = direction.magnitude();
            // 线距为零（位移平行于方向）的一族线切不出来
            if (!(length > 0.0) || std::abs(direction.x * offset.y - direction.y * offset.x) <= 1.0e-12 * length)
            {
                return false;
            }
            GiHatchPatternLine line;
            line.base = toDmVector(in.basePoint);
            line.direction = direction / length;
            line.offset = offset;
            line.dashes.assign(in.dashes.data, in.dashes.data + in.dashes.count);
            pattern.lines.push_back(std::move(line));
        }
        c.wd().traits().setFill(&pattern);
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL polyline(YiCadGiContextHandle ctx, const YiCadPoint2dArrayView* points,
                                       const YiCadDoubleArrayView* bulges, const YiCadDoubleArrayView* widths,
                                       uint32_t flags) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        constexpr uint32_t knownFlags = YICAD_GI_POLYLINE_CLOSED | YICAD_GI_POLYLINE_CONTINUOUS_LINETYPE;
        if (points == nullptr || points->count < 2 || !validPointArray(*points) || (flags & ~knownFlags) != 0)
        {
            return false;
        }
        const bool closed = (flags & YICAD_GI_POLYLINE_CLOSED) != 0;
        const std::size_t segments = closed ? points->count : points->count - 1;
        const uint32_t bulgeCount = bulges == nullptr ? 0 : bulges->count;
        const uint32_t widthCount = widths == nullptr ? 0 : widths->count;
        if ((bulges != nullptr && !validDoubleArray(*bulges)) || (widths != nullptr && !validDoubleArray(*widths)) ||
            (bulgeCount != 0 && bulgeCount != segments) || (widthCount != 0 && widthCount != 2 * segments))
        {
            return false;
        }
        if (!c.consume(points->count))
        {
            return true;
        }
        const std::vector<DmVector> vertices = toPoints(*points);
        std::vector<double> bulgeValues;
        if (bulgeCount != 0)
        {
            bulgeValues.assign(bulges->data, bulges->data + bulgeCount);
        }
        std::vector<GiSegmentWidth> widthValues;
        for (uint32_t i = 0; i + 1 < widthCount; i += 2)
        {
            if (widths->data[i] < 0.0 || widths->data[i + 1] < 0.0)
            {
                return false;
            }
            widthValues.push_back(GiSegmentWidth{widths->data[i], widths->data[i + 1]});
        }
        GiPolylineFlags giFlags = GiPolylineFlags::None;
        if (closed)
        {
            giFlags = giFlags | GiPolylineFlags::Closed;
        }
        if ((flags & YICAD_GI_POLYLINE_CONTINUOUS_LINETYPE) != 0)
        {
            giFlags = giFlags | GiPolylineFlags::ContinuousLinetype;
        }
        c.wd().geometry().polyline(vertices, bulgeValues, widthValues, giFlags);
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL circle(YiCadGiContextHandle ctx, YiCadPoint2d center, double radius) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!finitePoint(center) || !finiteValue(radius) || radius <= 0.0)
        {
            return false;
        }
        if (c.consume(1))
        {
            c.wd().geometry().circle(toDmVector(center), radius);
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL arc(YiCadGiContextHandle ctx, YiCadPoint2d center, double radius, double startAngle,
                                  double sweepAngle) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!finitePoint(center) || !finiteValue(radius) || radius <= 0.0 || !finiteValue(startAngle) ||
            !finiteValue(sweepAngle) || sweepAngle == 0.0)
        {
            return false;
        }
        if (c.consume(1))
        {
            c.wd().geometry().arc(toDmVector(center), radius, startAngle, sweepAngle);
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL ellipseArc(YiCadGiContextHandle ctx, YiCadPoint2d center, YiCadVector2d majorAxis,
                                         double ratio, double startParameter, double endParameter) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!finitePoint(center) || !finitePoint(majorAxis) || !(toDmVector(majorAxis).magnitude() > 0.0) ||
            !finiteValue(ratio) || ratio <= 0.0 || ratio > 1.0 || !finiteValue(startParameter) ||
            !finiteValue(endParameter))
        {
            return false;
        }
        if (c.consume(1))
        {
            c.wd().geometry().ellipseArc(toDmVector(center), toDmVector(majorAxis), ratio, startParameter,
                                         endParameter);
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL nurbs(YiCadGiContextHandle ctx, uint32_t degree,
                                    const YiCadPoint2dArrayView* controlPoints, const YiCadDoubleArrayView* knots,
                                    uint32_t closed) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (controlPoints == nullptr || knots == nullptr || degree < 1 || degree > 25 || closed > 1 ||
            !validPointArray(*controlPoints) || !validDoubleArray(*knots) ||
            controlPoints->count < degree + 1 || knots->count != controlPoints->count + degree + 1)
        {
            return false;
        }
        for (uint32_t i = 1; i < knots->count; ++i)
        {
            if (knots->data[i] < knots->data[i - 1])
            {
                return false;
            }
        }
        GiNurbs curve;
        curve.degree = static_cast<int>(degree);
        curve.knots.assign(knots->data, knots->data + knots->count);
        curve.controlPoints = toPoints(*controlPoints);
        curve.closed = closed != 0;
        if (!curve.isValid())
        {
            return false;
        }
        if (c.consume(controlPoints->count))
        {
            c.wd().geometry().nurbs(curve);
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL fill(YiCadGiContextHandle ctx, const YiCadGiLoopV4* loops, uint32_t loopCount,
                                   uint32_t fillRule) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (loops == nullptr || loopCount == 0 || loopCount > 100000 ||
            (fillRule != YICAD_GI_FILL_EVEN_ODD && fillRule != YICAD_GI_FILL_NON_ZERO))
        {
            return false;
        }
        std::vector<GiLoop> values;
        values.reserve(loopCount);
        std::size_t total = 0;
        for (uint32_t i = 0; i < loopCount; ++i)
        {
            const YiCadGiLoopV4& loop = loops[i];
            if (loop.points.count < 3 || !validPointArray(loop.points) || !validDoubleArray(loop.bulges) ||
                (loop.bulges.count != 0 && loop.bulges.count != loop.points.count))
            {
                return false;
            }
            GiLoop value;
            value.points = toPoints(loop.points);
            value.bulges.assign(loop.bulges.data, loop.bulges.data + loop.bulges.count);
            total += loop.points.count;
            values.push_back(std::move(value));
        }
        if (c.consume(total))
        {
            c.wd().geometry().fill(values, fillRule == YICAD_GI_FILL_NON_ZERO ? GiFillRule::NonZero
                                                                              : GiFillRule::EvenOdd);
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL triangles(YiCadGiContextHandle ctx, const YiCadPoint2dArrayView* vertices,
                                        const uint32_t* indices, uint32_t indexCount) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (vertices == nullptr || !validPointArray(*vertices) || indexCount == 0 || indexCount % 3 != 0 ||
            indices == nullptr || indexCount > 3000000)
        {
            return false;
        }
        for (uint32_t i = 0; i < indexCount; ++i)
        {
            if (indices[i] >= vertices->count)
            {
                return false;
            }
        }
        if (c.consume(vertices->count + indexCount / 3))
        {
            const std::vector<DmVector> points = toPoints(*vertices);
            c.wd().geometry().triangles(points, std::span<const std::uint32_t>(indices, indexCount));
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL text(YiCadGiContextHandle ctx, YiCadStringView value, YiCadReadResourceHandle textStyle,
                                   const YiCadTextPlacementV4* placement) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        QString string;
        if (placement == nullptr || placement->structSize < YICAD_TEXT_PLACEMENT_V4_MIN_SIZE ||
            !copyStringView(value, string) || !finitePoint(placement->insertionPoint) ||
            !finitePoint(placement->alignmentPoint) || !finiteValue(placement->height) || placement->height <= 0.0 ||
            !finiteValue(placement->rotation) || !finiteValue(placement->widthFactor) ||
            placement->widthFactor <= 0.0 || !finiteValue(placement->obliqueAngle) ||
            placement->horizontalAlignment < YICAD_TEXT_ALIGN_LEFT ||
            placement->horizontalAlignment > YICAD_TEXT_ALIGN_FIT ||
            placement->verticalAlignment < YICAD_TEXT_ALIGN_BASELINE ||
            placement->verticalAlignment > YICAD_TEXT_ALIGN_TOP)
        {
            return false;
        }
        DmDocument* document = c.document();
        DmTextStyle* style = nullptr;
        if (textStyle != nullptr)
        {
            if (!ownsResource(document, YICAD_READ_TEXT_STYLE, textStyle))
            {
                return false;
            }
            style = const_cast<DmTextStyle*>(static_cast<const DmTextStyle*>(textStyle));
        }
        else if (document != nullptr)
        {
            DmTextStyleTable* table = document->getTextStyleTable();
            style = table->find(QStringLiteral("Standard"));
            if (style == nullptr)
            {
                style = table->getActive();
            }
        }
        // 没有文档时没有文字样式，排不了版
        if (style == nullptr || string.isEmpty())
        {
            return style != nullptr;
        }
        if (!c.consume(static_cast<std::size_t>(string.size())))
        {
            return true;
        }
        // 由单行文字排版：它的 worldDraw 只输出字形，属性取当前的
        TextData data(toDmVector(placement->insertionPoint), placement->height,
                      static_cast<ETextVertMode>(placement->verticalAlignment),
                      static_cast<ETextHorzMode>(placement->horizontalAlignment), string, style, placement->rotation,
                      EUpdateMode::NoUpdate);
        data.setAlignment(toDmVector(placement->alignmentPoint));
        data.setWidthFactor(placement->widthFactor);
        data.setSlashAngle(placement->obliqueAngle);
        DmText layout(nullptr, data);
        layout.update();
        layout.worldDraw(c.wd());
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL image(YiCadGiContextHandle ctx, YiCadStringView path, YiCadPoint2d origin,
                                    YiCadVector2d u, YiCadVector2d v, uint32_t widthPixels,
                                    uint32_t heightPixels) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        QString file;
        if (!copyStringView(path, file) || file.isEmpty() || !finitePoint(origin) || !finitePoint(u) ||
            !finitePoint(v) || widthPixels == 0 || heightPixels == 0 || widthPixels > 1000000 ||
            heightPixels > 1000000)
        {
            return false;
        }
        if (c.consume(1))
        {
            GiImage value;
            value.origin = toDmVector(origin);
            value.u = toDmVector(u);
            value.v = toDmVector(v);
            value.width = static_cast<int>(widthPixels);
            value.height = static_cast<int>(heightPixels);
            value.path = file;
            c.wd().geometry().image(value);
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL point(YiCadGiContextHandle ctx, YiCadPoint2d position) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!finitePoint(position))
        {
            return false;
        }
        if (c.consume(1))
        {
            c.wd().geometry().point(toDmVector(position));
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL ray(YiCadGiContextHandle ctx, YiCadPoint2d base, YiCadVector2d direction) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!finitePoint(base) || !finitePoint(direction) || !(toDmVector(direction).magnitude() > 0.0))
        {
            return false;
        }
        if (c.consume(1))
        {
            c.wd().geometry().ray(toDmVector(base), toDmVector(direction));
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL xline(YiCadGiContextHandle ctx, YiCadPoint2d base, YiCadVector2d direction) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (!finitePoint(base) || !finitePoint(direction) || !(toDmVector(direction).magnitude() > 0.0))
        {
            return false;
        }
        if (c.consume(1))
        {
            c.wd().geometry().xline(toDmVector(base), toDmVector(direction));
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL drawBlock(YiCadGiContextHandle ctx, YiCadReadResourceHandle block,
                                        const YiCadMatrix2d* transform) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (transform == nullptr || !finiteMatrix(*transform) || !ownsResource(c.document(), YICAD_READ_BLOCK, block))
        {
            return false;
        }
        if (c.consume(1))
        {
            // 块里的随块属性取当前属性（插件设过的，没设过的是实体自己的）
            GiByBlockTraits byBlock;
            byBlock.color = c.color;
            byBlock.lineWeight = c.lineWeight;
            byBlock.lineType = c.lineType;
            c.wd().geometry().drawShared(*static_cast<const DmBlock*>(block), toGiTransform(*transform), byBlock);
        }
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL pushTransform(YiCadGiContextHandle ctx, const YiCadMatrix2d* transform) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (transform == nullptr || !finiteMatrix(*transform) || c.transformDepth >= 1000)
        {
            return false;
        }
        c.wd().geometry().pushTransform(toGiTransform(*transform));
        ++c.transformDepth;
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL popTransform(YiCadGiContextHandle ctx) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        // 只能弹出插件自己压入的
        if (c.transformDepth <= 0)
        {
            return false;
        }
        c.wd().geometry().popTransform();
        --c.transformDepth;
        return true;
    });
}

YiCadResult YICAD_PLUGIN_CALL setScreenSpace(YiCadGiContextHandle ctx, const YiCadPoint2d* anchor) noexcept
{
    return guarded(ctx, [&](PluginGiContext& c) {
        if (anchor != nullptr && !finitePoint(*anchor))
        {
            return false;
        }
        if (anchor == nullptr)
        {
            c.wd().traits().setScreenSpace(nullptr);
            c.screenSpace = false;
            return true;
        }
        const DmVector value = toDmVector(*anchor);
        c.wd().traits().setScreenSpace(&value);
        c.screenSpace = true;
        return true;
    });
}

YiCadReadResourceHandle YICAD_PLUGIN_CALL findResource(YiCadGiContextHandle ctx, YiCadReadResourceKind kind,
                                                       YiCadStringView name) noexcept
{
    try
    {
        PluginGiContext* c = contextOf(ctx);
        QString value;
        if (c == nullptr || c->document() == nullptr || !copyStringView(name, value) || value.isEmpty())
        {
            return nullptr;
        }
        DmDocument* document = c->document();
        switch (kind)
        {
        case YICAD_READ_LINE_TYPE:
        {
            DmLineTypeTable* table = document->getLineTypeTable();
            if (DmLineType* lineType = table->find(value))
            {
                return lineType;
            }
            // 随层、随块是两条保留记录：按名字认（名字来自插件的输入，同读文件时的做法）
            if (value.compare(QStringLiteral("ByLayer"), Qt::CaseInsensitive) == 0)
            {
                return table->getLineTypeByLayer();
            }
            if (value.compare(QStringLiteral("ByBlock"), Qt::CaseInsensitive) == 0)
            {
                return table->getLineTypeByBlock();
            }
            return nullptr;
        }
        case YICAD_READ_LAYER:
            return document->getLayerTable()->find(value);
        case YICAD_READ_TEXT_STYLE:
            return document->getTextStyleTable()->find(value);
        case YICAD_READ_BLOCK:
            return document->getBlockTable()->find(value);
        default:
            return nullptr;
        }
    }
    catch (...)
    {
        return nullptr;
    }
}

const YiCadGiApiV4 kGiApi{
    static_cast<uint32_t>(sizeof(YiCadGiApiV4)),
    YICAD_PLUGIN_ABI_V4,
    &setColor,
    &setLayer,
    &setLineType,
    &setLineTypeScale,
    &setLineWeight,
    &setTransparency,
    &setSelectionMarker,
    &setFillPattern,
    &polyline,
    &circle,
    &arc,
    &ellipseArc,
    &nurbs,
    &fill,
    &triangles,
    &text,
    &image,
    &point,
    &ray,
    &xline,
    &drawBlock,
    &pushTransform,
    &popTransform,
    &setScreenSpace,
    &findResource,
};

} // namespace

PluginGiContext::PluginGiContext(IGiWorldDraw& wd, const DmEntity* entity, const char* className)
    : m_wd(wd)
    , m_className(className)
{
    color = DmColor(DM::FlagByBlock);
    if (entity != nullptr)
    {
        m_document = entity->getDocument();
        const DmPen pen = entity->getPen(false);
        if (!pen.getFlag(DM::FlagInvalid))
        {
            color = pen.getColor();
            lineWeight = pen.getWidth();
            lineType = pen.getLineType();
        }
    }
}

PluginGiContext::~PluginGiContext()
{
    // 插件没弹出的变换替它弹出，交给接收方的输出保持成对
    try
    {
        while (transformDepth > 0)
        {
            m_wd.geometry().popTransform();
            --transformDepth;
        }
        if (screenSpace)
        {
            m_wd.traits().setScreenSpace(nullptr);
        }
    }
    catch (...)
    {
    }
}

bool PluginGiContext::consume(std::size_t count)
{
    if (m_truncated)
    {
        return false;
    }
    if (count > kMaxElements - m_used)
    {
        m_truncated = true;
        YICAD_LOG(yicad::log::plugin(), yicad::LogLevel::Warning)
            << "插件实体 " << (m_className ? m_className : "") << " 的一次 worldDraw 输出超过 " << kMaxElements
            << " 个点与图元，之后的图元已丢弃";
        return false;
    }
    m_used += count;
    return true;
}

const YiCadGiApiV4& pluginGiApi() noexcept
{
    return kGiApi;
}
