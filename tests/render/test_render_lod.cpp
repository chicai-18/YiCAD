/// @file test_render_lod.cpp
/// @brief 按屏幕尺寸简化显示（doc/RENDER_PLAN.md 第 4.3.10 节，阶段 6）
///
/// - 小字：字高小于约 2 像素时字形不画，画沿基线、跨整个字形串的细条；
/// - 亚像素对象：包围框小于 1 像素时画成一个点（否则可能一个采样也盖不到）；
/// - 小圆弧：屏幕半径很小时只画一个四边形，结果与画 8 段环带逐像素一致；
/// - 密填充：图案线距小于约 2 像素时图案线不画，按平均覆盖率画实心（抖动写采样掩码），平均亮度与逐条画线相当。
/// 判断都在着色器里按每帧常量做，画布的 setLevelOfDetail(false) 关掉它们作对照。
///
/// 另有阶段 6 的大图纸各项：
/// - 整图重建时 GI 流的记录与分块的编译在多个线程上并行（第 4.3.11 节），结果与顺序执行逐像素一致；
/// - 样条与椭圆按弦高容差离散，放大到弦高在屏幕上超过半个像素时重新离散（样条在后台线程上，先用旧结果画）。

#include <gtest/gtest.h>

#include <cmath>
#include <functional>
#include <vector>

#include <QElapsedTimer>
#include <QImage>
#include <QThread>

#include "DmArc.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmEntityContainer.h"
#include "DmHatch.h"
#include "DmLine.h"
#include "DmPattern.h"
#include "DmRegion.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "EntityTable.h"
#include "GsModel.h"
#include "GsParallel.h"
#include "GuiDocumentView.h"
#include "RenderHarness.h"
#include "Transaction.h"
#include "DmText.h"

namespace
{
using yicad_test::RenderRequest;
using yicad_test::RenderRequirement;
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

/// @brief 把实体加进文档（经事务）
template <typename T>
T* addEntity(DmDocument& document, T* entity, const DmColor& color)
{
    entity->setDocument(&document);
    DmPen pen = entity->getPen(false);
    pen.setColor(color);
    entity->setPen(pen);
    entity->update();
    inTransaction(document, [&]() { document.getEntityTable()->add(entity); });
    return entity;
}

/// @brief 像素与背景有明显差别
bool colored(const QImage& image, int x, int y, QRgb background)
{
    if (x < 0 || y < 0 || x >= image.width() || y >= image.height())
    {
        return false;
    }
    const QRgb p = image.pixel(x, y);
    return std::abs(qRed(p) - qRed(background)) > 40 || std::abs(qGreen(p) - qGreen(background)) > 40
        || std::abs(qBlue(p) - qBlue(background)) > 40;
}

/// @brief (x, y) 周围 radius 像素内有没有着色的像素
bool coloredNear(const QImage& image, int x, int y, int radius, QRgb background)
{
    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            if (colored(image, x + dx, y + dy, background))
            {
                return true;
            }
        }
    }
    return false;
}

/// @brief 把图存到构建目录的 tests/render/output/（失败时查看）
void saveImage(const char* name, const QImage& image)
{
    image.save(QStringLiteral(YICAD_RENDER_OUTPUT_DIR "/%1.png").arg(QString::fromUtf8(name)));
}

/// @brief 矩形边界（四条直线）
DmEntityContainerPtr rectangle(double x0, double y0, double x1, double y1)
{
    auto boundary = std::make_shared<DmEntityContainer>(nullptr);
    boundary->addEntity(new DmLine(DmVector(x0, y0), DmVector(x1, y0)));
    boundary->addEntity(new DmLine(DmVector(x1, y0), DmVector(x1, y1)));
    boundary->addEntity(new DmLine(DmVector(x1, y1), DmVector(x0, y1)));
    boundary->addEntity(new DmLine(DmVector(x0, y1), DmVector(x0, y0)));
    return boundary;
}

/// @brief 区域里红色的平均覆盖率：红色通道相对背景的升高 ÷ 纯红能升高的量
double redCoverage(const QImage& image, int x0, int y0, int x1, int y1, QRgb background)
{
    double sum = 0.0;
    int count = 0;
    for (int y = y0; y < y1; ++y)
    {
        for (int x = x0; x < x1; ++x)
        {
            sum += (qRed(image.pixel(x, y)) - qRed(background)) / double(255 - qRed(background));
            ++count;
        }
    }
    return count > 0 ? sum / count : 0.0;
}
}  // namespace

TEST(RenderLodTest, 小字在字高不足阈值时画成跨整个字形串的细条)
{
    RenderRequest r = request("text_truetype.dxf");
    r.requirement = RenderRequirement::TrueTypeFont;
    QString reason;
    if (!yicad_test::requirementMet(r.requirement, &reason))
    {
        GTEST_SKIP() << reason.toStdString();
    }
    RenderScene scene(r);
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    const std::vector<DmEntity*> texts = visibleEntitiesOfType(document, {DM::EntityText});
    ASSERT_FALSE(texts.empty());
    // 第一行单行文字字高 0.5；改成两头各一个 I、中间一串空格：画字形时中间是空的，画细条时中间连着
    auto* text = static_cast<DmText*>(texts.front());
    inTransaction(document, [&]() {
        document.getEntityTable()->startModify(text);
        text->setText(QStringLiteral("I                I"));
        text->update();
        // 挪离原点：原点标记画在叠加层里，会盖住画面中心
        text->move(DmVector(100.0, 100.0));
    });
    const DmVector center = (text->getMin() + text->getMax()) * 0.5;
    const int cx = r.width / 2;
    const int cy = r.height / 2;

    // 字高 1.5 像素：细条连着中间
    scene.view().setView(center, 0.5 / 1.5);
    QImage image = scene.grab();
    const QRgb background = image.pixel(0, 0);
    saveImage("lod_text_bar", image);
    EXPECT_TRUE(coloredNear(image, cx, cy, 1, background)) << "字高 1.5 像素时中间应有细条";

    // 字高 3 像素：画字形，中间是空格
    scene.view().setView(center, 0.5 / 3.0);
    image = scene.grab();
    saveImage("lod_text_glyphs", image);
    EXPECT_FALSE(coloredNear(image, cx, cy, 1, background)) << "字高 3 像素时中间是空格";

    // 关掉简化：字高 1.5 像素也画字形
    scene.view().setLevelOfDetail(false);
    scene.view().setView(center, 0.5 / 1.5);
    image = scene.grab();
    EXPECT_FALSE(coloredNear(image, cx, cy, 1, background)) << "不简化时中间是空格";
}

TEST(RenderLodTest, 亚像素对象画成一个点)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    // 每像素 1 个单位：0.2 见方的三角形、长 0.3 的直线都远小于一个像素
    addEntity(document,
              new DmSolid(nullptr, SolidData({DmVector(1000.0, 0.0), DmVector(1000.2, 0.0), DmVector(1000.0, 0.2)})),
              DmColor(255, 0, 0));
    addEntity(document, new DmLine(nullptr, DmVector(1020.0, 0.0), DmVector(1020.3, 0.0)), DmColor(255, 0, 0));
    scene.view().setView(DmVector(1010.0, 0.0), 1.0);
    const QImage image = scene.grab();
    const QRgb background = image.pixel(0, 0);
    const int cy = scene.view().height() / 2;
    const int cx = scene.view().width() / 2;
    EXPECT_TRUE(coloredNear(image, cx - 10, cy, 2, background)) << "亚像素的三角形";
    EXPECT_TRUE(coloredNear(image, cx + 10, cy, 2, background)) << "亚像素的直线";
}

TEST(RenderLodTest, 小圆弧只画一个四边形与画环带逐像素一致)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    // 半径 0.3～6 像素的圆与圆弧（每像素 1 个单位），跨过"只画一个四边形"的阈值（4 像素）与"亚像素"的阈值（1 像素）
    const double radii[] = {0.3, 0.8, 1.5, 2.5, 3.5, 4.5, 6.0};
    double x = 2000.0;
    for (double radius : radii)
    {
        addEntity(document, new DmCircle(nullptr, CircleData(DmVector(x, 0.0), radius)), DmColor(255, 255, 0));
        addEntity(document,
                  new DmArc(nullptr, ArcData(DmVector(x, 20.0), DmVector(0.0, 0.0, 1.0), radius, 0.3, 2.2)),
                  DmColor(0, 255, 255));
        x += 20.0;
    }
    for (bool lineWidth : {false, true})
    {
        scene.view().setDraftMode(lineWidth);
        scene.view().setView(DmVector(2060.0, 10.0), 1.0);
        scene.view().setLevelOfDetail(false);
        const QImage rings = scene.grab();
        scene.view().setLevelOfDetail(true);
        const QImage quads = scene.grab();
        yicad_test::expectIdenticalImage(lineWidth ? QStringLiteral("lod_small_arcs_lineweight")
                                                   : QStringLiteral("lod_small_arcs"),
                                         rings, quads);
    }
}

TEST(RenderLodTest, 密填充在线距不足阈值时按覆盖率画实心)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    // 两块 40 × 40 的填充：水平实线、行距 1；水平虚线（划线 2、空白 2，占周期的一半；周期在屏幕上不算过密）、行距 1
    DmPattern solid;
    solid.setPatternData({{0.0, 0.0, 0.0, 0.0, 1.0}});
    DmPattern dashed;
    dashed.setPatternData({{0.0, 0.0, 0.0, 0.0, 1.0, 2.0, -2.0}});
    auto addHatch = [&](DmPattern& pattern, double x0) {
        HatchData data(false, 1.0, 0.0, &pattern);
        data.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(rectangle(x0, 0.0, x0 + 40.0, 40.0), {})));
        addEntity(document, new DmHatch(nullptr, data), DmColor(255, 0, 0));
    };
    addHatch(solid, 3000.0);
    addHatch(dashed, 3050.0);

    // 线距 3 像素（不密）：简化与否逐像素一致（画图案线，替身不画）
    scene.view().setView(DmVector(3045.0, 20.0), 1.0 / 3.0);
    scene.view().setLevelOfDetail(false);
    const QImage linesOff = scene.grab();
    scene.view().setLevelOfDetail(true);
    const QImage linesOn = scene.grab();
    yicad_test::expectIdenticalImage(QStringLiteral("lod_hatch_sparse"), linesOff, linesOn);

    // 线距 1.25 像素（密）：简化时按覆盖率画实心，平均覆盖率与逐条画线相当（实线约 1/1.25）
    const double wpp = 0.8;
    scene.view().setView(DmVector(3045.0, 20.0), wpp);
    scene.view().setLevelOfDetail(false);
    const QImage denseOff = scene.grab();
    scene.view().setLevelOfDetail(true);
    const QImage denseOn = scene.grab();
    const QRgb background = denseOn.pixel(0, 0);
    // 世界坐标 -> 像素：画面中心对应 (3045, 20)
    auto px = [&](double worldX) { return static_cast<int>(std::lround(scene.view().width() / 2 + (worldX - 3045.0) / wpp)); };
    auto py = [&](double worldY) { return static_cast<int>(std::lround(scene.view().height() / 2 - (worldY - 20.0) / wpp)); };
    // 只取内部（离边界 5 像素以上）
    const double onSolid = redCoverage(denseOn, px(3000.0) + 5, py(40.0) + 5, px(3040.0) - 5, py(0.0) - 5, background);
    const double offSolid = redCoverage(denseOff, px(3000.0) + 5, py(40.0) + 5, px(3040.0) - 5, py(0.0) - 5, background);
    const double onDashed = redCoverage(denseOn, px(3050.0) + 5, py(40.0) + 5, px(3090.0) - 5, py(0.0) - 5, background);
    const double offDashed = redCoverage(denseOff, px(3050.0) + 5, py(40.0) + 5, px(3090.0) - 5, py(0.0) - 5, background);
    EXPECT_NEAR(onSolid, 0.8, 0.1);
    EXPECT_NEAR(onSolid, offSolid, 0.15);
    // 虚线：划线占一半，另加每条划线两端的圆头（周期 5 像素里一个像素），(0.5 + 0.2) × 0.8
    EXPECT_NEAR(onDashed, 0.56, 0.1);
    EXPECT_NEAR(onDashed, offDashed, 0.15);
}

namespace
{
/// @brief 画到图形模型没有还没换上的重新离散为止（后台在算时等一会儿再画），返回最后一帧；out 记下是否等过后台
QImage grabUntilRefined(RenderScene& scene, bool* waited = nullptr)
{
    QImage image = scene.grab();
    QElapsedTimer timer;
    timer.start();
    while (scene.view().graphicsModel()->refinementPending() && timer.elapsed() < 10000)
    {
        if (waited)
        {
            *waited = true;
        }
        QThread::msleep(5);
        image = scene.grab();
    }
    EXPECT_FALSE(scene.view().graphicsModel()->refinementPending()) << "10 秒内没有离散完";
    return image;
}
}  // namespace

TEST(RenderLodTest, 椭圆放大到弦高超过半个像素时重新离散)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    // 半径 1000 的整椭圆（长短轴相等）：默认 120 段，每段 3°，弦高 1000 × (1 - cos 1.5°) ≈ 0.34
    const DmVector center(5000.0, 0.0);
    addEntity(document,
              new DmEllipse(nullptr, EllipseData(center, DmVector(1000.0, 0.0), DmVector(0.0, 0.0, 1.0), 1.0, true, 0.0,
                                                 2.0 * M_PI)),
              DmColor(255, 255, 0));
    // 画面中心对准椭圆上 91.5° 处（两个分段点 90°、93° 正中，折线离曲线最远），每像素 0.01：默认的折线离中心约 34 像素
    const double angle = 91.5 * M_PI / 180.0;
    const DmVector onCurve = center + DmVector(std::cos(angle), std::sin(angle)) * 1000.0;
    scene.view().setView(onCurve, 0.01);
    const int cx = scene.view().width() / 2;
    const int cy = scene.view().height() / 2;
    const QImage coarse = scene.grab();
    const QRgb background = coarse.pixel(0, 0);
    EXPECT_FALSE(coloredNear(coarse, cx, cy, 2, background)) << "第一帧用默认的折线，离曲线约 34 像素";
    const QImage fine = grabUntilRefined(scene);
    EXPECT_TRUE(coloredNear(fine, cx, cy, 1, background)) << "重新离散后折线贴着曲线";

    // 缩小回去：默认容差够用了，恢复默认的折线，与没放大过的模型逐像素一致
    scene.view().setView(center, 5.0);
    const QImage zoomedOut = grabUntilRefined(scene);
    scene.view().graphicsModel()->invalidate();
    yicad_test::expectIdenticalImage(QStringLiteral("lod_refine_restored"), scene.grab(), zoomedOut);
}

TEST(RenderLodTest, 样条放大到弦高超过半个像素时在后台重新离散)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    // 一条跨 2000 个单位的三次样条：默认弦高容差 2000 × 2e-4 = 0.4，每像素 0.001 时是 400 像素
    auto* spline = new DmSpline(nullptr, SplineData(3, false));
    DmSpline::setControlPointsKnotsByClose(spline, false,
                                           {DmVector(6000.0, 0.0), DmVector(6500.0, 800.0), DmVector(7500.0, 800.0),
                                            DmVector(8000.0, 0.0)});
    addEntity(document, spline, DmColor(0, 255, 255));
    // 画面中心对准曲线上的一点（参数在定义域的 0.37 处，不是分段点）
    const GiNurbs curve = spline->toNurbs();
    double t1 = 0.0;
    double t2 = 1.0;
    curve.domain(t1, t2);
    const DmVector onCurve = curve.evaluate(t1 + (t2 - t1) * 0.37);
    scene.view().setView(onCurve, 0.001);
    bool waited = false;
    const QImage fine = grabUntilRefined(scene, &waited);
    EXPECT_TRUE(waited) << "样条在后台离散，第一帧先用旧结果画";
    const QRgb background = fine.pixel(0, 0);
    EXPECT_TRUE(coloredNear(fine, scene.view().width() / 2, scene.view().height() / 2, 1, background))
        << "重新离散后折线贴着曲线";
}

namespace
{
/// @brief 在 (10000, 0) 起 600 × 300 的范围里加两万个直线、圆、圆弧、样条（分块有十几个）
void addManyEntities(DmDocument& document)
{
    inTransaction(document, [&]() {
        for (int i = 0; i < 20000; ++i)
        {
            const double x = 10000.0 + (i % 200) * 3.0;
            const double y = (i / 200) * 3.0;
            DmEntity* e = nullptr;
            switch (i % 4)
            {
            case 0: e = new DmLine(nullptr, DmVector(x, y), DmVector(x + 2.0, y + 1.0)); break;
            case 1: e = new DmCircle(nullptr, CircleData(DmVector(x + 1.0, y + 1.0), 0.8)); break;
            case 2: e = new DmArc(nullptr, ArcData(DmVector(x + 1.0, y + 1.0), DmVector(0.0, 0.0, 1.0), 1.0, 0.2, 2.5)); break;
            default:
            {
                auto* s = new DmSpline(nullptr, SplineData(3, false));
                DmSpline::setControlPointsKnotsByClose(s, false, {DmVector(x, y), DmVector(x + 0.7, y + 2.0),
                                                                  DmVector(x + 1.4, y - 1.0), DmVector(x + 2.0, y + 1.0)});
                e = s;
                break;
            }
            }
            e->setDocument(&document);
            e->update();
            document.getEntityTable()->add(e);
        }
    });
}
}  // namespace

TEST(RenderLodTest, 多线程编译与单线程编译逐像素一致)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    // 整图重建时各线程分着编译十几个分块
    addManyEntities(document);
    scene.view().setView(DmVector(10300.0, 150.0), 1.0);
    ASSERT_GT(gsWorkerCount(), 0u);
    gsSetMaxWorkers(1);
    scene.view().graphicsModel()->invalidate();
    const QImage serial = scene.grab();
    gsSetMaxWorkers(0);
    scene.view().graphicsModel()->invalidate();
    const QImage parallel = scene.grab();
    yicad_test::expectIdenticalImage(QStringLiteral("lod_parallel_compile"), serial, parallel);
}

TEST(RenderLodTest, 场景超出预算时分几帧先粗后细画完与一次画完逐像素一致)
{
    RenderScene scene(request("linetypes.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    addManyEntities(document);
    // 一条横贯的长直线：松散四叉树里在浅层的分块，渐进绘制的第一批就有它
    addEntity(document, new DmLine(nullptr, DmVector(9950.0, 151.5), DmVector(10650.0, 151.5)), DmColor(255, 0, 255));
    scene.view().setView(DmVector(10300.0, 150.0), 1.0);
    const QImage whole = scene.grab();
    ASSERT_TRUE(scene.view().isSceneComplete());

    // 每帧最多画 5 万个顶点：场景要分几帧画；第一帧先画大的
    scene.view().setSceneBudget(50000);
    scene.view().graphicsModel()->invalidate();
    const QImage first = scene.grab();
    EXPECT_FALSE(scene.view().isSceneComplete()) << "超出预算，第一帧只画了一部分";
    const QRgb background = first.pixel(0, 0);
    const int cy = scene.view().height() / 2 - 1;  // y = 151.5 在中心行之上 1.5 像素
    EXPECT_TRUE(coloredNear(first, 5, cy, 1, background)) << "第一批画浅层分块里的长直线";
    int frames = 1;
    QImage last = first;
    while (!scene.view().isSceneComplete() && frames < 500)
    {
        last = scene.grab();
        ++frames;
    }
    EXPECT_TRUE(scene.view().isSceneComplete());
    EXPECT_GT(frames, 2) << "分了几帧画";
    yicad_test::expectIdenticalImage(QStringLiteral("lod_progressive"), whole, last);
}
