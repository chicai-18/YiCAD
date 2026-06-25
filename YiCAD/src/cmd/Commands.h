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
/// @brief 命令行命令管理类，维护命令到ActionType的映射，支持配置文件加载

#ifndef COMMANDS_H
#define COMMANDS_H

#include <QString>
#include <map>

#include "Datamodel.h"

#define COMMANDS Commands::instance()

/// @brief 命令行命令管理器，维护所有命令与ActionType的映射关系
/// 与GUI分离以便支持不同语言界面与命令接口
/// 实现为单例模式
class Commands
{
public:
    /// @brief 获取唯一实例
    /// @return 命令管理器单例指针
    static Commands* instance();

    //QStringList complete(const QString& cmd);

    /// @brief 将命令字符串转换为对应的ActionType
    /// @param [in] cmd 命令字符串
    /// @param [in] verbose 是否输出详细信息
    /// @return 对应的ActionType
    DM::ActionType cmdToAction(const QString& cmd, bool verbose = true);

    /// @brief 将快捷键编码转换为对应的ActionType
    /// @param [in] code 按键编码字符串
    /// @return 对应的ActionType
    DM::ActionType keycodeToAction(const QString& code);

    /// @brief 从程序配置文件及用户配置文件加载命令映射
    /// @return true表示加载成功
    bool load();

    /// @brief 通过XML读取的数据加载
    /// @param [in] data 命令数据列表
    /// @param [in] clearOld 是否清除旧数据
    void loadFromData(const std::vector<std::tuple<DM::ActionType, QString,
                       QStringList>>& data, bool clearOld);

    /// @brief 读取配置文件获得所有组名
    /// @return 组名列表
    QStringList getGroups() const;

    /// @brief 获取命令的翻译文本
    /// @param [in] cmd 英文命令
    /// @return 翻译后的命令
    static QString command(const QString& cmd);

    /// @brief 获取ActionType对应的描述文本（翻译后的命令名）
    /// @param [in] type ActionType
    /// @return 描述文本，未找到返回空字符串
    QString description(DM::ActionType type) const;

    /// @brief 检查给定字符串是否匹配指定命令
    /// @param [in] cmd 要检查的命令
    /// @param [in] str 用户输入的字符串
    /// @param [in] action 相关的ActionType
    /// @return true表示匹配
    static bool checkCommand(const QString& cmd, const QString& str,
                             DM::ActionType action = DM::ActionNone);

    /// @brief 获取"可用命令"的本地化文本
    /// @return 本地化文本
    static QString msgAvailableCommands();

    //void updateAlias();

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
    /// @param [in] configFile 配置文件路径
    /// @param [in,out] group 组名
    /// @param [in] restrictMatch 是否精确匹配组，为false没有找到则返回第一个
    /// @return 组内的命令数据列表
    static std::vector<std::tuple<DM::ActionType, QString, QStringList>>
        readConfigFile(const QString& configFile, QString& group,
                       const bool restrictMatch);

    /// @brief 获取当前命令数据
    /// @return 命令数据列表
    std::vector<std::tuple<DM::ActionType, QString, QStringList>>
        getData() const;

    /// @brief 保存命令数据到文件
    /// @param [in] data 命令数据列表
    /// @param [in] group 组名
    /// @param [in] file 文件路径
    /// @return true表示保存成功
    static bool saveToFile(const std::vector<std::tuple<DM::ActionType,
                           QString, QStringList>>& data, const QString& group,
                           const QString& file);

    /// @brief 获取命令到ActionType的映射
    /// @return 命令映射表
    std::map<QString, DM::ActionType> getActionCommands();

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

    /// @brief 初始化字符串到ActionType的静态映射表
    static void initStrActionMap();

private:
    static Commands* m_pUniqueInstance;  ///< 单例实例指针

    ///< ActionType到命令列表的映射
    std::map<DM::ActionType, QStringList> m_actKeysMap;

    ///< 命令字符串到ActionType的映射
    std::map<QString, DM::ActionType> m_keyActMap;

    ///< 配置文件当前组数据：action、description、keys三元组
    std::vector<std::tuple<DM::ActionType, QString, QStringList>> m_data;

    ///< 字符串到ActionType的静态映射表
    static std::map<QString, DM::ActionType> m_strActMap;

    ///< ActionType到字符串的静态反向映射表
    static std::map<DM::ActionType, QString> m_actStrMap;

    ///< 静态映射表是否已初始化
    static bool g_strActMapInilized;

    ///< 配置文件全路径（exe目录下）
    QString m_strConfigFile;

    ///< 用户配置文件全路径（如 C:\\Users\\...\\AppData\\Local\\YiCAD\\keyconfig.xml）
    QString m_strUserConfig;

    ///< 当前命令对应的组，一个组对应一套命令
    QString m_curGroup;
};

#endif // COMMANDS_H
