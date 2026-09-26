#include "NativePluginLoader.h"

#include <QDir>
#include <QFileInfo>

#include <type_traits>
#include <utility>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace
{

NativePluginLoadResult makeError(
    NativePluginLoadErrorCode code,
    const QString& dllPath,
    QString message,
    std::uint32_t systemError = 0)
{
    NativePluginLoadResult result;
    result.code = code;
    result.dllPath = dllPath;
    result.message = std::move(message);
    result.systemError = systemError;
    return result;
}

#if defined(Q_OS_WIN)
QString windowsErrorMessage(DWORD errorCode)
{
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<wchar_t*>(&buffer),
        0,
        nullptr);

    QString message;
    if (length != 0 && buffer != nullptr)
    {
        message = QString::fromWCharArray(
                      buffer,
                      static_cast<int>(length))
                      .trimmed();
    }
    if (buffer != nullptr)
    {
        LocalFree(buffer);
    }

    if (message.isEmpty())
    {
        message = QStringLiteral("Windows error %1").arg(errorCode);
    }
    return message;
}

NativePluginLoadResult missingEntryError(
    NativePluginLoadErrorCode code,
    const QString& dllPath,
    const char* entryName,
    DWORD systemError)
{
    return makeError(
        code,
        dllPath,
        QStringLiteral("Plugin DLL is missing required entry '%1': %2")
            .arg(
                QString::fromLatin1(entryName),
                windowsErrorMessage(systemError)),
        static_cast<std::uint32_t>(systemError));
}
#endif

} // namespace

static_assert(!std::is_copy_constructible_v<NativePluginLoader>);
static_assert(!std::is_copy_assignable_v<NativePluginLoader>);
static_assert(std::is_nothrow_move_constructible_v<NativePluginLoader>);
static_assert(std::is_nothrow_move_assignable_v<NativePluginLoader>);

bool NativePluginLoadResult::isSuccess() const noexcept
{
    return code == NativePluginLoadErrorCode::None;
}

NativePluginLoader::~NativePluginLoader() noexcept
{
    unload();
}

NativePluginLoader::NativePluginLoader(NativePluginLoader&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr)),
      m_dllPath(std::move(other.m_dllPath)),
      m_abiVersionFunction(
          std::exchange(other.m_abiVersionFunction, nullptr)),
      m_initFunction(std::exchange(other.m_initFunction, nullptr)),
      m_shutdownFunction(std::exchange(other.m_shutdownFunction, nullptr))
{
    other.m_dllPath.clear();
}

NativePluginLoader& NativePluginLoader::operator=(
    NativePluginLoader&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    unload();
    m_handle = std::exchange(other.m_handle, nullptr);
    m_dllPath = std::move(other.m_dllPath);
    m_abiVersionFunction = std::exchange(
        other.m_abiVersionFunction,
        nullptr);
    m_initFunction = std::exchange(other.m_initFunction, nullptr);
    m_shutdownFunction = std::exchange(other.m_shutdownFunction, nullptr);
    other.m_dllPath.clear();
    return *this;
}

NativePluginLoadResult NativePluginLoader::load(const QString& dllPath)
{
    unload();

    const QString trimmedPath = dllPath.trimmed();
    if (trimmedPath.isEmpty())
    {
        return makeError(
            NativePluginLoadErrorCode::EmptyPath,
            {},
            QStringLiteral("Plugin DLL path cannot be empty."));
    }

    if (!QDir::isAbsolutePath(trimmedPath))
    {
        return makeError(
            NativePluginLoadErrorCode::PathIsNotAbsolute,
            trimmedPath,
            QStringLiteral("Plugin DLL path must be absolute: %1")
                .arg(trimmedPath));
    }

    const QString cleanedPath = QDir::cleanPath(trimmedPath);
    const QFileInfo dllFileInfo(cleanedPath);
    if (dllFileInfo.suffix().compare(
            QStringLiteral("dll"),
            Qt::CaseInsensitive) != 0)
    {
        return makeError(
            NativePluginLoadErrorCode::InvalidDllExtension,
            cleanedPath,
            QStringLiteral("Plugin path must have a .dll extension: %1")
                .arg(cleanedPath));
    }

    if (!dllFileInfo.exists())
    {
        return makeError(
            NativePluginLoadErrorCode::DllDoesNotExist,
            cleanedPath,
            QStringLiteral("Plugin DLL does not exist: %1").arg(cleanedPath));
    }

    if (!dllFileInfo.isFile())
    {
        return makeError(
            NativePluginLoadErrorCode::DllPathIsNotFile,
            cleanedPath,
            QStringLiteral("Plugin DLL path is not a file: %1")
                .arg(cleanedPath));
    }

    const QString canonicalPath = dllFileInfo.canonicalFilePath();
    if (canonicalPath.isEmpty() || !QFileInfo(canonicalPath).isAbsolute())
    {
        return makeError(
            NativePluginLoadErrorCode::DllPathNormalizationFailed,
            cleanedPath,
            QStringLiteral("Plugin DLL path could not be normalized: %1")
                .arg(cleanedPath));
    }

    const QString normalizedPath = QDir::cleanPath(canonicalPath);
    if (QFileInfo(normalizedPath).suffix().compare(
            QStringLiteral("dll"),
            Qt::CaseInsensitive) != 0)
    {
        return makeError(
            NativePluginLoadErrorCode::InvalidDllExtension,
            normalizedPath,
            QStringLiteral("Normalized plugin path must have a .dll extension: %1")
                .arg(normalizedPath));
    }

#if !defined(Q_OS_WIN)
    return makeError(
        NativePluginLoadErrorCode::UnsupportedPlatform,
        normalizedPath,
        QStringLiteral("Native YiCAD plugins are supported only on Windows."));
#else
    /// @brief 显式包含插件目录和系统安全目录，不让当前工作目录参与依赖搜索。
    constexpr DWORD LoadFlags =
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;
    HMODULE handle = LoadLibraryExW(
        reinterpret_cast<LPCWSTR>(normalizedPath.utf16()),
        nullptr,
        LoadFlags);
    if (handle == nullptr)
    {
        const DWORD systemError = GetLastError();
        return makeError(
            NativePluginLoadErrorCode::LoadLibraryFailed,
            normalizedPath,
            QStringLiteral("Cannot load plugin DLL '%1': %2")
                .arg(normalizedPath, windowsErrorMessage(systemError)),
            static_cast<std::uint32_t>(systemError));
    }

    const auto releaseHandle = [&handle]() noexcept
    {
        if (handle != nullptr)
        {
            FreeLibrary(handle);
            handle = nullptr;
        }
    };

    const FARPROC abiVersionEntry = GetProcAddress(
        handle,
        "yicad_plugin_get_abi_version");
    if (abiVersionEntry == nullptr)
    {
        const DWORD systemError = GetLastError();
        releaseHandle();
        return missingEntryError(
            NativePluginLoadErrorCode::MissingAbiVersionEntry,
            normalizedPath,
            "yicad_plugin_get_abi_version",
            systemError);
    }

    const FARPROC initEntry = GetProcAddress(handle, "yicad_plugin_init");
    if (initEntry == nullptr)
    {
        const DWORD systemError = GetLastError();
        releaseHandle();
        return missingEntryError(
            NativePluginLoadErrorCode::MissingInitEntry,
            normalizedPath,
            "yicad_plugin_init",
            systemError);
    }

    const FARPROC shutdownEntry = GetProcAddress(
        handle,
        "yicad_plugin_shutdown");
    if (shutdownEntry == nullptr)
    {
        const DWORD systemError = GetLastError();
        releaseHandle();
        return missingEntryError(
            NativePluginLoadErrorCode::MissingShutdownEntry,
            normalizedPath,
            "yicad_plugin_shutdown",
            systemError);
    }

    m_handle = handle;
    m_dllPath = normalizedPath;
    /// @brief 固定 C ABI 类型负责保留入口的 __cdecl 调用约定。
    m_abiVersionFunction = reinterpret_cast<YiCadPluginGetAbiVersionFn>(
        abiVersionEntry);
    m_initFunction = reinterpret_cast<YiCadPluginInitFn>(initEntry);
    m_shutdownFunction = reinterpret_cast<YiCadPluginShutdownFn>(shutdownEntry);

    NativePluginLoadResult result;
    result.dllPath = normalizedPath;
    return result;
#endif
}

void NativePluginLoader::unload() noexcept
{
#if defined(Q_OS_WIN)
    if (m_handle != nullptr)
    {
        FreeLibrary(reinterpret_cast<HMODULE>(m_handle));
    }
#endif

    m_handle = nullptr;
    m_dllPath.clear();
    m_abiVersionFunction = nullptr;
    m_initFunction = nullptr;
    m_shutdownFunction = nullptr;
}

bool NativePluginLoader::isLoaded() const noexcept
{
    return m_handle != nullptr &&
           m_abiVersionFunction != nullptr &&
           m_initFunction != nullptr &&
           m_shutdownFunction != nullptr;
}

const QString& NativePluginLoader::dllPath() const noexcept
{
    return m_dllPath;
}

YiCadPluginGetAbiVersionFn
NativePluginLoader::abiVersionFunction() const noexcept
{
    return m_abiVersionFunction;
}

YiCadPluginInitFn NativePluginLoader::initFunction() const noexcept
{
    return m_initFunction;
}

YiCadPluginShutdownFn NativePluginLoader::shutdownFunction() const noexcept
{
    return m_shutdownFunction;
}
