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
/// @brief 命令注册表：以字符串 ID 为键的命令工厂表，替代
/// UIActionHandler.cpp 里原先的 153-case switch。
///
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4 第7.4节任务①②。内置命令
/// 在自己的 .cpp 里用文件作用域静态对象自注册，加命令等于加一个文件，不需要
/// 再回来改 UIActionHandler.cpp。这个自注册模式能可靠工作，依赖阶段0把 YiCadCore
/// 选成 OBJECT 库而非 STATIC 库的决定——OBJECT 库不会因为"没人引用"而把
/// 整个翻译单元的目标文件从链接里剔除，STATIC 库的归档器则会。
///
/// `DM::ActionType` 是过渡期的桥接键，供既有内置命令使用，不是新命令的
/// 必需项：扩展命令（`IExtensionContext::registerExclusiveCommand` 等，ID 形如
/// "ext.dim.linear"）只有字符串 ID，由 `UIActionHandler::activateCommand` 按 ID 启动。
///
/// 内置命令的命令行别名与说明来自 keyconfig.xml（`Commands`，以
/// `DM::ActionType` 为键）；纯字符串命令没有枚举值可挂，别名与说明随
/// `CommandInfo` 一起注册在这里。
///
/// 命令有三种注册类型（`CommandKind`，doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第二步第 2 项与第三步）：交互命令（工厂返回 `std::unique_ptr<IExclusiveCommand>`，
/// 由视图的命令总线运行）、即时命令（一个函数，不建命令对象、不占总线，没有打开
/// 图纸时也能执行）与临时视图工具（平移：不占总线，叠在业务栈顶，见
/// TransientViewTool.h）。原先的旧版 Action 类型在第四步随旧 Action 体系删除。
/// `UIActionHandler::activateCommand` 按注册类型分派。

#ifndef COMMANDREGISTRY_H
#define COMMANDREGISTRY_H

#include "Datamodel.h"
#include "DmVector.h"

#include <functional>
#include <map>
#include <memory>

#include <QString>
#include <QStringList>

class DmDocument;
class DmEntity;
class IDocumentView;
class IExclusiveCommand;
class QObject;
class QWidget;
class TransientViewTool;

/// @brief 构造命令所需的运行时环境。
///
/// 绝大多数命令只用 document/view；sender 供图层命令透传触发的按钮。即时命令在
/// 没有打开图纸时 document/view 为空。entity/point 是命令要作用的实体与位置：
/// 选择层双击实体启动它的编辑命令（registerEntityEditor）时为双击的实体与位置，
/// 修改实体属性时为被修改的实体。原先供 ActionSelect 回调的 handler 字段随先选后建
/// 命令的迁移删除（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步）。
struct CommandContext
{
    DmDocument* document = nullptr;
    IDocumentView* view = nullptr;
    QObject* sender = nullptr;
    DmEntity* entity = nullptr;
    DmVector point{false};
};

/// @brief 交互命令工厂：构造一个新的命令实例；返回空表示本次不启动
/// （如块编辑中再次编辑块时给出警告）。
using ExclusiveCommandFactory = std::function<std::unique_ptr<IExclusiveCommand>(const CommandContext&)>;

/// @brief 无参构造的交互命令的工厂
template <typename Command>
ExclusiveCommandFactory exclusiveCommandFactory()
{
    return [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand> { return std::make_unique<Command>(); };
}

/// @brief 即时命令：执行即完成，不建命令对象、不占命令总线。
using InstantCommand = std::function<void(const CommandContext&)>;

/// @brief 即时命令执行前如何处理正在运行的命令
enum class InstantInterrupt
{
    EndUninterruptible, ///< 结束不可打断的命令（多行文字编辑），默认：与原先压栈时一致
    KeepAll,            ///< 什么也不结束：原 isViewAction() 的 Action（缩放）不打断任何命令
    EndAll              ///< 先结束全部命令，被否决时不执行：原 isExclusive() 的 Action（新建、打开、保存图纸）
};

/// @brief 临时视图工具工厂：构造一个新的工具实例；返回空表示本次不启动。
using ViewToolFactory = std::function<std::unique_ptr<TransientViewTool>(const CommandContext&)>;

/// @brief 命令的注册类型
enum class CommandKind
{
    None,      ///< 未注册
    Exclusive, ///< 交互命令，由视图的命令总线运行
    Instant,   ///< 即时命令
    ViewTool   ///< 临时视图工具，不占命令总线，叠在业务栈顶
};

/// @brief 交互命令的选项条工厂。命令调用
/// `GUIDIALOGFACTORY->requestCommandOptions(this, true, update)` 时，宿主在选项条
/// 容器里放入本工厂构造的控件（宿主持有其所有权）。
/// @param parent 选项条容器
/// @param command 请求显示选项的命令
/// @param update 透传 requestCommandOptions 的同名参数
using ExclusiveCommandOptionsFactory =
    std::function<QWidget*(QWidget* parent, IExclusiveCommand* command, bool update)>;

/// @brief 命令的附加信息。
struct CommandInfo
{
    /// @brief 显示名，命令行提示里的 "[说明]" 前缀用。
    QString description;
    /// @brief 命令行别名，大小写不敏感；与 keyconfig.xml 里的内置别名重名时内置优先。
    QStringList aliases;
    /// @brief 交互命令的选项条；为空表示该命令没有选项条。
    ExclusiveCommandOptionsFactory commandOptionsFactory;
    /// @brief 即时命令执行前如何处理正在运行的命令；只对即时命令有效。
    InstantInterrupt instantInterrupt = InstantInterrupt::EndUninterruptible;
};

/// @brief 命令注册表。字符串 ID 为主键，`DM::ActionType` 只是过渡期桥接。
/// @note 仅限 UI 主线程访问，无内部同步（与 PluginRegistry 的既有约定一致）。
class CommandRegistry
{
public:
    static CommandRegistry& instance();

    /// @brief 注册一个交互命令。
    /// @param id 稳定的点号命名空间字符串 ID（如 "draw.line"），不能为空。
    /// @param factory 命令工厂，不能为空。
    /// @param info 说明、别名与选项条；别名与已注册命令的别名冲突时整体拒绝。
    /// @return 成功返回 true；id 已存在、别名冲突或参数非法返回 false，
    /// 失败时不留下任何部分注册的状态。
    bool registerExclusiveCommand(const QString& id, ExclusiveCommandFactory factory, CommandInfo info = {});

    /// @brief 注册一个内置交互命令，同时建立 legacy ActionType 桥接。
    /// @return 成功返回 true；id 或 legacyType 已存在时返回 false，不留下部分注册的状态。
    bool registerExclusiveCommand(DM::ActionType legacyType, const QString& id, ExclusiveCommandFactory factory);

    /// @brief 注册一个即时命令，其余同 registerExclusiveCommand()。
    bool registerInstantCommand(const QString& id, InstantCommand command, CommandInfo info = {});

    /// @brief 注册一个内置即时命令，同时建立 legacy ActionType 桥接。
    /// @return 成功返回 true；id 或 legacyType 已存在时返回 false，不留下部分注册的状态。
    bool registerInstantCommand(DM::ActionType legacyType, const QString& id, InstantCommand command,
                                CommandInfo info = {});

    /// @brief 注册一个临时视图工具，其余同 registerExclusiveCommand()。
    bool registerViewTool(const QString& id, ViewToolFactory factory, CommandInfo info = {});

    /// @brief 注册一个内置临时视图工具，同时建立 legacy ActionType 桥接。
    /// @return 成功返回 true；id 或 legacyType 已存在时返回 false，不留下部分注册的状态。
    bool registerViewTool(DM::ActionType legacyType, const QString& id, ViewToolFactory factory);

    /// @brief 为已注册的命令建立 legacy ActionType 桥接（keyconfig.xml 仍以枚举为键）。
    /// @return id 未注册或 legacyType 已桥接时返回 false。
    bool bindLegacyType(DM::ActionType legacyType, const QString& id);

    /// @brief 注销一个命令，连同它的别名与 legacy 桥接。
    /// @return id 未注册时返回 false。
    bool unregisterCommand(const QString& id);

    /// @brief 登记某类实体的双击编辑命令（如多行文字的就地编辑）：选择层双击这类实体时
    ///        按 ID 启动它，上下文的 entity/point 为双击的实体与位置。命令注销时登记随之删除
    /// @return 这类实体已有编辑命令，或 commandId 不是已注册的交互命令时返回 false
    bool registerEntityEditor(DM::EntityType type, const QString& commandId);
    /// @brief 这类实体的双击编辑命令；没有时返回空
    QString entityEditor(DM::EntityType type) const;

    /// @brief id 是否已注册。
    bool hasCommand(const QString& id) const;

    /// @brief 命令的注册类型；未注册返回 CommandKind::None。
    CommandKind kind(const QString& id) const;

    /// @brief legacyType 是否已经迁移到本注册表。
    bool hasLegacyMapping(DM::ActionType legacyType) const;

    /// @brief legacyType 桥接到的字符串 ID；未桥接返回空串。
    QString commandId(DM::ActionType legacyType) const;

    /// @brief 桥接到 id 的 legacy ActionType；未桥接返回 DM::ActionNone。
    DM::ActionType legacyType(const QString& id) const;

    /// @brief 按命令行别名查命令 ID（大小写不敏感）；未找到返回空串。
    QString commandForAlias(const QString& alias) const;

    /// @brief 全部已注册的别名（小写），供命令行自动补全。
    QStringList aliases() const;

    /// @brief 命令的说明；未注册或未提供时返回空串。
    QString description(const QString& id) const;

    /// @brief 交互命令的选项条工厂；未注册或未提供时返回空函数。
    ExclusiveCommandOptionsFactory commandOptionsFactory(const QString& id) const;

    /// @brief 即时命令执行前如何处理正在运行的命令；未注册时返回默认值。
    InstantInterrupt instantInterrupt(const QString& id) const;

    /// @brief 按字符串 ID 构造交互命令，并把 id 记到命令上；未注册、不是交互
    /// 命令或工厂返回空时返回空。
    std::unique_ptr<IExclusiveCommand> createCommand(const QString& id, const CommandContext& ctx) const;

    /// @brief 按字符串 ID 构造临时视图工具，并把 id 记到工具上；未注册、不是
    /// 临时视图工具或工厂返回空时返回空。
    std::unique_ptr<TransientViewTool> createViewTool(const QString& id, const CommandContext& ctx) const;

    /// @brief 执行即时命令。
    /// @return 未注册或不是即时命令时返回 false，什么也不做。
    bool runInstant(const QString& id, const CommandContext& ctx) const;

private:
    CommandRegistry() = default;

    struct Entry
    {
        CommandKind kind = CommandKind::None;
        ExclusiveCommandFactory commandFactory;   ///< kind 为 Exclusive 时有效
        InstantCommand instant;                   ///< kind 为 Instant 时有效
        ViewToolFactory viewToolFactory;          ///< kind 为 ViewTool 时有效
        CommandInfo info;
    };

    /// @brief 各种注册共用：校验 id 与别名，登记条目
    bool addEntry(const QString& id, Entry entry);
    /// @brief 带 legacy 桥接的注册共用：legacyType 未桥接时才注册，注册成功后建立桥接
    bool addBridged(DM::ActionType legacyType, const QString& id, const std::function<bool()>& registerEntry);

    std::map<QString, Entry> m_commands;
    std::map<DM::ActionType, QString> m_legacyBridge;
    /// @brief 小写别名 -> 命令 ID
    std::map<QString, QString> m_aliases;
    /// @brief 实体类型 -> 双击编辑命令 ID
    std::map<DM::EntityType, QString> m_entityEditors;
};

#endif  // COMMANDREGISTRY_H
