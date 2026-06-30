#ifndef PLUGIN_REGISTRY_H
#define PLUGIN_REGISTRY_H

#include "YiCadPluginAbi.h"

#include <QString>
#include <QVector>

enum class PluginRegistryErrorCode
{
    None,
    TransactionAlreadyActive,
    NoActiveTransaction,
    InvalidPluginId,
    PluginIdMismatch,
    DuplicatePluginId,
    InvalidCommand,
    DuplicateCommand,
    InvalidRibbonButton,
    MissingRibbonCommand,
    InvalidImportFilter,
    DuplicateImportFormat,
    DuplicateImportExtension,
    InvalidExportFilter,
    DuplicateExportFormat
};

struct PluginRegistryError
{
    PluginRegistryErrorCode code = PluginRegistryErrorCode::None;
    QString message;

    bool isError() const noexcept;
};

struct PluginRecord
{
    QString pluginId;
    QString pluginName;
    QString pluginVersion;
    QString dllPath;
    QString dllDirectory;
};

struct PluginCommandRecord
{
    QString pluginId;
    QString commandId;
    QString displayName;
    /// @brief 回调及 userData 归 pluginId 对应插件所有，shutdown 前有效。
    YiCadCommandCallback callback = nullptr;
    void* userData = nullptr;
};

struct PluginRibbonButtonRecord
{
    QString pluginId;
    QString tab;
    QString group;
    QString commandId;
    QString iconPath;
};

struct PluginImportFilterRecord
{
    QString pluginId;
    QString formatId;
    QString displayName;
    QString extension;
    YiCadImportCallback callback = nullptr;
    void* userData = nullptr;
};

struct PluginExportFilterRecord
{
    QString pluginId;
    QString formatId;
    QString displayName;
    QString extension;
    YiCadExportCallback callback = nullptr;
    void* userData = nullptr;
};

/// @brief 原子保存插件在初始化期间声明的宿主注册项。
/// @note 注册和回调仅允许由 UI 主线程调用；该类型不提供线程同步。
/// @note 所有已提交回调均绑定 pluginId，其地址由插件保证在 shutdown 前有效。
class PluginRegistry
{
public:
    /// @brief 开始一个插件注册事务。
    /// @return 没有其他活动事务时返回 true。
    bool beginRegistration();

    /// @brief 暂存一个命令，字符串会复制为宿主所有。
    bool stageCommand(
        const QString& pluginId,
        const QString& commandId,
        const QString& displayName,
        YiCadCommandCallback callback,
        void* userData);

    /// @brief 暂存一个 Ribbon 按钮，命令引用在提交时统一校验。
    bool stageRibbonButton(
        const QString& pluginId,
        const QString& tab,
        const QString& group,
        const QString& commandId,
        const QString& iconPath);

    /// @brief 暂存一个导入过滤器，扩展名会移除前导点并转为小写。
    bool stageImportFilter(
        const QString& pluginId,
        const QString& formatId,
        const QString& displayName,
        const QString& extension,
        YiCadImportCallback callback,
        void* userData);

    /// @brief 暂存一个导出过滤器，扩展名会移除前导点并转为小写。
    bool stageExportFilter(
        const QString& pluginId,
        const QString& formatId,
        const QString& displayName,
        const QString& extension,
        YiCadExportCallback callback,
        void* userData);

    /// @brief 校验并原子提交当前事务。
    /// @param plugin 已初始化插件的宿主侧元数据副本。
    /// @return 全部记录提交成功时返回 true；失败时自动回滚。
    bool commitRegistration(const PluginRecord& plugin);

    /// @brief 丢弃当前事务中的全部暂存记录；可重复调用。
    void rollbackRegistration() noexcept;

    bool hasActiveRegistration() const noexcept;
    const PluginRegistryError& lastError() const noexcept;

    const QVector<PluginRecord>& plugins() const noexcept;
    const QVector<PluginCommandRecord>& commands() const noexcept;
    const QVector<PluginRibbonButtonRecord>& ribbonButtons() const noexcept;
    const QVector<PluginImportFilterRecord>& importFilters() const noexcept;
    const QVector<PluginExportFilterRecord>& exportFilters() const noexcept;

    const PluginRecord* findPlugin(const QString& pluginId) const noexcept;
    const PluginCommandRecord* findCommand(
        const QString& pluginId,
        const QString& commandId) const noexcept;
    const PluginImportFilterRecord* findImportFilter(
        const QString& extension) const;
    const PluginExportFilterRecord* findExportFilter(
        const QString& pluginId,
        const QString& formatId) const noexcept;

    /// @brief 按插件 ID 和命令 ID 执行已提交命令。
    /// @return 找到命令且回调正常返回时返回 true。
    bool executeCommand(
        const QString& pluginId,
        const QString& commandId) const noexcept;

    /// @brief 生成导入扩展名使用的规范键。
    static QString normalizeExtension(QString extension);

private:
    bool adoptTransactionPlugin(const QString& pluginId);
    bool fail(PluginRegistryErrorCode code, const QString& message);
    void clearTransaction() noexcept;

    bool m_registrationActive = false;
    bool m_transactionFailed = false;
    QString m_transactionPluginId;
    PluginRegistryError m_lastError;

    QVector<PluginCommandRecord> m_stagedCommands;
    QVector<PluginRibbonButtonRecord> m_stagedRibbonButtons;
    QVector<PluginImportFilterRecord> m_stagedImportFilters;
    QVector<PluginExportFilterRecord> m_stagedExportFilters;

    QVector<PluginRecord> m_plugins;
    QVector<PluginCommandRecord> m_commands;
    QVector<PluginRibbonButtonRecord> m_ribbonButtons;
    QVector<PluginImportFilterRecord> m_importFilters;
    QVector<PluginExportFilterRecord> m_exportFilters;
};

#endif // PLUGIN_REGISTRY_H
