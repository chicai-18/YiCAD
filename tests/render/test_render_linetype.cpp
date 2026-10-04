/// @file test_render_linetype.cpp
/// @brief 线型的显示语义（doc/RENDER_PLAN.md 第 4.5 节，阶段 5）
///
/// - 随层、随块线型（第 10 节阶段 5）：实体引用的是文档线型表里的 ByLayer、ByBlock 保留记录（与 AutoCAD/ODA 相同），
///   属性对话框、读盘、DXF 导入给出的都是这两条记录，图形系统按它们解析成图层、块参照的线型；
/// - LTSCALE 只改每帧常量，不重新编译（第 4.3.6 节）；
/// - 超长的虚线在 double 下分段（第 4.5.5 节）：与同样画法、不分段的线一致（头部、段的分界、尾部）；
/// - 闭合曲线的整周期（第 4.5.1 节，与 AutoCAD 核对过）：短于一个周期画实线，否则至少两个周期；
/// - 填充图案线按图案与相位画（第 4.5.1 节，GI 的 setLinePattern），与 Model 里逐段切好的划线一致。

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <numbers>
#include <vector>

#include <QImage>

#include "CmdManager.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmEntity.h"
#include "DmEntityContainer.h"
#include "DmHatch.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmPattern.h"
#include "DmRegion.h"
#include "DocumentCmd.h"
#include "EntityTable.h"
#include "GsCompiler.h"
#include "GsModel.h"
#include "GuiDocumentView.h"
#include "RenderHarness.h"
#include "ScopedTimer.h"
#include "Transaction.h"

namespace
{
using yicad_test::RenderRequest;
using yicad_test::RenderScene;
using yicad_test::visibleEntitiesOfType;

RenderRequest request(const char* drawing)
{
    RenderRequest r;
    r.drawing = QString::fromUtf8(drawing);
    return r;
}

/// @brief 在一个事务里改文档（提交后变更集交给图形模型）
void inTransaction(DmDocument& document, const std::function<void()>& change)
{
    Transaction t("test", &document);
    t.start();
    change();
    t.commit();
}

/// @brief 在作用域内开启埋点，退出时恢复
class ProfilerOn
{
public:
    ProfilerOn()
        : m_previous(yicad::Profiler::isEnabled())
    {
        yicad::Profiler::setEnabled(true);
    }
    ~ProfilerOn() { yicad::Profiler::setEnabled(m_previous); }

private:
    bool m_previous;
};

/// @brief 整图重建、编译与场景底图重画的次数
struct Counts
{
    long long regen = 0;
    long long compile = 0;
    long long scene = 0;

    static Counts now()
    {
        return {yicad::counters::regen().count(), yicad::counters::gsCompile().count(),
                yicad::counters::scene().count()};
    }

    Counts since(const Counts& before) const
    {
        return {regen - before.regen, compile - before.compile, scene - before.scene};
    }
};

/// @brief 同一状态下整图重建后的图，作为局部更新结果的对照
QImage fullRebuild(RenderScene& scene)
{
    scene.view().graphicsModel()->invalidate();
    return scene.grab();
}

/// @brief 经命令改 LTSCALE（同线型管理器）
void setLtscale(DmDocument& document, double scale)
{
    inTransaction(document, [&]() {
        QHash<QString, DmVariable> variables;
        variables.insert(QStringLiteral("$LTSCALE"), DmVariable(scale, 40));
        document.getCmdManager()->addAndExecuteCmd(new ModifyDocVariablesCmd(&document, variables));
    });
}

/// @brief 加一条线型为 lineType 的直线（经事务，变更集交给图形模型）
DmLine* addLine(DmDocument& document, const DmVector& a, const DmVector& b, DmLineType* lineType)
{
    auto* line = new DmLine(nullptr, a, b);
    line->setDocument(&document);
    DmPen pen = line->getPen(false);
    pen.setLineType(lineType);
    line->setPen(pen);
    inTransaction(document, [&]() { document.getEntityTable()->add(line); });
    return line;
}

/// @brief 一行像素里亮的连续段（划线）：起止列
/// @details 每列取 row 与上下各一行里最亮的：线正好落在两行像素之间时，每行都只有一半的亮度
std::vector<std::pair<int, int>> dashRuns(const QImage& image, int row)
{
    std::vector<std::pair<int, int>> runs;
    int start = -1;
    for (int x = 0; x < image.width(); ++x)
    {
        int gray = 0;
        for (int y = std::max(row - 1, 0); y <= std::min(row + 1, image.height() - 1); ++y)
        {
            gray = std::max(gray, qGray(image.pixel(x, y)));
        }
        const bool on = gray > 100;
        if (on && start < 0)
        {
            start = x;
        }
        if (!on && start >= 0)
        {
            runs.emplace_back(start, x - 1);
            start = -1;
        }
    }
    if (start >= 0)
    {
        runs.emplace_back(start, image.width() - 1);
    }
    return runs;
}

/// @brief 两行上的划线一一对应、两端相差不超过 1 个像素
/// @details 分段的相位按 double 算，不分段的对照线在着色器里按 float 算，划线两端会差零点几个像素；
///          相位算错时会偏好几个像素、段数也会变
void expectSameDashes(const char* name, const QImage& image, int expectedRow, int actualRow)
{
    const std::vector<std::pair<int, int>> expected = dashRuns(image, expectedRow);
    const std::vector<std::pair<int, int>> actual = dashRuns(image, actualRow);
    if (expected.empty() || actual.size() != expected.size())
    {
        image.save(QStringLiteral(YICAD_RENDER_OUTPUT_DIR "/%1.png").arg(QString::fromUtf8(name)));
    }
    ASSERT_FALSE(expected.empty()) << name;
    ASSERT_EQ(actual.size(), expected.size()) << name << "（图像写到了构建目录的 tests/render/output/）";
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_LE(std::abs(actual[i].first - expected[i].first), 1) << name << " 第 " << i << " 段起点";
        EXPECT_LE(std::abs(actual[i].second - expected[i].second), 1) << name << " 第 " << i << " 段终点";
    }
}

/// @brief 加一个线型为 lineType 的圆（经事务，变更集交给图形模型）
void addCircle(DmDocument& document, const DmVector& center, double radius, DmLineType* lineType)
{
    auto* circle = new DmCircle(nullptr, CircleData(center, radius));
    circle->setDocument(&document);
    DmPen pen = circle->getPen(false);
    pen.setLineType(lineType);
    circle->setPen(pen);
    inTransaction(document, [&]() { document.getEntityTable()->add(circle); });
}

/// @brief 画面中心、半径 radius（像素）的圆上空白的段数；圆没有画出来时为 -1
/// @details 逐个角度取样，每个样本取周围 3×3 里最亮的（线落在两个像素之间时各只有一半亮度）
int circleGaps(const QImage& image, double radius)
{
    constexpr int kSamples = 1440;
    const double cx = image.width() / 2.0;
    const double cy = image.height() / 2.0;
    std::vector<bool> ink(kSamples);
    for (int i = 0; i < kSamples; ++i)
    {
        const double angle = 2.0 * std::numbers::pi * i / kSamples;
        const int x = static_cast<int>(std::lround(cx + radius * std::cos(angle)));
        const int y = static_cast<int>(std::lround(cy + radius * std::sin(angle)));
        int gray = 0;
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (image.valid(x + dx, y + dy))
                {
                    gray = std::max(gray, qGray(image.pixel(x + dx, y + dy)));
                }
            }
        }
        ink[i] = gray > 100;
    }
    const auto first = std::find(ink.begin(), ink.end(), true);
    if (first == ink.end())
    {
        return -1;
    }
    // 从一个有墨的样本起绕一圈
    const int start = static_cast<int>(first - ink.begin());
    int gaps = 0;
    bool inGap = false;
    for (int k = 1; k <= kSamples; ++k)
    {
        const bool on = ink[(start + k) % kSamples];
        if (!on && !inGap)
        {
            ++gaps;
        }
        inGap = !on;
    }
    return gaps;
}
}  // namespace

TEST(RenderLinetypeTest, 改LTSCALE只改每帧常量不重新编译)
{
    ProfilerOn profiler;
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    const QImage before = scene.grab();

    Counts start = Counts::now();
    setLtscale(document, 0.5);
    const QImage changed = scene.grab();
    const Counts done = Counts::now().since(start);
    EXPECT_EQ(done.regen, 0) << "改 LTSCALE 不整图重建";
    EXPECT_EQ(done.compile, 0) << "没有超长虚线时不重新编译";
    EXPECT_EQ(done.scene, 1) << "场景底图重画一次";
    EXPECT_NE(changed, before) << "虚线变密，画面应当变了";
    EXPECT_DOUBLE_EQ(scene.view().graphicsModel()->globalLineTypeScale(), 0.5);
    yicad_test::expectIdenticalImage(QStringLiteral("linetype_ltscale_changed"), fullRebuild(scene), changed);

    // 撤销回到原来的画面
    document.getCmdManager()->undo();
    yicad_test::expectIdenticalImage(QStringLiteral("linetype_ltscale_undo"), before, scene.grab());
}

TEST(RenderLinetypeTest, 超长虚线分段后与不分段的同样画法一致)
{
    // DASHED 周期 0.75。长线 15000.3（20000.4 个周期，超过 16384 个周期，分两段，分界在 8192 个周期 = 6144 处）；
    // 对照线 11250.3（15000.4 个周期，不分段）与它同起点、余量相同（头部划线相同），中间各处的相位都相同；
    // 尾部对照线 750.3 与长线同终点。同一画面里三条线上下各差 1 个单位，裁出各自的横条逐像素比
    ProfilerOn profiler;
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    DmLineType* dashed = document.getLineTypeTable()->find(QStringLiteral("DASHED"));
    ASSERT_NE(dashed, nullptr);
    const double x0 = 1000.0;
    const double length = 15000.3;
    addLine(document, DmVector(x0, 0.0), DmVector(x0 + length, 0.0), dashed);
    addLine(document, DmVector(x0, 1.0), DmVector(x0 + length - 5000 * 0.75, 1.0), dashed);
    addLine(document, DmVector(x0 + length - 750.3, 2.0), DmVector(x0 + length, 2.0), dashed);

    // 每像素 0.02（划线 25 像素）；画面 640 × 480 的中心行对应 y = 1，三条线在 290、240、190 行
    const double wpp = 0.02;
    auto compareAt = [&](double x, int otherRow, const char* name) {
        scene.view().setView(DmVector(x, 1.0), wpp);
        expectSameDashes(name, scene.grab(), otherRow, 290);
    };
    compareAt(x0 + 2.0, 240, "头部划线");
    compareAt(x0 + 6144.0, 240, "段的分界");
    compareAt(x0 + 9000.3, 240, "第二段中间");
    compareAt(x0 + length - 2.0, 190, "尾部划线");

    // LTSCALE 改了：分了段的线所在的分块重新编译（段的相位按 LTSCALE 算），结果与整图重建一致
    const Counts start = Counts::now();
    setLtscale(document, 0.5);
    scene.view().setView(DmVector(x0 + 6144.0, 1.0), wpp);
    const QImage changed = scene.grab();
    const Counts done = Counts::now().since(start);
    EXPECT_EQ(done.regen, 0);
    EXPECT_GE(done.compile, 1) << "分了段的超长虚线要按新的 LTSCALE 重新分段";
    yicad_test::expectIdenticalImage(QStringLiteral("linetype_piece_ltscale"), fullRebuild(scene), changed);
}

TEST(RenderLinetypeTest, 闭合曲线短于一个周期画实线否则至少两个整周期)
{
    // 与 AutoCAD 2026 核对过（accoreconsole 把周长 0.95～3.55 个周期的一排圆打印成 PNG 后量出，RENDER_PLAN.md 第 4.5.1 节）：
    // 周长不到一个周期画实线；否则周期数取 round、至少 2 个，图案从起点等比伸缩。
    // DASHED 周期 0.75，每个周期一段空白，所以空白的段数就是周期数。参考图纸的容差比对看不出这几段空白，这里直接数
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    DmLineType* dashed = document.getLineTypeTable()->find(QStringLiteral("DASHED"));
    ASSERT_NE(dashed, nullptr);

    struct Case
    {
        double periods;  ///< 周长 / 周期
        int gaps;        ///< 空白的段数
    };
    const Case cases[] = {{0.8, 0}, {1.2, 2}, {2.4, 2}, {2.6, 3}};
    constexpr double kRadiusPixels = 150.0;
    for (std::size_t i = 0; i < std::size(cases); ++i)
    {
        // 放在参考图纸的内容之外，每个圆单独占满画面
        const double radius = cases[i].periods * 0.75 / (2.0 * std::numbers::pi);
        const DmVector center(1000.0 + 10.0 * static_cast<double>(i), 0.0);
        addCircle(document, center, radius, dashed);
        scene.view().setView(center, radius / kRadiusPixels);
        EXPECT_EQ(circleGaps(scene.grab(), kRadiusPixels), cases[i].gaps) << "周长 " << cases[i].periods << " 个周期";
    }
}

namespace
{
/// @brief 编译器用的上下文：线型序号 1，周期 0.75、第一段划线中点 0.25（DASHED 0.5、-0.25）
class LinetypeContext final : public GsCompileContext
{
public:
    std::uint16_t layerIndex(const DmLayer*) override { return kGsLayerNone; }
    std::uint16_t lineTypeIndex(const DmLineType*) override { return 1; }
    const std::vector<DmVector>& sampleNurbs(const GiNurbs&) override { return m_samples; }
    bool needsFlatten(const IGiDrawable&, const GiTransform&, const GsAttributes&, bool*) override { return false; }
    std::uint16_t patternIndex(const std::vector<double>&) override { return 2; }
    bool splitLongRuns() const override { return true; }
    double globalLineTypeScale() const override { return ltscale; }
    bool lineTypeMetrics(const GsLineTypeRef& lineType, double& period, double& firstDashCenter) override
    {
        if (lineType.kind != GsKind::Value || lineType.index != 1)
        {
            return false;
        }
        period = 0.75;
        firstDashCenter = 0.25;
        return true;
    }
    double ltscale = 1.0;

private:
    std::vector<DmVector> m_samples;
};
}  // namespace

TEST(RenderLinetypeTest, 编译器按double分段并给出各段的参数)
{
    DmLineType dashed(DmLineTypeData(QStringLiteral("DASHED"), QString(), QString(), {0.5, -0.25}));
    DmLine line(DmVector(0.0, 0.0), DmVector(15000.3, 0.0));
    line.setPen(DmPen(DmColor(255, 0, 0), DM::Width00, &dashed));
    LinetypeContext context;
    GsCompiler compiler(context);
    GsCompiled out;
    compiler.compileNode(line, 0, DmVector(0.0, 0.0), out);

    // 20000.4 个周期：两段，分界在 8192 个周期（6144）处；余量 0.4 个周期，头部划线到 0.15
    ASSERT_EQ(out.prims.size(), 2u);
    EXPECT_TRUE(out.hasPieces);
    ASSERT_EQ(out.runs.size(), 1u);
    EXPECT_DOUBLE_EQ(out.runs[0].length, 15000.3);
    const GsPrimRecord& first = out.prims[0];
    const GsPrimRecord& second = out.prims[1];
    EXPECT_EQ(first.kinds & (kGsKindsPiece | kGsKindsRunStart | kGsKindsRunEnd), kGsKindsPiece | kGsKindsRunStart);
    EXPECT_EQ(second.kinds & (kGsKindsPiece | kGsKindsRunStart | kGsKindsRunEnd), kGsKindsPiece | kGsKindsRunEnd);
    EXPECT_FLOAT_EQ(first.runLength, 6144.0f);
    EXPECT_FLOAT_EQ(second.runLength, static_cast<float>(15000.3 - 6144.0));
    // 第一段：相位 = 第一段划线中点 - 头部终点 = 0.25 - 0.15；头部终点 0.15；尾部起点在段外
    EXPECT_NEAR(first.dash[0], 0.1, 1.0e-5);
    EXPECT_NEAR(first.dash[1], 0.15, 1.0e-5);
    EXPECT_GT(first.dash[2], first.runLength);
    // 第二段：6144 正好是整数个周期，相位同第一段；头部终点为负；尾部起点 = 15000.3 - 0.15 - 6144
    EXPECT_NEAR(second.dash[0], 0.1, 1.0e-5);
    EXPECT_LT(second.dash[1], 0.0f);
    EXPECT_NEAR(second.dash[2], 15000.3 - 0.15 - 6144.0, 1.0e-3);
    // 两段的点：每段两个，弧长参数各自从 0 起
    const auto& points = out.records[static_cast<std::size_t>(GsClass::Segment)];
    ASSERT_EQ(points.size(), 4u);
    EXPECT_FLOAT_EQ(points[0].z, 0.0f);
    EXPECT_FLOAT_EQ(points[1].z, 6144.0f);
    EXPECT_FLOAT_EQ(points[2].x, 6144.0f);
    EXPECT_FLOAT_EQ(points[2].z, 0.0f);

    // LTSCALE 为 2 时周期 1.5，10000.2 个周期，不分段
    context.ltscale = 2.0;
    out.clear();
    compiler.compileNode(line, 0, DmVector(0.0, 0.0), out);
    EXPECT_EQ(out.prims.size(), 1u);
    EXPECT_FALSE(out.hasPieces);
    EXPECT_EQ(out.prims[0].kinds & kGsKindsPiece, 0u);
}

TEST(RenderLinetypeTest, 实体线型为线型表里的随层记录时按图层的线型画)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    const QImage expected = scene.grab();

    // 每个实体搬到一个以它自己的线型为线型的新图层上，线型改成线型表里的 ByLayer 记录
    // （属性对话框的线型下拉框给出的就是这条记录）；颜色、线宽照 0 层
    DmLineType* byLayer = document.getLineTypeTable()->find(LineType::ByLayer);
    ASSERT_NE(byLayer, nullptr);
    DmLayer* layer0 = document.getLayerTable()->find(QStringLiteral("0"));
    ASSERT_NE(layer0, nullptr);
    const std::vector<DmEntity*> entities = visibleEntitiesOfType(
        document, {DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse, DM::EntityPolyline});
    ASSERT_FALSE(entities.empty());
    inTransaction(document, [&]() {
        std::map<DmLineType*, DmLayer*> layers;
        for (DmEntity* e : entities)
        {
            DmPen pen = e->getPen(false);
            DmLineType* lineType = pen.getLineType();
            DmLayer*& layer = layers[lineType];
            if (!layer)
            {
                layer = new DmLayer();
                layer->setDocument(&document);
                DmPen layerPen = layer0->getPen();
                layerPen.setLineType(lineType);
                layer->setData(DmLayerData(QStringLiteral("L_") + lineType->getLineTypeName(), layerPen, false, false));
                ASSERT_TRUE(document.getLayerTable()->add_direct(layer));
            }
            document.getEntityTable()->startModify(e);
            e->setLayer(layer);
            pen.setLineType(byLayer);
            e->setPen(pen);
        }
    });
    yicad_test::expectIdenticalImage(QStringLiteral("linetype_bylayer_record"), expected, scene.grab());
}

TEST(RenderLinetypeTest, 块里实体线型为线型表里的随块记录时按块参照的线型画)
{
    RenderScene scene(request("blocks.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    const QImage expected = scene.grab();

    // 块 DASHED_TRI 里的三角形自带 DASHED：改成线型表里的 ByBlock 记录，两个块参照的线型改成 DASHED
    DmBlock* block = document.getBlockTable()->find(QStringLiteral("DASHED_TRI"));
    ASSERT_NE(block, nullptr);
    DmLineType* byBlock = document.getLineTypeTable()->find(LineType::ByBlock);
    ASSERT_NE(byBlock, nullptr);
    DmLineType* dashed = nullptr;
    inTransaction(document, [&]() {
        for (DmEntity* e : block->getEntityTable())
        {
            DmPen pen = e->getPen(false);
            dashed = pen.getLineType();
            block->getEntityTable().startModify(e);
            pen.setLineType(byBlock);
            e->setPen(pen);
        }
    });
    ASSERT_NE(dashed, nullptr);
    ASSERT_EQ(dashed->getLineTypeName(), QStringLiteral("DASHED"));
    // 块参照的线型还是随层（0 层为连续线），三角形画成实线，画面应当变了：确认块里的修改到了图形系统
    const QImage middle = scene.grab();
    EXPECT_NE(middle, expected) << "改了块里的线型，画面却没变";
    int inserts = 0;
    inTransaction(document, [&]() {
        for (DmEntity* e : visibleEntitiesOfType(document, {DM::EntityBlockReference}))
        {
            auto* insert = static_cast<DmBlockReference*>(e);
            if (insert->getName() != QStringLiteral("DASHED_TRI"))
            {
                continue;
            }
            DmPen pen = insert->getPen(false);
            document.getEntityTable()->startModify(insert);
            pen.setLineType(dashed);
            insert->setPen(pen);
            insert->update();
            ++inserts;
        }
    });
    EXPECT_EQ(inserts, 2);
    const QImage after = scene.grab();
    yicad_test::expectIdenticalImage(QStringLiteral("linetype_byblock_record"), expected, after);
}

namespace
{
/// @brief 加一个图案填充，并把 Model 里同一算法逐段切好的划线（getFilledEntities）上移 10 个单位、按实线加进文档作对照
/// @details 图形系统按图案与相位画整段的图案线（第 4.5.1 节），两边的划线位置应当一致
DmHatch* addPatternHatchWithCutDashes(DmDocument& document, DmPattern& pattern, DmEntityContainerPtr outer,
                                      const std::vector<DmEntityContainerPtr>& holes)
{
    HatchData data(false, 1.0, 0.0, &pattern);
    data.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(outer, holes)));
    auto* hatch = new DmHatch(nullptr, data);
    hatch->setDocument(&document);
    hatch->update();
    inTransaction(document, [&]() {
        document.getEntityTable()->add(hatch);
        for (DmEntity* dash : hatch->getSubEntities())
        {
            DmEntity* copy = dash->clone();
            copy->setParent(nullptr);
            copy->setDocument(&document);
            copy->move(DmVector(0.0, 10.0));
            document.getEntityTable()->add(copy);
        }
    });
    return hatch;
}
}  // namespace

TEST(RenderLinetypeTest, 填充图案线按图案与相位画且与逐段切好的划线一致)
{
    // 图案线水平、过 (100.3, 0)、行距 1，划线 0.5、空白 0.25；边界 [100, 104] × [0, 4]
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    DmPattern pattern;
    pattern.setPatternData({{0.0, 100.3, 0.0, 0.0, 1.0, 0.5, -0.25}});
    auto boundary = std::make_shared<DmEntityContainer>(nullptr);
    boundary->addEntity(new DmLine(DmVector(100.0, 0.0), DmVector(104.0, 0.0)));
    boundary->addEntity(new DmLine(DmVector(104.0, 0.0), DmVector(104.0, 4.0)));
    boundary->addEntity(new DmLine(DmVector(104.0, 4.0), DmVector(100.0, 4.0)));
    boundary->addEntity(new DmLine(DmVector(100.0, 4.0), DmVector(100.0, 0.0)));
    DmHatch* hatch = addPatternHatchWithCutDashes(document, pattern, boundary, {});
    ASSERT_FALSE(hatch->getPatternRuns().empty());

    // 每像素 0.025：画面中心行对应 y = 7，y = 2 的图案线在 440 行，它的对照在 40 行
    scene.view().setView(DmVector(102.0, 7.0), 0.025);
    expectSameDashes("填充图案线", scene.grab(), 40, 440);
}

TEST(RenderLinetypeTest, 圆环图案填充穿过孔洞的图案线分两段画)
{
    // 外边界圆心 (102, 2) 半径 4，孔洞半径 2；图案线水平、过 (102.3, 0)、行距 1，划线 0.5、空白 0.25。
    // 穿过孔洞的图案线分成两段，各段的相位都锚定在图案原点
    RenderRequest r = request("linetypes.dxf");
    r.height = 800;
    RenderScene scene(r);
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    DmPattern pattern;
    pattern.setPatternData({{0.0, 102.3, 0.0, 0.0, 1.0, 0.5, -0.25}});
    auto outer = std::make_shared<DmEntityContainer>(nullptr);
    outer->addEntity(new DmCircle(outer.get(), CircleData(DmVector(102.0, 2.0), 4.0)));
    auto hole = std::make_shared<DmEntityContainer>(nullptr);
    hole->addEntity(new DmCircle(hole.get(), CircleData(DmVector(102.0, 2.0), 2.0)));
    DmHatch* hatch = addPatternHatchWithCutDashes(document, pattern, outer, {hole});
    // y = 2 的图案线穿过孔洞，分成两段
    int throughHole = 0;
    for (const DmHatchPatternRun& run : hatch->getPatternRuns())
    {
        if (std::abs(run.start.y - 2.0) < 1.0e-9)
        {
            ++throughHole;
        }
    }
    EXPECT_EQ(throughHole, 2);

    // 每像素 0.025、画面高 800：中心行 400 对应 y = 7；y = 2（穿过孔洞）在 600 行、对照在 200 行，
    // y = 5（只穿过外圆）在 480 行、对照在 80 行
    scene.view().setView(DmVector(102.0, 7.0), 0.025);
    const QImage image = scene.grab();
    const std::vector<std::pair<int, int>> throughHoleDashes = dashRuns(image, 600);
    EXPECT_GE(throughHoleDashes.size(), 4u);
    expectSameDashes("圆环穿过孔洞的图案线", image, 200, 600);
    expectSameDashes("圆环只穿过外圆的图案线", image, 80, 480);
}
