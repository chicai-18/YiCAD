#ifndef PLUGIN_FILE_IO_ADAPTER_H
#define PLUGIN_FILE_IO_ADAPTER_H

#include "FilterInterface.h"
#include "PluginRegistry.h"

class HostApi;
class PluginManager;
class QByteArray;

/// @brief 将已提交的原生插件文件回调适配到现有 FilterInterface。
class PluginFileIOAdapter final : public FilterInterface
{
public:
    PluginFileIOAdapter(
        const PluginImportFilterRecord& filter,
        PluginManager& manager,
        HostApi& hostApi);
    PluginFileIOAdapter(
        const PluginExportFilterRecord& filter,
        PluginManager& manager,
        HostApi& hostApi);

    bool canImport(const QString& file) const override;
    bool canExport(const QString& formatType) const override;
    bool fileImport(DmDocument& document, const QString& file) override;
    bool fileExport(
        DmDocument& document,
        const QString& file,
        const QString& formatType) override;

private:
    enum class Mode
    {
        Import,
        Export
    };

    bool prepareCallback(
        DmDocument& document,
        const QString& file,
        YiCadDocumentHandle& handle,
        QByteArray& utf8Path) const noexcept;

    Mode m_mode;
    PluginImportFilterRecord m_importFilter;
    PluginExportFilterRecord m_exportFilter;
    PluginManager& m_manager;
    HostApi& m_hostApi;
};

#endif // PLUGIN_FILE_IO_ADAPTER_H
