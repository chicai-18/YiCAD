/// @file test_graphics_stream.cpp
/// @brief GI 流：记录后重放与直接绘制一致（含嵌套绘制与共享对象），序列化往返（RENDER_PLAN.md 第 4.2.3 节）

#include <gtest/gtest.h>

#include <map>
#include <sstream>

#include <QImage>

#include "DmBlock.h"
#include "DmColor.h"
#include "DmLayer.h"
#include "DmLineType.h"
#include "GiNurbs.h"
#include "GiStream.h"
#include "GiTextDump.h"
#include "IGiFont.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"
#include "Stream.h"

namespace
{

/// @brief 只有一个字形的字体；字形是一条竖线
class OneGlyphFont final : public IGiFont
{
public:
    const IGiDrawable* glyph(char32_t code) const override { return code == U'I' ? &m_glyph : nullptr; }

private:
    class Glyph final : public IGiDrawable
    {
    public:
        void worldDraw(IGiWorldDraw& wd) const override
        {
            const DmVector pts[2] = { DmVector(0.0, 0.0), DmVector(0.0, 1.0) };
            wd.geometry().polyline(pts, {}, {}, GiPolylineFlags::None);
        }
    };
    Glyph m_glyph;
};

/// @brief 嵌套绘制的子对象：自己的颜色与线宽，一个圆
class Child final : public IGiDrawable
{
public:
    void setAttributes(IGiSubEntityTraits& traits) const override
    {
        traits.setColor(DmColor(10, 20, 30));
        traits.setLineWeight(DM::Width05);
    }
    void worldDraw(IGiWorldDraw& wd) const override { wd.geometry().circle(DmVector(1.0, 2.0), 3.0); }
};

/// @brief 用到全部图元与属性的可绘制对象
class Everything final : public IGiDrawable
{
public:
    Everything(const DmLayer* layer, const DmLineType* lineType, const IGiDrawable* shared, const IGiFont* font,
               const QImage* pixels)
        : m_layer(layer)
        , m_lineType(lineType)
        , m_shared(shared)
        , m_font(font)
        , m_pixels(pixels)
    {
    }

    void setAttributes(IGiSubEntityTraits& traits) const override
    {
        traits.setColor(DmColor(DM::FlagByLayer));
        traits.setLayer(m_layer);
        traits.setLineType(m_lineType);
        traits.setLineWeight(DM::WidthByLayer);
    }

    void worldDraw(IGiWorldDraw& wd) const override
    {
        IGiGeometry& g = wd.geometry();
        IGiSubEntityTraits& t = wd.traits();
        const DmVector pts[3] = { DmVector(0.0, 0.0), DmVector(1.0, 0.0), DmVector(1.0, 1.0) };
        const double bulges[3] = { 0.0, 0.5, -0.25 };
        const GiSegmentWidth widths[3] = { { 0.1, 0.2 }, { 0.0, 0.0 }, { 0.3, 0.3 } };
        g.polyline(pts, bulges, widths, GiPolylineFlags::Closed | GiPolylineFlags::ContinuousLinetype);
        t.setColor(DmColor(1, 2, 3, 4));
        g.circle(DmVector(5.0, 5.0), 2.0);
        g.arc(DmVector(-1.0, 0.0), 1.5, 0.25, 1.0);
        g.ellipseArc(DmVector(0.0, -3.0), DmVector(2.0, 1.0), 0.5, 0.1, 5.0);
        GiNurbs curve;
        curve.degree = 2;
        curve.closed = false;
        curve.knots = { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
        curve.controlPoints = { DmVector(0.0, 0.0), DmVector(1.0, 2.0), DmVector(2.0, 0.0) };
        g.nurbs(curve);
        const GiLoop loops[2] = { GiLoop{ { DmVector(0.0, 0.0), DmVector(4.0, 0.0), DmVector(4.0, 4.0) }, {} },
                                  GiLoop{ { DmVector(1.0, 1.0), DmVector(2.0, 1.0), DmVector(2.0, 2.0) }, { 0.0, 0.3, 0.0 } } };
        g.fill(loops, GiFillRule::EvenOdd);
        const std::uint32_t indices[3] = { 0, 1, 2 };
        g.triangles(pts, indices);
        t.setLinePattern(GiLinePattern{ { 0.5, -0.25, 0.0 }, 0.125 });
        t.setLineTypeScale(2.0);
        t.setTransparency(128);
        t.setSelectionMarker(7);
        const DmVector anchor(9.0, 9.0);
        t.setScreenSpace(&anchor);
        g.point(DmVector(3.0, 4.0));
        t.setScreenSpace(nullptr);
        g.ray(DmVector(0.0, 1.0), DmVector(1.0, 1.0));
        g.xline(DmVector(0.0, 2.0), DmVector(-1.0, 0.5));
        GiGlyphRun run;
        run.font = m_font;
        run.glyphs = { GiGlyph{ U'I', GiTransform::translation(DmVector(1.0, 0.0)) },
                       GiGlyph{ U'中', GiTransform::scaling(DmVector(2.0, 2.0)) } };
        g.glyphRun(run);
        GiImage image;
        image.origin = DmVector(1.0, 1.0);
        image.u = DmVector(3.0, 0.0);
        image.v = DmVector(0.0, 2.0);
        image.width = 30;
        image.height = 20;
        image.path = QStringLiteral("图片/a.png");
        image.pixels = m_pixels;
        g.image(image);
        g.pushTransform(GiTransform::rotation(0.5));
        g.draw(Child());
        g.popTransform();
        GiByBlockTraits byBlock;
        byBlock.color = DmColor(200, 100, 0);
        byBlock.lineWeight = DM::Width13;
        byBlock.lineType = m_lineType;
        g.drawShared(*m_shared, GiTransform::scaling(DmVector(2.0, 1.0), DmVector(1.0, 1.0)), byBlock);
    }

private:
    const DmLayer* m_layer;
    const DmLineType* m_lineType;
    const IGiDrawable* m_shared;
    const IGiFont* m_font;
    const QImage* m_pixels;
};

/// @brief 按名字存取引用的编解码器
class NameCodec final : public IGiReferenceCodec
{
public:
    std::map<std::string, const void*> objects;     ///< 读回时按名字找
    std::map<const void*, std::string> names;       ///< 写出时按对象取名

    void add(const std::string& name, const void* object)
    {
        objects[name] = object;
        names[object] = name;
    }

    std::string layerName(const DmLayer* layer) const override { return nameOf(layer); }
    const DmLayer* findLayer(const std::string& name) const override { return find<DmLayer>(name); }
    std::string lineTypeName(const DmLineType* lineType) const override { return nameOf(lineType); }
    const DmLineType* findLineType(const std::string& name) const override { return find<DmLineType>(name); }
    std::string drawableName(const IGiDrawable* drawable) const override { return nameOf(drawable); }
    const IGiDrawable* findDrawable(const std::string& name) const override { return find<IGiDrawable>(name); }
    std::string fontName(const IGiFont* font) const override { return nameOf(font); }
    const IGiFont* findFont(const std::string& name) const override { return find<IGiFont>(name); }
    std::string imageName(const QImage* image) const override { return nameOf(image); }
    const QImage* findImage(const std::string& name) const override { return find<QImage>(name); }

private:
    std::string nameOf(const void* object) const
    {
        auto it = names.find(object);
        return it == names.end() ? std::string() : it->second;
    }

    template<typename T>
    const T* find(const std::string& name) const
    {
        auto it = objects.find(name);
        return it == objects.end() ? nullptr : static_cast<const T*>(it->second);
    }
};

struct GiStreamFixture : ::testing::Test
{
    DmLayer layer{ QStringLiteral("L1") };
    DmLineType dashed{ QStringLiteral("DASHED") };
    DmBlock block{ nullptr, DmBlockData(QStringLiteral("BLK"), DmVector(0.0, 0.0), false) };
    OneGlyphFont font;
    QImage pixels{ 4, 4, QImage::Format_ARGB32 };
    Everything drawable{ &layer, &dashed, &block, &font, &pixels };

    NameCodec codec()
    {
        NameCodec c;
        // 块名要与 GiTextDump 写出的一致；这里只为序列化取名，读回时按名字找回同一个对象
        c.add("L1", static_cast<const DmLayer*>(&layer));
        c.add("DASHED", static_cast<const DmLineType*>(&dashed));
        c.add("BLK", static_cast<const IGiDrawable*>(&block));
        c.add("FONT", static_cast<const IGiFont*>(&font));
        c.add("PIXELS", static_cast<const QImage*>(&pixels));
        return c;
    }
};

}  // namespace

TEST_F(GiStreamFixture, 记录后重放与直接绘制一致)
{
    const GiStream stream = GiStreamRecorder::record(drawable);
    ASSERT_FALSE(stream.isEmpty());
    EXPECT_EQ(giDump(GiStreamDrawable(stream)), giDump(drawable));
}

TEST_F(GiStreamFixture, 嵌套绘制重放后仍是一个带属性的嵌套段)
{
    const GiStream stream = GiStreamRecorder::record(drawable);
    const std::string text = giDump(GiStreamDrawable(stream));
    EXPECT_NE(text.find("pushTransform [0.877583 0.479426 -0.479426 0.877583 0 0]\n"
                        "draw {\n"
                        "  color rgb(10,20,30)\n"
                        "  lineweight 18\n"
                        "  ---\n"
                        "  circle c=(1,2) r=3\n"
                        "}\n"
                        "popTransform\n"),
              std::string::npos)
        << text;
    EXPECT_NE(text.find("drawShared BLK xf=[2 0 0 1 -1 0] byBlock color=rgb(200,100,0) lineweight=60 linetype=DASHED"),
              std::string::npos)
        << text;
}

TEST_F(GiStreamFixture, 属性段与图元段分开重放)
{
    const GiStream stream = GiStreamRecorder::record(drawable);
    const std::string text = giDump(GiStreamDrawable(stream));
    // setAttributes 的四项在 "---" 之前，worldDraw 里改的颜色在之后
    EXPECT_EQ(text.substr(0, text.find("---")), "color ByLayer\nlayer L1\nlinetype DASHED\nlineweight ByLayer\n");
    EXPECT_NE(text.find("---\npolyline closed continuous"), std::string::npos);
}

TEST_F(GiStreamFixture, 序列化往返)
{
    const GiStream stream = GiStreamRecorder::record(drawable);
    const NameCodec names = codec();

    std::ostringstream bytes(std::ios::binary);
    OutputStream out(bytes);
    stream.write(out, names);

    std::istringstream in(bytes.str(), std::ios::binary);
    InputStream reader(in);
    GiStream back;
    ASSERT_TRUE(back.read(reader, names));
    EXPECT_EQ(back, stream);
    EXPECT_EQ(giDump(GiStreamDrawable(back)), giDump(drawable));
}

TEST_F(GiStreamFixture, 读回时找不到的引用为空且共享对象跳过)
{
    const GiStream stream = GiStreamRecorder::record(drawable);
    std::ostringstream bytes(std::ios::binary);
    OutputStream out(bytes);
    stream.write(out, codec());

    // 读回时只认图层
    NameCodec partial;
    partial.add("L1", static_cast<const DmLayer*>(&layer));
    std::istringstream in(bytes.str(), std::ios::binary);
    InputStream reader(in);
    GiStream back;
    ASSERT_TRUE(back.read(reader, partial));
    const std::string text = giDump(GiStreamDrawable(back));
    EXPECT_NE(text.find("layer L1"), std::string::npos);
    EXPECT_NE(text.find("linetype ByBlock"), std::string::npos);
    EXPECT_NE(text.find("glyphRun font=none"), std::string::npos);
    EXPECT_NE(text.find("pixels=none"), std::string::npos);
    EXPECT_EQ(text.find("drawShared"), std::string::npos);
}

TEST_F(GiStreamFixture, 版本号不认识时读回失败)
{
    std::ostringstream bytes(std::ios::binary);
    OutputStream out(bytes);
    out << static_cast<std::uint32_t>(GiStream::kVersion + 1);

    std::istringstream in(bytes.str(), std::ios::binary);
    InputStream reader(in);
    GiStream back;
    EXPECT_FALSE(back.read(reader, codec()));
    EXPECT_TRUE(back.isEmpty());
}
