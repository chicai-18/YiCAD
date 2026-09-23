/// @file test_extension_manager.cpp
/// @brief ExtensionManager 的单测
///
/// 覆盖阶段4（doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4 §7.4任务③）的核心
/// 行为：注册顺序 == 启动顺序、重复 Id 被拒绝、BootAll 之后不能再
/// Register、Shutdown 按反序执行且幂等、Shutdown 后可以重新
/// Register/BootAll；以及每个扩展专属上下文的命名空间校验、命令在
/// OnShutdown 之后注销、上下文存活到 OnShutdown。
///
/// ExtensionManager 与 CommandRegistry 都是进程范围的单例，同一个测试二进制
/// 内的所有用例共享同一份状态。每个用例结束前都显式 Shutdown()，让管理器
/// 回到初始状态（同时注销扩展注册的命令），避免用例之间互相污染；命令 ID
/// 各用例互不相同。

#include <gtest/gtest.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CommandRegistry.h"
#include "ExtensionManager.h"
#include "IExtension.h"
#include "IExtensionContext.h"
#include "IExtensionHost.h"
#include "UIRibbonRegistry.h"

namespace
{
/// @brief 记录 OnRegister/OnShutdown 调用顺序，并可附带自定义行为的测试替身。
class RecordingExtension : public IExtension
{
public:
    RecordingExtension(std::string id, std::vector<std::string>& log,
                       std::function<void(IExtensionContext&)> onRegister = {},
                       std::function<void()> onShutdown = {})
        : m_id(std::move(id)), m_log(log), m_onRegister(std::move(onRegister)),
          m_onShutdown(std::move(onShutdown))
    {
    }

    void OnRegister(IExtensionContext& ctx) override
    {
        m_log.push_back(m_id + ":register");
        if (m_onRegister)
        {
            m_onRegister(ctx);
        }
    }

    void OnShutdown() override
    {
        m_log.push_back(m_id + ":shutdown");
        if (m_onShutdown)
        {
            m_onShutdown();
        }
    }

    std::string_view Id() const override { return m_id; }

private:
    std::string m_id;
    std::vector<std::string>& m_log;
    std::function<void(IExtensionContext&)> m_onRegister;
    std::function<void()> m_onShutdown;
};

/// @brief 最小的 IExtensionHost 测试替身：Ribbon 用真实的注册表（按扩展
/// 限定命名空间的入口也是真实实现），其余服务只记录调用。
class FakeExtensionHost : public IExtensionHost
{
public:
    UIRibbonRegistrar& ribbonFor(std::string_view extensionId) override
    {
        auto& registrar = m_scoped[std::string(extensionId)];
        if (!registrar)
        {
            registrar = std::make_unique<UIRibbonScopedRegistrar>(ribbon, extensionId);
        }
        return *registrar;
    }

    QWidget* mainWindow() override { return nullptr; }
    DmDocument* currentDocument() const override { return nullptr; }
    GuiDocumentView* currentDocumentView() const override { return nullptr; }

    bool registerSettingsPage(const QString& id, const QString&, const QString&,
                              std::function<void()>) override
    {
        settingsPages.push_back(id);
        return true;
    }

    bool activateCommand(const QString& commandId) override
    {
        activated.push_back(commandId);
        return CommandRegistry::instance().hasCommand(commandId);
    }

    UIRibbonRegistry ribbon;
    std::vector<QString> settingsPages;
    std::vector<QString> activated;

private:
    std::map<std::string, std::unique_ptr<UIRibbonScopedRegistrar>> m_scoped;
};

CommandFactory nullFactory()
{
    return [](const CommandContext&) -> ActionInterface* { return nullptr; };
}
}  // namespace

TEST(ExtensionManagerTest, 注册顺序等于启动顺序且Shutdown按反序执行)
{
    std::vector<std::string> log;
    FakeExtensionHost host;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("a", log)));
    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("b", log)));
    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("c", log)));

    manager.BootAll(host);
    EXPECT_EQ(log, (std::vector<std::string>{"a:register", "b:register", "c:register"}));

    manager.Shutdown();
    EXPECT_EQ(log, (std::vector<std::string>{"a:register", "b:register", "c:register",
                                              "c:shutdown", "b:shutdown", "a:shutdown"}));
}

TEST(ExtensionManagerTest, 重复Id被拒绝)
{
    std::vector<std::string> log;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("dup", log)));
    EXPECT_FALSE(manager.Register(std::make_unique<RecordingExtension>("dup", log)));
    EXPECT_NE(manager.Find("dup"), nullptr);
    EXPECT_EQ(manager.Find("nonexistent"), nullptr);

    manager.Shutdown();
}

TEST(ExtensionManagerTest, BootAll之后不能再注册且重复BootAll是空操作)
{
    std::vector<std::string> log;
    FakeExtensionHost host;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("x", log)));
    manager.BootAll(host);
    ASSERT_EQ(log, (std::vector<std::string>{"x:register"}));

    // BootAll 之后不能再注册新扩展。
    EXPECT_FALSE(manager.Register(std::make_unique<RecordingExtension>("y", log)));

    // 重复 BootAll 不会重新触发已注册扩展的 OnRegister。
    manager.BootAll(host);
    EXPECT_EQ(log, (std::vector<std::string>{"x:register"}));

    manager.Shutdown();
}

TEST(ExtensionManagerTest, Shutdown幂等且复位后可以重新注册)
{
    std::vector<std::string> log;
    FakeExtensionHost host;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("first", log)));
    manager.BootAll(host);
    manager.Shutdown();
    manager.Shutdown();  // 幂等：不应该再次调用 OnShutdown。
    EXPECT_EQ(log, (std::vector<std::string>{"first:register", "first:shutdown"}));

    // 复位之后可以重新走一遍完整生命周期。
    log.clear();
    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("second", log)));
    manager.BootAll(host);
    manager.Shutdown();
    EXPECT_EQ(log, (std::vector<std::string>{"second:register", "second:shutdown"}));
}

TEST(ExtensionManagerTest, 从未BootAll就Shutdown不会调用OnShutdown)
{
    std::vector<std::string> log;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("never_booted", log)));
    manager.Shutdown();
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(manager.Find("never_booted"), nullptr);
}

TEST(ExtensionManagerTest, 扩展注册的命令与设置页必须在自己的命名空间内)
{
    std::vector<std::string> log;
    FakeExtensionHost host;
    auto& manager = ExtensionManager::instance();
    std::vector<bool> results;

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>(
        "ext.ns", log,
        [&results](IExtensionContext& ctx)
        {
            EXPECT_EQ(ctx.extensionId(), "ext.ns");
            results.push_back(ctx.registerCommand("ext.ns.ok", nullFactory(), {}));
            // 只有前缀、没有后续内容
            results.push_back(ctx.registerCommand("ext.ns", nullFactory(), {}));
            // 前缀后不是 '.'：不能被 "ext.ns" 认领
            results.push_back(ctx.registerCommand("ext.nsx.bad", nullFactory(), {}));
            results.push_back(ctx.registerCommand("draw.ns_bad", nullFactory(), {}));
            results.push_back(ctx.registerSettingsPage("ext.ns.page", "Page", QString(), [] {}));
            results.push_back(ctx.registerSettingsPage("options.ns_bad", "Page", QString(), [] {}));
        })));
    manager.BootAll(host);

    EXPECT_EQ(results, (std::vector<bool>{true, false, false, false, true, false}));
    EXPECT_TRUE(CommandRegistry::instance().hasCommand("ext.ns.ok"));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("ext.ns"));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("ext.nsx.bad"));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("draw.ns_bad"));
    EXPECT_EQ(host.settingsPages, (std::vector<QString>{"ext.ns.page"}));

    manager.Shutdown();
}

TEST(ExtensionManagerTest, 扩展的Ribbon条目按命名空间校验但可以挂进内置面板)
{
    std::vector<std::string> log;
    FakeExtensionHost host;
    auto& manager = ExtensionManager::instance();
    std::vector<bool> results;

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>(
        "ext.rb", log,
        [&results](IExtensionContext& ctx)
        {
            // 引用内置面板 ID 不受命名空间限制，自己的条目 ID 受限制。
            results.push_back(ctx.ribbon().addAction(
                {.id = "ext.rb.button", .panelId = "builtin.panel", .text = "B", .trigger = [] {}}));
            results.push_back(ctx.ribbon().addAction(
                {.id = "builtin.button", .panelId = "builtin.panel", .text = "B", .trigger = [] {}}));
            // 给内置命令加按钮时，按钮 ID 默认取命令 ID，不在命名空间内被拒绝；
            // 显式给一个自己的 ID 就可以。
            results.push_back(ctx.ribbon().addAction({.panelId = "builtin.panel", .commandId = "draw.line"}));
            results.push_back(ctx.ribbon().addAction(
                {.id = "ext.rb.line", .panelId = "builtin.panel", .commandId = "draw.line"}));
            results.push_back(ctx.ribbon().addPanel({.id = "ext.rb.panel", .categoryId = "builtin.category"}));
            results.push_back(ctx.ribbon().addCategory({.id = "category.rb_bad"}));
        })));
    manager.BootAll(host);

    EXPECT_EQ(results, (std::vector<bool>{true, false, false, true, true, false}));
    manager.Shutdown();
}

TEST(ExtensionManagerTest, Shutdown在OnShutdown之后注销扩展命令且可以重新注册)
{
    std::vector<std::string> log;
    FakeExtensionHost host;
    auto& manager = ExtensionManager::instance();
    IExtensionContext* keptContext = nullptr;
    bool activatedDuringShutdown = false;

    auto makeExtension = [&]()
    {
        return std::make_unique<RecordingExtension>(
            "ext.life", log,
            [&keptContext](IExtensionContext& ctx)
            {
                keptContext = &ctx;
                EXPECT_TRUE(ctx.registerCommand("ext.life.cmd", nullFactory(), {.aliases = {"lifecmd"}}));
            },
            [&keptContext, &activatedDuringShutdown]()
            {
                // 上下文保证有效到 OnShutdown 返回；此时自己的命令还没注销。
                activatedDuringShutdown = keptContext->activateCommand("ext.life.cmd");
            });
    };

    ASSERT_TRUE(manager.Register(makeExtension()));
    manager.BootAll(host);
    EXPECT_TRUE(CommandRegistry::instance().hasCommand("ext.life.cmd"));
    EXPECT_EQ(CommandRegistry::instance().commandForAlias("lifecmd"), "ext.life.cmd");

    manager.Shutdown();
    EXPECT_TRUE(activatedDuringShutdown);
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("ext.life.cmd"));
    EXPECT_TRUE(CommandRegistry::instance().commandForAlias("lifecmd").isEmpty());

    // 注销干净之后，同一个扩展可以重新注册同一条命令。
    ASSERT_TRUE(manager.Register(makeExtension()));
    manager.BootAll(host);
    EXPECT_TRUE(CommandRegistry::instance().hasCommand("ext.life.cmd"));
    manager.Shutdown();
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("ext.life.cmd"));
}
