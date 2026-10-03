/// @file RenderHarness.cpp
/// @brief 出图测试的公共部分，见 RenderHarness.h

#include "RenderHarness.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include <QColor>
#include <QDir>
#include <QFileInfo>

#include "DmDocument.h"
#include "DmEntity.h"
#include "DmFontList.h"
#include "DmSettings.h"
#include "EntityTable.h"
#include "GuiDocumentView.h"
#include "IHighlightSource.h"
#include "ISelectionSource.h"
#include "support/DxfTestRuntime.h"

#ifndef YICAD_RENDER_SOURCE_DIR
#error "test_render 需要 YICAD_RENDER_SOURCE_DIR（tests/render 源码目录）"
#endif
#ifndef YICAD_RENDER_OUTPUT_DIR
#error "test_render 需要 YICAD_RENDER_OUTPUT_DIR（比对失败时写实际图像的目录）"
#endif
#ifndef YICAD_SUPPORT_DIR
#error "test_render 需要 YICAD_SUPPORT_DIR（YiCAD/support，本机的 SHX 字体放在它的 fonts/）"
#endif

namespace yicad_test
{
namespace
{

/// @brief 一个像素的某个通道差值超过它就算这个像素不同
constexpr int kChannelTolerance = 16;
/// @brief 不同的像素占比超过它就算不一致
constexpr double kMaxDifferentRatio = 0.002;

QString drawingsDir()
{
    return QStringLiteral(YICAD_RENDER_SOURCE_DIR "/drawings");
}

QString baselineDir()
{
    return QStringLiteral(YICAD_RENDER_SOURCE_DIR "/baseline");
}

/// @brief 整个测试进程共用一个 DXF 插件运行时；另把本机的 SHX 字体目录交给字体列表，
///        把当前目录设到参考图纸目录（图像实体按当前目录解析相对路径，见 HostApi::createImage）
class RenderEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        DMSETTINGS->beginGroup(QStringLiteral("/Paths"));
        m_previousFonts = DMSETTINGS->readEntry(QStringLiteral("/Fonts"));
        DMSETTINGS->writeEntry(QStringLiteral("/Fonts"), QStringLiteral(YICAD_SUPPORT_DIR "/fonts"));
        DMSETTINGS->endGroup();

        m_previousDir = QDir::currentPath();
        QDir::setCurrent(drawingsDir());

        s_runtime = std::make_unique<DxfRuntime>();
    }

    void TearDown() override
    {
        s_runtime.reset();
        QDir::setCurrent(m_previousDir);
        DMSETTINGS->beginGroup(QStringLiteral("/Paths"));
        DMSETTINGS->writeEntry(QStringLiteral("/Fonts"), m_previousFonts);
        DMSETTINGS->endGroup();
    }

    static DxfRuntime* runtime() { return s_runtime.get(); }

private:
    QString m_previousFonts;
    QString m_previousDir;
    static std::unique_ptr<DxfRuntime> s_runtime;
};

std::unique_ptr<DxfRuntime> RenderEnvironment::s_runtime;

[[maybe_unused]] ::testing::Environment* const g_environment =
    ::testing::AddGlobalTestEnvironment(new RenderEnvironment);

/// @brief 两张图的差异：不同的像素数、最大通道差，以及标出差异的图
struct ImageDiff
{
    int differing = 0;
    int maxDelta = 0;
    QImage marked;
};

ImageDiff diffImages(const QImage& expected, const QImage& actual)
{
    ImageDiff diff;
    diff.marked = QImage(actual.size(), QImage::Format_RGB32);
    for (int y = 0; y < actual.height(); ++y)
    {
        const auto* e = reinterpret_cast<const QRgb*>(expected.constScanLine(y));
        const auto* a = reinterpret_cast<const QRgb*>(actual.constScanLine(y));
        auto* m = reinterpret_cast<QRgb*>(diff.marked.scanLine(y));
        for (int x = 0; x < actual.width(); ++x)
        {
            const int delta = std::max({std::abs(qRed(e[x]) - qRed(a[x])), std::abs(qGreen(e[x]) - qGreen(a[x])),
                                        std::abs(qBlue(e[x]) - qBlue(a[x]))});
            diff.maxDelta = std::max(diff.maxDelta, delta);
            if (delta > kChannelTolerance)
            {
                ++diff.differing;
                m[x] = qRgb(255, 0, 0);
            }
            else
            {
                const int gray = qGray(a[x]) / 3;
                m[x] = qRgb(gray, gray, gray);
            }
        }
    }
    return diff;
}

/// @brief 把实际图像（与差异图）写到输出目录，返回实际图像的路径
QString writeOutput(const QString& name, const QImage& actual, const QImage* marked)
{
    const QString actualPath = QStringLiteral(YICAD_RENDER_OUTPUT_DIR "/%1.actual.png").arg(name);
    QDir().mkpath(QFileInfo(actualPath).absolutePath());
    actual.save(actualPath);
    if (marked)
    {
        marked->save(QStringLiteral(YICAD_RENDER_OUTPUT_DIR "/%1.diff.png").arg(name));
    }
    return actualPath;
}

}  // namespace

bool requirementMet(RenderRequirement requirement, QString* reason)
{
    switch (requirement)
    {
    case RenderRequirement::None:
        return true;
    case RenderRequirement::ShxFont:
        if (DMFONTLIST->requestFont(QStringLiteral("txt.shx"), false) == nullptr)
        {
            *reason = QStringLiteral("没有 SHX 字体 txt.shx（放进 YiCAD/support/fonts/ 后运行）");
            return false;
        }
        return true;
    case RenderRequirement::TrueTypeFont:
        if (DMFONTLIST->requestFont(QStringLiteral("arial.ttf"), false) == nullptr)
        {
            *reason = QStringLiteral("系统没有 Arial 字体");
            return false;
        }
        return true;
    }
    return true;
}

bool ListSelection::isSelected(const DmEntity& entity) const
{
    return std::find(entities.begin(), entities.end(), &entity) != entities.end();
}

bool ListHidden::isHidden(const DmEntity& entity) const
{
    return std::find(entities.begin(), entities.end(), &entity) != entities.end();
}

std::vector<DmEntity*> visibleEntitiesOfType(DmDocument& document, std::initializer_list<DM::EntityType> types)
{
    std::vector<DmEntity*> found;
    for (DmEntity* entity : *document.getEntityTable())
    {
        if (entity->isVisible() && std::find(types.begin(), types.end(), entity->getEntityType()) != types.end())
        {
            found.push_back(entity);
        }
    }
    return found;
}

RenderScene::RenderScene(const RenderRequest& request)
    : m_document(std::make_unique<DmDocument>())
{
    DxfRuntime* runtime = RenderEnvironment::runtime();
    if (runtime == nullptr || !runtime->loaded())
    {
        m_error = QStringLiteral("DXF 插件没有加载：") + (runtime ? runtime->diagnostics() : QString());
        return;
    }

    const QString path = QDir(drawingsDir()).filePath(request.drawing);
    if (!runtime->readFile(*m_document, path))
    {
        m_error = QStringLiteral("读入失败：%1\n插件消息：%2").arg(path, runtime->messages().join(QLatin1Char('\n')));
        return;
    }
    m_document->setGridOn(request.grid);
    if (request.selectCircles)
    {
        m_selection.entities = visibleEntitiesOfType(*m_document, {DM::EntityCircle, DM::EntityBlockReference});
    }
    if (request.highlightArcs)
    {
        m_highlight.entities = visibleEntitiesOfType(*m_document, {DM::EntityArc});
    }

    m_view = std::make_unique<GuiDocumentView>(nullptr, Qt::WindowFlags(), m_document.get());
    GuiDocumentView& view = *m_view;
    // 颜色取程序的默认值，不读测试进程的设置
    view.setBackground(QColor(Colors::BACKGROUND));
    view.setGridColor(QColor(Colors::GRID));
    view.setMetaGridColor(QColor(Colors::META_GRID));
    view.setSelectedColor(QColor(Colors::SELECT));
    view.setHighlightColor(QColor(Colors::HIGHLIGHT));
    view.setIsDrawCursor(false);
    view.setDocumentSelectionSource(&m_selection);
    view.setDocumentHighlightSource(&m_highlight);
    view.setHiddenSource(&m_hidden);
    view.resize(request.width, request.height);
    // 不显示在屏幕上，但按可见控件走尺寸流程：没有 show() 的 QOpenGLWidget 收不到尺寸事件，
    // resizeGL 不被调用，画笔的设备尺寸是 0，画不出任何东西
    view.setAttribute(Qt::WA_DontShowOnScreen);
    view.show();

    // 第一次取图建立 GL 上下文
    (void)view.grabFramebuffer();
    // 按有限实体的范围取景：不用 zoomAuto()，射线与构造线会把实体表的范围撑到无穷大
    DmVector min(false);
    DmVector max(false);
    for (DmEntity* entity : *m_document->getEntityTable())
    {
        if (!entity->isVisible() || entity->getEntityType() == DM::EntityRay ||
            entity->getEntityType() == DM::EntityXline)
        {
            continue;
        }
        min = min.valid ? DmVector::minimum(min, entity->getMin()) : entity->getMin();
        max = max.valid ? DmVector::maximum(max, entity->getMax()) : entity->getMax();
    }
    if (!min.valid || !max.valid)
    {
        m_error = QStringLiteral("图纸里没有有限大小的实体：") + path;
        return;
    }
    const double unitsPerPixel =
        std::max((max.x - min.x) / request.width, (max.y - min.y) / request.height) * request.margin;
    view.setView((min + max) / 2.0, unitsPerPixel);
    view.setDraftMode(request.lineWidth);
    (void)view.grabFramebuffer();
}

RenderScene::~RenderScene() = default;

QImage RenderScene::grab()
{
    if (!m_view)
    {
        return QImage();
    }
    return m_view->grabFramebuffer().convertToFormat(QImage::Format_RGB32);
}

QImage renderDrawing(const RenderRequest& request, QString* error)
{
    RenderScene scene(request);
    if (!scene.error().isEmpty())
    {
        *error = scene.error();
        return QImage();
    }
    QImage image = scene.grab();
    if (image.isNull())
    {
        *error = QStringLiteral("grabFramebuffer 返回空图像");
    }
    return image;
}

void expectMatchesBaseline(const QString& name, const QImage& actual)
{
    ASSERT_FALSE(actual.isNull());
    const QString baselinePath = QDir(baselineDir()).filePath(name + QStringLiteral(".png"));

    if (qEnvironmentVariableIntValue("YICAD_RENDER_UPDATE_BASELINE") != 0)
    {
        QDir().mkpath(QFileInfo(baselinePath).absolutePath());
        ASSERT_TRUE(actual.save(baselinePath)) << baselinePath.toStdString();
        std::printf("已更新基准图像 %s\n", baselinePath.toLocal8Bit().constData());
        return;
    }

    QImage expected(baselinePath);
    if (expected.isNull())
    {
        const QString written = writeOutput(name, actual, nullptr);
        FAIL() << "缺少基准图像 " << baselinePath.toStdString() << "；实际图像已写到 " << written.toStdString()
               << "，确认无误后设 YICAD_RENDER_UPDATE_BASELINE=1 重跑以生成";
    }
    expected = expected.convertToFormat(QImage::Format_RGB32);
    if (expected.size() != actual.size())
    {
        const QString written = writeOutput(name, actual, nullptr);
        FAIL() << "尺寸不同：基准 " << expected.width() << "x" << expected.height() << "，实际 " << actual.width()
               << "x" << actual.height() << "；实际图像见 " << written.toStdString();
    }

    const ImageDiff diff = diffImages(expected, actual);
    const int allowed = static_cast<int>(kMaxDifferentRatio * actual.width() * actual.height());
    if (diff.differing > allowed)
    {
        const QString written = writeOutput(name, actual, &diff.marked);
        ADD_FAILURE() << name.toStdString() << "：" << diff.differing << " 个像素超出容差（允许 " << allowed
                      << "，最大通道差 " << diff.maxDelta << "）；实际图像与差异图见 " << written.toStdString();
    }
}

void expectSameImage(const QString& name, const QImage& expected, const QImage& actual)
{
    ASSERT_FALSE(expected.isNull());
    ASSERT_FALSE(actual.isNull());
    ASSERT_EQ(expected.size(), actual.size());

    const ImageDiff diff = diffImages(expected, actual);
    const int allowed = static_cast<int>(kMaxDifferentRatio * actual.width() * actual.height());
    if (diff.differing > allowed)
    {
        const QString written = writeOutput(name, actual, &diff.marked);
        expected.save(QStringLiteral(YICAD_RENDER_OUTPUT_DIR "/%1.expected.png").arg(name));
        ADD_FAILURE() << name.toStdString() << "：" << diff.differing << " 个像素超出容差（允许 " << allowed
                      << "，最大通道差 " << diff.maxDelta << "）；实际图像与差异图见 " << written.toStdString();
    }
}

}  // namespace yicad_test
