#include "PluginUiAdapter.h"

#include "CommandRegistry.h"
#include "PluginRegistry.h"
#include "UIRibbonRegistry.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

#include <utility>

namespace
{

/// @brief 插件自建的类目、面板与按钮的 ID 前缀。插件命令 ID 形如 "pluginId/commandId"，
///        内置与扩展的 ID 都不含 '/'，两边不会重名。
const QString kPluginIdPrefix = QStringLiteral("plugin:");

/// @brief 按标题找已注册的类目，与接入注册表前的 SARibbonBar::categoryByName 一样按显示名
///        匹配：插件可以把按钮放进内置或扩展的类目。
const UIRibbonCategoryDef* findCategoryByTitle(const UIRibbonRegistry& ribbon, const QString& title)
{
    for (const UIRibbonCategoryDef& category : ribbon.categories())
    {
        if (category.title == title)
        {
            return &category;
        }
    }
    return nullptr;
}

/// @brief 在类目里按标题找面板，与原先的 SARibbonCategory::pannelByName 一致。
const UIRibbonPanelDef* findPanelByTitle(
    const UIRibbonRegistry& ribbon,
    const QString& categoryId,
    const QString& title)
{
    for (const UIRibbonPanelDef* panel : ribbon.panelsOf(categoryId))
    {
        if (panel->title == title)
        {
            return panel;
        }
    }
    return nullptr;
}

/// @brief 插件声明的图标路径：相对路径相对插件 DLL 所在目录；文件不存在时不设图标。
QString resolveIconPath(const QString& declared, const QString& dllDirectory)
{
    if (declared.isEmpty())
    {
        return {};
    }
    const QFileInfo declaredIcon(declared);
    const QFileInfo iconFile(
        declaredIcon.isAbsolute()
            ? declaredIcon.absoluteFilePath()
            : QDir(dllDirectory).absoluteFilePath(declared));
    return iconFile.exists() && iconFile.isFile() ? iconFile.absoluteFilePath() : QString();
}

} // namespace

PluginUiAdapter::PluginUiAdapter(PluginRegistry& registry)
    : m_registry(registry)
{
}

PluginUiAdapter::~PluginUiAdapter()
{
    for (const QString& id : m_registeredCommands)
    {
        CommandRegistry::instance().unregisterCommand(id);
    }
}

QString PluginUiAdapter::hostCommandId(const QString& pluginId, const QString& commandId)
{
    return pluginId + QLatin1Char('/') + commandId;
}

bool PluginUiAdapter::registerAll(UIRibbonRegistry& ribbon)
{
    if (m_registered)
    {
        qWarning("PluginUiAdapter: registerAll called more than once");
        return false;
    }
    m_registered = true;

    // 按钮引用命令，命令先注册；finalize 会剔除命令未注册的按钮
    const bool commandsRegistered = registerCommands();
    const bool buttonsRegistered = registerRibbonButtons(ribbon);
    return commandsRegistered && buttonsRegistered;
}

bool PluginUiAdapter::registerCommands()
{
    CommandRegistry& commands = CommandRegistry::instance();
    bool success = true;
    for (const PluginCommandRecord& command : m_registry.commands())
    {
        const QString id = hostCommandId(command.pluginId, command.commandId);
        // 回调每次经 PluginRegistry 按二元键查找，查不到时什么也不做
        InstantCommand run =
            [registry = &m_registry, pluginId = command.pluginId, commandId = command.commandId](const CommandContext&)
        {
            registry->executeCommand(pluginId, commandId);
        };

        CommandInfo info;
        info.description = command.displayName;
        // "pluginId/commandId" 同时是命令行别名：命令行输入这个形式时启动命令（原先由
        // UIActionHandler 的外部命令执行器最后解析），也进命令行的自动补全
        info.aliases = QStringList{id};
        if (!commands.registerInstantCommand(id, run, info))
        {
            // 别名不区分大小写，两个插件的命令 ID 只差大小写时别名冲突：命令照常注册，
            // 只是不能从命令行输入
            info.aliases.clear();
            if (!commands.registerInstantCommand(id, std::move(run), std::move(info)))
            {
                qWarning("PluginUiAdapter: command '%s' could not be registered; skipped", qUtf8Printable(id));
                success = false;
                continue;
            }
            qWarning("PluginUiAdapter: alias of command '%s' conflicts with another command; "
                     "it cannot be entered on the command line",
                     qUtf8Printable(id));
        }
        m_registeredCommands.append(id);
    }
    return success;
}

bool PluginUiAdapter::registerRibbonButtons(UIRibbonRegistry& ribbon)
{
    bool success = true;
    // 插件可以给同一命令声明多个按钮，按钮 ID 从第二个起加序号
    QSet<QString> actionIds;
    for (const PluginRibbonButtonRecord& record : m_registry.ribbonButtons())
    {
        const PluginRecord* plugin = m_registry.findPlugin(record.pluginId);
        const PluginCommandRecord* command = m_registry.findCommand(record.pluginId, record.commandId);
        const QString commandId = hostCommandId(record.pluginId, record.commandId);
        if (plugin == nullptr || command == nullptr || !m_registeredCommands.contains(commandId))
        {
            qWarning("PluginUiAdapter: Ribbon button of command '%s' has no registered command; skipped",
                     qUtf8Printable(commandId));
            success = false;
            continue;
        }

        QString categoryId;
        if (const UIRibbonCategoryDef* category = findCategoryByTitle(ribbon, record.tab))
        {
            categoryId = category->id;
        }
        else
        {
            categoryId = kPluginIdPrefix + record.tab;
            if (!ribbon.addCategory({.id = categoryId, .title = record.tab}))
            {
                success = false;
                continue;
            }
        }

        QString panelId;
        if (const UIRibbonPanelDef* panel = findPanelByTitle(ribbon, categoryId, record.group))
        {
            // 挂进已有面板的按钮跟随该面板的样式
            panelId = panel->id;
        }
        else
        {
            // 插件自建的面板与接入注册表前一样，按钮逐个以大按钮放入
            panelId = kPluginIdPrefix + record.tab + QLatin1Char('/') + record.group;
            if (!ribbon.addPanel({.id = panelId, .categoryId = categoryId, .title = record.group, .largeButtons = true}))
            {
                success = false;
                continue;
            }
        }

        const QString baseId = kPluginIdPrefix + commandId;
        QString actionId = baseId;
        for (int n = 2; actionIds.contains(actionId); ++n)
        {
            actionId = QStringLiteral("%1#%2").arg(baseId).arg(n);
        }
        actionIds.insert(actionId);

        UIRibbonActionDef def;
        def.id = actionId;
        def.panelId = panelId;
        def.text = command->displayName;
        def.iconPath = resolveIconPath(record.iconPath, plugin->dllDirectory);
        def.commandId = commandId;
        def.objectName = baseId;
        if (!ribbon.addAction(std::move(def)))
        {
            success = false;
        }
    }
    return success;
}
