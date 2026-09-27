/// @file test_render_reference.cpp
/// @brief 参考图纸出图比对（RENDER_PLAN.md 第 5 节 0.2 步）
///
/// 每个用例把 tests/render/drawings/ 里的一张参考图纸（tools/gen_render_references.py 生成，
/// autocad_linetype.dxf 是 AutoCAD 画的线型对照图纸）画成图像，与 tests/render/baseline/ 里的
/// 基准图像比对。基准图像记录的是当前旧渲染器的样子，包括已知的问题（RGB 全 0 画成白色、非等比块里
/// X<Y 的圆仍是圆、离原点远时的浮点抖动等，见 RENDER_PLAN.md 第 3.2 节），是重构每一步的回归依据；
/// 有意改变显示时，确认新图后设 YICAD_RENDER_UPDATE_BASELINE=1 重跑以更新基准图像。
///
/// 依赖用户自备字体的用例在字体缺失时跳过（SHX 字体不入库，CI 上没有）。

#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include "RenderHarness.h"

namespace
{
using yicad_test::RenderRequest;
using yicad_test::RenderRequirement;

/// @brief 一个出图用例：基准图像名与出图请求
struct RenderCase
{
    const char* name;
    RenderRequest request;
};

void PrintTo(const RenderCase& c, std::ostream* out)
{
    *out << c.name;
}

RenderRequest drawing(const char* file)
{
    RenderRequest request;
    request.drawing = QString::fromUtf8(file);
    return request;
}

RenderRequest withLineWidth(RenderRequest request)
{
    request.lineWidth = true;
    return request;
}

RenderRequest withGrid(RenderRequest request)
{
    request.grid = true;
    return request;
}

RenderRequest withSelection(RenderRequest request)
{
    request.selectCircles = true;
    request.highlightArcs = true;
    return request;
}

RenderRequest requiring(RenderRequest request, RenderRequirement requirement)
{
    request.requirement = requirement;
    return request;
}

RenderRequest sized(RenderRequest request, int width, int height)
{
    request.width = width;
    request.height = height;
    return request;
}

const RenderCase kCases[] = {
    {"entities", drawing("entities.dxf")},
    {"entities_grid", withGrid(drawing("entities.dxf"))},
    {"entities_selected_highlighted", withSelection(drawing("entities.dxf"))},
    {"linetypes", sized(drawing("linetypes.dxf"), 1200, 800)},
    {"lineweights_display_off", drawing("lineweights.dxf")},
    {"lineweights_display_on", withLineWidth(drawing("lineweights.dxf"))},
    {"colors", drawing("colors.dxf")},
    {"blocks", sized(drawing("blocks.dxf"), 960, 540)},
    {"far_coords", drawing("far_coords.dxf")},
    {"image", drawing("image.dxf")},
    {"autocad_linetype", sized(drawing("autocad_linetype.dxf"), 960, 1040)},
    {"text_shx", requiring(drawing("text_shx.dxf"), RenderRequirement::ShxFont)},
    {"text_truetype", requiring(drawing("text_truetype.dxf"), RenderRequirement::TrueTypeFont)},
};

class RenderReferenceTest : public ::testing::TestWithParam<RenderCase>
{
};

TEST_P(RenderReferenceTest, 与基准图像一致)
{
    const RenderCase& c = GetParam();
    QString reason;
    if (!yicad_test::requirementMet(c.request.requirement, &reason))
    {
        GTEST_SKIP() << reason.toStdString();
    }

    QString error;
    const QImage image = yicad_test::renderDrawing(c.request, &error);
    ASSERT_FALSE(image.isNull()) << error.toStdString();
    yicad_test::expectMatchesBaseline(QString::fromUtf8(c.name), image);
}

INSTANTIATE_TEST_SUITE_P(ReferenceDrawings, RenderReferenceTest, ::testing::ValuesIn(kCases),
                         [](const ::testing::TestParamInfo<RenderCase>& info) { return std::string(info.param.name); });

}  // namespace
