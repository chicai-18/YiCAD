/// @file test_baseline_runtime.cpp
/// @brief 基准图纸的运行期数据（doc/BASELINE.md 第 4 节，渲染方案 RENDER_PLAN.md 0.1 步）
///
/// 设置环境变量 YICAD_BENCHMARK_DIR 指向 tools/gen_benchmark_drawings.py 的输出目录时运行，
/// 否则跳过；CI 不设置它。要在有显卡与显示环境的开发机上跑：用例打开一个真实的 UIView 窗口，
/// 经本机显卡绘制（不是 test_render 的 Mesa 软件渲染）。
///
/// 对每份图纸依次做，用代码调用而不经过鼠标事件：
/// - 经 DmDocument::readFile() 打开（document.open），显示后首帧的整图重建（render.regen）；
/// - 缩放到全图，连续重绘，取稳态帧（render.paintGL、每帧绘制调用与上传字节，以及 GPU 耗时）；
/// - 每帧平移 1 像素，取场景整幅重画的帧（渲染方案阶段 4 加：平移、缩放时每帧都这样，CPU 与 GPU 耗时）；
/// - 要求整图重建（REGEN：DmDocument::requestFullRebuild 加 notifyDocumentModified），
///   重绘一帧，取整图重建的耗时（render.regen；渲染方案阶段 2 加：实体里不再缓存顶点，每次整图重建都要重新生成）；
/// - 在事务里移动 20 条直线，每次一条，提交后重绘一帧（渲染方案阶段 4 加：旧渲染器整图重建，
///   图形系统只处理变更集，render.gsChanges 与 render.gsCompile）；
/// - 换 20 次高亮的实体，每次重绘一帧（render.frameAfterHighlight，其中 render.regen 或
///   阶段 1 起的 render.regenHighlight，以及场景底图的重画 render.scene）；
/// - 在 20 条直线的中点点选（snap.catchEntity），选中后重绘一帧（render.frameAfterSelection，
///   其中 render.regen 或阶段 1 起的 render.regenSelection）；
/// - 框选盖住全部实体（selection.selectWindow），重绘一帧；
/// - 从 20 条直线的端点沿直线方向求虚拟交点（snap.nearestVirtualIntersection）。
///
/// 除注明 GPU 的一项外，数值是 CPU 侧的提交耗时（与程序里 YICAD_PROFILE=1 的埋点相同，不等 GPU 完成）。
/// GPU 耗时用 GL_TIME_ELAPSED 查询包住 paintGL 量得（渲染方案阶段 1 加：稳态帧的 CPU 提交与图纸大小无关，
/// 画整图的开销在 GPU 上）。显存占用用 GL_NVX_gpu_memory_info 量打开图纸前后可用显存之差（渲染方案阶段 4 加；
/// 只有 NVIDIA 驱动有这个扩展，别的显卡上为 0）。
/// 结果以表格打印在标准输出，由人抄进 BASELINE.md。

// GLEW 必须先于 Qt 拉入的 gl.h（QOpenGLContext 会带进来）
#include <GL/glew.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QWindow>

#include "AppDocument.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "HighlightSet.h"
#include "ICommandHost.h"
#include "ScopedTimer.h"
#include "SelectionSet.h"
#include "Snapper.h"
#include "Transaction.h"
#include "UIView.h"
#include "support/DxfTestRuntime.h"
#include "support/FakeDocumentManager.h"

namespace
{
/// @brief 取样次数：点选、换高亮、虚拟交点各做这么多次
constexpr int kSamples = 20;
/// @brief 稳态帧：先热身，再计数
constexpr int kWarmupFrames = 5;
constexpr int kSteadyFrames = 30;
/// @brief 文档修改后的整图重建做这么多次（大图纸上一次就要几秒）
constexpr int kModifiedRegens = 3;
/// @brief 窗口的绘图区尺寸（像素）
constexpr int kViewWidth = 1600;
constexpr int kViewHeight = 900;

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

/// @brief 用 GL_TIME_ELAPSED 查询量每帧 paintGL 在 GPU 上的耗时
/// @details 只包住 paintGL 发出的命令，不含 QOpenGLWidget 之后的多重采样解析与窗口合成
class GpuTimedView : public UIView
{
public:
    using UIView::UIView;

    /// @brief 最近一帧 paintGL 的 GPU 耗时（毫秒），等 GPU 完成后返回；还没画过时返回 0
    double lastFrameGpuMs()
    {
        if (m_query == 0)
        {
            return 0.0;
        }
        makeCurrent();
        GLuint64 nanoseconds = 0;
        glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, &nanoseconds);
        doneCurrent();
        return static_cast<double>(nanoseconds) / 1.0e6;
    }

protected:
    void paintGL() override
    {
        if (m_query == 0)
        {
            // 第一帧不计时：GLEW 由图形系统在第一帧建设备时初始化，在那之前 glGenQueries 还是空指针
            UIView::paintGL();
            glGenQueries(1, &m_query);
            return;
        }
        glBeginQuery(GL_TIME_ELAPSED, m_query);
        UIView::paintGL();
        glEndQuery(GL_TIME_ELAPSED);
    }

private:
    GLuint m_query = 0;  ///< 计时查询对象，第一帧时在画布的上下文里建
};

/// @brief 显卡上可用的显存（KB，GL_NVX_gpu_memory_info）；没有这个扩展时为 0
/// @details 量的是整块显卡，其他进程的分配也算在内；只在同一次运行里前后相减
long long availableVideoMemoryKb()
{
    QOffscreenSurface surface;
    surface.create();
    QOpenGLContext context;
    if (!context.create() || !context.makeCurrent(&surface))
    {
        return 0;
    }
    constexpr GLenum kCurrentAvailableVidMemNvx = 0x9049;
    while (glGetError() != GL_NO_ERROR)
    {
    }
    GLint kb = 0;
    glGetIntegerv(kCurrentAvailableVidMemNvx, &kb);
    const bool ok = glGetError() == GL_NO_ERROR;
    context.doneCurrent();
    return ok ? kb : 0;
}

/// @brief 一份图纸的结果
struct Result
{
    QString name;
    std::string renderer;  ///< 画这份图纸的 OpenGL 实现（GL_RENDERER），双显卡的机器上看用的是哪一块
    int entities = 0;
    double openMs = 0.0;
    double firstRegenMs = 0.0;             ///< 显示后首帧的整图重建
    double steadyFrameMs = 0.0;
    double steadyGpuMs = 0.0;
    double steadyDrawCalls = 0.0;
    double steadyUploadBytes = 0.0;
    double panFrameMs = 0.0;               ///< 平移 1 像素后的帧（场景整幅重画）
    double panGpuMs = 0.0;
    double modifiedRegenMs = 0.0;          ///< 要求整图重建（REGEN）后的整图重建
    double videoMemoryMb = 0.0;            ///< 打开图纸、画过稳态帧后少了的可用显存
    double frameAfterModifyMs = 0.0;       ///< 移动一条直线后首帧
    long long modifyRegens = 0;            ///< 移动 20 次的整图重建次数
    double modifyChangesMs = 0.0;          ///< 其中处理变更集 render.gsChanges
    double modifyCompileMs = 0.0;          ///< 其中编译分块 render.gsCompile
    double frameAfterHighlightMs = 0.0;
    double regenMs = 0.0;
    long long highlightRegens = 0;         ///< 换高亮时整图重建的次数
    double regenHighlightMs = 0.0;
    long long highlightSceneRedraws = 0;   ///< 换高亮时场景底图重画的次数
    double frameUploadBytes = 0.0;
    double catchEntityMs = 0.0;
    double frameAfterClickMs = 0.0;
    long long clickRegens = 0;             ///< 点选时整图重建的次数
    double clickRegenSelectionMs = 0.0;
    double selectWindowMs = 0.0;
    int selectedByWindow = 0;
    double frameAfterSelectAllMs = 0.0;
    double selectAllRegenSelectionMs = 0.0;
    double virtualIntersectionMs = 0.0;
};

/// @brief 等窗口真正显示出来（首次绘制会建 GL 上下文与画笔）
void waitExposed(QWidget& widget)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 10000)
    {
        QCoreApplication::processEvents();
        if (widget.windowHandle() && widget.windowHandle()->isExposed())
        {
            break;
        }
    }
    QCoreApplication::processEvents();
}

/// @brief 画一帧：请求重绘并处理事件，直到 paintGL 真的执行了一次
/// @details 不用 repaint()：Qt 6 在合成窗口上把一个刷新周期内的多次 repaint() 合并成一次
///          （QWidgetRepaintManager 的节流），连续调用只画一帧。程序里也是 update() 后由事件循环画。
///          要求埋点开着（靠 render.paintGL 的次数判断画完）。
void renderFrame(QWidget& widget)
{
    const long long before = yicad::counters::paintGL().count();
    widget.update();
    QElapsedTimer timer;
    timer.start();
    while (yicad::counters::paintGL().count() == before && timer.elapsed() < 5000)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }
    EXPECT_GT(yicad::counters::paintGL().count(), before) << "5 秒内没有画出一帧";
}

/// @brief 图纸里均匀分布的若干条直线
std::vector<DmLine*> sampleLines(DmDocument& document, int wanted)
{
    std::vector<DmLine*> all;
    for (DmEntity* entity : *document.getEntityTable())
    {
        if (auto* line = dynamic_cast<DmLine*>(entity))
        {
            all.push_back(line);
        }
    }
    std::vector<DmLine*> result;
    if (all.empty())
    {
        return result;
    }
    const std::size_t step = std::max<std::size_t>(1, all.size() / static_cast<std::size_t>(wanted));
    for (std::size_t i = 0; i < all.size() && static_cast<int>(result.size()) < wanted; i += step)
    {
        result.push_back(all[i]);
    }
    return result;
}

Result measure(yicad_test::DxfRuntime& runtime, const QString& path)
{
    Result result;
    result.name = QFileInfo(path).fileName();

    yicad_test::FakeDocumentManager documents;
    AppDocument appDocument(documents);
    DmDocument& document = appDocument.document();

    // 打开
    yicad::Profiler::resetAll();
    EXPECT_TRUE(runtime.readFile(document, path)) << path.toStdString();
    result.openMs = yicad::counters::openDocument().averageMs();
    result.entities = document.getEntityTable()->count();

    const long long memoryBefore = availableVideoMemoryKb();
    GpuTimedView view(nullptr, Qt::WindowFlags(), &appDocument);
    view.resize(kViewWidth, kViewHeight);
    view.show();
    waitExposed(view);
    renderFrame(view);
    result.firstRegenMs = yicad::counters::regen().averageMs();
    view.makeCurrent();
    result.renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    view.doneCurrent();
    view.zoomAuto();

    // 稳态帧
    for (int i = 0; i < kWarmupFrames; ++i)
    {
        renderFrame(view);
    }
    yicad::Profiler::resetAll();
    double gpuMs = 0.0;
    for (int i = 0; i < kSteadyFrames; ++i)
    {
        renderFrame(view);
        gpuMs += view.lastFrameGpuMs();
    }
    result.steadyFrameMs = yicad::counters::paintGL().averageMs();
    result.steadyGpuMs = gpuMs / kSteadyFrames;
    result.steadyDrawCalls = yicad::counters::drawCalls().average();
    result.steadyUploadBytes = yicad::counters::uploadBytes().average();
    // 平移：每帧移 1 像素，场景整幅重画
    yicad::Profiler::resetAll();
    gpuMs = 0.0;
    for (int i = 0; i < kSteadyFrames; ++i)
    {
        view.zoomPan(1, 0);
        renderFrame(view);
        gpuMs += view.lastFrameGpuMs();
    }
    result.panFrameMs = yicad::counters::paintGL().averageMs();
    result.panGpuMs = gpuMs / kSteadyFrames;
    const long long memoryAfter = availableVideoMemoryKb();
    result.videoMemoryMb = memoryBefore > 0 && memoryAfter > 0 ? (memoryBefore - memoryAfter) / 1024.0 : 0.0;

    // 要求整图重建（REGEN）：图形模型收到 fullRebuild 的变更集时整图重建
    yicad::Profiler::resetAll();
    for (int i = 0; i < kModifiedRegens; ++i)
    {
        document.requestFullRebuild();
        document.notifyDocumentModified();
        renderFrame(view);
    }
    result.modifiedRegenMs = yicad::counters::regen().averageMs();

    const std::vector<DmLine*> lines = sampleLines(document, kSamples);
    EXPECT_FALSE(lines.empty());

    // 修改一个实体：在事务里移动一条直线，提交后画一帧
    yicad::Profiler::resetAll();
    long long modifyFrames = 0;
    double modifyFrameMs = 0.0;
    for (DmLine* line : lines)
    {
        Transaction t("benchmark move", &document);
        t.start();
        document.getEntityTable()->startModify(line);
        line->move(DmVector(1.0, 0.0));
        line->update();
        t.commit();
        const long long beforeNs = yicad::counters::paintGL().totalNs();
        const long long beforeCount = yicad::counters::paintGL().count();
        renderFrame(view);
        modifyFrameMs += (yicad::counters::paintGL().totalNs() - beforeNs) / 1.0e6;
        modifyFrames += yicad::counters::paintGL().count() - beforeCount;
    }
    result.frameAfterModifyMs = modifyFrames > 0 ? modifyFrameMs / modifyFrames : 0.0;
    result.modifyRegens = yicad::counters::regen().count();
    result.modifyChangesMs = yicad::counters::gsChanges().averageMs();
    result.modifyCompileMs = yicad::counters::gsCompile().averageMs();

    // 换高亮：相当于命令里光标从一个候选实体移到另一个上
    HighlightSet& highlight = *static_cast<ICommandHost&>(view).highlight();
    yicad::Profiler::resetAll();
    for (DmLine* line : lines)
    {
        highlight.clear();
        highlight.add(line);
        renderFrame(view);
    }
    highlight.clear();
    result.frameAfterHighlightMs = yicad::counters::frameAfterHighlight().averageMs();
    result.regenMs = yicad::counters::regen().averageMs();
    result.highlightRegens = yicad::counters::regen().count();
    result.regenHighlightMs = yicad::counters::regenHighlight().averageMs();
    result.highlightSceneRedraws = yicad::counters::scene().count();
    result.frameUploadBytes = yicad::counters::uploadBytes().average();
    renderFrame(view);

    // 点选：在直线中点拾取，选中后重绘
    SelectionSet& selection = appDocument.selection();
    Snapper snapper(&document, &view);
    yicad::Profiler::resetAll();
    for (DmLine* line : lines)
    {
        DmEntity* picked = snapper.catchEntity(line->getMiddlePoint());
        selection.clear();
        if (picked)
        {
            selection.add(picked);
        }
        renderFrame(view);
    }
    result.catchEntityMs = yicad::counters::catchEntity().averageMs();
    result.frameAfterClickMs = yicad::counters::frameAfterSelection().averageMs();
    result.clickRegens = yicad::counters::regen().count();
    result.clickRegenSelectionMs = yicad::counters::regenSelection().averageMs();

    // 全选：框选盖住全部实体
    selection.clear();
    renderFrame(view);
    document.getEntityTable()->updateContainer();
    const DmVector min = document.getEntityTable()->getEntityContainer()->getMin();
    const DmVector max = document.getEntityTable()->getEntityContainer()->getMax();
    const DmVector margin = (max - min) * 0.01 + DmVector(1.0, 1.0);
    yicad::Profiler::resetAll();
    selection.selectWindow(min - margin, max + margin);
    renderFrame(view);
    result.selectWindowMs = yicad::counters::selectWindow().averageMs();
    result.selectedByWindow = selection.count();
    result.frameAfterSelectAllMs = yicad::counters::frameAfterSelection().averageMs();
    result.selectAllRegenSelectionMs = yicad::counters::regenSelection().averageMs();
    selection.clear();
    renderFrame(view);

    // 虚拟交点：从直线端点沿直线方向
    yicad::Profiler::resetAll();
    for (DmLine* line : lines)
    {
        const DmVector start = line->getStartpoint();
        const DmVector end = line->getEndpoint();
        const double angle = std::atan2(end.y - start.y, end.x - start.x);
        double dist = 0.0;
        document.getEntityTable()->getNearestVirtualIntersection(end, angle, &dist);
    }
    result.virtualIntersectionMs = yicad::counters::nearestVirtualIntersection().averageMs();

    view.hide();
    return result;
}

std::string fixed(double value, int decimals)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    return buffer;
}

/// @brief 以 Markdown 表格打印，可直接贴进 BASELINE.md
void print(const std::vector<Result>& results)
{
    std::string out = "\n基准图纸运行期数据（毫秒，另有注明的除外）\n\n| 指标 |";
    for (const Result& r : results)
    {
        out += " " + r.name.toStdString() + " |";
    }
    out += "\n|------|";
    for (std::size_t i = 0; i < results.size(); ++i)
    {
        out += "------:|";
    }
    out += "\n";
    auto row = [&](const char* label, auto value) {
        out += std::string("| ") + label + " |";
        for (const Result& r : results)
        {
            out += " " + value(r) + " |";
        }
        out += "\n";
    };
    row("OpenGL 渲染器", [](const Result& r) { return r.renderer; });
    row("顶层实体数", [](const Result& r) { return std::to_string(r.entities); });
    row("打开文档 `document.open`", [](const Result& r) { return fixed(r.openMs, 1); });
    row("显示后首帧的整图重建 `render.regen`", [](const Result& r) { return fixed(r.firstRegenMs, 2); });
    row("稳态帧 `render.paintGL`", [](const Result& r) { return fixed(r.steadyFrameMs, 2); });
    row("稳态帧 GPU 耗时（`GL_TIME_ELAPSED`）", [](const Result& r) { return fixed(r.steadyGpuMs, 2); });
    row("稳态帧绘制调用（次/帧）", [](const Result& r) { return fixed(r.steadyDrawCalls, 0); });
    row("稳态帧上传（字节/帧）", [](const Result& r) { return fixed(r.steadyUploadBytes, 0); });
    row("平移一帧 `render.paintGL`（场景整幅重画）", [](const Result& r) { return fixed(r.panFrameMs, 2); });
    row("平移一帧 GPU 耗时（`GL_TIME_ELAPSED`）", [](const Result& r) { return fixed(r.panGpuMs, 2); });
    row("显存占用（MB，`GL_NVX_gpu_memory_info`）", [](const Result& r) { return fixed(r.videoMemoryMb, 1); });
    row("要求整图重建后的整图重建 `render.regen`", [](const Result& r) { return fixed(r.modifiedRegenMs, 2); });
    row("移动一条直线后首帧 `render.paintGL`", [](const Result& r) { return fixed(r.frameAfterModifyMs, 2); });
    row("移动 20 次的整图重建次数", [](const Result& r) { return std::to_string(r.modifyRegens); });
    row("其中处理变更集 `render.gsChanges`", [](const Result& r) { return fixed(r.modifyChangesMs, 3); });
    row("其中编译分块 `render.gsCompile`", [](const Result& r) { return fixed(r.modifyCompileMs, 3); });
    row("换高亮后首帧 `render.frameAfterHighlight`", [](const Result& r) { return fixed(r.frameAfterHighlightMs, 2); });
    row("其中整图重建 `render.regen`", [](const Result& r) { return fixed(r.regenMs, 2); });
    row("换高亮 20 次的整图重建次数", [](const Result& r) { return std::to_string(r.highlightRegens); });
    row("其中局部重建 `render.regenHighlight`", [](const Result& r) { return fixed(r.regenHighlightMs, 3); });
    row("换高亮 20 次的场景底图重画次数 `render.scene`",
        [](const Result& r) { return std::to_string(r.highlightSceneRedraws); });
    row("换高亮后首帧上传（字节）", [](const Result& r) { return fixed(r.frameUploadBytes, 0); });
    row("点选 `snap.catchEntity`", [](const Result& r) { return fixed(r.catchEntityMs, 3); });
    row("点选后首帧 `render.frameAfterSelection`", [](const Result& r) { return fixed(r.frameAfterClickMs, 2); });
    row("点选 20 次的整图重建次数", [](const Result& r) { return std::to_string(r.clickRegens); });
    row("其中局部重建 `render.regenSelection`", [](const Result& r) { return fixed(r.clickRegenSelectionMs, 3); });
    row("全选框选 `selection.selectWindow`", [](const Result& r) { return fixed(r.selectWindowMs, 2); });
    row("框选选中数", [](const Result& r) { return std::to_string(r.selectedByWindow); });
    row("全选后首帧 `render.frameAfterSelection`", [](const Result& r) { return fixed(r.frameAfterSelectAllMs, 2); });
    row("其中局部重建 `render.regenSelection`",
        [](const Result& r) { return fixed(r.selectAllRegenSelectionMs, 2); });
    row("虚拟交点 `snap.nearestVirtualIntersection`", [](const Result& r) { return fixed(r.virtualIntersectionMs, 3); });
    std::fputs(out.c_str(), stdout);
    std::fflush(stdout);
}
}  // namespace

TEST(BaselineRuntimeTest, 基准图纸运行期数据)
{
    const QString dir = qEnvironmentVariable("YICAD_BENCHMARK_DIR");
    if (dir.isEmpty())
    {
        GTEST_SKIP() << "未设置 YICAD_BENCHMARK_DIR（tools/gen_benchmark_drawings.py 的输出目录），跳过";
    }

    yicad_test::DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    ProfilerOn profiler;

    std::vector<Result> results;
    for (const char* size : {"small", "medium", "large"})
    {
        const QString path = QDir(dir).filePath(QStringLiteral("benchmark_%1.dxf").arg(QLatin1String(size)));
        if (!QFileInfo::exists(path))
        {
            std::printf("缺少 %s，跳过\n", path.toLocal8Bit().constData());
            continue;
        }
        results.push_back(measure(runtime, path));
    }
    ASSERT_FALSE(results.empty()) << "YICAD_BENCHMARK_DIR 里没有基准图纸：" << dir.toStdString();
    print(results);
    yicad::Profiler::resetAll();
}
