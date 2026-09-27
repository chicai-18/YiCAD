/// @file DxfTestRuntime.h
/// @brief 只装 DXF 插件的插件运行时，供测试读写 DXF
///
/// 加载构建目录里真实的 YiCadDxfPlugin.dll，走与程序相同的插件运行时
/// （PluginManager、HostApi、PluginFileIOAdapter），导入导出与程序一样经格式注册表
/// （PluginFormatRegistration 把插件格式登记进 FilterRegistry）找过滤器。
///
/// 使用者要链接 YiCadShell（插件运行时在 src/shell/plugin_runtime/），定义
/// YICAD_DXF_PLUGIN_DLL 为插件 DLL 的绝对路径，并让测试目标依赖 YiCadDxfPlugin。
/// 原先写在 test_dxf_encoding.cpp 里，渲染方案阶段 0 起基线用例与 test_render 也用它。

#ifndef DXFTESTRUNTIME_H
#define DXFTESTRUNTIME_H

#include <memory>

#include <QFile>
#include <QSet>
#include <QStringList>
#include <QTemporaryDir>

#include "DmDocument.h"
#include "FilterInterface.h"
#include "FilterRegistry.h"
#include "HostApi.h"
#include "PluginFileIOAdapter.h"
#include "PluginFormatRegistration.h"
#include "PluginManager.h"
#include "PluginRegistry.h"

#ifndef YICAD_DXF_PLUGIN_DLL
#error "DxfTestRuntime.h 需要 YICAD_DXF_PLUGIN_DLL（DXF 插件 DLL 的绝对路径）"
#endif

namespace yicad_test
{

/// @brief 测试用宿主上下文：只登记打开的文档，不显示消息，没有视图
class TestHostContext final : public PluginHostContext
{
public:
    void showPluginMessage(const QString& message) override
    {
        messages.append(message);
    }

    DmDocument* currentDocument() const noexcept override
    {
        return current;
    }

    bool isDocumentOpen(const DmDocument* document) const noexcept override
    {
        return open.contains(document);
    }

    GuiDocumentView* documentView(const DmDocument*) const noexcept override
    {
        return nullptr;
    }

    QStringList messages;
    DmDocument* current = nullptr;
    QSet<const DmDocument*> open;
};

/// @brief 只装 DXF 插件的插件运行时
class DxfRuntime
{
public:
    DxfRuntime()
        : m_host(m_context, m_registry)
    {
        // 清单放在临时目录，dll 写绝对路径，指向构建目录里的插件
        QFile manifest(m_manifestDir.filePath(QStringLiteral("dxf.xml")));
        if (manifest.open(QIODevice::WriteOnly))
        {
            manifest.write(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<plugin dll=\"%1\"/>\n")
                               .arg(QStringLiteral(YICAD_DXF_PLUGIN_DLL))
                               .toUtf8());
            manifest.close();
        }
        m_manager = std::make_unique<PluginManager>(m_host, m_registry, m_manifestDir.path());
        m_manager->loadAll();
        m_formats = std::make_unique<PluginFormatRegistration>(m_registry, *m_manager, m_host);
    }

    /// @brief 插件已加载并注册了 DXF 的导入与导出
    bool loaded() const
    {
        return m_manager->isPluginActive(QStringLiteral("com.yicad.dxf")) && !m_registry.importFilters().isEmpty()
               && !m_registry.exportFilters().isEmpty();
    }

    /// @brief 加载失败时的诊断信息
    QString diagnostics() const
    {
        QStringList lines;
        for (const PluginManagerRecord& record : m_manager->records())
        {
            lines.append(record.dllPath + QStringLiteral(": ") + record.error.message);
        }
        return lines.join(QLatin1Char('\n'));
    }

    /// @brief 直接用插件的导入过滤器把文件读进文档
    bool importFile(DmDocument& document, const QString& path)
    {
        open(document);
        std::unique_ptr<FilterInterface> filter = FilterRegistry::instance().importFilter(path);
        return dynamic_cast<PluginFileIOAdapter*>(filter.get()) != nullptr && filter->fileImport(document, path);
    }

    /// @brief 与程序打开图纸相同，经 DmDocument::readFile() 读入（计入 document.open 埋点）
    bool readFile(DmDocument& document, const QString& path)
    {
        open(document);
        return document.readFile(path).ok();
    }

    bool exportFile(DmDocument& document, const QString& path)
    {
        open(document);
        const QString format = exportFormat();
        std::unique_ptr<FilterInterface> filter = FilterRegistry::instance().exportFilter(format);
        return dynamic_cast<PluginFileIOAdapter*>(filter.get()) != nullptr && filter->fileExport(document, path, format);
    }

    /// @brief 插件声明的导入格式数
    int importFormatCount() const { return static_cast<int>(m_registry.importFilters().size()); }

    /// @brief 插件声明的导出格式数
    int exportFormatCount() const { return static_cast<int>(m_registry.exportFilters().size()); }

    /// @brief 插件的 DXF 导出格式名
    QString exportFormat() const { return PluginRegistry::canonicalExportFormat(m_registry.exportFilters().front()); }

    /// @brief 插件发给宿主的消息（导入警告等）
    const QStringList& messages() const { return m_context.messages; }

private:
    void open(DmDocument& document)
    {
        m_context.open.insert(&document);
        m_context.current = &document;
    }

    TestHostContext m_context;
    PluginRegistry m_registry;
    HostApi m_host;
    QTemporaryDir m_manifestDir;
    std::unique_ptr<PluginManager> m_manager;
    std::unique_ptr<PluginFormatRegistration> m_formats;  ///< 先于插件 shutdown 析构，与程序相同
};

}  // namespace yicad_test

#endif  // DXFTESTRUNTIME_H
