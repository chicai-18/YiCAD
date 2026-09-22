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

/// @file YiCadLog.h
/// @brief 轻量日志门面：在既有 Debug 单例之上补齐分类与按分类分级的能力
///
/// 既有的 Debug（Debug.h）有全局级别，但没有分类：想打开捕捉的调试输出，
/// 就会连带打开渲染每帧的输出。本门面按「分类 + 级别」两维过滤，默认只放行
/// Warning 及以上，因此在产品路径上不会产生任何每帧输出。
///
/// 用法：
/// @code
///     YICAD_LOG(yicad::log::render(), yicad::LogLevel::Debug)
///         << "帧耗时 " << ms << " ms";
/// @endcode
///
/// 运行期通过环境变量 YICAD_LOG 配置，逗号分隔的 `分类:级别` 列表，
/// 分类 `*` 表示默认级别：
/// @code
///     YICAD_LOG=*:warning,render:debug,snap:info
/// @endcode
///
/// 级别名不区分大小写，可用 off/critical/error/warning/info/debug/trace。
/// 未通过过滤的语句不会构造消息字符串——流式参数根本不会求值。

#ifndef YICADLOG_H
#define YICADLOG_H

#include <sstream>
#include <string>

namespace yicad
{

/// @brief 日志级别，数值越大越详细
enum class LogLevel
{
    Off = 0,       ///< 完全关闭
    Critical = 1,  ///< 无法继续运行
    Error = 2,     ///< 操作失败但可继续
    Warning = 3,   ///< 可疑但已处理（默认放行到此级别）
    Info = 4,      ///< 正常流程中的关键节点
    Debug = 5,     ///< 排查问题用
    Trace = 6      ///< 逐帧、逐实体级别的细节
};

/// @brief 日志分类。按分类名注册，进程内唯一，构造后地址稳定。
class LogCategory
{
public:
    /// @brief 构造一个分类
    /// @param [in] name 分类名，出现在输出前缀里，也是 YICAD_LOG 的键
    /// @param [in] defaultLevel 未被环境变量覆盖时的级别
    explicit LogCategory(const char* name, LogLevel defaultLevel = LogLevel::Warning);

    /// @brief 分类名
    const char* name() const { return m_name; }

    /// @brief 当前生效级别
    LogLevel level() const { return m_level; }

    /// @brief 运行期改级别（供设置页或测试使用）
    void setLevel(LogLevel level) { m_level = level; }

    /// @brief 该级别是否放行
    bool isEnabled(LogLevel level) const
    {
        return level != LogLevel::Off && static_cast<int>(level) <= static_cast<int>(m_level);
    }

private:
    const char* m_name;
    LogLevel m_level;
};

/// @brief 一条日志消息。析构时才真正写出，便于流式拼装。
class LogMessage
{
public:
    LogMessage(const LogCategory& category, LogLevel level);
    ~LogMessage();

    LogMessage(const LogMessage&) = delete;
    LogMessage& operator=(const LogMessage&) = delete;

    /// @brief 流式写入
    template <typename T>
    LogMessage& operator<<(const T& value)
    {
        m_stream << value;
        return *this;
    }

private:
    const LogCategory& m_category;
    LogLevel m_level;
    std::ostringstream m_stream;
};

namespace log
{
/// @brief 渲染相关（帧耗时、painter 生命周期）
LogCategory& render();
/// @brief 捕捉与拾取
LogCategory& snap();
/// @brief 选择（点选、框选）
LogCategory& selection();
/// @brief 持久化读写
LogCategory& persistence();
/// @brief 插件加载与宿主 API
LogCategory& plugin();
}  // namespace log

/// @brief 按 YICAD_LOG 环境变量配置全部已注册分类。
///        由 DmSystem::init 调用一次；重复调用是安全的。
void configureLoggingFromEnvironment();

}  // namespace yicad

/// @brief 条件日志。过滤不通过时右侧完全不求值。
#define YICAD_LOG(category, level)          \
    if (!(category).isEnabled(level)) {     \
    } else                                  \
        ::yicad::LogMessage((category), (level))

#endif  // YICADLOG_H
