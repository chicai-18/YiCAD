#include "PluginRegistry.h"

#include <utility>

namespace
{

template<typename Record>
bool containsFormat(
    const QVector<Record>& records,
    const QString& pluginId,
    const QString& formatId) noexcept
{
    for (const auto& record : records)
    {
        if (record.pluginId == pluginId && record.formatId == formatId)
        {
            return true;
        }
    }
    return false;
}

bool containsCommand(
    const QVector<PluginCommandRecord>& records,
    const QString& pluginId,
    const QString& commandId) noexcept
{
    for (const auto& record : records)
    {
        if (record.pluginId == pluginId && record.commandId == commandId)
        {
            return true;
        }
    }
    return false;
}

bool containsImportExtension(
    const QVector<PluginImportFilterRecord>& records,
    const QString& extension) noexcept
{
    for (const auto& record : records)
    {
        if (record.extension == extension)
        {
            return true;
        }
    }
    return false;
}

} // namespace

bool PluginRegistryError::isError() const noexcept
{
    return code != PluginRegistryErrorCode::None;
}

bool PluginRegistry::beginRegistration()
{
    if (m_registrationActive)
    {
        return fail(
            PluginRegistryErrorCode::TransactionAlreadyActive,
            QStringLiteral("已有插件注册事务正在进行"));
    }

    clearTransaction();
    m_registrationActive = true;
    m_lastError = {};
    return true;
}

bool PluginRegistry::stageCommand(
    const QString& pluginId,
    const QString& commandId,
    const QString& displayName,
    YiCadCommandCallback callback,
    void* userData)
{
    if (!adoptTransactionPlugin(pluginId))
    {
        return false;
    }
    if (commandId.isEmpty() || displayName.isEmpty() || callback == nullptr)
    {
        return fail(
            PluginRegistryErrorCode::InvalidCommand,
            QStringLiteral("命令 ID、显示名称和回调不能为空"));
    }
    if (containsCommand(m_commands, pluginId, commandId) ||
        containsCommand(m_stagedCommands, pluginId, commandId))
    {
        return fail(
            PluginRegistryErrorCode::DuplicateCommand,
            QStringLiteral("插件命令二元键重复：%1/%2")
                .arg(pluginId, commandId));
    }

    m_stagedCommands.append(
        {pluginId, commandId, displayName, callback, userData});
    return true;
}

bool PluginRegistry::stageRibbonButton(
    const QString& pluginId,
    const QString& tab,
    const QString& group,
    const QString& commandId,
    const QString& iconPath)
{
    if (!adoptTransactionPlugin(pluginId))
    {
        return false;
    }
    if (tab.isEmpty() || group.isEmpty() || commandId.isEmpty())
    {
        return fail(
            PluginRegistryErrorCode::InvalidRibbonButton,
            QStringLiteral("Ribbon 页签、分组和命令 ID 不能为空"));
    }

    m_stagedRibbonButtons.append(
        {pluginId, tab, group, commandId, iconPath});
    return true;
}

bool PluginRegistry::stageImportFilter(
    const QString& pluginId,
    const QString& formatId,
    const QString& displayName,
    const QString& extension,
    YiCadImportCallback callback,
    void* userData)
{
    if (!adoptTransactionPlugin(pluginId))
    {
        return false;
    }

    const auto normalizedExtension = normalizeExtension(extension);
    if (formatId.isEmpty() || displayName.isEmpty() ||
        normalizedExtension.isEmpty() || callback == nullptr)
    {
        return fail(
            PluginRegistryErrorCode::InvalidImportFilter,
            QStringLiteral("导入格式 ID、显示名称、扩展名和回调不能为空"));
    }
    if (containsFormat(m_importFilters, pluginId, formatId) ||
        containsFormat(m_stagedImportFilters, pluginId, formatId))
    {
        return fail(
            PluginRegistryErrorCode::DuplicateImportFormat,
            QStringLiteral("插件导入格式二元键重复：%1/%2")
                .arg(pluginId, formatId));
    }
    if (containsImportExtension(m_importFilters, normalizedExtension) ||
        containsImportExtension(m_stagedImportFilters, normalizedExtension))
    {
        return fail(
            PluginRegistryErrorCode::DuplicateImportExtension,
            QStringLiteral("导入扩展名重复：%1").arg(normalizedExtension));
    }

    m_stagedImportFilters.append(
        {pluginId,
         formatId,
         displayName,
         normalizedExtension,
         callback,
         userData});
    return true;
}

bool PluginRegistry::stageExportFilter(
    const QString& pluginId,
    const QString& formatId,
    const QString& displayName,
    const QString& extension,
    YiCadExportCallback callback,
    void* userData)
{
    if (!adoptTransactionPlugin(pluginId))
    {
        return false;
    }

    const auto normalizedExtension = normalizeExtension(extension);
    if (formatId.isEmpty() || displayName.isEmpty() ||
        normalizedExtension.isEmpty() || callback == nullptr)
    {
        return fail(
            PluginRegistryErrorCode::InvalidExportFilter,
            QStringLiteral("导出格式 ID、显示名称、扩展名和回调不能为空"));
    }
    if (containsFormat(m_exportFilters, pluginId, formatId) ||
        containsFormat(m_stagedExportFilters, pluginId, formatId))
    {
        return fail(
            PluginRegistryErrorCode::DuplicateExportFormat,
            QStringLiteral("插件导出格式二元键重复：%1/%2")
                .arg(pluginId, formatId));
    }

    m_stagedExportFilters.append(
        {pluginId,
         formatId,
         displayName,
         normalizedExtension,
         callback,
         userData});
    return true;
}

bool PluginRegistry::commitRegistration(const PluginRecord& plugin)
{
    if (!m_registrationActive)
    {
        return fail(
            PluginRegistryErrorCode::NoActiveTransaction,
            QStringLiteral("没有活动的插件注册事务"));
    }
    if (m_transactionFailed)
    {
        clearTransaction();
        return false;
    }
    if (plugin.pluginId.isEmpty())
    {
        fail(
            PluginRegistryErrorCode::InvalidPluginId,
            QStringLiteral("插件 ID 不能为空"));
        clearTransaction();
        return false;
    }
    if (!m_transactionPluginId.isEmpty() &&
        m_transactionPluginId != plugin.pluginId)
    {
        fail(
            PluginRegistryErrorCode::PluginIdMismatch,
            QStringLiteral("插件元数据 ID 与注册项 ID 不一致"));
        clearTransaction();
        return false;
    }
    if (findPlugin(plugin.pluginId) != nullptr)
    {
        fail(
            PluginRegistryErrorCode::DuplicatePluginId,
            QStringLiteral("插件 ID 重复：%1").arg(plugin.pluginId));
        clearTransaction();
        return false;
    }

    for (const auto& ribbon : std::as_const(m_stagedRibbonButtons))
    {
        if (!containsCommand(
                m_stagedCommands,
                ribbon.pluginId,
                ribbon.commandId))
        {
            fail(
                PluginRegistryErrorCode::MissingRibbonCommand,
                QStringLiteral("Ribbon 引用了未注册的同插件命令：%1/%2")
                    .arg(ribbon.pluginId, ribbon.commandId));
            clearTransaction();
            return false;
        }
    }

    // 先构造完整候选状态，再统一交换，保证异常不会产生部分提交。
    auto plugins = m_plugins;
    auto commands = m_commands;
    auto ribbonButtons = m_ribbonButtons;
    auto importFilters = m_importFilters;
    auto exportFilters = m_exportFilters;

    plugins.append(plugin);
    commands += m_stagedCommands;
    ribbonButtons += m_stagedRibbonButtons;
    importFilters += m_stagedImportFilters;
    exportFilters += m_stagedExportFilters;

    m_plugins.swap(plugins);
    m_commands.swap(commands);
    m_ribbonButtons.swap(ribbonButtons);
    m_importFilters.swap(importFilters);
    m_exportFilters.swap(exportFilters);

    clearTransaction();
    m_lastError = {};
    return true;
}

void PluginRegistry::rollbackRegistration() noexcept
{
    clearTransaction();
}

bool PluginRegistry::hasActiveRegistration() const noexcept
{
    return m_registrationActive;
}

const PluginRegistryError& PluginRegistry::lastError() const noexcept
{
    return m_lastError;
}

const QVector<PluginRecord>& PluginRegistry::plugins() const noexcept
{
    return m_plugins;
}

const QVector<PluginCommandRecord>& PluginRegistry::commands() const noexcept
{
    return m_commands;
}

const QVector<PluginRibbonButtonRecord>&
PluginRegistry::ribbonButtons() const noexcept
{
    return m_ribbonButtons;
}

const QVector<PluginImportFilterRecord>&
PluginRegistry::importFilters() const noexcept
{
    return m_importFilters;
}

const QVector<PluginExportFilterRecord>&
PluginRegistry::exportFilters() const noexcept
{
    return m_exportFilters;
}

const PluginRecord* PluginRegistry::findPlugin(
    const QString& pluginId) const noexcept
{
    for (const auto& plugin : m_plugins)
    {
        if (plugin.pluginId == pluginId)
        {
            return &plugin;
        }
    }
    return nullptr;
}

const PluginCommandRecord* PluginRegistry::findCommand(
    const QString& pluginId,
    const QString& commandId) const noexcept
{
    for (const auto& command : m_commands)
    {
        if (command.pluginId == pluginId && command.commandId == commandId)
        {
            return &command;
        }
    }
    return nullptr;
}

const PluginImportFilterRecord* PluginRegistry::findImportFilter(
    const QString& extension) const
{
    const auto normalizedExtension = normalizeExtension(extension);
    for (const auto& filter : m_importFilters)
    {
        if (filter.extension == normalizedExtension)
        {
            return &filter;
        }
    }
    return nullptr;
}

const PluginExportFilterRecord* PluginRegistry::findExportFilter(
    const QString& pluginId,
    const QString& formatId) const noexcept
{
    for (const auto& filter : m_exportFilters)
    {
        if (filter.pluginId == pluginId && filter.formatId == formatId)
        {
            return &filter;
        }
    }
    return nullptr;
}

bool PluginRegistry::executeCommand(
    const QString& pluginId,
    const QString& commandId) const noexcept
{
    const auto* command = findCommand(pluginId, commandId);
    if (command == nullptr || command->callback == nullptr)
    {
        return false;
    }

    try
    {
        command->callback(command->userData);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

QString PluginRegistry::normalizeExtension(QString extension)
{
    while (extension.startsWith(QLatin1Char('.')))
    {
        extension.remove(0, 1);
    }
    return extension.toLower();
}

bool PluginRegistry::adoptTransactionPlugin(const QString& pluginId)
{
    if (!m_registrationActive)
    {
        return fail(
            PluginRegistryErrorCode::NoActiveTransaction,
            QStringLiteral("没有活动的插件注册事务"));
    }
    if (m_transactionFailed)
    {
        return false;
    }
    if (pluginId.isEmpty())
    {
        return fail(
            PluginRegistryErrorCode::InvalidPluginId,
            QStringLiteral("插件 ID 不能为空"));
    }
    if (findPlugin(pluginId) != nullptr)
    {
        return fail(
            PluginRegistryErrorCode::DuplicatePluginId,
            QStringLiteral("插件 ID 重复：%1").arg(pluginId));
    }
    if (m_transactionPluginId.isEmpty())
    {
        m_transactionPluginId = pluginId;
        return true;
    }
    if (m_transactionPluginId != pluginId)
    {
        return fail(
            PluginRegistryErrorCode::PluginIdMismatch,
            QStringLiteral("一个注册事务只能包含同一插件的注册项"));
    }
    return true;
}

bool PluginRegistry::fail(
    PluginRegistryErrorCode code,
    const QString& message)
{
    m_lastError = {code, message};
    if (m_registrationActive)
    {
        m_transactionFailed = true;
    }
    return false;
}

void PluginRegistry::clearTransaction() noexcept
{
    m_registrationActive = false;
    m_transactionFailed = false;
    m_transactionPluginId.clear();
    m_stagedCommands.clear();
    m_stagedRibbonButtons.clear();
    m_stagedImportFilters.clear();
    m_stagedExportFilters.clear();
}
