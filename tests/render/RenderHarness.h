/// @file RenderHarness.h
/// @brief 出图测试的公共部分：读参考图纸、离屏画成图像、与基准图像按容差比对
///
/// 画的是程序里的画布 GuiDocumentView（渲染方案阶段 4 起旧渲染器与 GS 各画一遍），
/// 经 QOpenGLWidget::grabFramebuffer() 离屏取图，不显示窗口。GL 实现是 MesaLoader.cpp 装入的
/// Mesa llvmpipe，结果在不同机器上一致，所以基准图像可以入库。

#ifndef YICAD_TEST_RENDER_HARNESS_H
#define YICAD_TEST_RENDER_HARNESS_H

#include <QImage>
#include <QString>

#include <vector>

class DmDocument;
class DmEntity;

namespace yicad_test
{

/// @brief 参考图纸依赖的外部资源；缺少时用例跳过（用户的决定：缺字体就跳过）
enum class RenderRequirement
{
    None,
    ShxFont,       ///< txt.shx（用户自备，不入库，CI 上没有）
    TrueTypeFont   ///< 系统字体 Arial
};

/// @brief 一张出图：哪张图纸、画布多大、怎么显示
struct RenderRequest
{
    QString drawing;                ///< tests/render/drawings/ 下的文件名
    int width = 640;                ///< 画布宽（像素）
    int height = 480;               ///< 画布高（像素）
    double margin = 1.06;           ///< 适屏后再缩小的倍数，给图形四周留白
    bool lineWidth = false;         ///< 显示线宽（GuiDocumentView::setDraftMode）
    bool grid = false;              ///< 显示网格
    bool selectCircles = false;     ///< 圆与块参照按选中绘制（经 ISelectionSource）
    bool highlightArcs = false;     ///< 圆弧按高亮绘制（经 IHighlightSource）
    RenderRequirement requirement = RenderRequirement::None;
};

/// @brief 资源是否就位；不就位时 reason 给出跳过的理由
bool requirementMet(RenderRequirement requirement, QString* reason);

/// @brief 按请求画出图像（RGB32）；失败时返回空图像并在 error 里说明
QImage renderDrawing(const RenderRequest& request, QString* error);

/// @brief 与 tests/render/baseline/<name>.png 比对，结果以 gtest 断言报告
/// @details 设了环境变量 YICAD_RENDER_UPDATE_BASELINE=1 时改为写入基准图像。
///          不一致或缺少基准图像时，把实际图像与差异图写到构建目录的 tests/render/output/。
void expectMatchesBaseline(const QString& name, const QImage& actual);

}  // namespace yicad_test

#endif  // YICAD_TEST_RENDER_HARNESS_H
