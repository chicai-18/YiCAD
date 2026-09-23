/// @file test_view_tool_control.cpp
/// @brief ViewToolControl 的分发顺序与光标仲裁单测
///
/// 用纯粹的 mock IViewTool（不依赖 DmDocument/GuiDocumentView）验证：
///   - 业务工具栈后进先出、选择工具次之、导航工具兜底的分发顺序；
///   - 一旦某层返回非 NotHandled，分发即停止，不再下传；
///   - 光标仲裁按相同顺序取首个非 nullopt 的偏好；
///   - 全体无偏好时不触碰当前光标（阶段2对 DS 参考实现的刻意偏离，
///     见 ViewToolControl.h 顶部注释）。

#include <gtest/gtest.h>

#include <QMouseEvent>
#include <QPointF>

#include "support/FakeDocumentView.h"
#include "ViewToolControl.h"

namespace
{
/// @brief 可编程的 mock 工具：记录调用次数，返回值/光标偏好可配置
class MockTool : public IViewTool
{
public:
    explicit MockTool(std::string name) : m_name(std::move(name)) {}

    ViewToolResult mousePressEvent(QMouseEvent*) override
    {
        ++pressCount;
        return pressResult;
    }

    std::optional<DM::CursorType> getCursor() const override { return cursor; }

    void onActivate() override { ++activateCount; }
    void onDeactivate() override { ++deactivateCount; }

    std::string m_name;
    int pressCount = 0;
    int activateCount = 0;
    int deactivateCount = 0;
    ViewToolResult pressResult = ViewToolResult::NotHandled;
    std::optional<DM::CursorType> cursor;
};

QMouseEvent makePress()
{
    return QMouseEvent(QEvent::MouseButtonPress, QPointF(1, 1), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
}
}  // namespace

TEST(ViewToolControlTest, 无任何工具时事件未处理)
{
    FakeDocumentView view;
    ViewToolControl control(&view);
    QMouseEvent e = makePress();
    EXPECT_EQ(control.mousePressEvent(&e), ViewToolResult::NotHandled);
}

TEST(ViewToolControlTest, 导航工具作为兜底能收到事件)
{
    FakeDocumentView view;
    ViewToolControl control(&view);
    MockTool nav("nav");
    nav.pressResult = ViewToolResult::Handled;
    control.setNavigationTool(&nav);

    QMouseEvent e = makePress();
    EXPECT_EQ(control.mousePressEvent(&e), ViewToolResult::Handled);
    EXPECT_EQ(nav.pressCount, 1);
}

TEST(ViewToolControlTest, 业务工具优先于选择工具优先于导航工具)
{
    FakeDocumentView view;
    ViewToolControl control(&view);

    MockTool nav("nav");
    MockTool select("select");
    MockTool business("business");
    nav.pressResult = select.pressResult = business.pressResult = ViewToolResult::Handled;

    control.setNavigationTool(&nav);
    control.setSelectionTool(&select);
    control.activate(&business);

    QMouseEvent e = makePress();
    control.mousePressEvent(&e);

    // 业务工具应该是唯一被调用的一个：它先被尝试且返回 Handled，
    // 分发到此为止，选择/导航工具都不应该被调用。
    EXPECT_EQ(business.pressCount, 1);
    EXPECT_EQ(select.pressCount, 0);
    EXPECT_EQ(nav.pressCount, 0);
}

TEST(ViewToolControlTest, 未处理时继续下传直到导航工具)
{
    FakeDocumentView view;
    ViewToolControl control(&view);

    MockTool nav("nav");
    MockTool business("business");
    // business 不处理（NotHandled 是默认值），应继续下传到 nav
    nav.pressResult = ViewToolResult::Handled;

    control.setNavigationTool(&nav);
    control.activate(&business);

    QMouseEvent e = makePress();
    EXPECT_EQ(control.mousePressEvent(&e), ViewToolResult::Handled);
    EXPECT_EQ(business.pressCount, 1);
    EXPECT_EQ(nav.pressCount, 1);
}

TEST(ViewToolControlTest, 业务工具栈后进先出)
{
    FakeDocumentView view;
    ViewToolControl control(&view);

    MockTool first("first");
    MockTool second("second");
    // 两个都不处理事件，只用来验证调用顺序（后激活的先被问）
    control.activate(&first);
    control.activate(&second);

    QMouseEvent e = makePress();
    control.mousePressEvent(&e);

    EXPECT_EQ(first.pressCount, 1);
    EXPECT_EQ(second.pressCount, 1);
    // 都返回 NotHandled，最终整体未处理
    EXPECT_EQ(control.mousePressEvent(&e), ViewToolResult::NotHandled);
}

TEST(ViewToolControlTest, activate对同一工具幂等)
{
    FakeDocumentView view;
    ViewToolControl control(&view);
    MockTool tool("t");

    control.activate(&tool);
    control.activate(&tool);
    EXPECT_EQ(tool.activateCount, 1);
    EXPECT_TRUE(control.isActive(&tool));

    control.deactivate(&tool);
    EXPECT_EQ(tool.deactivateCount, 1);
    EXPECT_FALSE(control.isActive(&tool));

    // 重复停用不应该再次调用 onDeactivate
    control.deactivate(&tool);
    EXPECT_EQ(tool.deactivateCount, 1);
}

TEST(ViewToolControlTest, 光标仲裁取首个有偏好者)
{
    FakeDocumentView view;
    ViewToolControl control(&view);

    MockTool nav("nav");
    MockTool business("business");
    nav.cursor = DM::ArrowCursor;
    business.cursor = DM::ClosedHandCursor;

    control.setNavigationTool(&nav);
    control.activate(&business);

    QMouseEvent e = makePress();
    control.mousePressEvent(&e);  // 触发一次 refreshCursor

    ASSERT_TRUE(view.lastCursor().has_value());
    EXPECT_EQ(*view.lastCursor(), DM::ClosedHandCursor);
}

TEST(ViewToolControlTest, 全体无偏好时不触碰当前光标)
{
    FakeDocumentView view;
    ViewToolControl control(&view);

    MockTool nav("nav");  // 不设置 cursor，保持 nullopt
    control.setNavigationTool(&nav);

    QMouseEvent e = makePress();
    control.mousePressEvent(&e);

    // 从未调用过 setMouseCursor —— 没有任何工具表达偏好
    EXPECT_TRUE(view.cursorHistory.empty());
}

TEST(ViewToolControlTest, 光标偏好变化时才重新应用)
{
    FakeDocumentView view;
    ViewToolControl control(&view);

    MockTool nav("nav");
    nav.cursor = DM::OpenHandCursor;
    control.setNavigationTool(&nav);

    QMouseEvent e1 = makePress();
    QMouseEvent e2 = makePress();
    control.mousePressEvent(&e1);
    control.mousePressEvent(&e2);

    // 两次分发解析出的都是同一个光标，第二次应被去重，不重复调用
    EXPECT_EQ(view.cursorHistory.size(), 1u);
}
