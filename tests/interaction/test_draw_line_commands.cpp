/// @file test_draw_line_commands.cpp
/// @brief 直线类绘图命令（业务工具化第三步第②批）的单元测试
///
/// 逐项对照原 Action 经 GuiEventHandler 运行时的行为：注册类型与枚举桥接、第一步
/// 提示、右键退回、命令行输入、选项条的打开与关闭（被旧 Action 挂起时收起），以及
/// 放置工具的光标。不执行提交（见 CommandTestFixture.h）。

#include <gtest/gtest.h>

#include "BaseExclusiveCommand.h"
#include "CircleData.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "DrawLineBisectorCommand.h"
#include "DrawLineCommand.h"
#include "DrawLinePolygonCommand.h"
#include "DrawPolylineCommand.h"
#include "support/CommandTestFixture.h"

using namespace yicad_test;

namespace
{
/// @brief 本批迁移的命令、桥接的枚举与第一步提示
struct FirstStep
{
    DM::ActionType type;
    const char* id;
    const char* hint;
    const char* right;
};

const FirstStep kCommands[] = {
    {DM::ActionDrawLine, "draw.line", "Specify first point", "Cancel"},
    {DM::ActionDrawPolyline, "draw.polyline", "Specify first point", "Cancel"},
    {DM::ActionDrawLineRectangle, "draw.line_rectangle", "Specify first corner", "Cancel"},
    {DM::ActionDrawLinePolygonCenCor, "draw.line_polygon_cen_cor", "Specify center", ""},
    {DM::ActionDrawLinePolygonCenTan, "draw.line_polygon_cen_tan", "Specify center", ""},
    {DM::ActionDrawLineBisector, "draw.line_bisector", "Select first line", "Cancel"},
    {DM::ActionDrawLineTangent1, "draw.line_tangent1", "Specify point", "Cancel"},
    {DM::ActionDrawLineTangent2, "draw.line_tangent2", "Select first circle or ellipse", "Cancel"},
    {DM::ActionDrawLineOrthTan, "draw.line_orth_tan", "Select a line", "Cancel"},
    {DM::ActionDrawLineFree, "draw.line_free", "Click and drag to draw a line", "Cancel"},
    {DM::ActionDrawRay, "draw.ray", "Specify first point", "Cancel"},
    {DM::ActionDrawXline, "draw.xline", "Specify first point", "Cancel"},
    {DM::ActionDrawPoint, "draw.point", "Specify location", "Cancel"},
};

/// @brief 有选项条的命令
const char* const kWithOptions[] = {"draw.line", "draw.polyline", "draw.line_polygon_cen_cor",
                                    "draw.line_polygon_cen_tan", "draw.line_bisector"};

bool hasOptions(const char* id)
{
    for (const char* withOptions : kWithOptions)
    {
        if (QLatin1String(withOptions) == QLatin1String(id))
        {
            return true;
        }
    }
    return false;
}

struct DrawLineFixture : CommandFixture
{
    template <typename Command>
    Command* active() const
    {
        return dynamic_cast<Command*>(bus.activeCommand());
    }
};
}  // namespace

TEST_F(DrawLineFixture, 注册为交互命令并保留枚举桥接)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        EXPECT_EQ(CommandRegistry::instance().kind(step.id), CommandKind::Exclusive);
        EXPECT_EQ(CommandRegistry::instance().commandId(step.type), QString::fromLatin1(step.id));
    }
}

TEST_F(DrawLineFixture, 启动后给出第一步提示并按需打开选项条)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        ui.options.clear();
        ASSERT_TRUE(start(step.id));
        EXPECT_EQ(ui.lastHint(), QString::fromLatin1(step.hint));
        EXPECT_EQ(ui.lastRightHint(), QString::fromLatin1(step.right));

        const OptionsRequest* shown = lastOptions(step.id);
        if (hasOptions(step.id))
        {
            ASSERT_NE(shown, nullptr);
            EXPECT_TRUE(shown->on);
            EXPECT_FALSE(shown->update);
        }
        else
        {
            EXPECT_EQ(shown, nullptr);
        }

        endCommand();
        EXPECT_FALSE(bus.hasActiveCommand());
        const OptionsRequest* hidden = lastOptions(step.id);
        if (hasOptions(step.id))
        {
            ASSERT_NE(hidden, nullptr);
            EXPECT_FALSE(hidden->on);
        }
    }
}

TEST_F(DrawLineFixture, 右键在第一步结束命令)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        ASSERT_TRUE(start(step.id));
        rightClick();
        EXPECT_FALSE(bus.hasActiveCommand());
    }
}

TEST_F(DrawLineFixture, 放置工具光标选线类为选择光标其余为十字)
{
    const std::pair<const char*, DM::CursorType> cursors[] = {
        {"draw.line", DM::CadCursor},
        {"draw.line_bisector", DM::SelectCursor},
        {"draw.line_orth_tan", DM::SelectCursor},
        {"draw.line_tangent2", DM::SelectCursor},
        {"draw.line_tangent1", DM::CadCursor},
    };
    for (const auto& [id, expected] : cursors)
    {
        SCOPED_TRACE(id);
        ASSERT_TRUE(start(id));
        ASSERT_TRUE(cursor().has_value());
        EXPECT_EQ(*cursor(), expected);
        endCommand();
    }
}

TEST_F(DrawLineFixture, 画直线指定起点后预览并右键开始新线段组)
{
    ASSERT_TRUE(start("draw.line"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify next point"));
    EXPECT_EQ(ui.lastRightHint(), QStringLiteral("Back"));
    EXPECT_EQ(view.getRelativeZero(), DmVector(0, 0));

    move(10, 5);
    EXPECT_EQ(previewCount(), 1);

    // 右键：开始下一线段组，回到指定第一点，不结束命令
    rightClick();
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first point"));
    EXPECT_EQ(previewCount(), 0);
}

TEST_F(DrawLineFixture, 画直线命令行文本都被当作redo接受)
{
    // 与原 ActionDrawLine 一致（既有缺陷，照原样保留）：Commands::checkCommand 对
    // help/close/undo 以外的关键字一律返回 true（cmd/Commands.cpp 的 checkCommand），
    // 所以任何文本都匹配 "redo"：被接受、执行重做，不会被当作新命令
    ASSERT_TRUE(start("draw.line"));
    EXPECT_TRUE(typeText(QStringLiteral("help")));
    EXPECT_FALSE(ui.messages.empty());
    EXPECT_TRUE(typeText(QStringLiteral("circle")));
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Cannot redo: End of history reached"));
}

TEST_F(DrawLineFixture, 画直线没有线段时闭合与撤销给出提示)
{
    ASSERT_TRUE(start("draw.line"));
    auto* command = active<DrawLineCommand>();
    ASSERT_NE(command, nullptr);

    typeCoordinate(0, 0);
    command->close();
    ASSERT_FALSE(ui.messages.empty());
    EXPECT_TRUE(ui.messages.back().startsWith(QStringLiteral("Cannot close sequence of lines")));

    // 撤销起点：回到指定第一点
    command->undo();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first point"));
    command->undo();
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Cannot undo: Begin of history reached"));
}

TEST_F(DrawLineFixture, 画多段线指定起点后闭合给出提示)
{
    ASSERT_TRUE(start("draw.polyline"));
    auto* command = active<DrawPolylineCommand>();
    ASSERT_NE(command, nullptr);

    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify next point"));
    move(10, 0);
    EXPECT_EQ(previewCount(), 1);

    command->close();
    EXPECT_EQ(ui.messages.back(),
              QStringLiteral("Cannot close sequence of lines: Not enough entities defined yet."));
    command->undo();
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Cannot undo: Not enough entities defined yet."));
}

TEST_F(DrawLineFixture, 画多段线有线宽时预览为填充四边形)
{
    ASSERT_TRUE(start("draw.polyline"));
    auto* command = active<DrawPolylineCommand>();
    ASSERT_NE(command, nullptr);
    command->setStartWeight(2.0);
    command->setEndWeight(2.0);

    typeCoordinate(0, 0);
    move(10, 0);
    ASSERT_EQ(previewCount(), 1);
    EXPECT_EQ(view.getPreviewContainer()->first()->getEntityType(), DM::EntitySolid);
}

TEST_F(DrawLineFixture, 画多段线右键退回时丢弃起点)
{
    ASSERT_TRUE(start("draw.polyline"));
    typeCoordinate(0, 0);
    rightClick();
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first point"));
    // 起点已丢弃：移动鼠标不再预览
    move(10, 0);
    EXPECT_EQ(previewCount(), 0);
}

TEST_F(DrawLineFixture, 矩形指定第一个角点后预览右键退回)
{
    ASSERT_TRUE(start("draw.line_rectangle"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second corner"));
    EXPECT_EQ(ui.lastRightHint(), QStringLiteral("Back"));
    move(10, 10);
    EXPECT_EQ(previewCount(), 1);

    rightClick();
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first corner"));
    EXPECT_EQ(previewCount(), 0);
}

TEST_F(DrawLineFixture, 多边形命令行number进入输入边数且不接受这段文本)
{
    for (const char* id : {"draw.line_polygon_cen_cor", "draw.line_polygon_cen_tan"})
    {
        SCOPED_TRACE(id);
        ASSERT_TRUE(start(id));
        auto* command = active<LinePolygonCommand>();
        ASSERT_NE(command, nullptr);
        EXPECT_EQ(command->getNumber(), 3);

        EXPECT_FALSE(typeText(QStringLiteral("number")));
        EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter number:"));

        ui.options.clear();
        EXPECT_TRUE(typeText(QStringLiteral("6")));
        EXPECT_EQ(command->getNumber(), 6);
        // 选项条按命令的边数刷新，回到进入前的状态
        const OptionsRequest* refresh = lastOptions(id);
        ASSERT_NE(refresh, nullptr);
        EXPECT_TRUE(refresh->on);
        EXPECT_TRUE(refresh->update);
        EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify center"));
        endCommand();
    }
}

TEST_F(DrawLineFixture, 多边形边数超出范围给出提示)
{
    ASSERT_TRUE(start("draw.line_polygon_cen_cor"));
    typeText(QStringLiteral("number"));
    EXPECT_TRUE(typeText(QStringLiteral("10000")));
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Not a valid number. Try 1..9999"));
    EXPECT_EQ(active<LinePolygonCommand>()->getNumber(), 3);
    endCommand();

    ASSERT_TRUE(start("draw.line_polygon_cen_tan"));
    typeText(QStringLiteral("number"));
    // 中心+切点的上限含 9999
    EXPECT_TRUE(typeText(QStringLiteral("9999")));
    EXPECT_EQ(active<LinePolygonCommand>()->getNumber(), 9999);
}

TEST_F(DrawLineFixture, 多边形指定中心后按边数预览)
{
    ASSERT_TRUE(start("draw.line_polygon_cen_cor"));
    active<LinePolygonCommand>()->setNumber(5);
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify a corner"));
    move(10, 0);
    EXPECT_EQ(previewCount(), 5);
}

TEST_F(DrawLineFixture, 角平分线命令行设置长度与数量)
{
    ASSERT_TRUE(start("draw.line_bisector"));
    auto* command = active<DrawLineBisectorCommand>();
    ASSERT_NE(command, nullptr);

    EXPECT_FALSE(typeText(QStringLiteral("length")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter bisector length:"));
    EXPECT_TRUE(typeText(QStringLiteral("12.5")));
    EXPECT_DOUBLE_EQ(command->getLength(), 12.5);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Select first line"));

    // 与原 ActionDrawLineBisector 一致（既有缺陷，照原样保留）：Commands::checkCommand
    // 对 "length" 一律返回 true，先检查的 length 截住了任何文本，命令行进不了输入数量
    EXPECT_FALSE(typeText(QStringLiteral("number")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter bisector length:"));
    EXPECT_TRUE(typeText(QStringLiteral("300")));
    EXPECT_DOUBLE_EQ(command->getLength(), 300.0);
    EXPECT_EQ(command->getNumber(), 1);
}

TEST_F(DrawLineFixture, 射线在方向一步右键只退回状态)
{
    for (const char* id : {"draw.ray", "draw.xline"})
    {
        SCOPED_TRACE(id);
        ASSERT_TRUE(start(id));
        typeCoordinate(0, 0);
        EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify direction"));
        move(10, 10);
        EXPECT_EQ(previewCount(), 1);

        rightClick();
        EXPECT_TRUE(bus.hasActiveCommand());
        EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first point"));
        EXPECT_EQ(previewCount(), 0);
        endCommand();
    }
}

TEST_F(DrawLineFixture, 过点切线指定点后改为选择光标)
{
    ASSERT_TRUE(start("draw.line_tangent1"));
    typeCoordinate(1, 2);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Select circle, arc or ellipse"));
    EXPECT_EQ(view.getRelativeZero(), DmVector(1, 2));
    move(5, 5);
    ASSERT_TRUE(cursor().has_value());
    EXPECT_EQ(*cursor(), DM::SelectCursor);
}

TEST_F(DrawLineFixture, 两圆公切线第二步结束命令不崩溃)
{
    // 原 ActionDrawLineTangent2::finish 在选中第一个圆、还没选中第二个时解引用空指针
    auto* circle = new DmCircle(nullptr, CircleData(DmVector(0, 0), 5.0));
    circle->calculateBorders();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(circle));

    ASSERT_TRUE(start("draw.line_tangent2"));
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Select second circle or ellipse"));
    EXPECT_TRUE(circle->isHighlighted());

    endCommand();
    EXPECT_FALSE(circle->isHighlighted());
}

TEST_F(DrawLineFixture, 徒手线按下进入拖动释放回到第一步)
{
    ASSERT_TRUE(start("draw.line_free"));
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 0, 0, Qt::LeftButton);
    dispatch([&] { return control.mousePressEvent(&press); });
    // 只有一个点：释放时不提交
    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 0, 0, Qt::LeftButton);
    dispatch([&] { return control.mouseReleaseEvent(&release); });
    EXPECT_TRUE(bus.hasActiveCommand());

    // 右键：退回到第一步，再右键结束
    rightClick();
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(DrawLineFixture, 徒手线结束后预览容器恢复持有实体)
{
    ASSERT_TRUE(view.getPreviewContainer()->isOwner());
    ASSERT_TRUE(start("draw.line_free"));
    EXPECT_FALSE(view.getPreviewContainer()->isOwner());
    endCommand();
    EXPECT_TRUE(view.getPreviewContainer()->isOwner());
}

TEST_F(DrawLineFixture, 旧Action叠上来时收起选项条结束后重新显示)
{
    ASSERT_TRUE(start("draw.line"));
    ui.options.clear();
    bus.suspend();
    ASSERT_NE(lastOptions("draw.line"), nullptr);
    EXPECT_FALSE(lastOptions("draw.line")->on);

    bus.resume();
    ASSERT_NE(lastOptions("draw.line"), nullptr);
    EXPECT_TRUE(lastOptions("draw.line")->on);

    // 挂起期间被结束：不再重复收起选项条
    bus.suspend();
    ui.options.clear();
    endCommand();
    EXPECT_EQ(lastOptions("draw.line"), nullptr);
}

TEST_F(DrawLineFixture, 放置工具不接受Esc且中键让给导航层)
{
    ASSERT_TRUE(start("draw.polyline"));
    EXPECT_FALSE(pressKey(Qt::Key_Escape));
    EXPECT_TRUE(bus.hasActiveCommand());

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 0, 0, Qt::MiddleButton);
    dispatch([&] { return control.mousePressEvent(&press); });
    EXPECT_TRUE(panTool.isPanning());
}
