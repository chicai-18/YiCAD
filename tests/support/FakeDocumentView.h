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

    // 默认空实现（大多数测试不需要）；`eventHandler` 供测试按需注入一个
    // 真实的 GuiEventHandler，用于验证依赖 hasAction() 的让路/仲裁逻辑
    // （比如 SelectTool::getCursor()）。
    GuiEventHandler* eventHandler = nullptr;
    GuiEventHandler* getEventHandler() const override { return eventHandler; }

    bool isCleanUp() const override { return false; }

    // Snapper::deleteSnapper() 等无条件解引用返回值；同一个容器复用于
    // 所有 overlay 位置，测试不关心不同图层之间的隔离。
    DmEntityContainer* getOverlayContainer(DM::OverlayDocument) override { return &m_overlayContainer; }
    DmEntityContainer* getPreviewContainer() override { return &m_overlayContainer; }
    void specifyPreviewModified() override {}
    void specifyDocumentModified() override {}
    void setPreviewModelOffset(const DmVector&) override {}

    DmRect getViewRect() override { return DmRect(); }

    void setIsDrawCursor(const bool&) override {}

    void setCurrentAction(ActionInterface*) override {}
    ActionInterface* getCurrentAction() override { return nullptr; }
    void killSelectActions() override {}
    void emitSelectedChanged() override {}

    void enableCoordinateInput() override {}
    void disableCoordinateInput() override {}

    DmDocument* getDocument() const override { return nullptr; }
    DmVector getFactor() const override { return DmVector(1.0, 1.0); }

private:
    DmVector m_relativeZero{false};
    DmVector m_orthogonalZero{false};
    mutable GuiGrid m_grid;               ///< getGrid() 是 const，需要 mutable 才能返回非 const 指针
    DmEntityContainer m_overlayContainer;
};

#endif  // YICAD_TEST_FAKE_DOCUMENT_VIEW_H
