/// @file test_graphics_entities.cpp
/// @brief 各内置实体的 worldDraw 输出（RENDER_PLAN.md 2.2 步）
///
/// 简单实体逐行比对 GiTextDump 写出的文本；复合实体（填充、块参照、标注、引线）比对结构：
/// 哪些部分嵌套绘制、属性从哪里来、块参照的变换。最后用样本文档（tests/support/OcdSampleDocument.h）
/// 检查每个实体都画出了东西，且经 GI 流记录再重放与直接绘制一致。文字见 test_graphics_text.cpp。

#include <gtest/gtest.h>

#include <QImage>

#include "DmArc.h"
#include "DmAttributeDefinition.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmCircle.h"
#include "DmConstructionLine.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmEntityContainer.h"
#include "DmHatch.h"
#include "DmImage.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "DmLineStrip.h"
#include "DmPattern.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmRay.h"
#include "DmRegion.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "DmTriangle.h"
#include "DmXline.h"
#include "GiStream.h"
#include "GiTextDump.h"
#include "support/OcdSampleDocument.h"

namespace
{

constexpr double kDeg = 3.14159265358979323846 / 180.0;

/// @brief 文本里 needle 出现的次数
int count(const std::string& text, const std::string& needle)
{
    int n = 0;
    for (std::size_t pos = text.find(needle); pos != std::string::npos; pos = text.find(needle, pos + needle.size()))
    {
        ++n;
    }
    return n;
}

}  // namespace

// ---------------------------------------------------------------------------
// 属性
// ---------------------------------------------------------------------------

TEST(GiEntityTest, 属性按实体自身的画笔与图层给出)
{
    DmDocument doc;
    DmLayer* layer0 = doc.getLayerTable()->find(QStringLiteral("0"));
    ASSERT_NE(layer0, nullptr);
    DmPoint p(nullptr, PointData(DmVector(0.0, 0.0)));
    p.setLayer(layer0);
    p.setPen(DmPen(DmColor(DM::FlagByLayer), DM::WidthByLayer, doc.getLineTypeTable()->getLineTypeByLayer()));
    EXPECT_EQ(giDump(p), "color ByLayer\n"
                         "lineweight ByLayer\n"
                         "linetype ByLayer\n"
                         "layer 0\n"
                         "---\n"
                         "point (0,0)\n");

    p.setPen(DmPen(DmColor(10, 20, 30), DM::Width07, DmLineTypeTable::Continuous));
    p.setLayer(static_cast<DmLayer*>(nullptr));
    EXPECT_EQ(giDump(p), "color rgb(10,20,30)\n"
                         "lineweight 25\n"
                         "linetype Continuous\n"
                         "layer -\n"
                         "---\n"
                         "point (0,0)\n");
}

TEST(GiEntityTest, 实体线型比例不为1时给出)
{
    // GI 里线型比例的初值为 1，为 1 时不给出（其余用例的输出因此不变）
    DmDocument doc;
    DmPoint p(nullptr, PointData(DmVector(0.0, 0.0)));
    p.setPen(DmPen(DmColor(10, 20, 30), DM::Width07, doc.getLineTypeTable()->getLineTypeContinuous()));
    p.setLineTypeScale(0.5);
    EXPECT_EQ(giDump(p), "color rgb(10,20,30)\n"
                         "lineweight 25\n"
                         "linetype Continuous\n"
                         "layer -\n"
                         "linetypescale 0.5\n"
                         "---\n"
                         "point (0,0)\n");
}

TEST(GiEntityTest, 无效画笔的三项都取外层)
{
    // 多段线原先生成的子实体用无效画笔表示"全部随父实体"，与 ByBlock 等价
    DmPoint p(nullptr, PointData(DmVector(0.0, 0.0)));
    p.setPen(DmPen(DM::FlagInvalid));
    p.setLayer(static_cast<DmLayer*>(nullptr));
    EXPECT_EQ(giDump(p), "color ByBlock\n"
                         "lineweight ByBlock\n"
                         "linetype ByBlock\n"
                         "layer -\n"
                         "---\n"
                         "point (0,0)\n");
}

// ---------------------------------------------------------------------------
// 简单实体
// ---------------------------------------------------------------------------

TEST(GiEntityTest, 点直线射线构造线)
{
    EXPECT_EQ(giDumpGeometry(DmPoint(nullptr, PointData(DmVector(1.0, 2.0)))), "point (1,2)\n");
    EXPECT_EQ(giDumpGeometry(DmLine(DmVector(0.0, 0.0), DmVector(3.0, 4.0))), "polyline (0,0) (3,4)\n");
    EXPECT_EQ(giDumpGeometry(DmRay(nullptr, RayData(DmVector(1.0, 1.0), DmVector(0.6, 0.8)))),
              "ray (1,1) dir=(0.6,0.8)\n");
    EXPECT_EQ(giDumpGeometry(DmXline(nullptr, XLineData(DmVector(0.0, 5.0), DmVector(1.0, 0.0)))),
              "xline (0,5) dir=(1,0)\n");
    EXPECT_EQ(giDumpGeometry(DmConstructionLine(nullptr, DmConstructionLineData(DmVector(1.0, 1.0), DmVector(3.0, 2.0)))),
              "xline (1,1) dir=(2,1)\n");
}

TEST(GiEntityTest, 圆弧按翻正后的角度逆时针给出)
{
    EXPECT_EQ(giDumpGeometry(DmArc(nullptr, ArcData(DmVector(1.0, 1.0), DmVector(0.0, 0.0, 1.0), 2.0, 30.0 * kDeg,
                                                    120.0 * kDeg))),
              "arc c=(1,1) r=2 start=30 sweep=90\n");
    // 跨过 0°
    EXPECT_EQ(giDumpGeometry(DmArc(nullptr, ArcData(DmVector(0.0, 0.0), DmVector(0.0, 0.0, 1.0), 1.0, 300.0 * kDeg,
                                                    30.0 * kDeg))),
              "arc c=(0,0) r=1 start=300 sweep=90\n");
    // 法向朝 -Z 的圆弧：角度在镜像的坐标系里，翻正后逆时针从 60° 到 150°
    EXPECT_EQ(giDumpGeometry(DmArc(nullptr, ArcData(DmVector(0.0, 0.0), DmVector(0.0, 0.0, -1.0), 1.0, 30.0 * kDeg,
                                                    120.0 * kDeg))),
              "arc c=(0,0) r=1 start=60 sweep=90\n");
}

TEST(GiEntityTest, 圆与椭圆)
{
    EXPECT_EQ(giDumpGeometry(DmCircle(nullptr, CircleData(DmVector(2.0, 3.0), 5.0))), "circle c=(2,3) r=5\n");
    // 整椭圆：参数 0 到 360°
    EXPECT_EQ(giDumpGeometry(DmEllipse(nullptr, EllipseData(DmVector(0.0, 0.0), DmVector(2.0, 0.0),
                                                            DmVector(0.0, 0.0, 1.0), 0.5, true, 0.0, 0.0))),
              "ellipse c=(0,0) major=(2,0) ratio=0.5 params=0..360\n");
    EXPECT_EQ(giDumpGeometry(DmEllipse(nullptr, EllipseData(DmVector(0.0, 0.0), DmVector(0.0, 3.0),
                                                            DmVector(0.0, 0.0, 1.0), 0.5, false, 90.0 * kDeg,
                                                            180.0 * kDeg))),
              "ellipse c=(0,0) major=(0,3) ratio=0.5 params=90..180\n");
    // 终点参数小于起点：逆时针转过 0°
    EXPECT_EQ(giDumpGeometry(DmEllipse(nullptr, EllipseData(DmVector(0.0, 0.0), DmVector(2.0, 0.0),
                                                            DmVector(0.0, 0.0, 1.0), 0.5, false, 270.0 * kDeg,
                                                            45.0 * kDeg))),
              "ellipse c=(0,0) major=(2,0) ratio=0.5 params=270..405\n");
}

TEST(GiEntityTest, SOLID与三角形按扇形剖分)
{
    EXPECT_EQ(giDumpGeometry(DmSolid(nullptr, SolidData({ DmVector(0.0, 0.0), DmVector(2.0, 0.0), DmVector(2.0, 1.0),
                                                          DmVector(0.0, 1.0) }))),
              "triangles [(0,0) (2,0) (2,1)] [(0,0) (2,1) (0,1)]\n");
    EXPECT_EQ(giDumpGeometry(DmSolid(nullptr, SolidData({ DmVector(0.0, 0.0), DmVector(2.0, 0.0), DmVector(1.0, 1.0) }))),
              "triangles [(0,0) (2,0) (1,1)]\n");
    EXPECT_EQ(giDumpGeometry(DmTriangle(nullptr, TriangleData(DmVector(0.0, 0.0), DmVector(1.0, 0.0), DmVector(0.0, 1.0)))),
              "triangles [(0,0) (1,0) (0,1)]\n");
}

TEST(GiEntityTest, 多段线带凸度与线宽)
{
    std::vector<double> noWidths(4, 0.0);
    DmPolyline open(nullptr, PolylineData({ DmVector(0.0, 0.0), DmVector(1.0, 0.0), DmVector(1.0, 1.0) },
                                          { 0.0, 0.5 }, noWidths, false));
    open.update();
    EXPECT_EQ(giDumpGeometry(open), "polyline (0,0) (1,0) (1,1) bulges 0 0.5\n");

    std::vector<double> widths{ 0.1, 0.2, 0.0, 0.0, 0.3, 0.3 };
    DmPolyline closed(nullptr, PolylineData({ DmVector(0.0, 0.0), DmVector(2.0, 0.0), DmVector(1.0, 1.0) },
                                            { 0.0, 0.0, -0.25 }, widths, true));
    closed.update();
    EXPECT_EQ(giDumpGeometry(closed), "polyline closed (0,0) (2,0) (1,1) bulges 0 0 -0.25 widths 0.1:0.2 0:0 0.3:0.3\n");

    // 顶点与凸度个数对不上的无效多段线不画
    std::vector<double> w2(2, 0.0);
    DmPolyline invalid(nullptr, PolylineData({ DmVector(0.0, 0.0) }, { 0.0 }, w2, false));
    EXPECT_EQ(giDumpGeometry(invalid), "");
}

TEST(GiEntityTest, 线串整条连续)
{
    DmLineStrip open(nullptr, LineStripData({ DmVector(0.0, 0.0), DmVector(1.0, 1.0), DmVector(2.0, 0.0) }, false));
    EXPECT_EQ(giDumpGeometry(open), "polyline continuous (0,0) (1,1) (2,0)\n");
    DmLineStrip closed(nullptr, LineStripData({ DmVector(0.0, 0.0), DmVector(1.0, 1.0), DmVector(2.0, 0.0) }, true));
    EXPECT_EQ(giDumpGeometry(closed), "polyline closed continuous (0,0) (1,1) (2,0)\n");
}

TEST(GiEntityTest, 样条以NURBS给出)
{
    SplineData data(2, false, ESplineType::eControlPoints);
    data.setControlPoints({ DmVector(0.0, 0.0), DmVector(1.0, 2.0), DmVector(2.0, 0.0) });
    data.setKnots({ 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 });
    DmSpline spline(nullptr, data);
    spline.update();
    EXPECT_EQ(giDumpGeometry(spline), "nurbs degree=2 knots 0 0 0 1 1 1 ctrl (0,0) (1,2) (2,0)\n");

    // 样条自己的离散与 GI 的离散是同一份
    std::vector<DmVector> fromSpline;
    spline.getPoints(fromSpline);
    std::vector<DmVector> fromNurbs;
    spline.toNurbs().sample(fromNurbs);
    ASSERT_EQ(fromSpline.size(), fromNurbs.size());
    for (std::size_t i = 0; i < fromSpline.size(); ++i)
    {
        EXPECT_EQ(fromSpline[i].x, fromNurbs[i].x);
        EXPECT_EQ(fromSpline[i].y, fromNurbs[i].y);
    }

    // 控制点不够的样条不画
    SplineData tooFew(3, false, ESplineType::eControlPoints);
    tooFew.setControlPoints({ DmVector(0.0, 0.0), DmVector(1.0, 1.0) });
    EXPECT_EQ(giDumpGeometry(DmSpline(nullptr, tooFew)), "");
}

TEST(GiEntityTest, 图片给出四角与来源)
{
    DmImage image(nullptr, ImageData(0, DmVector(1.0, 2.0), DmVector(0.5, 0.0), DmVector(0.0, 0.5), DmVector(4.0, 2.0),
                                     "missing.png", 50, 50, 0));
    EXPECT_EQ(giDumpGeometry(image), "image origin=(1,2) u=(2,0) v=(0,1) size=4x2 path=missing.png pixels=yes\n");
}

// ---------------------------------------------------------------------------
// 复合实体
// ---------------------------------------------------------------------------

namespace
{

/// @brief 四条直线围成的矩形边界
DmEntityContainerPtr rectangle(double x0, double y0, double x1, double y1)
{
    auto boundary = std::make_shared<DmEntityContainer>(nullptr);
    boundary->addEntity(new DmLine(DmVector(x0, y0), DmVector(x1, y0)));
    boundary->addEntity(new DmLine(DmVector(x1, y0), DmVector(x1, y1)));
    boundary->addEntity(new DmLine(DmVector(x1, y1), DmVector(x0, y1)));
    boundary->addEntity(new DmLine(DmVector(x0, y1), DmVector(x0, y0)));
    return boundary;
}

std::string loopText(const DmEntityContainerPtr& contour)
{
    std::vector<DmVector> pts;
    DmRegion::getPointsOfOneBoundary(contour, pts);
    std::string s = "[";
    for (std::size_t i = 0; i < pts.size(); ++i)
    {
        s += (i ? " (" : "(") + std::to_string(static_cast<int>(pts[i].x)) + "," +
             std::to_string(static_cast<int>(pts[i].y)) + ")";
    }
    return s + "]";
}

}  // namespace

TEST(GiEntityTest, 实心填充交出边界与孔洞的环)
{
    DmDocument doc;
    const DmEntityContainerPtr outer = rectangle(0.0, 0.0, 4.0, 4.0);
    const DmEntityContainerPtr hole = rectangle(1.0, 1.0, 2.0, 2.0);
    HatchData data(true, 1.0, 0.0, std::wstring(L"SOLID"));
    data.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(outer, { hole })));
    DmHatch hatch(nullptr, data);
    hatch.setDocument(&doc);
    hatch.update();
    // 取点与原先的 DmRegion::getTriangles 相同，剖分交给接收方
    EXPECT_EQ(giDumpGeometry(hatch), "fill evenodd " + loopText(outer) + " " + loopText(hole) + "\n");
}

// 以多段线为边界的实心填充没有输出：Edge::getPoints（YiCAD/src/model/algorithm/FindClosedRegion.cpp:104）
// 没有多段线分支，DmRegion::getPointsOfOneBoundary 取不到点。DXF 导入的填充边界大多是多段线
// （HostApi.cpp 把 AutoCAD 的边界建成一条 DmPolyline），所以常见的实心填充都画不出来，
// 旧渲染器在阶段 0 就是如此（RENDER_PLAN.md 第 10 节阶段 0 的已知问题）。修好后启用本用例。
TEST(GiEntityTest, DISABLED_多段线边界的实心填充交出环)
{
    auto boundary = std::make_shared<DmEntityContainer>(nullptr);
    std::vector<double> widths(8, 0.0);
    boundary->addEntity(new DmPolyline(boundary.get(),
                                       PolylineData({ DmVector(0.0, 0.0), DmVector(4.0, 0.0), DmVector(4.0, 4.0),
                                                      DmVector(0.0, 4.0) },
                                                    { 0.0, 0.0, 0.0, 0.0 }, widths, true)));
    HatchData data(true, 1.0, 0.0, std::wstring(L"SOLID"));
    data.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(boundary, {})));
    DmHatch hatch(nullptr, data);
    hatch.update();
    EXPECT_NE(giDumpGeometry(hatch).find("fill evenodd [(0,0)"), std::string::npos);
}

TEST(GiEntityTest, 图案填充的实线图案线按连续线逐条画)
{
    DmDocument doc;
    DmPattern pattern;
    // 水平线，行距 1，实线（只有 5 项）
    pattern.setPatternData({ { 0.0, 0.0, 0.0, 0.0, 1.0 } });
    HatchData data(false, 1.0, 0.0, &pattern);
    data.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(rectangle(0.0, 0.0, 4.0, 4.0), {})));
    DmHatch hatch(nullptr, data);
    hatch.setDocument(&doc);
    hatch.update();
    const std::string text = giDumpGeometry(hatch);
    // 颜色、线宽随填充自己的；图案线按连续线，每条一整段，图案为空
    EXPECT_EQ(text.rfind("linetype Continuous\n", 0), 0u) << text;
    EXPECT_GE(count(text, "polyline"), 3) << text;
    EXPECT_EQ(count(text, "linepattern phase=0\n"), count(text, "polyline")) << text;
}

TEST(GiEntityTest, 图案填充的虚线图案线带图案与起点的相位)
{
    // 图案线：水平、过 (0.3, 0)、行距 1，划线 0.5、空白 0.25。边界 [0,4]：每行从 x = 0 画到 4，
    // 起点在图案线上相对图案原点的位置是 -0.3，相位 = -0.3 对 0.75 取模 = 0.45（相位锚定在图案原点，第 4.5.1 节）
    DmDocument doc;
    DmPattern pattern;
    pattern.setPatternData({ { 0.0, 0.3, 0.0, 0.0, 1.0, 0.5, -0.25 } });
    HatchData data(false, 1.0, 0.0, &pattern);
    data.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(rectangle(0.0, 0.0, 4.0, 4.0), {})));
    DmHatch hatch(nullptr, data);
    hatch.setDocument(&doc);
    hatch.update();
    std::string text = giDumpGeometry(hatch);
    EXPECT_NE(text.find("linepattern 0.5 -0.25 phase=0.45\npolyline (0,2) (4,2)\n"), std::string::npos) << text;
    EXPECT_EQ(count(text, "linepattern 0.5 -0.25 phase=0.45\n"), count(text, "polyline")) << text;

    // 移动、缩放时图案线跟着变：移动不改相位；按 2 倍缩放（基点 (0,0)）时划线与相位一起放大
    hatch.move(DmVector(10.0, 0.0));
    text = giDumpGeometry(hatch);
    EXPECT_NE(text.find("linepattern 0.5 -0.25 phase=0.45\npolyline (10,2) (14,2)\n"), std::string::npos) << text;
    hatch.scale(DmVector(0.0, 0.0), DmVector(2.0, 2.0));
    text = giDumpGeometry(hatch);
    EXPECT_NE(text.find("linepattern 1 -0.5 phase=0.9\npolyline (20,4) (28,4)\n"), std::string::npos) << text;
}

TEST(GiEntityTest, 圆环图案填充穿过孔洞的图案线分成两段各按图案原点取相位)
{
    // 外边界圆心 (0,0) 半径 4，孔洞半径 2；图案线水平、过原点、行距 1，划线 0.5、空白 0.25。
    // y = 0 的图案线被孔洞分成 [-4,-2] 与 [2,4] 两段，起点相对图案原点的位置 -4、2，对 0.75 取模都是 0.5；
    // y = 3 的只穿过外圆，从 x = -√7 起，相位 = -√7 对 0.75 取模 = 3 - √7
    DmDocument doc;
    DmPattern pattern;
    pattern.setPatternData({ { 0.0, 0.0, 0.0, 0.0, 1.0, 0.5, -0.25 } });
    HatchData data(false, 1.0, 0.0, &pattern);
    auto outer = std::make_shared<DmEntityContainer>(nullptr);
    outer->addEntity(new DmCircle(outer.get(), CircleData(DmVector(0.0, 0.0), 4.0)));
    auto hole = std::make_shared<DmEntityContainer>(nullptr);
    hole->addEntity(new DmCircle(hole.get(), CircleData(DmVector(0.0, 0.0), 2.0)));
    data.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(outer, { hole })));
    DmHatch hatch(nullptr, data);
    hatch.setDocument(&doc);
    hatch.update();
    const std::string text = giDumpGeometry(hatch);
    // 圆环内的点在区域里，孔洞里的不在（原先区域的射线法不计整圆边界的交点，圆环里一条图案线也生成不了）
    EXPECT_TRUE(hatch.getBoundary()->isPointInside(DmVector(3.0, 0.0)));
    EXPECT_TRUE(hatch.getBoundary()->isPointInside(DmVector(0.0, 3.0)));
    EXPECT_FALSE(hatch.getBoundary()->isPointInside(DmVector(0.0, 0.0)));
    EXPECT_NE(text.find("linepattern 0.5 -0.25 phase=0.5\npolyline (-4,0) (-2,0)\n"), std::string::npos) << text;
    EXPECT_NE(text.find("linepattern 0.5 -0.25 phase=0.5\npolyline (2,0) (4,0)\n"), std::string::npos) << text;
    EXPECT_NE(text.find("linepattern 0.5 -0.25 phase=0.354249\npolyline (-2.64575,3) (2.64575,3)\n"), std::string::npos)
        << text;
    // 穿过孔洞的 y = -1、0、1 各两段，其余的各一段；每段都带图案
    EXPECT_EQ(count(text, "linepattern 0.5 -0.25"), count(text, "polyline")) << text;
    EXPECT_EQ(count(text, ",0) ("), 2) << text;
}

TEST(GiEntityTest, 区域画边界与孔洞的轮廓)
{
    DmRegion region(nullptr, RegionData(rectangle(0.0, 0.0, 4.0, 4.0), { rectangle(1.0, 1.0, 2.0, 2.0) }));
    EXPECT_EQ(giDumpGeometry(region), "polyline (0,0) (4,0)\n"
                                      "polyline (4,0) (4,4)\n"
                                      "polyline (4,4) (0,4)\n"
                                      "polyline (0,4) (0,0)\n"
                                      "polyline (1,1) (2,1)\n"
                                      "polyline (2,1) (2,2)\n"
                                      "polyline (2,2) (1,2)\n"
                                      "polyline (1,2) (1,1)\n");
}

TEST(GiEntityTest, 容器里的实体逐个嵌套绘制)
{
    DmEntityContainer container(nullptr);
    auto* line = new DmLine(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
    line->setPen(DmPen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous));
    container.addEntity(line);
    EXPECT_EQ(giDumpGeometry(container), "draw {\n"
                                         "  color ByBlock\n"
                                         "  lineweight 0\n"
                                         "  linetype Continuous\n"
                                         "  layer -\n"
                                         "  ---\n"
                                         "  polyline (0,0) (1,0)\n"
                                         "}\n");
}

TEST(GiEntityTest, 块参照按阵列逐格引用块定义)
{
    DmDocument doc;
    auto* block = new DmBlock(&doc, DmBlockData(QStringLiteral("B"), DmVector(1.0, 1.0), false));
    doc.getBlockTable()->add_direct(block);
    auto* line = new DmLine(DmVector(1.0, 1.0), DmVector(2.0, 1.0));
    line->setPen(DmPen(DmColor(DM::FlagByBlock), DM::WidthByBlock, doc.getLineTypeTable()->getLineTypeByBlock()));
    line->setLayer(static_cast<DmLayer*>(nullptr));
    block->getEntityTable().add_direct(line);
    TextData attText(DmVector(0.0, 0.0), 1.0, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft, QStringLiteral("T"),
                     doc.getTextStyleTable()->getActive(), 0.0, EUpdateMode::NoUpdate);
    block->getEntityTable().add_direct(new DmAttributeDefinition(nullptr, attText, AttributeDefinitionData(
                                                                                         QStringLiteral("TAG"),
                                                                                         QStringLiteral("提示"))));

    // 插入点 (10,0)、比例 (2,3)、旋转 90°，两列、列距 5
    DmBlockReference insert(nullptr, DmBlockReferenceData(QStringLiteral("B"), DmVector(10.0, 0.0), DmVector(2.0, 3.0),
                                                          90.0 * kDeg, 2, 1, DmVector(5.0, 0.0), doc.getBlockTable(),
                                                          DM::NoUpdate));
    insert.setBlock(block);
    insert.setPen(DmPen(DmColor(255, 0, 0), DM::Width11, DmLineTypeTable::Continuous));
    // p → 插入点 + 旋转(缩放(p - 基点) + 阵列偏移)：线性部分 [0 2 -3 0]，平移 (13,-2)；第二格偏移 (5,0) 转 90° 为 (0,5)
    EXPECT_EQ(giDumpGeometry(insert),
              "drawShared B xf=[0 2 -3 0 13 -2] byBlock color=rgb(255,0,0) lineweight=50 linetype=Continuous\n"
              "drawShared B xf=[0 2 -3 0 13 3] byBlock color=rgb(255,0,0) lineweight=50 linetype=Continuous\n");

    // 块定义：实体各自嵌套绘制，属性定义不画（与块参照原先生成子实体时一致）
    EXPECT_EQ(giDumpGeometry(*block), "draw {\n"
                                      "  color ByBlock\n"
                                      "  lineweight ByBlock\n"
                                      "  linetype ByBlock\n"
                                      "  layer -\n"
                                      "  ---\n"
                                      "  polyline (1,1) (2,1)\n"
                                      "}\n");

    // 无效画笔的块参照：块里的 ByBlock 取外层
    insert.setPen(DmPen(DM::FlagInvalid));
    EXPECT_NE(giDumpGeometry(insert).find("byBlock color=ByBlock lineweight=ByBlock linetype=ByBlock"),
              std::string::npos);
}

TEST(GiEntityTest, 块参照的属性按各自的属性嵌套绘制)
{
    DmDocument doc;
    yicad_test::buildSample(doc);
    auto* insert = yicad_test::first<DmBlockReference>(*doc.getEntityTable(), DM::EntityBlockReference);
    ASSERT_NE(insert, nullptr);
    const std::string text = giDumpGeometry(*insert);
    EXPECT_EQ(text.rfind("drawShared " + yicad_test::kBlockName.toStdString() + " ", 0), 0u) << text;
    // 属性是单行文字：一个嵌套段，里面是字形串
    EXPECT_EQ(count(text, "\ndraw {\n"), 1) << text;
}

TEST(GiEntityTest, 标注与引线的各部分以它为父实体嵌套绘制)
{
    DmDocument doc;
    yicad_test::buildSample(doc);
    for (DM::EntityType type : { DM::EntityDimLinear, DM::EntityDimAligned, DM::EntityDimAngular,
                                 DM::EntityDimRadial, DM::EntityDimDiametric, DM::EntityDimLeader })
    {
        const DmEntity* e = yicad_test::first<DmEntity>(*doc.getEntityTable(), type);
        ASSERT_NE(e, nullptr) << type;
        const std::string text = giDumpGeometry(*e);
        // 顶层全是嵌套段：尺寸线、界线、箭头、文字容器
        EXPECT_EQ(text.rfind("draw {\n", 0), 0u) << type << "\n" << text;
        EXPECT_GE(count(text, "polyline"), 1) << type << "\n" << text;
        if (type != DM::EntityDimLeader)
        {
            // 箭头是块参照，块定义共享
            EXPECT_GE(count(text, "drawShared"), 1) << type << "\n" << text;
        }
    }
}

// ---------------------------------------------------------------------------
// 样本文档：每个实体都画出了东西，GI 流记录后重放一致
// ---------------------------------------------------------------------------

TEST(GiEntityTest, 样本文档的实体都有输出)
{
    DmDocument doc;
    yicad_test::buildSample(doc);
    for (const DmEntity* e : *doc.getEntityTable())
    {
        const std::string text = giDumpGeometry(*e);
        if (e->getEntityType() == DM::EntityHatch)
        {
            // 样本的填充以多段线为边界，见 DISABLED_多段线边界的实心填充交出环
            EXPECT_EQ(text, "") << "填充";
            continue;
        }
        EXPECT_FALSE(text.empty()) << "实体类型 " << e->getEntityType();
    }
}

TEST(GiEntityTest, 样本文档的实体经GI流记录后重放与直接绘制一致)
{
    DmDocument doc;
    yicad_test::buildSample(doc);
    for (const DmEntity* e : *doc.getEntityTable())
    {
        const GiStream stream = GiStreamRecorder::record(*e);
        EXPECT_EQ(giDump(GiStreamDrawable(stream)), giDump(*e)) << "实体类型 " << e->getEntityType();
    }
}
