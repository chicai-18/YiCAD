/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file YiCadLog.cpp
/// @brief 日志门面实现

#include "YiCadLog.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>

#include <QDebug>

namespace yicad
{
namespace
{

/// @brief 已注册分类的登记簿。分类多为静态局部对象，生命周期长于登记簿的
///        使用时机，这里只存指针。
std::vector<LogCategory*>& registry()
{
    static std::vector<LogCategory*> instance;
    return instance;
}

std::mutex& registryMutex()
{
    static std::mutex instance;
    return instance;
}

/// @brief 把级别名解析为枚举，无法识别时返回 fallback
LogLevel parseLevel(const std::string& text, LogLevel fallback)
{
    std::string lower;
    lower.reserve(text.size());
    for (char c : text)
    {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (lower == "off") return LogLevel::Off;
    if (lower == "critical") return LogLevel::Critical;
    if (lower == "error") return LogLevel::Error;
    if (lower == "warning" || lower == "warn") return LogLevel::Warning;
    if (lower == "info") return LogLevel::Info;
    if (lower == "debug") return LogLevel::Debug;
    if (lower == "trace") return LogLevel::Trace;
    return fallback;
}

const char* levelName(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Off:      return "OFF";
    case LogLevel::Critical: return "CRIT";
    case LogLevel::Error:    return "ERROR";
    case LogLevel::Warning:  return "WARN";
    case LogLevel::Info:     return "INFO";
    case LogLevel::Debug:    return "DEBUG";
    case LogLevel::Trace:    return "TRACE";
    }
    return "?";
}

/// @brief 去掉首尾空白
std::string trim(const std::string& s)
{
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    auto begin = std::find_if(s.begin(), s.end(), notSpace);
    auto end = std::find_if(s.rbegin(), s.rend(), notSpace).base();
    return (begin < end) ? std::string(begin, end) : std::string();
}

}  // namespace

// ---------------------------------------------------------------------------
// LogCategory
// ---------------------------------------------------------------------------

LogCategory::LogCategory(const char* name, LogLevel defaultLevel)
    : m_name(name)
    , m_level(defaultLevel)
{
    std::lock_guard<std::mutex> guard(registryMutex());
    registry().push_back(this);
}

// ---------------------------------------------------------------------------
// LogMessage
// ---------------------------------------------------------------------------

LogMessage::LogMessage(const LogCategory& category, LogLevel level)
    : m_category(category)
    , m_level(level)
{
}

LogMessage::~LogMessage()
{
    // 统一经 Qt 的消息处理器输出，这样日志能被 qInstallMessageHandler
    // 重定向到文件或界面，而不是散落在 stdout。
    const std::string text = m_stream.str();
    const QString line = QStringLiteral("[%1][%2] %3")
                             .arg(QString::fromLatin1(levelName(m_level)))
                             .arg(QString::fromLatin1(m_category.name()))
                             .arg(QString::fromStdString(text));

    switch (m_level)
    {
    case LogLevel::Off:
        break;
    case LogLevel::Critical:
    case LogLevel::Error:
        qCritical().noquote() << line;
        break;
    case LogLevel::Warning:
        qWarning().noquote() << line;
        break;
    default:
        qDebug().noquote() << line;
        break;
    }
}

// ---------------------------------------------------------------------------
// 内置分类
// ---------------------------------------------------------------------------

namespace log
{
LogCategory& render()
{
    static LogCategory category("render");
    return category;
}

LogCategory& snap()
{
    static LogCategory category("snap");
    return category;
}

LogCategory& selection()
{
    static LogCategory category("selection");
    return category;
}

LogCategory& persistence()
{
    static LogCategory category("persistence");
    return category;
}

LogCategory& plugin()
{
    static LogCategory category("plugin");
    return category;
}
}  // namespace log

// ---------------------------------------------------------------------------
// 环境变量配置
// ---------------------------------------------------------------------------

void configureLoggingFromEnvironment()
{
    // 先把内置分类实例化，否则登记簿里还没有它们，配置就落不到实处。
    log::render();
    log::snap();
    log::selection();
    log::persistence();
    log::plugin();

    const char* spec = std::getenv("YICAD_LOG");
    if (spec == nullptr || *spec == '\0')
    {
        return;
    }

    // 形如 "*:warning,render:debug,snap:info"
    const std::string text(spec);
    std::size_t pos = 0;
    while (pos <= text.size())
    {
        const std::size_t comma = text.find(',', pos);
        const std::string item =
            trim(text.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
        pos = (comma == std::string::npos) ? text.size() + 1 : comma + 1;

        if (item.empty())
        {
            continue;
        }

        const std::size_t colon = item.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }

        const std::string name = trim(item.substr(0, colon));
        const LogLevel level = parseLevel(trim(item.substr(colon + 1)), LogLevel::Warning);

        std::lock_guard<std::mutex> guard(registryMutex());
        for (LogCategory* category : registry())
        {
            if (name == "*" || name == category->name())
            {
                category->setLevel(level);
            }
        }
    }
}

}  // namespace yicad
