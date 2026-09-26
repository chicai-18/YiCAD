#include "PluginFileIOAdapter.h"

#include "HostApi.h"
#include "PluginManager.h"

#include <QByteArray>
#include <QFileInfo>

PluginFileIOAdapter::PluginFileIOAdapter(
    const PluginImportFilterRecord& filter,
    PluginManager& manager,
    HostApi& hostApi)
    : m_mode(Mode::Import)
    , m_importFilter(filter)
    , m_manager(manager)
    , m_hostApi(hostApi)
{
}

PluginFileIOAdapter::PluginFileIOAdapter(
    const PluginExportFilterRecord& filter,
    PluginManager& manager,
    HostApi& hostApi)
    : m_mode(Mode::Export)
    , m_exportFilter(filter)
    , m_manager(manager)
    , m_hostApi(hostApi)
{
}

bool PluginFileIOAdapter::canImport(const QString& file) const
{
    return m_mode == Mode::Import &&
           PluginRegistry::normalizeExtension(QFileInfo(file).suffix()) ==
               m_importFilter.extension;
}

bool PluginFileIOAdapter::canExport(const QString& formatType) const
{
    return m_mode == Mode::Export &&
           formatType ==
               PluginRegistry::canonicalExportFormat(m_exportFilter);
}

bool PluginFileIOAdapter::fileImport(
    DmDocument& document,
    const QString& file)
{
    if (m_mode != Mode::Import || m_importFilter.callback == nullptr)
    {
        return false;
    }

    YiCadDocumentHandle handle = nullptr;
    QByteArray utf8Path;
    if (!prepareCallback(document, file, handle, utf8Path))
    {
        return false;
    }

    try
    {
        return m_importFilter.callback(
                   handle, utf8Path.constData(), m_importFilter.userData) ==
               YICAD_SUCCESS;
    }
    catch (...)
    {
        return false;
    }
}

bool PluginFileIOAdapter::fileExport(
    DmDocument& document,
    const QString& file,
    const QString& formatType)
{
    if (!canExport(formatType) || m_exportFilter.callback == nullptr)
    {
        return false;
    }

    YiCadDocumentHandle handle = nullptr;
    QByteArray utf8Path;
    if (!prepareCallback(document, file, handle, utf8Path))
    {
        return false;
    }

    try
    {
        return m_exportFilter.callback(
                   handle, utf8Path.constData(), m_exportFilter.userData) ==
               YICAD_SUCCESS;
    }
    catch (...)
    {
        return false;
    }
}

bool PluginFileIOAdapter::prepareCallback(
    DmDocument& document,
    const QString& file,
    YiCadDocumentHandle& handle,
    QByteArray& utf8Path) const noexcept
{
    try
    {
        const QString& pluginId = m_mode == Mode::Import
            ? m_importFilter.pluginId
            : m_exportFilter.pluginId;
        if (!m_manager.isPluginActive(pluginId) || file.isEmpty() ||
            file.contains(QChar::Null))
        {
            return false;
        }

        utf8Path = file.toUtf8();
        if (utf8Path.isEmpty() || utf8Path.contains('\0') ||
            QString::fromUtf8(utf8Path) != file)
        {
            return false;
        }

        handle = m_hostApi.documentHandle(&document);
        return handle != nullptr &&
               m_hostApi.isDocumentHandleValid(handle);
    }
    catch (...)
    {
        return false;
    }
}
