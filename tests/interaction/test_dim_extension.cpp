/// @file test_dim_extension.cpp
/// @brief 标注扩展（业务工具化第三步第⑨批：标注 Action 改为命令 + 放置工具）的单元测试
///
/// 覆盖：注册的命令类型、别名与线性标注的选项条、按钮挂进"绘图/标注"面板；各命令的第一步
/// 提示与右键结束；线性标注逐步提示、右键退回与选项条的角度转给工具；引线的完成与退回；
/// 没有文档时标注样式什么也不做。生成标注要标注样式与事务，不在单测范围内（见
/// CommandTestFixture.h）。

#include <gtest/gtest.h>

#include <memory>

#include "DimCommands.h"
#include "DimExtension.h"
#include "ExtensionManager.h"
#include "UIRibbonRegistry.h"
#include "support/CommandTestFixture.h"
#include "support/FakeExtensionHost.h"

using namespace yicad_test;

namespace
{
/// @brief 启动标注扩展的命令夹具
struct DimFixture : CommandFixture
{
    FakeExtensionHost extensionHost;

    DimFixture()
    {
        ExtensionManager::instance().Register(std::make_unique<DimExtension>());
        ExtensionManager::instance().BootAll(extensionHost);
    }
    ~DimFixture() override { ExtensionManager::instance().Shutdown(); }
};

struct FirstStep
{
    const char* id;
    const char* hint;
};

const FirstStep kCommands[] = {
    {"ext.dim.aligned", "Specify first extension line origin"},
    {"ext.dim.linear", "Specify first extension line origin"},
    {"ext.dim.radial", "Select arc or circle entity"},
    {"ext.dim.diametric", "Select arc or circle entity"},
    {"ext.dim.angular", "Select first line"},
    {"ext.dim.leader", "Specify target point"},
    {"ext.dim.baseline", "Specify origin dimension"},
};
}  // namespace

TEST_F(DimFixture, 注册的命令类型别名与选项条)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const FirstStep& step : kCommands)
    {
        EXPECT_EQ(registry.kind(step.id), CommandKind::Exclusive) << step.id;
    }
    EXPECT_EQ(registry.kind("ext.dim.style"), CommandKind::Instant);
    EXPECT_EQ(registry.commandForAlias("dl"), QStringLiteral("ext.dim.linear"));
    EXPECT_EQ(registry.commandForAlias("yx"), QStringLiteral("ext.dim.leader"));
    // 只有线性标注有选项条
    EXPECT_TRUE(static_cast<bool>(registry.commandOptionsFactory("ext.dim.linear")));
    EXPECT_FALSE(static_cast<bool>(registry.commandOptionsFactory("ext.dim.aligned")));
    EXPECT_EQ(extensionHost.ribbon.entriesOf(UIRibbonIds::kPanelDraw2dDimension).size(), 8u);
}

TEST_F(DimFixture, 启动后给出第一步提示右键结束)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        ASSERT_TRUE(start(step.id));
        EXPECT_EQ(ui.lastHint(), QString::fromLatin1(step.hint));
        EXPECT_EQ(ui.lastRightHint(), QStringLiteral("Cancel"));
        rightClick();
        EXPECT_FALSE(bus.hasActiveCommand());
    }
}

TEST_F(DimFixture, 线性标注逐步提示与右键退回)
{
    ASSERT_TRUE(start("ext.dim.linear"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second extension line origin"));
    typeCoordinate(10, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify dimension line location"));
    rightClick();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second extension line origin"));
    rightClick();
    rightClick();
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(DimFixture, 线性标注的选项条角度转给工具)
{
    ASSERT_TRUE(start("ext.dim.linear"));
    auto* linear = dynamic_cast<DimLinearCommand*>(bus.activeCommand());
    ASSERT_NE(linear, nullptr);
    ASSERT_NE(lastOptions("ext.dim.linear"), nullptr);
    EXPECT_TRUE(lastOptions("ext.dim.linear")->on);
    linear->setAngle(0.5);
    EXPECT_DOUBLE_EQ(linear->angle(), 0.5);
    endCommand();
    EXPECT_FALSE(lastOptions("ext.dim.linear")->on);
}

TEST_F(DimFixture, 引线一个点时右键退回再右键结束)
{
    ASSERT_TRUE(start("ext.dim.leader"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify next point"));
    EXPECT_EQ(ui.lastRightHint(), QStringLiteral("Finish"));
    // 只有一个点：右键退回第一步（清空已设置的点）
    rightClick();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify target point"));
    rightClick();
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(DimFixture, 没有文档时标注样式什么也不做)
{
    EXPECT_TRUE(CommandRegistry::instance().runInstant(QStringLiteral("ext.dim.style"), CommandContext{}));
}

TEST_F(DimFixture, 登记标注的属性编辑修改标注文字)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (DM::EntityType type : {DM::EntityDimAligned, DM::EntityDimAngular, DM::EntityDimDiametric,
                                DM::EntityDimRadial, DM::EntityDimLinear})
    {
        EXPECT_EQ(registry.propertyEditor(type), QStringLiteral("ext.dim.properties")) << type;
    }
    EXPECT_EQ(registry.kind(QStringLiteral("ext.dim.properties")), CommandKind::Instant);
    // 没有实体时什么也不做
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.dim.properties"), CommandContext{&doc, &view, &selection}));
    EXPECT_TRUE(dialogs.shown.empty());
}
