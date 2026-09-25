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

/// @file Commands.h
/// @brief 命令行命令管理类，维护命令行别名到命令 ID 的映射，支持配置文件加载
///
/// 别名来自 keyconfig.xml（程序目录下的默认配置与用户目录下的覆盖），以
/// CommandRegistry 的字符串命令 ID 为键，例如
/// `<item command="draw.line" description="两点直线" keys="line,li,l"/>`。
/// 原先以 DM::ActionType 的枚举名为键（`action="ActionDrawLine"`），业务工具化
/// 第四步改为命令 ID（doc/COMMAND_TOOL_MIGRATION_PLAN.md）；读取旧格式时按原映射表
/// 转换，用户目录下的旧格式文件在加载时改写一次，并保留备份。

#ifndef COMMANDS_H
#define COMMANDS_H

#include <QString>
#include <QStringList>
#include <map>
#include <vector>

#define COMMANDS Commands::instance()

/// @brief keyconfig.xml 的一条：命令 ID、说明与命令行别名
struct CommandKeys
{
    QString commandId;   ///< CommandRegistry 的命令 ID，或 UIActionHandler 自己处理的内置命令
    QString description; ///< 说明，命令行提示的"[说明]"前缀与"命令设置"对话框显示它
    QStringList keys;    ///< 命令行别名，小写
};

/// @brief 命令行命令管理器，维护命令行别名与命令 ID 的映射关系
/// 与GUI分离以便支持不同语言界面与命令接口
/// 实现为单例模式
class Commands
{
public:
    /// @brief 获取唯一实例
    /// @return 命令管理器单例指针
    static Commands* instance();

    /// @brief 将命令字符串转换为对应的命令 ID
    /// @param [in] cmd 命令字符串，大小写不敏感
    /// @return 对应的命令 ID，未找到返回空串
    QString cmdToCommand(const QString& cmd) const;

    /// @brief 将快捷键编码转换为对应的命令 ID
    /// @param [in] code 按键编码字符串
    /// @return 对应的命令 ID，未找到返回空串
    QString keycodeToCommand(const QString& code) const;

    /// @brief 从程序配置文件及用户配置文件加载命令映射
    /// @details 用户配置文件是旧格式时先改写为新格式（见 migrateLegacyConfig()）
    /// @return true表示加载成功
    bool load();

    /// @brief 通过XML读取的数据加载
    /// @param [in] data 命令数据列表
    /// @param [in] clearOld 是否清除旧数据
    void loadFromData(const std::vector<CommandKeys>& data, bool clearOld);

    /// @brief 读取配置文件获得所有组名
    /// @return 组名列表
    QStringList getGroups() const;

    /// @brief 获取命令的翻译文本
    /// @param [in] cmd 英文命令
    /// @return 翻译后的命令
    static QString command(const QString& cmd);

    /// @brief 获取命令对应的描述文本（翻译后的命令名）
    /// @param [in] commandId 命令 ID
    /// @return 描述文本，未找到返回空字符串
    QString description(const QString& commandId) const;

    /// @brief 检查给定字符串是否匹配指定命令
    /// @param [in] cmd 要检查的命令
    /// @param [in] str 用户输入的字符串
    /// @return true表示匹配
    static bool checkCommand(const QString& cmd, const QString& str);

    /// @brief 获取"可用命令"的本地化文本
    /// @return 本地化文本
    static QString msgAvailableCommands();

    /// @brief 删除命令管理器实例
    void deleteCommands();

    // 功能键、Meta键和Alt键的前缀
    static const char* FnPrefix;   ///< Fn功能键前缀
    static const char* AltPrefix;  ///< Alt键前缀
    static const char* MetaPrefix; ///< Meta键前缀

    /// @brief 从命令行字符串中提取CLI计算器数学表达式
    /// @param [in] cmd 命令行字符串
    /// @return 数学表达式字符串，用于Math::eval()
    static QString filterCliCal(const QString& cmd);

    /// @brief 从配置文件查找指定名字的组，返回组内的数据
    /// @details 兼容旧格式：`action` 属性按原映射表（legacyCommandId()）转成命令 ID，
    ///          映射表里没有的条目丢弃（原先读取时同样丢弃）
    /// @param [in] configFile 配置文件路径
    /// @param [in,out] group 组名
    /// @param [in] restrictMatch 是否精确匹配组，为false没有找到则返回第一个
    /// @return 组内的命令数据列表
    static std::vector<CommandKeys> readConfigFile(const QString& configFile, QString& group,
                                                   const bool restrictMatch);

    /// @brief 获取当前命令数据
    /// @return 命令数据列表
    std::vector<CommandKeys> getData() const;

    /// @brief 保存命令数据到文件（新格式），文件里的其它组原样保留
    /// @param [in] data 命令数据列表
    /// @param [in] group 组名
    /// @param [in] file 文件路径
    /// @return true表示保存成功
    static bool saveToFile(const std::vector<CommandKeys>& data, const QString& group, const QString& file);

    /// @brief 旧格式的配置文件改写为新格式，改写前把原文件复制为 `<file>.bak`
    /// @details 只要有一条 `action` 属性就算旧格式；逐组按原映射表转换，映射表里没有的
    ///          条目丢弃。备份已存在时不覆盖（保留最早的那份）。
    /// @param [in] file 配置文件路径
    /// @return 改写了文件时返回 true；文件不存在、已是新格式或改写失败时返回 false
    static bool migrateLegacyConfig(const QString& file);

    /// @brief 原 DM::ActionType 枚举名对应的命令 ID（旧格式 keyconfig.xml 的转换表）
    /// @return 没有对应命令（从未实现或已删除）时返回空串
    static QString legacyCommandId(const QString& actionName);

    /// @brief 获取命令行别名到命令 ID 的映射
    /// @return 别名（小写）到命令 ID
    std::map<QString, QString> getKeyCommands() const;

    /// @brief 获取配置文件路径
    /// @return 配置文件全路径
    QString getConfigFile() const;

    /// @brief 获取用户配置文件路径
    /// @return 用户配置文件全路径
    QString getUserConfig() const;

private:
    /// @brief 私有构造函数，初始化命令管理器
    Commands();

    /// @brief 析构函数
    ~Commands();

    /// @brief 删除拷贝构造
    Commands(Commands&) = delete;

    /// @brief 删除拷贝赋值
    Commands& operator=(Commands&) = delete;

private:
    static Commands* m_pUniqueInstance;  ///< 单例实例指针

    ///< 命令行别名（小写）到命令 ID 的映射
    std::map<QString, QString> m_keyCommandMap;

    ///< 配置文件当前组数据
    std::vector<CommandKeys> m_data;

    ///< 配置文件全路径（exe目录下）
    QString m_strConfigFile;

    ///< 用户配置文件全路径（如 C:\\Users\\...\\AppData\\Local\\YiCAD\\keyconfig.xml）
    QString m_strUserConfig;

    ///< 当前命令对应的组，一个组对应一套命令
    QString m_curGroup;
};

#endif // COMMANDS_H
