/// @file test_render_incremental.cpp
/// @brief 图形系统的增量更新（doc/RENDER_PLAN.md 第 4.3.6～4.3.9 节）：改实体只处理变更集，选择集只改对象状态，
///        高亮集只重画叠加通道，临时隐藏与预览变换不重新编译，场景底图只在作废时重画，图片纹理跨重新编译复用
///
/// 局部更新的结果与同一状态下整图重建的结果比对：只看计数器会漏掉"没重建、画面没变"的错误，
/// 只看图像会漏掉"又整图重建了一次"的退步，所以两样都查。计数器要开着埋点才计数。
/// （阶段 1 的这组用例测的是旧渲染器的选中组、高亮组；第 4 阶段旧渲染器删除后改测图形系统。）

#include <gtest/gtest.h>

#include <cstdlib>
#include <functional>
#include <vector>

#include <QColor>
#include <QImage>

#include "CmdManager.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLine.h"
#include "DmSettings.h"
#include "EntityTable.h"
#include "GiTransform.h"
#include "GsDevice.h"
#include "GsModel.h"
#include "GsView.h"
#include "GuiDocumentView.h"
#include "RenderHarness.h"
#include "ScopedTimer.h"
#include "Transaction.h"

namespace
{
using yicad_test::RenderRequest;
using yicad_test::RenderScene;
using yicad_test::visibleEntitiesOfType;

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

/// @brief 整图重建、局部更新、编译与场景底图重画的次数
struct Counts
{
    long long regen = 0;           ///< GsModel 全部重建
    long long regenSelection = 0;  ///< 选择集变化：改对象状态
    long long regenHighlight = 0;  ///< 高亮集变化：重收集叠加通道的高亮
    long long changes = 0;         ///< 处理变更集
    long long compile = 0;         ///< 编译分块、共享几何
    long long scene = 0;           ///< 场景底图重画

    static Counts now()
    {
        return {yicad::counters::regen().count(),     yicad::counters::regenSelection().count(),
                yicad::counters::regenHighlight().count(), yicad::counters::gsChanges().count(),
                yicad::counters::gsCompile().count(), yicad::counters::scene().count()};
    }

    Counts since(const Counts& before) const
    {
        return {regen - before.regen,     regenSelection - before.regenSelection,
                regenHighlight - before.regenHighlight, changes - before.changes,
                compile - before.compile, scene - before.scene};
    }
};

RenderRequest request(const char* drawing)
{
    RenderRequest r;
    r.drawing = QString::fromUtf8(drawing);
    return r;
}

/// @brief 同一状态下整图重建后的图，作为局部更新结果的对照
QImage fullRebuild(RenderScene& scene)
{
    scene.view().graphicsModel()->invalidate();
    return scene.grab();
}

/// @brief 在一个事务里改文档（提交后变更集交给图形模型）
void inTransaction(DmDocument& document, const std::function<void()>& change)
{
    Transaction t("test", &document);
    t.start();
    change();
    t.commit();
}

}  // namespace

TEST(RenderIncrementalTest, 选择集变化只改对象状态且与整图重建的图一致)
{
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    const QImage unselected = scene.grab();

    scene.selection().entities = visibleEntitiesOfType(scene.document(), {DM::EntityCircle, DM::EntityBlockReference});
    ASSERT_FALSE(scene.selection().entities.empty());
    Counts before = Counts::now();
    scene.view().specifySelectChanged();
    const QImage selected = scene.grab();
    Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.compile, 0) << "选中只改对象状态，不重新编译几何";
    EXPECT_EQ(done.regenSelection, 1);
    EXPECT_EQ(done.scene, 1) << "选中的画在场景里，选择集变了要重画";
    yicad_test::expectSameImage(QStringLiteral("incremental_select"), fullRebuild(scene), selected);

    // 取消选择：对象状态与夹点要清掉
    scene.selection().entities.clear();
    before = Counts::now();
    scene.view().specifySelectChanged();
    const QImage deselected = scene.grab();
    done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.compile, 0);
    EXPECT_EQ(done.regenSelection, 1);
    yicad_test::expectSameImage(QStringLiteral("incremental_deselect"), unselected, deselected);
}

TEST(RenderIncrementalTest, 高亮集变化不重新编译且不重画场景底图)
{
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();

    scene.highlight().entities = visibleEntitiesOfType(scene.document(), {DM::EntityArc});
    ASSERT_FALSE(scene.highlight().entities.empty());
    const Counts before = Counts::now();
    scene.view().specifyHighlightChanged();
    const QImage highlighted = scene.grab();
    const Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.compile, 0);
    EXPECT_EQ(done.regenSelection, 0);
    EXPECT_EQ(done.regenHighlight, 1);
    EXPECT_EQ(done.scene, 0) << "高亮画在叠加通道，场景底图不作废";
    yicad_test::expectSameImage(QStringLiteral("incremental_highlight"), fullRebuild(scene), highlighted);
}

TEST(RenderIncrementalTest, 选中的实体不再高亮)
{
    // 选中优先：已选中的按选中色画在场景里，叠加通道不再重画它
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    const std::vector<DmEntity*> arcs = visibleEntitiesOfType(scene.document(), {DM::EntityArc});
    ASSERT_FALSE(arcs.empty());
    scene.highlight().entities = arcs;
    scene.view().specifyHighlightChanged();
    (void)scene.grab();

    scene.selection().entities = arcs;
    const Counts before = Counts::now();
    scene.view().specifySelectChanged();
    const QImage selected = scene.grab();
    const Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.regenSelection, 1);
    yicad_test::expectSameImage(QStringLiteral("incremental_select_highlighted"), fullRebuild(scene), selected);
}

TEST(RenderIncrementalTest, 没有变化时不重画场景底图)
{
    // 只移动光标的帧就是这样：叠加层变了，场景没变
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    const QImage first = scene.grab();

    const Counts before = Counts::now();
    const QImage second = scene.grab();
    const Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.compile, 0);
    EXPECT_EQ(done.scene, 0);
    EXPECT_FALSE(scene.view().graphicsView().lastFrameRedrewScene());
    yicad_test::expectSameImage(QStringLiteral("incremental_unchanged"), first, second);
}

TEST(RenderIncrementalTest, 视图与显示设置改变时重画场景底图)
{
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    GuiDocumentView& view = scene.view();

    auto sceneRedraws = [&scene](const std::function<void()>& change) {
        const long long before = yicad::counters::scene().count();
        change();
        (void)scene.grab();
        return yicad::counters::scene().count() - before;
    };

    EXPECT_EQ(sceneRedraws([&view]() { view.zoomPan(10, 5); }), 1) << "平移";
    EXPECT_EQ(sceneRedraws([&view]() { view.zoomIn(1.5, DmVector(0.0, 0.0)); }), 1) << "缩放";
    EXPECT_EQ(sceneRedraws([&view]() { view.setDraftMode(true); }), 1) << "线宽显示";
    EXPECT_EQ(sceneRedraws([&view]() { view.setBackground(QColor(Qt::white)); }), 1) << "背景色";
    EXPECT_EQ(sceneRedraws([&view]() { view.setSelectedColor(QColor(Qt::red)); }), 1) << "选中色";
    EXPECT_EQ(sceneRedraws([&view]() { view.setHighlightColor(QColor(Qt::red)); }), 0) << "高亮色：高亮在叠加通道";
    EXPECT_EQ(sceneRedraws([&view]() { view.resize(view.width() + 8, view.height()); }), 1) << "尺寸";
}

TEST(RenderIncrementalTest, 网格开关改变时重画场景底图)
{
    // 网格开关是文档变量，改它的地方不通知画布，画布每帧比对
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();

    const long long before = yicad::counters::scene().count();
    scene.document().setGridOn(true);
    const QImage withGrid = scene.grab();
    EXPECT_EQ(yicad::counters::scene().count() - before, 1);
    yicad_test::expectMatchesBaseline(QStringLiteral("entities_grid"), withGrid);
}

TEST(RenderIncrementalTest, 网格线落在两列像素正中间时也画)
{
    // 画布宽为偶数、视点正在一条网格线上、间距为整数像素时，竖线都恰好落在两列像素的正中间。
    // 原先按"到线的距离 < 0.5 像素"判断，两边都不画，浮点误差又一侧偏正一侧偏负，半个画面的竖线都不见了
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    scene.document().setGridOn(true);
    scene.view().setView(DmVector(0.0, 0.0), 1.0);
    const QImage image = scene.grab();
    ASSERT_EQ(image.width() % 2, 0);

    // 最上面一行：数网格色的像素列，左右两半应当一样多（实体压住的个别像素不算）
    const QRgb minor = QColor(Colors::GRID).rgb();
    const QRgb major = QColor(Colors::META_GRID).rgb();
    int left = 0;
    int right = 0;
    for (int x = 0; x < image.width(); ++x)
    {
        const QRgb c = image.pixel(x, 0) | 0xFF000000u;
        if (c == (minor | 0xFF000000u) || c == (major | 0xFF000000u))
        {
            (x < image.width() / 2 ? left : right) += 1;
        }
    }
    EXPECT_GT(left, 5);
    EXPECT_GT(right, 5);
    EXPECT_LE(std::abs(left - right), 2) << "左半 " << left << " 列，右半 " << right << " 列";
}

TEST(RenderIncrementalTest, 修改实体只处理变更集且与整图重建的图一致)
{
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    EntityTable& table = *document.getEntityTable();
    const std::vector<DmEntity*> lines = visibleEntitiesOfType(document, {DM::EntityLine});
    const std::vector<DmEntity*> circles = visibleEntitiesOfType(document, {DM::EntityCircle});
    ASSERT_FALSE(lines.empty());
    ASSERT_FALSE(circles.empty());

    // 移动一条直线
    Counts before = Counts::now();
    inTransaction(document, [&]() {
        table.startModify(lines.front());
        lines.front()->move(DmVector(3.0, 2.0));
        lines.front()->update();
    });
    const QImage moved = scene.grab();
    Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0) << "改一个实体不整图重建";
    EXPECT_GE(done.changes, 1);
    EXPECT_EQ(done.scene, 1);
    yicad_test::expectSameImage(QStringLiteral("incremental_modify"), fullRebuild(scene), moved);

    // 删除一个圆，撤销，重做
    before = Counts::now();
    inTransaction(document, [&]() { table.remove(circles.front()); });
    const QImage erased = scene.grab();
    EXPECT_EQ(Counts::now().since(before).regen, 0);
    yicad_test::expectSameImage(QStringLiteral("incremental_erase"), fullRebuild(scene), erased);

    document.getCmdManager()->undo();
    const QImage undone = scene.grab();
    yicad_test::expectSameImage(QStringLiteral("incremental_undo"), fullRebuild(scene), undone);
    yicad_test::expectSameImage(QStringLiteral("incremental_undo_moved"), moved, undone);

    document.getCmdManager()->redo();
    const QImage redone = scene.grab();
    yicad_test::expectSameImage(QStringLiteral("incremental_redo"), erased, redone);

    // 加一条直线
    before = Counts::now();
    inTransaction(document, [&]() {
        auto* line = new DmLine(nullptr, DmVector(0.0, 0.0), DmVector(40.0, 30.0));
        table.add(line);
    });
    const QImage added = scene.grab();
    EXPECT_EQ(Counts::now().since(before).regen, 0);
    yicad_test::expectSameImage(QStringLiteral("incremental_add"), fullRebuild(scene), added);
}

TEST(RenderIncrementalTest, 重新编译时复用图片纹理)
{
    ProfilerOn profiler;
    RenderScene scene(request("image.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();

    // 画过一帧，设备还活着（视图持有），acquire() 取到的就是它
    const std::shared_ptr<GsDevice> device = GsDevice::acquire();
    ASSERT_NE(device, nullptr);
    ASSERT_GE(device->imageLoads(), 2u) << "image.dxf 有两张图片";
    const std::uint64_t loads = device->imageLoads();
    const QImage rebuilt = fullRebuild(scene);
    // 整图重建期间原先的纹理留着（GsModel 的 m_retainedTextures），设备的缓存里找得到，不再解码、上传
    EXPECT_EQ(device->imageLoads(), loads) << "整图重建又解码了图片";
    yicad_test::expectMatchesBaseline(QStringLiteral("image"), rebuilt);
}

TEST(RenderIncrementalTest, 临时隐藏的实体不画且不重新编译)
{
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    const QImage original = scene.grab();
    const std::vector<DmEntity*> arcs = visibleEntitiesOfType(scene.document(), {DM::EntityArc});
    ASSERT_FALSE(arcs.empty());

    scene.hidden().entities = arcs;
    const Counts before = Counts::now();
    scene.view().specifyHiddenChanged();
    const QImage hidden = scene.grab();
    const Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.compile, 0) << "临时隐藏在每视图的状态位图里，不改几何";
    EXPECT_EQ(done.scene, 1);
    for (DmEntity* arc : arcs)
    {
        EXPECT_TRUE(arc->isVisible()) << "不改实体的可见性（P18）";
    }

    // 恢复显示：与原图相同
    scene.hidden().entities.clear();
    scene.view().specifyHiddenChanged();
    yicad_test::expectSameImage(QStringLiteral("incremental_unhide"), original, scene.grab());

    // 对照：真的删掉这些圆弧
    inTransaction(scene.document(), [&]() {
        for (DmEntity* arc : arcs)
        {
            scene.document().getEntityTable()->remove(arc);
        }
    });
    yicad_test::expectSameImage(QStringLiteral("incremental_hidden"), fullRebuild(scene), hidden);
}

TEST(RenderIncrementalTest, 预览变换只改实例记录)
{
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    GuiDocumentView& view = scene.view();
    const std::vector<DmEntity*> sources =
        visibleEntitiesOfType(scene.document(), {DM::EntityCircle, DM::EntityArc, DM::EntityBlockReference});
    ASSERT_FALSE(sources.empty());
    const DmVector offset(7.0, -4.0);

    // 预览几何生成一次，拖动只改变换
    DmEntityContainer* preview = view.getPreviewContainer();
    for (DmEntity* e : sources)
    {
        DmEntity* clone = e->clone();
        clone->setParent(nullptr);
        preview->addEntity(clone);
    }
    view.specifyPreviewModified();
    (void)scene.grab();
    const Counts before = Counts::now();
    view.setPreviewTransform(GiTransform::translation(offset));
    const QImage transformed = scene.grab();
    const Counts done = Counts::now().since(before);
    EXPECT_EQ(done.compile, 0) << "预览变换不重新编译";
    EXPECT_EQ(done.scene, 0) << "预览画在叠加通道";

    // 对照：预览实体真的移过去，变换为恒等
    preview->clear();
    for (DmEntity* e : sources)
    {
        DmEntity* clone = e->clone();
        clone->setParent(nullptr);
        clone->move(offset);
        clone->update();
        preview->addEntity(clone);
    }
    view.specifyPreviewModified();
    view.setPreviewTransform(GiTransform());
    yicad_test::expectSameImage(QStringLiteral("incremental_preview_transform"), scene.grab(), transformed);
    preview->clear();
    view.specifyPreviewModified();
}

TEST(RenderIncrementalTest, 高亮集很大时改走状态位图)
{
    // 超过阈值（1000 个对象）时逐个重画太贵，改在场景里按状态位图画，场景底图作废
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    DmDocument& document = scene.document();
    std::vector<DmEntity*> lines;
    inTransaction(document, [&]() {
        for (int i = 0; i < 1200; ++i)
        {
            auto* line = new DmLine(nullptr, DmVector(i * 0.05, -5.0), DmVector(i * 0.05, -4.0));
            document.getEntityTable()->add(line);
            lines.push_back(line);
        }
    });
    (void)scene.grab();

    scene.highlight().entities = lines;
    const Counts before = Counts::now();
    scene.view().specifyHighlightChanged();
    const QImage highlighted = scene.grab();
    const Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.compile, 0);
    EXPECT_EQ(done.scene, 1) << "高亮走状态位图，画在场景里";
    yicad_test::expectSameImage(QStringLiteral("incremental_highlight_bitmap"), fullRebuild(scene), highlighted);
}

TEST(RenderIncrementalTest, 抽查修订号找得到没有登记的修改)
{
    // YICAD_GS_VERIFY=1：每帧抽查节点的修订号，实体改了却没有登记变更时报告（RENDER_PLAN.md 第 4.3.6 节）
    const QByteArray previous = qgetenv("YICAD_GS_VERIFY");
    qputenv("YICAD_GS_VERIFY", "1");
    {
        RenderScene scene(request("entities.dxf"));
        ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
        DmDocument& document = scene.document();
        const std::vector<DmEntity*> lines = visibleEntitiesOfType(document, {DM::EntityLine});
        const std::vector<DmEntity*> circles = visibleEntitiesOfType(document, {DM::EntityCircle});
        ASSERT_FALSE(lines.empty());
        ASSERT_FALSE(circles.empty());
        const std::shared_ptr<GsModel> model = scene.view().graphicsModel();

        // 经事务的修改都登记了
        inTransaction(document, [&]() {
            document.getEntityTable()->startModify(lines.front());
            lines.front()->move(DmVector(1.0, 0.0));
            lines.front()->update();
        });
        (void)scene.grab();
        (void)scene.grab();
        EXPECT_EQ(model->verifyMismatches(), 0u);

        // 绕过事务直接改：抽查到
        circles.front()->move(DmVector(1.0, 0.0));
        circles.front()->update();
        (void)scene.grab();
        (void)scene.grab();
        EXPECT_GE(model->verifyMismatches(), 1u);
    }
    qputenv("YICAD_GS_VERIFY", previous);
}
