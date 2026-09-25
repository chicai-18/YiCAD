/// @file test_hatch_extension.cpp
/// @brief 填充扩展（业务工具化第三步第⑧批）的单元测试
///
/// 覆盖：注册的命令类型与别名、原内置 ID 不再存在、按钮挂进宿主的"绘图/其他"
/// 面板（排在插入图片之后）、取消填充对话框时启动失败。区域查找与生成填充要真正的实体
/// 与事务，不在单测范围内（见 CommandTestFixture.h）。

#include <gtest/gtest.h>

#include <memory>

#include "ExtensionManager.h"
#include "HatchExtension.h"
#include "UIRibbonRegistry.h"
#include "support/CommandTestFixture.h"
#include "support/FakeExtensionHost.h"

using namespace yicad_test;

namespace
{
/// @brief 启动填充扩展的命令夹具
struct HatchFixture : CommandFixture
{
    FakeExtensionHost extensionHost;

    HatchFixture()
    {
        ExtensionManager::instance().Register(std::make_unique<HatchExtension>());
        ExtensionManager::instance().BootAll(extensionHost);
    }
    ~HatchFixture() override { ExtensionManager::instance().Shutdown(); }
};
}  // namespace

TEST_F(HatchFixture, 注册为交互命令并带别名)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    EXPECT_EQ(registry.kind("ext.hatch.draw"), CommandKind::Exclusive);
    EXPECT_EQ(registry.commandForAlias("tc"), QStringLiteral("ext.hatch.draw"));
    for (const char* id : {"draw.hatch", "draw.hatch_no_select"})
    {
        EXPECT_FALSE(registry.hasCommand(id)) << id;
    }
}

TEST_F(HatchFixture, 按钮挂进宿主的其他面板)
{
    EXPECT_EQ(extensionHost.ribbon.entriesOf(UIRibbonIds::kPanelDraw2dOther).size(), 1u);
}

TEST_F(HatchFixture, 取消填充对话框时启动失败)
{
    // 对话框工厂的默认实现返回"取消"
    EXPECT_FALSE(start("ext.hatch.draw"));
    EXPECT_EQ(dialogs.shown, std::vector<QString>{QStringLiteral("UIDlgHatch")});
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(HatchFixture, 登记填充的属性对话框)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    EXPECT_EQ(registry.propertyEditor(DM::EntityHatch), QStringLiteral("ext.hatch.properties"));
    EXPECT_EQ(registry.kind(QStringLiteral("ext.hatch.properties")), CommandKind::Instant);
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.hatch.properties"), CommandContext{&doc, &view}));
    EXPECT_TRUE(dialogs.shown.empty());
}
