/// @file test_graphics_text.cpp
/// @brief 文字的 worldDraw：字形串（glyphRun）与字形变换（RENDER_PLAN.md 第 4.2.1 节）
///
/// 字形串只给出字体、字符码与每个字符的变换，字形几何由字体给出。验证的核心是：字形模板的笔画经
/// 字符的变换，正好落在字符自己的笔画上（字符的笔画副本仍在，捕捉与选择在用）。
/// 用系统的 Arial（默认文字样式 Standard 用它，TrueType 字形是三角形）；没有 Arial 时跳过。

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "DmCharTemplate.h"
#include "DmChar.h"
#include "DmDocument.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmTextStyleTable.h"
#include "DmTriangle.h"
#include "GiTextDump.h"
#include "IGiFont.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"

namespace
{

constexpr double kDeg = 3.14159265358979323846 / 180.0;

DmFont* arial()
{
    return DMFONTLIST->requestFont(QStringLiteral("arial.ttf"), false);
}

/// @brief 实体里的三角形笔画，按顺序
std::vector<std::array<DmVector, 3>> trianglesOf(const std::list<DmEntity*>& strokes)
{
    std::vector<std::array<DmVector, 3>> out;
    for (const DmEntity* e : strokes)
    {
        if (e->getEntityType() == DM::EntityTriangle)
        {
            out.push_back(static_cast<const DmTriangle*>(e)->getData().getPoints());
        }
    }
    return out;
}

/// @brief 两组三角形逐点相同（相对容差）
void expectSameTriangles(const std::vector<std::array<DmVector, 3>>& actual,
                         const std::vector<std::array<DmVector, 3>>& expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i)
    {
        for (int k = 0; k < 3; ++k)
        {
            const double tol = 1.0e-9 * (1.0 + std::abs(expected[i][k].x) + std::abs(expected[i][k].y));
            EXPECT_NEAR(actual[i][k].x, expected[i][k].x, tol) << "三角形 " << i << " 顶点 " << k;
            EXPECT_NEAR(actual[i][k].y, expected[i][k].y, tol) << "三角形 " << i << " 顶点 " << k;
        }
    }
}

/// @brief 收集字形串：嵌套绘制展开，其余图元忽略
class GlyphCollector final : public IGiWorldDraw, public IGiGeometry, public IGiSubEntityTraits
{
public:
    std::vector<GiGlyphRun> runs;
    std::vector<std::array<DmVector, 3>> tris;  ///< 三角形图元（字形模板的笔画）

    IGiGeometry& geometry() override { return *this; }
    IGiSubEntityTraits& traits() override { return *this; }
    GiRegenType regenType() const override { return GiRegenType::Display; }
    double deviation() const override { return 1.0e-3; }
    bool isDragging() const override { return false; }

    void setColor(const DmColor&) override {}
    void setLayer(const DmLayer*) override {}
    void setLineType(const DmLineType*) override {}
    void setLineTypeScale(double) override {}
    void setLinePattern(const GiLinePattern&) override {}
    void setLineWeight(DM::LineWidth) override {}
    void setTransparency(std::uint8_t) override {}
    void setSelectionMarker(std::int32_t) override {}
    void setScreenSpace(const DmVector*) override {}

    void polyline(std::span<const DmVector>, std::span<const double>, std::span<const GiSegmentWidth>,
                  GiPolylineFlags) override {}
    void circle(const DmVector&, double) override {}
    void arc(const DmVector&, double, double, double) override {}
    void ellipseArc(const DmVector&, const DmVector&, double, double, double) override {}
    void nurbs(const GiNurbs&) override {}
    void fill(std::span<const GiLoop>, GiFillRule) override {}
    void triangles(std::span<const DmVector> vertices, std::span<const std::uint32_t> indices) override
    {
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            tris.push_back({ vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]] });
        }
    }
    void glyphRun(const GiGlyphRun& run) override { runs.push_back(run); }
    void image(const GiImage&) override {}
    void point(const DmVector&) override {}
    void ray(const DmVector&, const DmVector&) override {}
    void xline(const DmVector&, const DmVector&) override {}
    void draw(const IGiDrawable& drawable) override { drawable.worldDraw(*this); }
    void drawShared(const IGiDrawable&, const GiTransform&, const GiByBlockTraits&) override {}
    void pushTransform(const GiTransform&) override {}
    void popTransform() override {}
};

/// @brief 模板的三角形笔画（经模板自己的 worldDraw 取得）经变换
std::vector<std::array<DmVector, 3>> transformed(const DmCharTemplate& templ, const GiTransform& xf)
{
    GlyphCollector collector;
    templ.worldDraw(collector);
    std::vector<std::array<DmVector, 3>> out = collector.tris;
    for (auto& tri : out)
    {
        for (DmVector& p : tri)
        {
            p = xf.apply(p);
        }
    }
    return out;
}

/// @brief 字形串里的字符，连成一个字符串
QString codesOf(const std::vector<GiGlyphRun>& runs)
{
    QString s;
    for (const GiGlyphRun& run : runs)
    {
        for (const GiGlyph& g : run.glyphs)
        {
            s += QString::fromUcs4(&g.code, 1);
        }
    }
    return s;
}

/// @brief 字形串的字形经各自的变换，与实体的三角形笔画逐一对应
void expectGlyphsMatchStrokes(const DmEntity& text)
{
    GlyphCollector collector;
    text.worldDraw(collector);
    std::vector<std::array<DmVector, 3>> fromGlyphs;
    for (const GiGlyphRun& run : collector.runs)
    {
        ASSERT_NE(run.font, nullptr);
        for (const GiGlyph& g : run.glyphs)
        {
            const auto* templ = dynamic_cast<const DmCharTemplate*>(run.font->glyph(g.code));
            ASSERT_NE(templ, nullptr);
            for (const auto& tri : transformed(*templ, g.transform))
            {
                fromGlyphs.push_back(tri);
            }
        }
    }
    const auto strokes = trianglesOf(text.getSubEntities());
    ASSERT_FALSE(strokes.empty());
    expectSameTriangles(fromGlyphs, strokes);
}

}  // namespace

TEST(GiTextTest, 字形变换把模板笔画映射到字符的笔画上)
{
    DmFont* font = arial();
    if (!font)
    {
        GTEST_SKIP() << "系统没有 Arial 字体";
    }
    DmCharTemplate* templ = font->findLetter(QStringLiteral("A"));
    ASSERT_NE(templ, nullptr);

    // 不倾斜、宽度系数 1：模板原样复制；倾斜 15°、宽度系数 0.7：模板经切变
    for (const auto& [widthFactor, slash] : { std::pair{ 1.0, 0.0 }, std::pair{ 0.7, 15.0 * kDeg } })
    {
        std::unique_ptr<DmChar> c(templ->generateChar(widthFactor, slash));
        c->scale(DmVector(0.0, 0.0), DmVector(2.5, 2.5));
        c->rotate(DmVector(1.0, 1.0), DmVector(30.0 * kDeg));
        c->mirror(DmVector(0.0, 0.0), DmVector(1.0, 2.0));
        c->move(DmVector(5.0, -3.0));
        expectSameTriangles(transformed(*templ, c->getGlyphTransform()), trianglesOf(c->getSubEntities()));

        const IGiFont* glyphFont = nullptr;
        char32_t code = 0;
        ASSERT_TRUE(c->getGlyph(glyphFont, code));
        EXPECT_EQ(glyphFont, font);
        EXPECT_EQ(code, U'A');
        // 复制的字符带着变换
        std::unique_ptr<DmEntity> copy(c->clone());
        EXPECT_EQ(static_cast<DmChar*>(copy.get())->getGlyphTransform(), c->getGlyphTransform());
    }
}

TEST(GiTextTest, 单行文字是一个字形串)
{
    if (!arial())
    {
        GTEST_SKIP() << "系统没有 Arial 字体";
    }
    DmDocument doc;
    DmText text(nullptr, TextData(DmVector(10.0, 5.0), 2.0, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft,
                                  QStringLiteral("AB C"), doc.getTextStyleTable()->getActive(), 20.0 * kDeg));
    text.setDocument(&doc);
    text.update();

    const std::string dump = giDumpGeometry(text);
    // 字符的画笔：颜色 ByBlock（取文字）、线宽 0、实线；空格没有字形
    EXPECT_EQ(dump.rfind("draw {\n"
                         "  color ByBlock\n"
                         "  lineweight 0\n"
                         "  linetype Continuous\n"
                         "  layer -\n"
                         "  ---\n"
                         "  glyphRun font=yes A@",
                         0),
              0u)
        << dump;
    GlyphCollector collector;
    text.worldDraw(collector);
    EXPECT_EQ(collector.runs.size(), 1u);
    EXPECT_EQ(codesOf(collector.runs), QStringLiteral("ABC"));
    expectGlyphsMatchStrokes(text);
}

TEST(GiTextTest, 单行文字的宽度系数与倾斜角经字形变换表达)
{
    if (!arial())
    {
        GTEST_SKIP() << "系统没有 Arial 字体";
    }
    DmDocument doc;
    DmText text(nullptr, TextData(DmVector(0.0, 0.0), 1.5, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft,
                                  QStringLiteral("Wx"), doc.getTextStyleTable()->getActive(), -35.0 * kDeg));
    text.setDocument(&doc);
    text.setWidthFactor(0.6);
    text.setSlashAngle(12.0 * kDeg);
    text.update();
    expectGlyphsMatchStrokes(text);
}

TEST(GiTextTest, 多行文字各段的字符连成字形串)
{
    if (!arial())
    {
        GTEST_SKIP() << "系统没有 Arial 字体";
    }
    DmDocument doc;
    DmMText text(nullptr, MTextData(DmVector(0.0, 0.0), 2.5, EMTextVertMode::kTextTop, EMTextHorzMode::kTextLeft,
                                    2.5 * 1.6, 50.0, QStringLiteral("AB\\PCD"), doc.getTextStyleTable()->getActive(),
                                    0.0));
    text.setDocument(&doc);
    text.update();
    GlyphCollector collector;
    text.worldDraw(collector);
    EXPECT_EQ(codesOf(collector.runs), QStringLiteral("ABCD"));
    expectGlyphsMatchStrokes(text);
}
