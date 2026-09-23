/// @file test_extension_manager.cpp
/// @brief ExtensionManager 的单测
///
/// 覆盖阶段4第二阶段（doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4 §7.4任务③）
/// 的核心行为：注册顺序 == 启动顺序、重复 Id 被拒绝、BootAll 之后不能再
/// Register、Shutdown 按反序执行且幂等、Shutdown 后可以重新
/// Register/BootAll。
///
/// ExtensionManager 是进程范围的单例，同一个测试二进制内的所有用例共享
/// 同一份状态。每个用例结束前都显式 Shutdown()，让管理器回到初始状态，
/// 避免用例之间互相污染（这也是 Shutdown() 设计成"复位 m_booted、可以
/// 重新 Register/BootAll"的原因之一）。
///
/// 用一个不需要真正可用的假 IExtensionContext（ribbon() 返回一个测试期间
/// 构造的裸 SARibbonBar，因为假 IExtension 根本不调用它）——ExtensionManager
/// 自己的行为不依赖 Ribbon/文档访问器是否真的能用，这正是 IExtensionContext
/// 刻意保持窄接口带来的可测试性。

#include <gtest/gtest.h>

#include <vector>

#include "ExtensionManager.h"
#include "IExtension.h"
#include "IExtensionContext.h"

#include "SARibbonBar.h"

namespace
{
/// @brief 记录 OnRegister/OnShutdown 调用顺序的测试替身。
class RecordingExtension : public IExtension
{
public:
    RecordingExtension(std::string id, std::vector<std::string>& log)
        : m_id(std::move(id)), m_log(log)
    {
    }

    void OnRegister(IExtensionContext&) override { m_log.push_back(m_id + ":register"); }
    void OnShutdown() override { m_log.push_back(m_id + ":shutdown"); }
    std::string_view Id() const override { return m_id; }

private:
    std::string m_id;
    std::vector<std::string>& m_log;
};

/// @brief 最小的 IExtensionContext 测试替身；假 IExtension 不会真的调用
/// 这些方法，只需要满足接口即可构造。
class FakeExtensionContext : public IExtensionContext
{
public:
    SARibbonBar& ribbon() override { return m_ribbon; }
    QWidget* mainWindow() override { return nullptr; }
    DmDocument* currentDocument() const override { return nullptr; }
    GuiDocumentView* currentDocumentView() const override { return nullptr; }
    void registerSettingsPage(const QString&, const QString&, const QString&,
                               std::function<void()>) override
    {
    }

private:
    SARibbonBar m_ribbon;
};
}  // namespace

TEST(ExtensionManagerTest, 注册顺序等于启动顺序且Shutdown按反序执行)
{
    std::vector<std::string> log;
    FakeExtensionContext ctx;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("a", log)));
    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("b", log)));
    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("c", log)));

    manager.BootAll(ctx);
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
    FakeExtensionContext ctx;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("x", log)));
    manager.BootAll(ctx);
    ASSERT_EQ(log, (std::vector<std::string>{"x:register"}));

    // BootAll 之后不能再注册新扩展。
    EXPECT_FALSE(manager.Register(std::make_unique<RecordingExtension>("y", log)));

    // 重复 BootAll 不会重新触发已注册扩展的 OnRegister。
    manager.BootAll(ctx);
    EXPECT_EQ(log, (std::vector<std::string>{"x:register"}));

    manager.Shutdown();
}

TEST(ExtensionManagerTest, Shutdown幂等且复位后可以重新注册)
{
    std::vector<std::string> log;
    FakeExtensionContext ctx;
    auto& manager = ExtensionManager::instance();

    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("first", log)));
    manager.BootAll(ctx);
    manager.Shutdown();
    manager.Shutdown();  // 幂等：不应该再次调用 OnShutdown。
    EXPECT_EQ(log, (std::vector<std::string>{"first:register", "first:shutdown"}));

    // 复位之后可以重新走一遍完整生命周期。
    log.clear();
    ASSERT_TRUE(manager.Register(std::make_unique<RecordingExtension>("second", log)));
    manager.BootAll(ctx);
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
