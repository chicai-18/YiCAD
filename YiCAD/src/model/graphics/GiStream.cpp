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

/// @file GiStream.cpp
/// @brief GiStream、GiStreamRecorder、GiStreamDrawable 实现

#include "GiStream.h"

#include <bit>
#include <cstring>
#include <type_traits>

#include "GiNurbs.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"
#include "Stream.h"

static_assert(std::endian::native == std::endian::little, "GiStream 按小端字节序存放");

namespace
{

/// @brief 记录的操作码
enum class GiOp : std::uint8_t
{
    AttributesEnd = 1,  ///< 属性记录结束，之后是图元
    SetColor,
    SetLayer,
    SetLineType,
    SetLineTypeScale,
    SetLinePattern,
    SetLineWeight,
    SetTransparency,
    SetSelectionMarker,
    SetScreenSpace,
    Polyline,
    Circle,
    Arc,
    EllipseArc,
    Nurbs,
    Fill,
    Triangles,
    GlyphRun,
    Image,
    Point,
    Ray,
    Xline,
    Draw,               ///< 嵌套绘制：后跟段长（uint64），段内是一个完整的"属性、AttributesEnd、图元"
    DrawShared,
    PushTransform,
    PopTransform,
    SetFill,            ///< 后跟有无图案（uint8），有时是线族数与每族的基点、方向、位移、划线
};

constexpr std::uint32_t kNullReference = 0xFFFFFFFFu;

}  // namespace

// ---------------------------------------------------------------------------
// 读写记录的底层工具
// ---------------------------------------------------------------------------

namespace
{

/// @brief 往字节数组里追加数值
class ByteWriter
{
public:
    explicit ByteWriter(std::vector<std::byte>& bytes)
        : m_bytes(bytes)
    {
    }

    template<typename T>
    void put(T value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        const std::size_t pos = m_bytes.size();
        m_bytes.resize(pos + sizeof(T));
        std::memcpy(m_bytes.data() + pos, &value, sizeof(T));
    }

    void op(GiOp code) { put(static_cast<std::uint8_t>(code)); }

    void point(const DmVector& p)
    {
        put(p.x);
        put(p.y);
    }

    void transform(const GiTransform& xf)
    {
        put(xf.a());
        put(xf.b());
        put(xf.c());
        put(xf.d());
        put(xf.tx());
        put(xf.ty());
    }

    void color(const DmColor& c)
    {
        put(static_cast<std::uint32_t>(c.getFlags()));
        put(static_cast<std::int32_t>(c.red()));
        put(static_cast<std::int32_t>(c.green()));
        put(static_cast<std::int32_t>(c.blue()));
        put(static_cast<std::int32_t>(c.alpha()));
    }

    void string(const QString& s)
    {
        const QByteArray utf8 = s.toUtf8();
        put(static_cast<std::uint32_t>(utf8.size()));
        const std::size_t pos = m_bytes.size();
        m_bytes.resize(pos + static_cast<std::size_t>(utf8.size()));
        if (!utf8.isEmpty())
        {
            std::memcpy(m_bytes.data() + pos, utf8.constData(), static_cast<std::size_t>(utf8.size()));
        }
    }

    void points(std::span<const DmVector> pts)
    {
        put(static_cast<std::uint32_t>(pts.size()));
        for (const DmVector& p : pts)
        {
            point(p);
        }
    }

    void doubles(std::span<const double> values)
    {
        put(static_cast<std::uint32_t>(values.size()));
        for (double v : values)
        {
            put(v);
        }
    }

    std::size_t size() const { return m_bytes.size(); }

    /// @brief 覆写已写出的一个数值（嵌套段的段长）
    template<typename T>
    void patch(std::size_t pos, T value)
    {
        std::memcpy(m_bytes.data() + pos, &value, sizeof(T));
    }

private:
    std::vector<std::byte>& m_bytes;
};

/// @brief 从字节区间里依次读数值；越界时停在末尾并置 failed
class ByteReader
{
public:
    ByteReader(const std::byte* begin, const std::byte* end)
        : m_cur(begin)
        , m_end(end)
    {
    }

    template<typename T>
    T get()
    {
        T value{};
        if (static_cast<std::size_t>(m_end - m_cur) < sizeof(T))
        {
            m_failed = true;
            m_cur = m_end;
            return value;
        }
        std::memcpy(&value, m_cur, sizeof(T));
        m_cur += sizeof(T);
        return value;
    }

    DmVector point()
    {
        const double x = get<double>();
        const double y = get<double>();
        return DmVector(x, y);
    }

    GiTransform transform()
    {
        const double a = get<double>();
        const double b = get<double>();
        const double c = get<double>();
        const double d = get<double>();
        const double tx = get<double>();
        const double ty = get<double>();
        return GiTransform(a, b, c, d, tx, ty);
    }

    DmColor color()
    {
        const auto flags = get<std::uint32_t>();
        const auto r = get<std::int32_t>();
        const auto g = get<std::int32_t>();
        const auto b = get<std::int32_t>();
        const auto a = get<std::int32_t>();
        DmColor c(r, g, b, a);
        c.setFlags(flags);
        return c;
    }

    QString string()
    {
        const auto size = get<std::uint32_t>();
        if (static_cast<std::size_t>(m_end - m_cur) < size)
        {
            m_failed = true;
            m_cur = m_end;
            return QString();
        }
        const QString s = QString::fromUtf8(reinterpret_cast<const char*>(m_cur), static_cast<qsizetype>(size));
        m_cur += size;
        return s;
    }

    std::vector<DmVector> points()
    {
        const auto count = get<std::uint32_t>();
        std::vector<DmVector> pts;
        pts.reserve(std::min<std::size_t>(count, remaining() / (2 * sizeof(double))));
        for (std::uint32_t i = 0; i < count && !m_failed; ++i)
        {
            pts.emplace_back(point());
        }
        return pts;
    }

    std::vector<double> doubles()
    {
        const auto count = get<std::uint32_t>();
        std::vector<double> values;
        values.reserve(std::min<std::size_t>(count, remaining() / sizeof(double)));
        for (std::uint32_t i = 0; i < count && !m_failed; ++i)
        {
            values.emplace_back(get<double>());
        }
        return values;
    }

    const std::byte* position() const { return m_cur; }
    void skip(std::size_t bytes) { m_cur += std::min(bytes, remaining()); }
    bool atEnd() const { return m_cur >= m_end; }
    bool failed() const { return m_failed; }
    std::size_t remaining() const { return static_cast<std::size_t>(m_end - m_cur); }

private:
    const std::byte* m_cur;
    const std::byte* m_end;
    bool m_failed = false;
};

// ---------------------------------------------------------------------------
// 记录器：实现 GI 的三个接口，把每次调用写成一条记录
// ---------------------------------------------------------------------------

class StreamWriter final : public IGiWorldDraw, public IGiGeometry, public IGiSubEntityTraits
{
public:
    StreamWriter(std::vector<std::byte>& bytes, std::vector<std::pair<GiReferenceKind, const void*>>& refs,
                 GiRegenType regenType, double deviation)
        : m_out(bytes)
        , m_refs(refs)
        , m_regenType(regenType)
        , m_deviation(deviation)
    {
    }

    /// @brief 记录一个可绘制对象的属性与图元（不含外层的段头）
    void recordDrawable(const IGiDrawable& drawable)
    {
        drawable.setAttributes(*this);
        m_out.op(GiOp::AttributesEnd);
        drawable.worldDraw(*this);
    }

    // IGiWorldDraw
    IGiGeometry& geometry() override { return *this; }
    IGiSubEntityTraits& traits() override { return *this; }
    GiRegenType regenType() const override { return m_regenType; }
    double deviation() const override { return m_deviation; }
    bool isDragging() const override { return false; }

    // IGiSubEntityTraits
    void setColor(const DmColor& color) override
    {
        m_out.op(GiOp::SetColor);
        m_out.color(color);
    }

    void setLayer(const DmLayer* layer) override
    {
        m_out.op(GiOp::SetLayer);
        m_out.put(reference(GiReferenceKind::Layer, layer));
    }

    void setLineType(const DmLineType* lineType) override
    {
        m_out.op(GiOp::SetLineType);
        m_out.put(reference(GiReferenceKind::LineType, lineType));
    }

    void setLineTypeScale(double scale) override
    {
        m_out.op(GiOp::SetLineTypeScale);
        m_out.put(scale);
    }

    void setLinePattern(const GiLinePattern& pattern) override
    {
        m_out.op(GiOp::SetLinePattern);
        m_out.doubles(pattern.dashes);
        m_out.put(pattern.phase);
    }

    void setFill(const GiHatchPattern* pattern) override
    {
        m_out.op(GiOp::SetFill);
        m_out.put(static_cast<std::uint8_t>(pattern ? 1 : 0));
        if (pattern)
        {
            m_out.put(static_cast<std::uint32_t>(pattern->lines.size()));
            for (const GiHatchPatternLine& line : pattern->lines)
            {
                m_out.point(line.base);
                m_out.point(line.direction);
                m_out.point(line.offset);
                m_out.doubles(line.dashes);
            }
        }
    }

    void setLineWeight(DM::LineWidth weight) override
    {
        m_out.op(GiOp::SetLineWeight);
        m_out.put(static_cast<std::int32_t>(weight));
    }

    void setTransparency(std::uint8_t alpha) override
    {
        m_out.op(GiOp::SetTransparency);
        m_out.put(alpha);
    }

    void setSelectionMarker(std::int32_t marker) override
    {
        m_out.op(GiOp::SetSelectionMarker);
        m_out.put(marker);
    }

    void setScreenSpace(const DmVector* anchor) override
    {
        m_out.op(GiOp::SetScreenSpace);
        m_out.put(static_cast<std::uint8_t>(anchor ? 1 : 0));
        if (anchor)
        {
            m_out.point(*anchor);
        }
    }

    // IGiGeometry
    void polyline(std::span<const DmVector> points, std::span<const double> bulges,
                  std::span<const GiSegmentWidth> widths, GiPolylineFlags flags) override
    {
        m_out.op(GiOp::Polyline);
        m_out.put(static_cast<std::uint32_t>(flags));
        m_out.points(points);
        m_out.doubles(bulges);
        m_out.put(static_cast<std::uint32_t>(widths.size()));
        for (const GiSegmentWidth& w : widths)
        {
            m_out.put(w.start);
            m_out.put(w.end);
        }
    }

    void circle(const DmVector& center, double radius) override
    {
        m_out.op(GiOp::Circle);
        m_out.point(center);
        m_out.put(radius);
    }

    void arc(const DmVector& center, double radius, double startAngle, double sweepAngle) override
    {
        m_out.op(GiOp::Arc);
        m_out.point(center);
        m_out.put(radius);
        m_out.put(startAngle);
        m_out.put(sweepAngle);
    }

    void ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio,
                    double startParam, double endParam) override
    {
        m_out.op(GiOp::EllipseArc);
        m_out.point(center);
        m_out.point(majorAxis);
        m_out.put(ratio);
        m_out.put(startParam);
        m_out.put(endParam);
    }

    void nurbs(const GiNurbs& curve) override
    {
        m_out.op(GiOp::Nurbs);
        m_out.put(static_cast<std::int32_t>(curve.degree));
        m_out.put(static_cast<std::uint8_t>(curve.closed ? 1 : 0));
        m_out.doubles(curve.knots);
        m_out.points(curve.controlPoints);
        m_out.doubles(curve.weights);
    }

    void fill(std::span<const GiLoop> loops, GiFillRule rule) override
    {
        m_out.op(GiOp::Fill);
        m_out.put(static_cast<std::uint8_t>(rule));
        m_out.put(static_cast<std::uint32_t>(loops.size()));
        for (const GiLoop& loop : loops)
        {
            m_out.points(loop.points);
            m_out.doubles(loop.bulges);
        }
    }

    void triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices) override
    {
        m_out.op(GiOp::Triangles);
        m_out.points(vertices);
        m_out.put(static_cast<std::uint32_t>(indices.size()));
        for (std::uint32_t index : indices)
        {
            m_out.put(index);
        }
    }

    void glyphRun(const GiGlyphRun& run) override
    {
        m_out.op(GiOp::GlyphRun);
        m_out.put(reference(GiReferenceKind::Font, run.font));
        m_out.put(static_cast<std::uint32_t>(run.glyphs.size()));
        for (const GiGlyph& glyph : run.glyphs)
        {
            m_out.put(static_cast<std::uint32_t>(glyph.code));
            m_out.transform(glyph.transform);
        }
    }

    void image(const GiImage& image) override
    {
        m_out.op(GiOp::Image);
        m_out.point(image.origin);
        m_out.point(image.u);
        m_out.point(image.v);
        m_out.put(static_cast<std::int32_t>(image.width));
        m_out.put(static_cast<std::int32_t>(image.height));
        m_out.string(image.path);
        m_out.put(reference(GiReferenceKind::Image, image.pixels));
    }

    void point(const DmVector& position) override
    {
        m_out.op(GiOp::Point);
        m_out.point(position);
    }

    void ray(const DmVector& base, const DmVector& direction) override
    {
        m_out.op(GiOp::Ray);
        m_out.point(base);
        m_out.point(direction);
    }

    void xline(const DmVector& base, const DmVector& direction) override
    {
        m_out.op(GiOp::Xline);
        m_out.point(base);
        m_out.point(direction);
    }

    void draw(const IGiDrawable& drawable) override
    {
        m_out.op(GiOp::Draw);
        const std::size_t lengthPos = m_out.size();
        m_out.put(std::uint64_t{0});
        const std::size_t begin = m_out.size();
        recordDrawable(drawable);
        m_out.patch(lengthPos, static_cast<std::uint64_t>(m_out.size() - begin));
    }

    void drawShared(const IGiDrawable& drawable, const GiTransform& transform,
                    const GiByBlockTraits& byBlock) override
    {
        m_out.op(GiOp::DrawShared);
        m_out.put(reference(GiReferenceKind::Drawable, &drawable));
        m_out.transform(transform);
        m_out.color(byBlock.color);
        m_out.put(static_cast<std::int32_t>(byBlock.lineWeight));
        m_out.put(reference(GiReferenceKind::LineType, byBlock.lineType));
    }

    void pushTransform(const GiTransform& transform) override
    {
        m_out.op(GiOp::PushTransform);
        m_out.transform(transform);
    }

    void popTransform() override
    {
        m_out.op(GiOp::PopTransform);
    }

private:
    /// @brief 引用在引用表里的下标，没有就追加；空引用是 kNullReference
    std::uint32_t reference(GiReferenceKind kind, const void* object)
    {
        if (!object)
        {
            return kNullReference;
        }
        for (std::size_t i = 0; i < m_refs.size(); ++i)
        {
            if (m_refs[i].first == kind && m_refs[i].second == object)
            {
                return static_cast<std::uint32_t>(i);
            }
        }
        m_refs.emplace_back(kind, object);
        return static_cast<std::uint32_t>(m_refs.size() - 1);
    }

    ByteWriter m_out;
    std::vector<std::pair<GiReferenceKind, const void*>>& m_refs;
    GiRegenType m_regenType;
    double m_deviation;
};

}  // namespace

// ---------------------------------------------------------------------------
// 重放：把一段记录交给接收方
// ---------------------------------------------------------------------------

/// @brief 按引用表与字节区间重放记录
class GiStreamReader
{
public:
    explicit GiStreamReader(const GiStream& stream)
        : m_stream(stream)
    {
    }

    const std::byte* begin() const { return m_stream.m_bytes.data(); }
    const std::byte* end() const { return m_stream.m_bytes.data() + m_stream.m_bytes.size(); }

    /// @brief 重放 [first, last) 开头的属性记录到 traits，返回 AttributesEnd 之后的位置
    const std::byte* replayAttributes(const std::byte* first, const std::byte* last, IGiSubEntityTraits& traits) const
    {
        ByteReader in(first, last);
        while (!in.atEnd() && !in.failed())
        {
            const auto code = static_cast<GiOp>(in.get<std::uint8_t>());
            if (code == GiOp::AttributesEnd)
            {
                break;
            }
            if (!replayTrait(code, in, traits))
            {
                // 属性段里只该有属性记录；遇到别的说明数据坏了，停止
                return last;
            }
        }
        return in.position();
    }

    /// @brief 重放 [first, last) 的图元与属性记录到 wd
    void replayBody(const std::byte* first, const std::byte* last, IGiWorldDraw& wd) const;

private:
    template<typename T>
    const T* object(std::uint32_t index, GiReferenceKind kind) const
    {
        if (index >= m_stream.m_references.size() || m_stream.m_references[index].kind != kind)
        {
            return nullptr;
        }
        return static_cast<const T*>(m_stream.m_references[index].object);
    }

    /// @brief 重放一条属性记录；code 不是属性记录时返回 false
    bool replayTrait(GiOp code, ByteReader& in, IGiSubEntityTraits& traits) const
    {
        switch (code)
        {
        case GiOp::SetColor:
            traits.setColor(in.color());
            return true;
        case GiOp::SetLayer:
            traits.setLayer(object<DmLayer>(in.get<std::uint32_t>(), GiReferenceKind::Layer));
            return true;
        case GiOp::SetLineType:
            traits.setLineType(object<DmLineType>(in.get<std::uint32_t>(), GiReferenceKind::LineType));
            return true;
        case GiOp::SetLineTypeScale:
            traits.setLineTypeScale(in.get<double>());
            return true;
        case GiOp::SetLinePattern:
        {
            GiLinePattern pattern;
            pattern.dashes = in.doubles();
            pattern.phase = in.get<double>();
            traits.setLinePattern(pattern);
            return true;
        }
        case GiOp::SetFill:
        {
            if (in.get<std::uint8_t>() == 0)
            {
                traits.setFill(nullptr);
                return true;
            }
            GiHatchPattern pattern;
            const auto count = in.get<std::uint32_t>();
            for (std::uint32_t i = 0; i < count && !in.failed(); ++i)
            {
                GiHatchPatternLine line;
                line.base = in.point();
                line.direction = in.point();
                line.offset = in.point();
                line.dashes = in.doubles();
                pattern.lines.emplace_back(std::move(line));
            }
            if (!in.failed())
            {
                traits.setFill(&pattern);
            }
            return true;
        }
        case GiOp::SetLineWeight:
            traits.setLineWeight(static_cast<DM::LineWidth>(in.get<std::int32_t>()));
            return true;
        case GiOp::SetTransparency:
            traits.setTransparency(in.get<std::uint8_t>());
            return true;
        case GiOp::SetSelectionMarker:
            traits.setSelectionMarker(in.get<std::int32_t>());
            return true;
        case GiOp::SetScreenSpace:
        {
            if (in.get<std::uint8_t>() != 0)
            {
                const DmVector anchor = in.point();
                traits.setScreenSpace(&anchor);
            }
            else
            {
                traits.setScreenSpace(nullptr);
            }
            return true;
        }
        default:
            return false;
        }
    }

    const GiStream& m_stream;
};

namespace
{

/// @brief 记录里的一段（嵌套绘制，或整个流），当作可绘制对象交给接收方
class RangeDrawable final : public IGiDrawable
{
public:
    RangeDrawable(const GiStreamReader& reader, const std::byte* first, const std::byte* last)
        : m_reader(reader)
        , m_first(first)
        , m_last(last)
    {
    }

    void setAttributes(IGiSubEntityTraits& traits) const override
    {
        m_reader.replayAttributes(m_first, m_last, traits);
    }

    void worldDraw(IGiWorldDraw& wd) const override
    {
        // 跳过属性记录：交给一个什么也不做的 traits
        class NullTraits final : public IGiSubEntityTraits
        {
        public:
            void setColor(const DmColor&) override {}
            void setLayer(const DmLayer*) override {}
            void setLineType(const DmLineType*) override {}
            void setLineTypeScale(double) override {}
            void setLinePattern(const GiLinePattern&) override {}
            void setFill(const GiHatchPattern*) override {}
            void setLineWeight(DM::LineWidth) override {}
            void setTransparency(std::uint8_t) override {}
            void setSelectionMarker(std::int32_t) override {}
            void setScreenSpace(const DmVector*) override {}
        };
        NullTraits nullTraits;
        const std::byte* body = m_reader.replayAttributes(m_first, m_last, nullTraits);
        m_reader.replayBody(body, m_last, wd);
    }

private:
    const GiStreamReader& m_reader;
    const std::byte* m_first;
    const std::byte* m_last;
};

}  // namespace

void GiStreamReader::replayBody(const std::byte* first, const std::byte* last, IGiWorldDraw& wd) const
{
    ByteReader in(first, last);
    IGiGeometry& geo = wd.geometry();
    while (!in.atEnd() && !in.failed())
    {
        const auto code = static_cast<GiOp>(in.get<std::uint8_t>());
        if (replayTrait(code, in, wd.traits()))
        {
            continue;
        }
        switch (code)
        {
        case GiOp::Polyline:
        {
            const auto flags = static_cast<GiPolylineFlags>(in.get<std::uint32_t>());
            const std::vector<DmVector> pts = in.points();
            const std::vector<double> bulges = in.doubles();
            const auto widthCount = in.get<std::uint32_t>();
            std::vector<GiSegmentWidth> widths;
            widths.reserve(std::min<std::size_t>(widthCount, in.remaining() / (2 * sizeof(double))));
            for (std::uint32_t i = 0; i < widthCount && !in.failed(); ++i)
            {
                GiSegmentWidth w;
                w.start = in.get<double>();
                w.end = in.get<double>();
                widths.emplace_back(w);
            }
            if (!in.failed())
            {
                geo.polyline(pts, bulges, widths, flags);
            }
            break;
        }
        case GiOp::Circle:
        {
            const DmVector center = in.point();
            const double radius = in.get<double>();
            geo.circle(center, radius);
            break;
        }
        case GiOp::Arc:
        {
            const DmVector center = in.point();
            const double radius = in.get<double>();
            const double start = in.get<double>();
            const double sweep = in.get<double>();
            geo.arc(center, radius, start, sweep);
            break;
        }
        case GiOp::EllipseArc:
        {
            const DmVector center = in.point();
            const DmVector major = in.point();
            const double ratio = in.get<double>();
            const double start = in.get<double>();
            const double end = in.get<double>();
            geo.ellipseArc(center, major, ratio, start, end);
            break;
        }
        case GiOp::Nurbs:
        {
            GiNurbs curve;
            curve.degree = in.get<std::int32_t>();
            curve.closed = in.get<std::uint8_t>() != 0;
            curve.knots = in.doubles();
            curve.controlPoints = in.points();
            curve.weights = in.doubles();
            if (!in.failed())
            {
                geo.nurbs(curve);
            }
            break;
        }
        case GiOp::Fill:
        {
            const auto rule = static_cast<GiFillRule>(in.get<std::uint8_t>());
            const auto loopCount = in.get<std::uint32_t>();
            std::vector<GiLoop> loops;
            for (std::uint32_t i = 0; i < loopCount && !in.failed(); ++i)
            {
                GiLoop loop;
                loop.points = in.points();
                loop.bulges = in.doubles();
                loops.emplace_back(std::move(loop));
            }
            if (!in.failed())
            {
                geo.fill(loops, rule);
            }
            break;
        }
        case GiOp::Triangles:
        {
            const std::vector<DmVector> vertices = in.points();
            const auto indexCount = in.get<std::uint32_t>();
            std::vector<std::uint32_t> indices;
            indices.reserve(std::min<std::size_t>(indexCount, in.remaining() / sizeof(std::uint32_t)));
            for (std::uint32_t i = 0; i < indexCount && !in.failed(); ++i)
            {
                indices.emplace_back(in.get<std::uint32_t>());
            }
            if (!in.failed())
            {
                geo.triangles(vertices, indices);
            }
            break;
        }
        case GiOp::GlyphRun:
        {
            GiGlyphRun run;
            run.font = object<IGiFont>(in.get<std::uint32_t>(), GiReferenceKind::Font);
            const auto count = in.get<std::uint32_t>();
            for (std::uint32_t i = 0; i < count && !in.failed(); ++i)
            {
                GiGlyph glyph;
                glyph.code = static_cast<char32_t>(in.get<std::uint32_t>());
                glyph.transform = in.transform();
                run.glyphs.emplace_back(glyph);
            }
            if (!in.failed())
            {
                geo.glyphRun(run);
            }
            break;
        }
        case GiOp::Image:
        {
            GiImage image;
            image.origin = in.point();
            image.u = in.point();
            image.v = in.point();
            image.width = in.get<std::int32_t>();
            image.height = in.get<std::int32_t>();
            image.path = in.string();
            image.pixels = object<QImage>(in.get<std::uint32_t>(), GiReferenceKind::Image);
            if (!in.failed())
            {
                geo.image(image);
            }
            break;
        }
        case GiOp::Point:
            geo.point(in.point());
            break;
        case GiOp::Ray:
        {
            const DmVector base = in.point();
            const DmVector dir = in.point();
            geo.ray(base, dir);
            break;
        }
        case GiOp::Xline:
        {
            const DmVector base = in.point();
            const DmVector dir = in.point();
            geo.xline(base, dir);
            break;
        }
        case GiOp::Draw:
        {
            const auto length = static_cast<std::size_t>(in.get<std::uint64_t>());
            const std::byte* segment = in.position();
            if (length > in.remaining())
            {
                return;
            }
            in.skip(length);
            geo.draw(RangeDrawable(*this, segment, segment + length));
            break;
        }
        case GiOp::DrawShared:
        {
            const IGiDrawable* drawable = object<IGiDrawable>(in.get<std::uint32_t>(), GiReferenceKind::Drawable);
            const GiTransform xf = in.transform();
            GiByBlockTraits byBlock;
            byBlock.color = in.color();
            byBlock.lineWeight = static_cast<DM::LineWidth>(in.get<std::int32_t>());
            byBlock.lineType = object<DmLineType>(in.get<std::uint32_t>(), GiReferenceKind::LineType);
            // 读回时找不到共享对象（名字换不回来）就跳过
            if (drawable && !in.failed())
            {
                geo.drawShared(*drawable, xf, byBlock);
            }
            break;
        }
        case GiOp::PushTransform:
            geo.pushTransform(in.transform());
            break;
        case GiOp::PopTransform:
            geo.popTransform();
            break;
        default:
            // 不认识的操作码：数据坏了，停止
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// GiStream
// ---------------------------------------------------------------------------

void GiStream::clear()
{
    m_bytes.clear();
    m_references.clear();
}

void GiStream::write(OutputStream& out, const IGiReferenceCodec& codec) const
{
    out << kVersion;
    out << static_cast<std::uint32_t>(m_references.size());
    for (const Reference& ref : m_references)
    {
        std::string name;
        switch (ref.kind)
        {
        case GiReferenceKind::Layer:
            name = codec.layerName(static_cast<const DmLayer*>(ref.object));
            break;
        case GiReferenceKind::LineType:
            name = codec.lineTypeName(static_cast<const DmLineType*>(ref.object));
            break;
        case GiReferenceKind::Drawable:
            name = codec.drawableName(static_cast<const IGiDrawable*>(ref.object));
            break;
        case GiReferenceKind::Font:
            name = codec.fontName(static_cast<const IGiFont*>(ref.object));
            break;
        case GiReferenceKind::Image:
            name = codec.imageName(static_cast<const QImage*>(ref.object));
            break;
        }
        out << static_cast<std::uint8_t>(ref.kind) << name;
    }
    // Stream 的字符串以换行结尾，不能装任意字节，记录逐字节写出
    out << static_cast<std::uint64_t>(m_bytes.size());
    for (std::byte b : m_bytes)
    {
        out << static_cast<std::uint8_t>(b);
    }
}

bool GiStream::read(InputStream& in, const IGiReferenceCodec& codec)
{
    clear();
    std::uint32_t version = 0;
    in >> version;
    if (version != kVersion)
    {
        return false;
    }
    std::uint32_t refCount = 0;
    in >> refCount;
    m_references.reserve(refCount);
    for (std::uint32_t i = 0; i < refCount; ++i)
    {
        std::uint8_t kind = 0;
        std::string name;
        in >> kind >> name;
        Reference ref;
        ref.kind = static_cast<GiReferenceKind>(kind);
        if (!name.empty())
        {
            switch (ref.kind)
            {
            case GiReferenceKind::Layer:
                ref.object = codec.findLayer(name);
                break;
            case GiReferenceKind::LineType:
                ref.object = codec.findLineType(name);
                break;
            case GiReferenceKind::Drawable:
                ref.object = codec.findDrawable(name);
                break;
            case GiReferenceKind::Font:
                ref.object = codec.findFont(name);
                break;
            case GiReferenceKind::Image:
                ref.object = codec.findImage(name);
                break;
            }
        }
        m_references.emplace_back(ref);
    }
    std::uint64_t byteCount = 0;
    in >> byteCount;
    m_bytes.resize(static_cast<std::size_t>(byteCount));
    for (std::byte& b : m_bytes)
    {
        std::uint8_t value = 0;
        in >> value;
        b = static_cast<std::byte>(value);
    }
    return true;
}

void GiStream::remapReferences(const std::function<const void*(GiReferenceKind, const void*)>& map)
{
    for (Reference& ref : m_references)
    {
        ref.object = map(ref.kind, ref.object);
    }
}

// ---------------------------------------------------------------------------
// GiStreamRecorder、GiStreamDrawable
// ---------------------------------------------------------------------------

GiStream GiStreamRecorder::record(const IGiDrawable& drawable, GiRegenType regenType, double deviation)
{
    GiStream stream;
    std::vector<std::pair<GiReferenceKind, const void*>> refs;
    StreamWriter writer(stream.m_bytes, refs, regenType, deviation);
    writer.recordDrawable(drawable);
    stream.m_references.reserve(refs.size());
    for (const auto& [kind, object] : refs)
    {
        stream.m_references.push_back(GiStream::Reference{kind, object});
    }
    return stream;
}

GiStreamDrawable::GiStreamDrawable(const GiStream& stream)
    : m_stream(stream)
{
}

void GiStreamDrawable::setAttributes(IGiSubEntityTraits& traits) const
{
    const GiStreamReader reader(m_stream);
    reader.replayAttributes(reader.begin(), reader.end(), traits);
}

void GiStreamDrawable::worldDraw(IGiWorldDraw& wd) const
{
    const GiStreamReader reader(m_stream);
    RangeDrawable(reader, reader.begin(), reader.end()).worldDraw(wd);
}
