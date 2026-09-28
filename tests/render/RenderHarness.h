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

#include <initializer_list>
#include <memory>
#include <vector>

#include "Datamodel.h"
#include "IHighlightSource.h"
#include "ISelectionSource.h"

class DmDocument;
class DmEntity;
class GuiDocumentView;

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

/// @brief 按列表选中的实体；测试改了列表之后调 GuiDocumentView::specifySelectChanged()
class ListSelection : public ISelectionSource
{
public:
    bool isSelected(const DmEntity& entity) const override;
    std::vector<DmEntity*> selectedEntities() const override { return entities; }

    std::vector<DmEntity*> entities;  ///< 选中的实体，只放可见的顶层实体
};

/// @brief 按列表高亮的实体；测试改了列表之后调 GuiDocumentView::specifyHighlightChanged()
class ListHighlight : public IHighlightSource
{
public:
    std::vector<DmEntity*> highlightedEntities() const override { return entities; }

    std::vector<DmEntity*> entities;  ///< 高亮的实体，只放可见的顶层实体
};

/// @brief 图纸里指定类型的可见顶层实体，按实体表的顺序
std::vector<DmEntity*> visibleEntitiesOfType(DmDocument& document, std::initializer_list<DM::EntityType> types);

/// @brief 一张读入的参考图纸与它的离屏画布：构造时按请求取景并画过，之后可以改选择、高亮、视图再取图
class RenderScene
{
public:
    explicit RenderScene(const RenderRequest& request);
    ~RenderScene();

    /// @brief 读图纸或建画布失败时的说明；成功时为空
    const QString& error() const { return m_error; }

    DmDocument& document() { return *m_document; }
    GuiDocumentView& view() { return *m_view; }
    /// @brief 画布的选择来源；请求了 selectCircles 时里面是圆与块参照
    ListSelection& selection() { return m_selection; }
    /// @brief 画布的高亮来源；请求了 highlightArcs 时里面是圆弧
    ListHighlight& highlight() { return m_highlight; }

    /// @brief 画一帧并取图（RGB32）
    QImage grab();

private:
    QString m_error;
    std::unique_ptr<DmDocument> m_document;
    ListSelection m_selection;
    ListHighlight m_highlight;
    std::unique_ptr<GuiDocumentView> m_view;  ///< 最后建、最先释放：它是文档的监听者，又读两个来源
};

/// @brief 按请求画出图像（RGB32）；失败时返回空图像并在 error 里说明
QImage renderDrawing(const RenderRequest& request, QString* error);

/// @brief 与 tests/render/baseline/<name>.png 比对，结果以 gtest 断言报告
/// @details 设了环境变量 YICAD_RENDER_UPDATE_BASELINE=1 时改为写入基准图像。
///          不一致或缺少基准图像时，把实际图像与差异图写到构建目录的 tests/render/output/。
void expectMatchesBaseline(const QString& name, const QImage& actual);

/// @brief 两张图按与 expectMatchesBaseline() 相同的容差比对，结果以 gtest 断言报告
/// @details 不一致时把两张图与差异图写到构建目录的 tests/render/output/<name>.{expected,actual,diff}.png。
void expectSameImage(const QString& name, const QImage& expected, const QImage& actual);

}  // namespace yicad_test

#endif  // YICAD_TEST_RENDER_HARNESS_H
