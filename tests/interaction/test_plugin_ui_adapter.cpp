/// @file test_plugin_ui_adapter.cpp
/// @brief PluginUiAdapter 的单测：插件命令接入 CommandRegistry、插件按钮接入 UIRibbonRegistry
///
/// 覆盖阶段 4 收尾（doc/ARCHITECTURE_EVOLUTION_PLAN.md 7.11 节）：插件命令以
/// "pluginId/commandId" 注册成即时命令并兼作命令行别名、执行时回调插件、析构时注销；
/// 插件按钮按标题挂进已有的类目与面板，标题不匹配时新建类目与大按钮面板；同一命令的多个
/// 按钮、图标路径解析与别名大小写冲突。
///
/// PluginRegistry 直接按插件运行时的注册事务填充，不加载 DLL。命令注册在进程范围的
/// CommandRegistry 里，各用例的插件 ID 互不相同。

#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "CommandRegistry.h"
#include "PluginRegistry.h"
#include "PluginUiAdapter.h"
#include "SARibbonBar.h"
#include "SARibbonButtonGroupWidget.h"
#include "UIRibbonManager.h"
#include "UIRibbonRegistry.h"

namespace
{
/// @brief 插件命令回调：userData 是调用计数
void YICAD_PLUGIN_CALL countCall(void* userData)
{
    ++*static_cast<int*>(userData);
}

/// @brief 插件声明的一个按钮
struct Button
{
    QString tab;
    QString group;
    QString commandId;
    QString iconPath;
};

/// @brief 按插件运行时的做法提交一个插件：命令都用 countCall，userData 为 counter
void commitPlugin(
    PluginRegistry& registry,
    const QString& pluginId,
    const std::vector<std::pair<QString, QString>>& commands,
    const std::vector<Button>& buttons,
    int* counter,
    const QString& dllDirectory = {})
{
    ASSERT_TRUE(registry.beginRegistration());
    for (const auto& [commandId, displayName] : commands)
    {
        ASSERT_TRUE(registry.stageCommand(pluginId, commandId, displayName, &countCall, counter));
    }
    for (const Button& button : buttons)
    {
        ASSERT_TRUE(registry.stageRibbonButton(pluginId, button.tab, button.group, button.commandId, button.iconPath));
    }
    PluginRecord record;
    record.pluginId = pluginId;
    record.pluginName = pluginId;
    record.pluginVersion = QStringLiteral("1.0.0");
    record.dllDirectory = dllDirectory;
    ASSERT_TRUE(registry.commitRegistration(record));
}

/// @brief 面板里的全部按钮
std::vector<UIRibbonActionDef> actionsOf(const UIRibbonRegistry& ribbon, const QString& panelId)
{
    std::vector<UIRibbonActionDef> out;
    for (const UIRibbonEntry* entry : ribbon.entriesOf(panelId))
    {
        if (const auto* action = std::get_if<UIRibbonActionDef>(entry))
        {
            out.push_back(*action);
        }
    }
    return out;
}
}  // namespace

TEST(PluginUiAdapterTest, 插件命令注册为即时命令并回调插件)
{
    int calls = 0;
    PluginRegistry plugins;
    commitPlugin(plugins, "com.test.pua.run", {{"tool.run", "Run Tool"}}, {}, &calls);

    UIRibbonRegistry ribbon;
    PluginUiAdapter adapter(plugins);
    EXPECT_TRUE(adapter.registerAll(ribbon));

    const QString id = PluginUiAdapter::hostCommandId("com.test.pua.run", "tool.run");
    EXPECT_EQ(id, QStringLiteral("com.test.pua.run/tool.run"));
    CommandRegistry& commands = CommandRegistry::instance();
    EXPECT_EQ(commands.kind(id), CommandKind::Instant);
    EXPECT_EQ(commands.description(id), QStringLiteral("Run Tool"));
    // 执行前与其它即时命令一样先结束不可打断的命令（原先插件回调直接执行，什么也不结束）
    EXPECT_EQ(commands.instantInterrupt(id), InstantInterrupt::EndUninterruptible);

    // "pluginId/commandId" 兼作命令行别名；别名不区分大小写（原先的外部命令执行器区分）
    EXPECT_EQ(commands.commandForAlias("com.test.pua.run/tool.run"), id);
    EXPECT_EQ(commands.commandForAlias("COM.TEST.PUA.RUN/TOOL.RUN"), id);
    EXPECT_TRUE(commands.aliases().contains("com.test.pua.run/tool.run"));

    EXPECT_TRUE(commands.runInstant(id, CommandContext{}));
    EXPECT_EQ(calls, 1);

    // 只能登记一次
    EXPECT_FALSE(adapter.registerAll(ribbon));
}

TEST(PluginUiAdapterTest, 析构时注销命令与别名)
{
    int calls = 0;
    PluginRegistry plugins;
    commitPlugin(plugins, "com.test.pua.dtor", {{"x", "X"}}, {}, &calls);
    const QString id = PluginUiAdapter::hostCommandId("com.test.pua.dtor", "x");
    {
        UIRibbonRegistry ribbon;
        PluginUiAdapter adapter(plugins);
        ASSERT_TRUE(adapter.registerAll(ribbon));
        ASSERT_TRUE(CommandRegistry::instance().hasCommand(id));
    }
    EXPECT_FALSE(CommandRegistry::instance().hasCommand(id));
    EXPECT_TRUE(CommandRegistry::instance().commandForAlias(id).isEmpty());
}

TEST(PluginUiAdapterTest, 按标题挂进已有面板并跟随面板样式)
{
    int calls = 0;
    PluginRegistry plugins;
    commitPlugin(plugins, "com.test.pua.existing", {{"x", "Plugin X"}}, {{"Draw", "Line", "x", ""}}, &calls);

    UIRibbonRegistry ribbon;
    ASSERT_TRUE(ribbon.addCategory({.id = "test.pua.cat", .title = "Draw"}));
    ASSERT_TRUE(ribbon.addPanel({.id = "test.pua.line", .categoryId = "test.pua.cat", .title = "Line"}));

    PluginUiAdapter adapter(plugins);
    ASSERT_TRUE(adapter.registerAll(ribbon));

    // 没有新建类目
    EXPECT_EQ(ribbon.categories().size(), 1u);
    ASSERT_EQ(ribbon.panelsOf("test.pua.cat").size(), 1u);
    EXPECT_FALSE(ribbon.panelsOf("test.pua.cat").front()->largeButtons);

    const auto actions = actionsOf(ribbon, "test.pua.line");
    ASSERT_EQ(actions.size(), 1u);
    EXPECT_EQ(actions[0].id, QStringLiteral("plugin:com.test.pua.existing/x"));
    EXPECT_EQ(actions[0].objectName, QStringLiteral("plugin:com.test.pua.existing/x"));
    EXPECT_EQ(actions[0].text, QStringLiteral("Plugin X"));
    EXPECT_EQ(actions[0].commandId, QStringLiteral("com.test.pua.existing/x"));
    EXPECT_TRUE(actions[0].iconPath.isEmpty());

    // 命令已注册，finalize 保留按钮
    ribbon.finalize();
    EXPECT_EQ(actionsOf(ribbon, "test.pua.line").size(), 1u);
}

TEST(PluginUiAdapterTest, 标题不匹配时新建类目与大按钮面板且被后来的插件复用)
{
    int calls = 0;
    PluginRegistry plugins;
    commitPlugin(plugins, "com.test.pua.new1", {{"a", "A"}}, {{"Tools", "Misc", "a", ""}}, &calls);
    commitPlugin(plugins, "com.test.pua.new2", {{"b", "B"}}, {{"Tools", "Misc", "b", ""}, {"Tools", "Other", "b", ""}},
                 &calls);

    UIRibbonRegistry ribbon;
    ASSERT_TRUE(ribbon.addCategory({.id = "test.pua.builtin", .title = "Draw"}));

    PluginUiAdapter adapter(plugins);
    ASSERT_TRUE(adapter.registerAll(ribbon));

    // 新类目排在已注册的类目之后，两个插件共用
    ASSERT_EQ(ribbon.categories().size(), 2u);
    EXPECT_EQ(ribbon.categories()[1].id, QStringLiteral("plugin:Tools"));
    EXPECT_EQ(ribbon.categories()[1].title, QStringLiteral("Tools"));

    const auto panels = ribbon.panelsOf("plugin:Tools");
    ASSERT_EQ(panels.size(), 2u);
    EXPECT_EQ(panels[0]->id, QStringLiteral("plugin:Tools/Misc"));
    EXPECT_EQ(panels[0]->title, QStringLiteral("Misc"));
    EXPECT_TRUE(panels[0]->largeButtons);
    EXPECT_EQ(panels[1]->id, QStringLiteral("plugin:Tools/Other"));

    const auto misc = actionsOf(ribbon, "plugin:Tools/Misc");
    ASSERT_EQ(misc.size(), 2u);
    EXPECT_EQ(misc[0].commandId, QStringLiteral("com.test.pua.new1/a"));
    EXPECT_EQ(misc[1].commandId, QStringLiteral("com.test.pua.new2/b"));

    // 同一命令的第二个按钮 ID 加序号，objectName 不变
    const auto other = actionsOf(ribbon, "plugin:Tools/Other");
    ASSERT_EQ(other.size(), 1u);
    EXPECT_EQ(other[0].id, QStringLiteral("plugin:com.test.pua.new2/b#2"));
    EXPECT_EQ(other[0].objectName, QStringLiteral("plugin:com.test.pua.new2/b"));
}

TEST(PluginUiAdapterTest, 图标相对插件目录解析且文件不存在时不设图标)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile icon(dir.filePath("icon.svg"));
    ASSERT_TRUE(icon.open(QIODevice::WriteOnly));
    icon.write("<svg xmlns=\"http://www.w3.org/2000/svg\"/>");
    icon.close();

    int calls = 0;
    PluginRegistry plugins;
    commitPlugin(plugins, "com.test.pua.icon", {{"a", "A"}, {"b", "B"}, {"c", "C"}},
                 {{"Icons", "P", "a", "icon.svg"}, {"Icons", "P", "b", "missing.svg"}, {"Icons", "P", "c", dir.filePath("icon.svg")}},
                 &calls, dir.path());

    UIRibbonRegistry ribbon;
    PluginUiAdapter adapter(plugins);
    ASSERT_TRUE(adapter.registerAll(ribbon));

    const auto actions = actionsOf(ribbon, "plugin:Icons/P");
    ASSERT_EQ(actions.size(), 3u);
    const QString expected = QFileInfo(dir.filePath("icon.svg")).absoluteFilePath();
    EXPECT_EQ(actions[0].iconPath, expected);
    EXPECT_TRUE(actions[1].iconPath.isEmpty());
    EXPECT_EQ(actions[2].iconPath, expected);
}

TEST(PluginUiAdapterTest, 别名只差大小写时命令照常注册但不能从命令行输入)
{
    int calls = 0;
    PluginRegistry plugins;
    commitPlugin(plugins, "com.test.pua.Case", {{"x", "Upper"}}, {}, &calls);
    commitPlugin(plugins, "com.test.pua.case", {{"x", "Lower"}}, {}, &calls);

    UIRibbonRegistry ribbon;
    PluginUiAdapter adapter(plugins);
    EXPECT_TRUE(adapter.registerAll(ribbon));

    CommandRegistry& commands = CommandRegistry::instance();
    EXPECT_TRUE(commands.hasCommand("com.test.pua.Case/x"));
    EXPECT_TRUE(commands.hasCommand("com.test.pua.case/x"));
    // 别名归先注册的那个
    EXPECT_EQ(commands.commandForAlias("com.test.pua.case/x"), QStringLiteral("com.test.pua.Case/x"));

    EXPECT_TRUE(commands.runInstant("com.test.pua.case/x", CommandContext{}));
    EXPECT_EQ(calls, 1);
}

TEST(PluginUiAdapterTest, 装配后按钮以大按钮放进面板并按命令ID启动)
{
    int calls = 0;
    PluginRegistry plugins;
    commitPlugin(plugins, "com.test.pua.install", {{"x", "X"}}, {{"Assembled", "P", "x", ""}}, &calls);

    UIRibbonRegistry ribbon;
    PluginUiAdapter adapter(plugins);
    ASSERT_TRUE(adapter.registerAll(ribbon));
    ribbon.finalize();

    std::vector<QString> activated;
    SARibbonBar bar;
    UIRibbonManager manager(
        bar, ribbon, [&activated](const QString& commandId, QObject*) { activated.push_back(commandId); },
        [] { return UIRibbonContext{}; });
    manager.install();

    ASSERT_NE(manager.category("plugin:Assembled"), nullptr);
    QAction* action = manager.action("plugin:com.test.pua.install/x");
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->objectName(), QStringLiteral("plugin:com.test.pua.install/x"));
    // 大按钮直接放进面板，不在按钮组里
    // Qt 6 的 QAction 只有 associatedObjects()，从中取控件，等价于 Qt 5 的 associatedWidgets()
    QList<QWidget*> widgets;
    for (QObject* object : action->associatedObjects())
    {
        if (QWidget* widget = qobject_cast<QWidget*>(object))
        {
            widgets.append(widget);
        }
    }
    ASSERT_FALSE(widgets.isEmpty());
    for (QWidget* widget : widgets)
    {
        EXPECT_EQ(qobject_cast<SARibbonButtonGroupWidget*>(widget->parentWidget()), nullptr);
    }

    action->trigger();
    EXPECT_EQ(activated, (std::vector<QString>{"com.test.pua.install/x"}));
}
