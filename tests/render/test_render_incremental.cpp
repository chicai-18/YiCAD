/// @file test_render_incremental.cpp
/// @brief 旧渲染器的增量更新（doc/RENDER_PLAN.md 阶段 1）：选择集、高亮集变化只重建局部，
///        场景底图只在作废时重画，图片纹理跨整图重建复用
///
/// 局部更新的结果与同一状态下整图重建的结果比对：只看计数器会漏掉"没重建、画面没变"的错误，
/// 只看图像会漏掉"又整图重建了一次"的退步，所以两样都查。计数器要开着埋点才计数。

// GLEW 必须先于 Qt 拉入的 gl.h（见 GuiDocumentView.h 的说明）
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>

#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "DmDocument.h"
#include "GLImageTextureCache.h"
#include "GuiDocumentView.h"
#include "RenderHarness.h"
#include "ScopedTimer.h"

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

/// @brief 三种重建与场景底图重画的次数
struct Counts
{
    long long regen = 0;
    long long regenSelection = 0;
    long long regenHighlight = 0;
    long long scene = 0;

    static Counts now()
    {
        return {yicad::counters::regen().count(), yicad::counters::regenSelection().count(),
                yicad::counters::regenHighlight().count(), yicad::counters::scene().count()};
    }

    Counts since(const Counts& before) const
    {
        return {regen - before.regen, regenSelection - before.regenSelection, regenHighlight - before.regenHighlight,
                scene - before.scene};
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
    scene.view().specifyDocumentModified();
    return scene.grab();
}

}  // namespace

TEST(RenderIncrementalTest, 选择集变化只重建选中组且与整图重建的图一致)
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
    EXPECT_EQ(done.regenSelection, 1);
    EXPECT_EQ(done.scene, 1) << "选中组在场景底图里，选择集变了要重画";
    yicad_test::expectSameImage(QStringLiteral("incremental_select"), fullRebuild(scene), selected);

    // 取消选择：选中组与夹点要清掉
    scene.selection().entities.clear();
    before = Counts::now();
    scene.view().specifySelectChanged();
    const QImage deselected = scene.grab();
    done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.regenSelection, 1);
    yicad_test::expectSameImage(QStringLiteral("incremental_deselect"), unselected, deselected);
}

TEST(RenderIncrementalTest, 高亮集变化只重建高亮组且不重画场景底图)
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
    EXPECT_EQ(done.regenSelection, 0);
    EXPECT_EQ(done.regenHighlight, 1);
    EXPECT_EQ(done.scene, 0) << "高亮组画在叠加层，场景底图不作废";
    yicad_test::expectSameImage(QStringLiteral("incremental_highlight"), fullRebuild(scene), highlighted);
}

TEST(RenderIncrementalTest, 选中的实体从高亮组里去掉)
{
    // 高亮组不含选中的实体（选中优先），所以选择集变了高亮组也要重建
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
    // 只移动光标的帧就是这样：前景变了，场景没变
    ProfilerOn profiler;
    RenderScene scene(request("entities.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();
    const QImage first = scene.grab();

    const Counts before = Counts::now();
    const QImage second = scene.grab();
    const Counts done = Counts::now().since(before);
    EXPECT_EQ(done.regen, 0);
    EXPECT_EQ(done.scene, 0);
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
    EXPECT_EQ(sceneRedraws([&view]() { view.setHighlightColor(QColor(Qt::red)); }), 0) << "高亮色：高亮在叠加层";
    EXPECT_EQ(sceneRedraws([&view]() { view.specifyDocumentModified(); }), 1) << "文档修改";
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

TEST(RenderIncrementalTest, 整图重建时复用图片纹理)
{
    ProfilerOn profiler;
    RenderScene scene(request("image.dxf"));
    ASSERT_TRUE(scene.error().isEmpty()) << scene.error().toStdString();

    yicad::counters::uploadBytes().reset();
    const QImage rebuilt = fullRebuild(scene);
    ASSERT_EQ(yicad::counters::uploadBytes().count(), 1);
    // image.dxf 的两张图片 64×48 与 32×32，RGBA。这一帧上传的是几个实体的顶点与背景、前景的立即模式图元，
    // 远小于两张纹理；每次整图重建都重新上传纹理时（阶段 1 之前）会超过它
    constexpr long long kTextureBytes = (64 * 48 + 32 * 32) * 4;
    EXPECT_LT(yicad::counters::uploadBytes().total(), kTextureBytes) << "整图重建又上传了图片纹理";
    yicad_test::expectMatchesBaseline(QStringLiteral("image"), rebuilt);
}

TEST(GLImageTextureCacheTest, 同一来源只解码一次_整图重建释放没用到的纹理)
{
    QOffscreenSurface surface;
    surface.create();
    QOpenGLContext context;
    ASSERT_TRUE(context.create());
    ASSERT_TRUE(context.makeCurrent(&surface));
    ASSERT_EQ(glewInit(), static_cast<GLenum>(GLEW_OK));

    int loads = 0;
    auto load = [&loads]() {
        ++loads;
        QImage image(2, 2, QImage::Format_RGBA8888);
        image.fill(Qt::red);
        return image;
    };
    {
        opengl::GLImageTextureCache cache;
        const GLuint a = cache.texture(QStringLiteral("a"), load);
        EXPECT_EQ(cache.texture(QStringLiteral("a"), load), a);
        EXPECT_EQ(loads, 1);
        const GLuint b = cache.texture(QStringLiteral("b"), load);
        EXPECT_NE(a, b);
        EXPECT_EQ(loads, 2);

        // 一轮整图重建只用到 a：b 释放，a 保留
        cache.beginSweep();
        EXPECT_EQ(cache.texture(QStringLiteral("a"), load), a);
        cache.endSweep();
        EXPECT_EQ(loads, 2);
        EXPECT_EQ(glIsTexture(a), static_cast<GLboolean>(GL_TRUE));
        EXPECT_EQ(glIsTexture(b), static_cast<GLboolean>(GL_FALSE));

        // b 再出现时重新解码
        (void)cache.texture(QStringLiteral("b"), load);
        EXPECT_EQ(loads, 3);
    }
    context.doneCurrent();
}
