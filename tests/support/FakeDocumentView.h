/// @file FakeDocumentView.h
/// @brief 测试用的 IDocumentView 最小实现
///
/// 交互层工具（PanZoomTool、ViewToolControl 等）只依赖 IDocumentView，
/// 不依赖具体的 GuiDocumentView（OpenGL 画布控件），这正是阶段1抽出
/// IDocumentView 接口的意义——阶段2得以脱离 Qt 界面组件对它们单独测试。
///
/// 本类只对测试真正用到的几个方法（setMouseCursor / zoomPan / redraw）
/// 记录调用，其余纯虚方法给出编译期需要的最小实现。

#ifndef YICAD_TEST_FAKE_DOCUMENT_VIEW_H
#define YICAD_TEST_FAKE_DOCUMENT_VIEW_H

#include <optional>
#include <vector>

#include "DmEntityContainer.h"
#include "GuiGrid.h"
#include "IDocumentView.h"

class FakeDocumentView : public IDocumentView
{
public:
    // ---- 供测试断言的记录 ----
    std::vector<DM::CursorType> cursorHistory;
    int redrawCount = 0;
    int zoomPanCount = 0;
    int lastPanDx = 0;
    int lastPanDy = 0;
    int selectedChangedCount = 0;

    std::optional<DM::CursorType> lastCursor() const
    {
        return cursorHistory.empty() ? std::nullopt : std::optional<DM::CursorType>(cursorHistory.back());
    }

    // ---- IDocumentView ----
    void redraw() override { ++redrawCount; }
    void setMouseCursor(DM::CursorType c) override { cursorHistory.push_back(c); }
    void setCursor(const QCursor&) override {}
    QObject* asQObject() override { return nullptr; }

    void zoomIn(double, const DmVector&) override {}
    void zoomOut(double, const DmVector&) override {}
    void zoomAuto() override {}
    void zoomPan(int dx, int dy) override
    {
        ++zoomPanCount;
        lastPanDx = dx;
        lastPanDy = dy;
    }

    void setOverlayCorners(const DmVector&, const DmVector&) override {}
    void disableOverlayBox() override {}

    // Snapper::getSnapRange()/init() 无条件解引用 getGrid() 的返回值，
    // 不能给 nullptr；用一个真实的默认构造 GuiGrid 满足它。
    GuiGrid* getGrid() const override { return &m_grid; }

    void setDefaultSnapMode(SnapMode) override {}
    SnapMode getDefaultSnapMode() const override { return SnapMode{}; }
    void setSnapRestriction(DM::SnapRestriction) override {}

    DmVector toGui(DmVector v) const override { return v; }
    double toGuiDX(double d) const override { return d; }

    DmVector toGraph(DmVector v) const override { return v; }
    DmVector toGraph(int x, int y) const override { return DmVector(x, y); }
    double toGraphX(int x) const override { return x; }
    double toGraphY(int y) const override { return y; }
    double toGraphDX(int d) const override { return d; }

    DmVector const& getRelativeZero() const override { return m_relativeZero; }
    void moveRelativeZero(const DmVector& pos) override { m_relativeZero = pos; }

    void setOrthogonalZero(const DmVector& pos) override { m_orthogonalZero = pos; }
    DmVector const& getOrthogonalZero() const override { return m_orthogonalZero; }

    bool isCleanUp() const override { return false; }

    DmEntityContainer* getPreviewContainer() override { return &m_previewContainer; }
    void specifyPreviewModified() override {}
    void setPreviewTransform(const GiTransform& transform) override { previewTransform = transform; }
    std::shared_ptr<GsModel> graphicsModel() const override { return nullptr; }

    /// @brief 最近一次设置的预览变换（移动、复制等拖动时只改它）
    GiTransform previewTransform;

    DmRect getViewRect() override { return DmRect(); }

    void setIsDrawCursor(const bool&) override {}

    bool hasActiveCommand() const override { return false; }
    QString activeCommandId() const override { return QString(); }
    void emitSelectedChanged() override { ++selectedChangedCount; }

    void enableCoordinateInput() override {}
    void disableCoordinateInput() override {}

    DmDocument* getDocument() const override { return nullptr; }
    DmVector getFactor() const override { return DmVector(1.0, 1.0); }

private:
    DmVector m_relativeZero{false};
    DmVector m_orthogonalZero{false};
    mutable GuiGrid m_grid;               ///< getGrid() 是 const，需要 mutable 才能返回非 const 指针
    DmEntityContainer m_previewContainer; ///< 预览容器
};

#endif  // YICAD_TEST_FAKE_DOCUMENT_VIEW_H
