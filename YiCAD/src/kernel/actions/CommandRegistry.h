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

/// @file CommandRegistry.h
/// @brief 命令注册表：以字符串 ID 为键的 Action 工厂表，替代
/// UIActionHandler.cpp 里原先的 153-case switch。
///
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4 第7.4节任务①②。每个 Action
/// 在自己的 .cpp 里用文件作用域静态对象自注册（构造函数里调用
/// registerLegacyCommand），加命令等于加一个文件，不需要再回来改
/// UIActionHandler.cpp。这个自注册模式能可靠工作，依赖阶段0把 YiCadCore
/// 选成 OBJECT 库而非 STATIC 库的决定——OBJECT 库不会因为"没人引用"而把
/// 整个翻译单元的目标文件从链接里剔除，STATIC 库的归档器则会。
///
/// `DM::ActionType` 是过渡期的桥接键，供既有内置命令使用，不是新命令的
/// 必需项：`registerCommand(QString, ...)` 注册纯字符串 ID 的命令，扩展
/// 命令（`IExtensionContext::registerCommand`，ID 形如 "ext.dim.linear"）
/// 走的就是这条路径，由 `UIActionHandler::activateCommand` 按 ID 启动。
///
/// 内置命令的命令行别名与说明来自 keyconfig.xml（`Commands`，以
/// `DM::ActionType` 为键）；纯字符串命令没有枚举值可挂，别名与说明随
/// `CommandInfo` 一起注册在这里。

#ifndef COMMANDREGISTRY_H
#define COMMANDREGISTRY_H

#include "Datamodel.h"

#include <functional>
#include <map>

#include <QString>
#include <QStringList>

class ActionInterface;
class DmDocument;
class IDocumentView;
class QObject;
class QWidget;
class UIActionHandler;

/// @brief 构造 Action 所需的运行时环境。
///
/// 绝大多数命令只用 document/view；handler 仅供"先选后建"类命令使用
/// （ActionSelect 的构造函数需要一个 UIActionHandler*，用于选择完成后
/// 回调 setCurrentAction 触发后续动作）；sender 供 Layers* 系列命令透传
/// Ribbon 触发源（原 switch 里直接用 UIActionHandler::sender()）。
struct CommandContext
{
    DmDocument* document = nullptr;
    IDocumentView* view = nullptr;
    UIActionHandler* handler = nullptr;
    QObject* sender = nullptr;
};

/// @brief 命令工厂：给定运行时环境，构造并返回一个新的 Action 实例
/// （调用方持有返回的所有权）；返回 nullptr 表示本次不构造任何 Action
/// （对应原 switch 里"该 case 只做副作用、不建 Action"的分支）。
using CommandFactory = std::function<ActionInterface*(const CommandContext&)>;

/// @brief 命令选项条工厂。Action 调用 `GUIDIALOGFACTORY->requestOptions(this, true, update)`
/// 时，宿主在选项条容器里放入本工厂构造的控件（宿主持有其所有权）。
/// @param parent 选项条容器
/// @param action 请求显示选项的 Action
/// @param update 透传 requestOptions 的同名参数
using CommandOptionsFactory =
    std::function<QWidget*(QWidget* parent, ActionInterface* action, bool update)>;

/// @brief 纯字符串命令的附加信息。
struct CommandInfo
{
    /// @brief 显示名，命令行提示里的 "[说明]" 前缀用。
    QString description;
    /// @brief 命令行别名，大小写不敏感；与 keyconfig.xml 里的内置别名重名时内置优先。
    QStringList aliases;
    /// @brief 选项条；为空表示该命令没有选项条。
    CommandOptionsFactory optionsFactory;
};

/// @brief 命令注册表。字符串 ID 为主键，`DM::ActionType` 只是过渡期桥接。
/// @note 仅限 UI 主线程访问，无内部同步（与 PluginRegistry 的既有约定一致）。
class CommandRegistry
{
public:
    static CommandRegistry& instance();

    /// @brief 注册一个命令。
    /// @param id 稳定的点号命名空间字符串 ID（如 "draw.line"），不能为空。
    /// @param factory 命令工厂，不能为空。
    /// @param info 说明、别名与选项条；别名与已注册命令的别名冲突时整体拒绝。
    /// @return 成功返回 true；id 已存在、别名冲突或参数非法返回 false，
    /// 失败时不留下任何部分注册的状态。
    bool registerCommand(const QString& id, CommandFactory factory, CommandInfo info = {});

    /// @brief 注册一个内置命令，同时建立 legacy ActionType 到字符串 ID 的桥接。
    /// @return 成功返回 true；id 或 legacyType 已存在时返回 false。
    bool registerLegacyCommand(DM::ActionType legacyType, const QString& id,
                                CommandFactory factory);

    /// @brief 注销一个命令，连同它的别名与 legacy 桥接。
    /// @return id 未注册时返回 false。
    bool unregisterCommand(const QString& id);

    /// @brief id 是否已注册。
    bool hasCommand(const QString& id) const;

    /// @brief legacyType 是否已经迁移到本注册表。
    bool hasLegacyMapping(DM::ActionType legacyType) const;

    /// @brief legacyType 桥接到的字符串 ID；未桥接返回空串。
    QString commandId(DM::ActionType legacyType) const;

    /// @brief 按命令行别名查命令 ID（大小写不敏感）；未找到返回空串。
    QString commandForAlias(const QString& alias) const;

    /// @brief 全部已注册的别名（小写），供命令行自动补全。
    QStringList aliases() const;

    /// @brief 命令的说明；未注册或未提供时返回空串。
    QString description(const QString& id) const;

    /// @brief 命令的选项条工厂；未注册或未提供时返回空函数。
    CommandOptionsFactory optionsFactory(const QString& id) const;

    /// @brief 按 legacy ActionType 构造 Action；未注册返回 nullptr。
    ActionInterface* create(DM::ActionType legacyType, const CommandContext& ctx) const;

    /// @brief 按字符串 ID 构造 Action，并把 id 记到 Action 上
    /// （ActionInterface::getCommandId）；未注册或工厂返回 nullptr 时返回 nullptr。
    ActionInterface* create(const QString& id, const CommandContext& ctx) const;

private:
    CommandRegistry() = default;

    struct Entry
    {
        CommandFactory factory;
        CommandInfo info;
    };

    std::map<QString, Entry> m_commands;
    std::map<DM::ActionType, QString> m_legacyBridge;
    /// @brief 小写别名 -> 命令 ID
    std::map<QString, QString> m_aliases;
};

/// @brief 复刻原 switch 里"未选中先建 ActionSelect 收集选择，选中后建真正
/// Action"这一同形态 case 的共享工厂。
/// @param noSelectLegacyType 未选中时，交给 ActionSelect 的"选择完成后"动作类型。
/// @param buildReal 已有选中集时，构造真正 Action 的工厂。
CommandFactory makeSelectFirstFactory(DM::ActionType noSelectLegacyType,
                                       CommandFactory buildReal);

#endif  // COMMANDREGISTRY_H
