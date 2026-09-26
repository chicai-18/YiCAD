#include "PluginFormatRegistration.h"

#include <memory>

#include "FilterRegistry.h"
#include "HostApi.h"
#include "PluginFileIOAdapter.h"
#include "PluginManager.h"
#include "PluginRegistry.h"

namespace
{

/// @brief 文件对话框的过滤串：插件的显示名后面补上 "(*.后缀)"，已经带了就不补
QString pluginNameFilter(
    const QString& displayName,
    const QString& extension)
{
    const auto suffix = QStringLiteral("(*.%1)").arg(extension);
    return displayName.endsWith(suffix)
        ? displayName
        : QStringLiteral("%1 %2").arg(displayName, suffix);
}

} // namespace

PluginFormatRegistration::PluginFormatRegistration(
    PluginRegistry& registry,
    PluginManager& manager,
    HostApi& hostApi)
{
    FilterRegistry& filters = FilterRegistry::instance();
    for (const PluginImportFilterRecord& record : registry.importFilters())
    {
        if (!manager.isPluginActive(record.pluginId))
        {
            continue;
        }
        m_ids.push_back(filters.addImport(
            pluginNameFilter(record.displayName, record.extension),
            [record, &manager, &hostApi]() -> std::unique_ptr<FilterInterface> {
                return std::make_unique<PluginFileIOAdapter>(
                    record, manager, hostApi);
            }));
    }
    for (const PluginExportFilterRecord& record : registry.exportFilters())
    {
        if (!manager.isPluginActive(record.pluginId))
        {
            continue;
        }
        m_ids.push_back(filters.addExport(
            PluginRegistry::canonicalExportFormat(record),
            pluginNameFilter(record.displayName, record.extension),
            [record, &manager, &hostApi]() -> std::unique_ptr<FilterInterface> {
                return std::make_unique<PluginFileIOAdapter>(
                    record, manager, hostApi);
            }));
    }
}

PluginFormatRegistration::~PluginFormatRegistration()
{
    FilterRegistry& filters = FilterRegistry::instance();
    for (int id : m_ids)
    {
        filters.remove(id);
    }
}
