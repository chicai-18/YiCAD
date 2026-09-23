/// @file test_ribbon_registry.cpp
/// @brief UIRibbonRegistry / UIRibbonScopedRegistrar / UIRibbonManager 的单测
///
/// 覆盖阶段4 Ribbon 注册表（doc/ARCHITECTURE_EVOLUTION_PLAN.md 7.10 节）：
/// 注册校验（重复 ID、commandId 与 trigger 二选一、finalize 后拒绝）、
/// finalize 剔除孤儿条目与未注册命令、扩展入口的命名空间限制，以及装配器
/// 把注册数据变成 SARibbonBar 上的类目/按钮、点击按 ID 启动命令、按上下文
/// 重算可用状态。
///
/// 注册表不是单例，每个用例各建一个；用到的命令 ID 注册在进程范围的
/// CommandRegistry 里，各用例互不相同。

#include <gtest/gtest.h>

#include <vector>

#include <QAction>

#include "CommandRegistry.h"
#include "DmDocument.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "UIRibbonManager.h"
#include "UIRibbonRegistry.h"

namespace
{
void registerNullCommand(const char* id)
{
    if (!CommandRegistry::instance().hasCommand(id))
    {
        CommandRegistry::instance().registerCommand(
            id, [](const CommandContext&) -> ActionInterface* { return nullptr; });
    }
}
}  // namespace

TEST(RibbonRegistryTest, 重复ID被拒绝)
{
    UIRibbonRegistry r;
    EXPECT_TRUE(r.addCategory({.id = "cat"}));
    EXPECT_FALSE(r.addCategory({.id = "cat"}));
    EXPECT_TRUE(r.addPanel({.id = "cat.p", .categoryId = "cat"}));
    EXPECT_FALSE(r.addPanel({.id = "cat.p", .categoryId = "cat"}));
    EXPECT_TRUE(r.addAction({.id = "cat.p.a", .panelId = "cat.p", .trigger = [] {}}));
    // 按钮与控件共用一个条目 ID 空间
    EXPECT_FALSE(r.addWidget({.id = "cat.p.a", .panelId = "cat.p", .factory = [](QWidget*) { return nullptr; }}));
    EXPECT_FALSE(r.addCategory({}));
}

TEST(RibbonRegistryTest, 按钮的commandId与trigger必须恰好给一个)
{
    UIRibbonRegistry r;
    EXPECT_FALSE(r.addAction({.id = "a1", .panelId = "p"}));
    EXPECT_FALSE(r.addAction({.id = "a2", .panelId = "p", .commandId = "test.rr.x", .trigger = [] {}}));
    EXPECT_TRUE(r.addAction({.id = "a3", .panelId = "p", .commandId = "test.rr.x"}));
    // ID 为空时取 commandId
    EXPECT_TRUE(r.addAction({.panelId = "p", .commandId = "test.rr.y"}));
    EXPECT_FALSE(r.addAction({.panelId = "p", .commandId = "test.rr.y"}));
}

TEST(RibbonRegistryTest, finalize剔除孤儿与未注册命令且之后拒绝注册)
{
    registerNullCommand("test.rr.known");

    UIRibbonRegistry r;
    ASSERT_TRUE(r.addCategory({.id = "cat"}));
    ASSERT_TRUE(r.addPanel({.id = "p", .categoryId = "cat"}));
    ASSERT_TRUE(r.addPanel({.id = "orphan.p", .categoryId = "missing"}));
    ASSERT_TRUE(r.addAction({.id = "known", .panelId = "p", .commandId = "test.rr.known"}));
    ASSERT_TRUE(r.addAction({.id = "unknown", .panelId = "p", .commandId = "test.rr.unknown"}));
    ASSERT_TRUE(r.addAction({.id = "orphan", .panelId = "missing.p", .trigger = [] {}}));
    ASSERT_TRUE(r.addAction({.id = "in.orphan.panel", .panelId = "orphan.p", .trigger = [] {}}));
    // 右侧常驻按钮组不需要注册面板
    ASSERT_TRUE(r.addAction({.id = "right", .panelId = UIRibbonIds::kPanelRightButtons, .trigger = [] {}}));

    r.finalize();
    EXPECT_TRUE(r.isFinalized());
    EXPECT_EQ(r.panelsOf("missing").size(), 0u);
    ASSERT_EQ(r.panelsOf("cat").size(), 1u);
    ASSERT_EQ(r.entriesOf("p").size(), 1u);
    EXPECT_EQ(std::get<UIRibbonActionDef>(*r.entriesOf("p").front()).id, QStringLiteral("known"));
    EXPECT_EQ(r.entriesOf("missing.p").size(), 0u);
    EXPECT_EQ(r.entriesOf("orphan.p").size(), 0u);
    EXPECT_EQ(r.entriesOf(UIRibbonIds::kPanelRightButtons).size(), 1u);

    EXPECT_FALSE(r.addAction({.id = "late", .panelId = "p", .trigger = [] {}}));
    EXPECT_FALSE(r.addCategory({.id = "late.cat"}));
}

TEST(RibbonRegistryTest, 条目按注册顺序排列)
{
    UIRibbonRegistry r;
    ASSERT_TRUE(r.addCategory({.id = "cat"}));
    ASSERT_TRUE(r.addPanel({.id = "p1", .categoryId = "cat"}));
    ASSERT_TRUE(r.addPanel({.id = "p2", .categoryId = "cat"}));
    ASSERT_TRUE(r.addAction({.id = "x", .panelId = "p2", .trigger = [] {}}));
    ASSERT_TRUE(r.addWidget({.id = "w", .panelId = "p1", .factory = [](QWidget*) { return nullptr; }}));
    ASSERT_TRUE(r.addAction({.id = "y", .panelId = "p1", .trigger = [] {}}));
    r.finalize();

    const auto panels = r.panelsOf("cat");
    ASSERT_EQ(panels.size(), 2u);
    EXPECT_EQ(panels[0]->id, QStringLiteral("p1"));
    EXPECT_EQ(panels[1]->id, QStringLiteral("p2"));
    const auto entries = r.entriesOf("p1");
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_TRUE(std::holds_alternative<UIRibbonWidgetDef>(*entries[0]));
    EXPECT_TRUE(std::holds_alternative<UIRibbonActionDef>(*entries[1]));
}

TEST(RibbonRegistryTest, 扩展入口只接受自己命名空间内的ID)
{
    UIRibbonRegistry r;
    UIRibbonScopedRegistrar ext(r, "ext.demo");

    EXPECT_TRUE(ext.addCategory({.id = "ext.demo.cat"}));
    EXPECT_FALSE(ext.addCategory({.id = "category.demo"}));
    EXPECT_TRUE(ext.addPanel({.id = "ext.demo.p", .categoryId = "category.draw2d"}));
    EXPECT_FALSE(ext.addPanel({.id = "ext.demox.p", .categoryId = "category.draw2d"}));
    EXPECT_TRUE(ext.addAction({.panelId = "draw2d.dimension", .commandId = "ext.demo.cmd"}));
    EXPECT_FALSE(ext.addAction({.panelId = "draw2d.dimension", .commandId = "draw.line"}));
    EXPECT_TRUE(ext.addWidget({.id = "ext.demo.w", .panelId = "ext.demo.p", .factory = [](QWidget*) { return nullptr; }}));
    EXPECT_FALSE(ext.addWidget({.id = "w", .panelId = "ext.demo.p", .factory = [](QWidget*) { return nullptr; }}));
}

TEST(RibbonManagerTest, 装配类目与按钮并按命令ID启动)
{
    registerNullCommand("test.rm.cmd");

    UIRibbonRegistry r;
    ASSERT_TRUE(r.addCategory({.id = "test.cat", .title = "Test", .objectName = "testCategory"}));
    ASSERT_TRUE(r.addPanel({.id = "test.p", .categoryId = "test.cat", .title = "P"}));
    ASSERT_TRUE(r.addPanel({.id = "test.empty", .categoryId = "test.cat", .title = "Empty"}));
    ASSERT_TRUE(r.addCategory({.id = "test.empty_cat", .title = "EmptyCat"}));
    ASSERT_TRUE(r.addAction({.id = "test.btn", .panelId = "test.p", .text = "Btn", .commandId = "test.rm.cmd"}));
    int triggered = 0;
    ASSERT_TRUE(r.addAction({.id = "test.fn", .panelId = "test.p", .text = "Fn", .trigger = [&triggered] { ++triggered; }}));
    r.finalize();

    std::vector<QString> activated;
    std::vector<QObject*> sources;
    SARibbonBar bar;
    UIRibbonManager manager(
        bar, r,
        [&](const QString& commandId, QObject* source)
        {
            activated.push_back(commandId);
            sources.push_back(source);
        },
        [] { return UIRibbonContext{}; });
    manager.install();

    SARibbonCategory* category = manager.category("test.cat");
    ASSERT_NE(category, nullptr);
    EXPECT_EQ(category->objectName(), QStringLiteral("testCategory"));
    // 没有条目的类目不装配
    EXPECT_EQ(manager.category("test.empty_cat"), nullptr);

    QAction* btn = manager.action("test.btn");
    ASSERT_NE(btn, nullptr);
    btn->trigger();
    EXPECT_EQ(activated, (std::vector<QString>{"test.rm.cmd"}));
    EXPECT_EQ(sources, (std::vector<QObject*>{btn}));

    QAction* fn = manager.action("test.fn");
    ASSERT_NE(fn, nullptr);
    fn->trigger();
    EXPECT_EQ(triggered, 1);
    EXPECT_EQ(activated.size(), 1u);
}

TEST(RibbonManagerTest, 按上下文重算可用状态)
{
    UIRibbonRegistry r;
    const UIRibbonEnableFn documentOpen = UIRibbonCondition::requireAll(UIRibbonRequires::DocumentOpen);
    ASSERT_TRUE(r.addCategory({.id = "test.ctx.cat", .title = "Ctx"}));
    ASSERT_TRUE(r.addPanel({.id = "test.ctx.p", .categoryId = "test.ctx.cat", .title = "P"}));
    ASSERT_TRUE(r.addAction({.id = "test.ctx.always", .panelId = "test.ctx.p", .text = "A", .trigger = [] {}}));
    ASSERT_TRUE(r.addAction(
        {.id = "test.ctx.doc", .panelId = "test.ctx.p", .text = "D", .trigger = [] {}, .enableFn = documentOpen}));
    r.finalize();

    DmDocument doc;
    DmDocument* current = nullptr;
    SARibbonBar bar;
    UIRibbonManager manager(bar, r, {}, [&current] { return UIRibbonContext{current}; });
    manager.install();

    EXPECT_TRUE(manager.action("test.ctx.always")->isEnabled());
    EXPECT_FALSE(manager.action("test.ctx.doc")->isEnabled());

    current = &doc;
    manager.evaluateActivation();
    EXPECT_TRUE(manager.action("test.ctx.doc")->isEnabled());

    current = nullptr;
    manager.evaluateActivation();
    EXPECT_FALSE(manager.action("test.ctx.doc")->isEnabled());
    EXPECT_TRUE(manager.action("test.ctx.always")->isEnabled());
}
