#ifndef NATIVE_PLUGIN_LOADER_H
#define NATIVE_PLUGIN_LOADER_H

#include "YiCadPluginAbi.h"

#include <QString>

#include <cstdint>

enum class NativePluginLoadErrorCode
{
    None,
    EmptyPath,
    PathIsNotAbsolute,
    InvalidDllExtension,
    DllDoesNotExist,
    DllPathIsNotFile,
    DllPathNormalizationFailed,
    UnsupportedPlatform,
    LoadLibraryFailed,
    MissingAbiVersionEntry,
    MissingInitEntry,
    MissingShutdownEntry
};

struct NativePluginLoadResult
{
    NativePluginLoadErrorCode code = NativePluginLoadErrorCode::None;
    QString dllPath;
    QString message;
    std::uint32_t systemError = 0;

    /// @brief 返回 DLL 及全部固定入口是否加载成功。
    bool isSuccess() const noexcept;
};

class NativePluginLoader
{
public:
    NativePluginLoader() noexcept = default;
    ~NativePluginLoader() noexcept;

    NativePluginLoader(const NativePluginLoader&) = delete;
    NativePluginLoader& operator=(const NativePluginLoader&) = delete;

    NativePluginLoader(NativePluginLoader&& other) noexcept;
    NativePluginLoader& operator=(NativePluginLoader&& other) noexcept;

    /// @brief 加载绝对路径指定的 DLL，并解析全部固定入口。
    /// @param dllPath 待加载 DLL 的绝对路径。
    /// @return 包含明确错误码和系统错误信息的加载结果。
    NativePluginLoadResult load(const QString& dllPath);

    /// @brief 释放当前 DLL；可重复调用。
    void unload() noexcept;

    bool isLoaded() const noexcept;
    const QString& dllPath() const noexcept;

    YiCadPluginGetAbiVersionFn abiVersionFunction() const noexcept;
    YiCadPluginInitFn initFunction() const noexcept;
    YiCadPluginShutdownFn shutdownFunction() const noexcept;

private:
    void* m_handle = nullptr;
    QString m_dllPath;
    YiCadPluginGetAbiVersionFn m_abiVersionFunction = nullptr;
    YiCadPluginInitFn m_initFunction = nullptr;
    YiCadPluginShutdownFn m_shutdownFunction = nullptr;
};

#endif // NATIVE_PLUGIN_LOADER_H
