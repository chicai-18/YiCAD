/// @file GiTextDump.cpp
/// @brief GiTextDump 实现

#include "GiTextDump.h"

#include <cmath>
#include <sstream>

#include "DmBlock.h"
#include "DmColor.h"
#include "DmLayer.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "GiNurbs.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"

namespace
{

/// @brief 6 位有效数字，-0 写成 0
std::string num(double v)
{
    if (std::abs(v) < 5.0e-10)
    {
        v = 0.0;
    }
    std::ostringstream out;
    out.precision(6);
    out << v;
    return out.str();
}

std::string deg(double radians)
{
    return num(radians * 180.0 / M_PI);
}

std::string pt(const DmVector& p)
{
    return "(" + num(p.x) + "," + num(p.y) + ")";
}

std::string xf(const GiTransform& t)
{
    return "[" + num(t.a()) + " " + num(t.b()) + " " + num(t.c()) + " " + num(t.d()) + " " + num(t.tx()) + " " +
           num(t.ty()) + "]";
}

std::string color(const DmColor& c)
{
    if (c.isByLayer())
    {
        return "ByLayer";
    }
    if (c.isByBlock())
    {
        return "ByBlock";
    }
    return "rgb(" + std::to_string(c.red()) + "," + std::to_string(c.green()) + "," + std::to_string(c.blue()) + ")";
}

std::string weight(DM::LineWidth w)
{
    switch (w)
    {
    case DM::WidthByLayer:
        return "ByLayer";
    case DM::WidthByBlock:
        return "ByBlock";
    case DM::WidthDefault:
        return "Default";
    default:
        return std::to_string(static_cast<int>(w));
    }
}

std::string lineType(const DmLineType* lt)
{
    if (!lt || DmLineTypeTable::isByBlock(lt))
    {
        return "ByBlock";
    }
    if (DmLineTypeTable::isByLayer(lt))
    {
        return "ByLayer";
    }
    return const_cast<DmLineType*>(lt)->getLineTypeName().toStdString();
}

std::string code(char32_t c)
{
    if (c >= 0x21 && c < 0x7F)
    {
        return std::string(1, static_cast<char>(c));
    }
    std::ostringstream out;
    out << "U+" << std::hex << std::uppercase << static_cast<unsigned>(c);
    return out.str();
}

/// @brief 把每次调用写成一行
class Dumper final : public IGiWorldDraw, public IGiGeometry, public IGiSubEntityTraits
{
public:
    explicit Dumper(bool withAttributes)
        : m_withAttributes(withAttributes)
    {
    }

    void dump(const IGiDrawable& drawable)
    {
        if (m_withAttributes)
        {
            drawable.setAttributes(*this);
            line("---");
        }
        drawable.worldDraw(*this);
    }

    std::string text() const { return m_out.str(); }

    // IGiWorldDraw
    IGiGeometry& geometry() override { return *this; }
    IGiSubEntityTraits& traits() override { return *this; }
    GiRegenType regenType() const override { return GiRegenType::Display; }
    double deviation() const override { return 1.0e-3; }
    bool isDragging() const override { return false; }

    // IGiSubEntityTraits
    void setColor(const DmColor& c) override { line("color " + color(c)); }
    void setLayer(const DmLayer* layer) override
    {
        line("layer " + (layer ? layer->getName().toStdString() : std::string("-")));
    }
    void setLineType(const DmLineType* lt) override { line("linetype " + lineType(lt)); }
    void setLineTypeScale(double scale) override { line("linetypescale " + num(scale)); }
    void setLinePattern(const GiLinePattern& pattern) override
    {
        std::string s = "linepattern";
        for (double d : pattern.dashes)
        {
            s += " " + num(d);
        }
        line(s + " phase=" + num(pattern.phase));
    }
    void setFill(const GiHatchPattern* pattern) override
    {
        if (!pattern)
        {
            line("fillstyle solid");
            return;
        }
        std::string s = "fillstyle pattern";
        for (const GiHatchPatternLine& l : pattern->lines)
        {
            s += " {base=" + pt(l.base) + " dir=" + pt(l.direction) + " offset=" + pt(l.offset);
            for (double d : l.dashes)
            {
                s += " " + num(d);
            }
            s += "}";
        }
        line(s);
    }
    void setLineWeight(DM::LineWidth w) override { line("lineweight " + weight(w)); }
    void setTransparency(std::uint8_t alpha) override { line("transparency " + std::to_string(alpha)); }
    void setSelectionMarker(std::int32_t marker) override { line("marker " + std::to_string(marker)); }
    void setScreenSpace(const DmVector* anchor) override { line("screenspace " + (anchor ? pt(*anchor) : "-")); }

    // IGiGeometry
    void polyline(std::span<const DmVector> points, std::span<const double> bulges,
                  std::span<const GiSegmentWidth> widths, GiPolylineFlags flags) override
    {
        std::string s = "polyline";
        if (hasFlag(flags, GiPolylineFlags::Closed))
        {
            s += " closed";
        }
        if (hasFlag(flags, GiPolylineFlags::ContinuousLinetype))
        {
            s += " continuous";
        }
        for (const DmVector& p : points)
        {
            s += " " + pt(p);
        }
        if (!bulges.empty())
        {
            s += " bulges";
            for (double b : bulges)
            {
                s += " " + num(b);
            }
        }
        if (!widths.empty())
        {
            s += " widths";
            for (const GiSegmentWidth& w : widths)
            {
                s += " " + num(w.start) + ":" + num(w.end);
            }
        }
        line(s);
    }

    void circle(const DmVector& center, double radius) override
    {
        line("circle c=" + pt(center) + " r=" + num(radius));
    }

    void arc(const DmVector& center, double radius, double startAngle, double sweepAngle) override
    {
        line("arc c=" + pt(center) + " r=" + num(radius) + " start=" + deg(startAngle) + " sweep=" + deg(sweepAngle));
    }

    void ellipseArc(const DmVector& center, const DmVector& majorAxis, double ratio, double startParam,
                    double endParam) override
    {
        line("ellipse c=" + pt(center) + " major=" + pt(majorAxis) + " ratio=" + num(ratio) + " params=" +
             deg(startParam) + ".." + deg(endParam));
    }

    void nurbs(const GiNurbs& curve) override
    {
        std::string s = "nurbs degree=" + std::to_string(curve.degree) + (curve.closed ? " closed" : "") + " knots";
        for (double k : curve.knots)
        {
            s += " " + num(k);
        }
        s += " ctrl";
        for (const DmVector& p : curve.controlPoints)
        {
            s += " " + pt(p);
        }
        line(s);
    }

    void fill(std::span<const GiLoop> loops, GiFillRule rule) override
    {
        std::string s = std::string("fill ") + (rule == GiFillRule::EvenOdd ? "evenodd" : "nonzero");
        for (const GiLoop& loop : loops)
        {
            s += " [";
            for (std::size_t i = 0; i < loop.points.size(); ++i)
            {
                s += (i ? " " : "") + pt(loop.points[i]);
            }
            if (!loop.bulges.empty())
            {
                s += " bulges";
                for (double b : loop.bulges)
                {
                    s += " " + num(b);
                }
            }
            s += "]";
        }
        line(s);
    }

    void triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices) override
    {
        std::string s = "triangles";
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            s += " [" + pt(vertices[indices[i]]) + " " + pt(vertices[indices[i + 1]]) + " " +
                 pt(vertices[indices[i + 2]]) + "]";
        }
        line(s);
    }

    void glyphRun(const GiGlyphRun& run) override
    {
        std::string s = std::string("glyphRun font=") + (run.font ? "yes" : "none");
        for (const GiGlyph& g : run.glyphs)
        {
            s += " " + code(g.code) + "@" + xf(g.transform);
        }
        line(s);
    }

    void image(const GiImage& image) override
    {
        line("image origin=" + pt(image.origin) + " u=" + pt(image.u) + " v=" + pt(image.v) + " size=" +
             std::to_string(image.width) + "x" + std::to_string(image.height) + " path=" +
             image.path.toStdString() + " pixels=" + (image.pixels ? "yes" : "none"));
    }

    void point(const DmVector& position) override { line("point " + pt(position)); }

    void ray(const DmVector& base, const DmVector& direction) override
    {
        line("ray " + pt(base) + " dir=" + pt(direction));
    }

    void xline(const DmVector& base, const DmVector& direction) override
    {
        line("xline " + pt(base) + " dir=" + pt(direction));
    }

    void draw(const IGiDrawable& drawable) override
    {
        line("draw {");
        ++m_depth;
        drawable.setAttributes(*this);
        line("---");
        drawable.worldDraw(*this);
        --m_depth;
        line("}");
    }

    void drawShared(const IGiDrawable& drawable, const GiTransform& transform, const GiByBlockTraits& byBlock) override
    {
        const auto* block = dynamic_cast<const DmBlock*>(&drawable);
        line("drawShared " + (block ? block->getName().toStdString() : std::string("?")) + " xf=" + xf(transform) +
             " byBlock color=" + color(byBlock.color) + " lineweight=" + weight(byBlock.lineWeight) +
             " linetype=" + lineType(byBlock.lineType));
    }

    void pushTransform(const GiTransform& transform) override { line("pushTransform " + xf(transform)); }
    void popTransform() override { line("popTransform"); }

private:
    void line(const std::string& s)
    {
        m_out << std::string(static_cast<std::size_t>(m_depth) * 2, ' ') << s << "\n";
    }

    bool m_withAttributes;
    int m_depth = 0;
    std::ostringstream m_out;
};

}  // namespace

std::string giDump(const IGiDrawable& drawable)
{
    Dumper dumper(true);
    dumper.dump(drawable);
    return dumper.text();
}

std::string giDumpGeometry(const IGiDrawable& drawable)
{
    Dumper dumper(false);
    dumper.dump(drawable);
    return dumper.text();
}
