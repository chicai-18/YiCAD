/// @file test_draw_curve_commands.cpp
/// @brief 曲线类绘图命令（业务工具化第三步第③批）的单元测试
///
/// 对照原 Action 的行为：注册类型、第一步提示、选项条、右键退回、命令行
/// 输入、三点圆弧切换为圆心圆弧、选项条参数转给工具、插入图片取消对话框时启动失败。
/// 不执行提交（见 CommandTestFixture.h）。

#include <gtest/gtest.h>

#include "DrawArcCommand.h"
#include "DrawArcTangentialCommand.h"
#include "DrawCircleTan2Command.h"
#include "DrawCloudLineCommand.h"
#include "DrawSplineCommand.h"
#include "support/CommandTestFixture.h"

using namespace yicad_test;

namespace
{
struct FirstStep
{
    const char* id;
    const char* hint;
    const char* right;
    bool options;
};

const FirstStep kCommands[] = {
    {"ext.draw.arc", "Specify center", "Cancel", true},
    {"ext.draw.arc_3p", "Specify startpoint or [center]", "Cancel", false},
    {"ext.draw.arc_tangential", "Specify base entity", "Cancel", true},
    {"ext.draw.circle", "Specify center", "Cancel", false},
    {"ext.draw.circle_2p", "Specify first point", "Cancel", false},
    {"ext.draw.circle_3p", "Specify first point", "Cancel", false},
    {"ext.draw.circle_tan2", "Specify the first line/arc/circle", "Cancel", true},
    {"ext.draw.circle_tan3", "Specify the first line/arc/circle", "Cancel", false},
    {"ext.draw.ellipse_axis", "Specify ellipse center", "Cancel", false},
    {"ext.draw.ellipse_arc_axis", "Specify ellipse center", "Cancel", false},
    {"ext.draw.ellipse_inscribe", "Specify the first line", "Cancel", false},
    {"ext.draw.spline", "Specify first control point", "Cancel", true},
    {"ext.draw.spline_points", "Specify first control point", "Cancel", true},
    {"ext.draw.cloud_line_rectangle", "Specify first point", "Cancel", true},
    {"ext.draw.cloud_line_polygon", "Specify first point", "Cancel", true},
    {"ext.draw.cloud_line_free", "Specify first point", "Cancel", true},
};

struct DrawCurveFixture : BuiltinCommandFixture
{
    template <typename Command>
    Command* active() const
    {
        return dynamic_cast<Command*>(bus.activeCommand());
    }
};
}  // namespace

TEST_F(DrawCurveFixture, 注册为交互命令)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        EXPECT_EQ(CommandRegistry::instance().kind(step.id), CommandKind::Exclusive);
    }
    EXPECT_EQ(CommandRegistry::instance().kind("ext.draw.image"), CommandKind::Exclusive);
}

TEST_F(DrawCurveFixture, 启动后给出第一步提示并按需打开选项条右键结束)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        ui.options.clear();
        ASSERT_TRUE(start(step.id));
        EXPECT_EQ(ui.lastHint(), QString::fromLatin1(step.hint));
        EXPECT_EQ(ui.lastRightHint(), QString::fromLatin1(step.right));
        const OptionsRequest* shown = lastOptions(step.id);
        if (step.options)
        {
            ASSERT_NE(shown, nullptr);
            EXPECT_TRUE(shown->on);
        }
        else
        {
            EXPECT_EQ(shown, nullptr);
        }
        rightClick();
        EXPECT_FALSE(bus.hasActiveCommand());
    }
}

TEST_F(DrawCurveFixture, 插入图片取消选择对话框时启动失败)
{
    // 文件对话框取消时路径为空：原 Action 被标记为结束，现在命令启动失败
    EXPECT_FALSE(start("ext.draw.image"));
    EXPECT_EQ(dialogs.shown, std::vector<QString>{QStringLiteral("QFileDialog")});
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(lastOptions("ext.draw.image"), nullptr);
}

TEST_F(DrawCurveFixture, 圆心圆弧逐步提示并接受命令行半径)
{
    ASSERT_TRUE(start("ext.draw.arc"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify radius"));
    move(10, 0);
    EXPECT_EQ(previewCount(), 1);

    EXPECT_FALSE(typeText(QStringLiteral("abc")));
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Not a valid expression"));
    EXPECT_TRUE(typeText(QStringLiteral("5")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify start angle:"));
    EXPECT_TRUE(typeText(QStringLiteral("30")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify arc angle"));

    // 右键退回上一步
    rightClick();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify start angle:"));
}

TEST_F(DrawCurveFixture, 圆心圆弧的方向由选项条转给工具)
{
    ASSERT_TRUE(start("ext.draw.arc"));
    auto* command = active<DrawArcCommand>();
    ASSERT_NE(command, nullptr);
    EXPECT_FALSE(command->isClockwise());
    command->setClockwise(true);
    EXPECT_TRUE(command->isClockwise());
    // 与原 Action 一致：右键退回时圆弧复位，方向回到逆时针
    typeCoordinate(0, 0);
    rightClick();
    EXPECT_FALSE(command->isClockwise());
}

TEST_F(DrawCurveFixture, 三点圆弧命令行输入文字切换为圆心圆弧且不接受)
{
    ASSERT_TRUE(start("ext.draw.arc_3p"));
    // 与原 Action 一致（既有缺陷）：任何文字都匹配 center
    EXPECT_FALSE(typeText(QStringLiteral("line")));
    EXPECT_EQ(bus.activeCommandId(), QStringLiteral("ext.draw.arc"));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify center"));
}

TEST_F(DrawCurveFixture, 三点圆弧第二点后预览直线)
{
    ASSERT_TRUE(start("ext.draw.arc_3p"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second point"));
    move(10, 0);
    EXPECT_EQ(previewCount(), 1);
}

TEST_F(DrawCurveFixture, 相切圆弧的锁定参数在命令上)
{
    ASSERT_TRUE(start("ext.draw.arc_tangential"));
    auto* command = active<DrawArcTangentialCommand>();
    ASSERT_NE(command, nullptr);
    EXPECT_FALSE(command->isLockRadius());
    EXPECT_DOUBLE_EQ(command->lockRadius(), 100.0);
    command->setIsLockAngle(true);
    command->setLockAngle(90.0);
    EXPECT_TRUE(command->isLockAngle());
    EXPECT_DOUBLE_EQ(command->lockAngle(), 90.0);
    // 没有选中基实体时重算预览什么也不做
    command->updatePreview();
    EXPECT_EQ(previewCount(), 0);
    ASSERT_TRUE(cursor().has_value());
    EXPECT_EQ(*cursor(), DM::SelectCursor);
}

TEST_F(DrawCurveFixture, 圆心画圆命令行半径无效时提示)
{
    ASSERT_TRUE(start("ext.draw.circle"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify point on circle"));
    move(5, 0);
    EXPECT_EQ(previewCount(), 1);
    EXPECT_FALSE(typeText(QStringLiteral("abc")));
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Not a valid expression"));
}

TEST_F(DrawCurveFixture, 两切圆半径由选项条转给工具)
{
    ASSERT_TRUE(start("ext.draw.circle_tan2"));
    auto* command = active<DrawCircleTan2Command>();
    ASSERT_NE(command, nullptr);
    command->setRadius(12.0);
    EXPECT_DOUBLE_EQ(command->getRadius(), 12.0);
}

TEST_F(DrawCurveFixture, 轴端点椭圆逐步提示与右键退回)
{
    ASSERT_TRUE(start("ext.draw.ellipse_arc_axis"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify endpoint of major axis"));
    move(10, 0);
    EXPECT_EQ(previewCount(), 1);
    typeCoordinate(10, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify endpoint or length of minor axis:"));
    // 椭圆弧：短轴之后是起止角
    typeCoordinate(0, 5);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify start angle"));
    rightClick();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify endpoint or length of minor axis:"));
}

TEST_F(DrawCurveFixture, 样条的阶数与闭合由选项条转给工具)
{
    ASSERT_TRUE(start("ext.draw.spline"));
    auto* command = active<DrawSplineCommand>();
    ASSERT_NE(command, nullptr);
    command->setDegree(2);
    EXPECT_EQ(command->getDegree(), 2);
    command->setClosed(true);
    EXPECT_TRUE(command->isClosed());
    command->undo();
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Cannot undo: Not enough entities defined yet."));

    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify next control point"));
}

TEST_F(DrawCurveFixture, 拟合点样条没有样条时撤销给出提示)
{
    ASSERT_TRUE(start("ext.draw.spline_points"));
    auto* command = active<DrawSplinePointsCommand>();
    ASSERT_NE(command, nullptr);
    command->undo();
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Cannot undo: Not enough entities defined yet."));
    command->setClosed(true);
    EXPECT_TRUE(command->isClosed());
}

TEST_F(DrawCurveFixture, 矩形云线第二点右键结束命令)
{
    ASSERT_TRUE(start("ext.draw.cloud_line_rectangle"));
    auto* command = active<DrawCloudLineRectangleCommand>();
    ASSERT_NE(command, nullptr);
    EXPECT_DOUBLE_EQ(command->getMinLength(), 5.0);
    EXPECT_DOUBLE_EQ(command->getMaxLength(), 10.0);

    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify next point"));
    move(100, 100);
    EXPECT_EQ(previewCount(), 1);
    // 与原 Action 一致：第二点右键经 init(-1) 结束命令
    rightClick();
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(DrawCurveFixture, 多边形云线点不够时回车给出错误提示)
{
    ASSERT_TRUE(start("ext.draw.cloud_line_polygon"));
    typeCoordinate(0, 0);
    typeCoordinate(100, 0);
    // 回车由主窗口合成为 Key_Enter，交给工具；与原 Action 一致，按键不接受
    EXPECT_FALSE(pressKey(Qt::Key_Enter));
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Can not create cloud line, please select again"));

    active<DrawCloudLinePolygonCommand>()->undo();
    EXPECT_TRUE(ui.messages.empty());
}

TEST_F(DrawCurveFixture, 自由云线反向参数在命令上)
{
    ASSERT_TRUE(start("ext.draw.cloud_line_free"));
    auto* command = active<DrawCloudLineFreeCommand>();
    ASSERT_NE(command, nullptr);
    EXPECT_FALSE(command->getReversed());
    command->setReversed(true);
    EXPECT_TRUE(command->getReversed());

    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Move cursor to get cloud line path..."));
}
