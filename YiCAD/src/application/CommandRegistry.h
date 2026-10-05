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
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4 第7.4节任务①②。命令都由扩展在
/// IExtension::OnRegister 里经 IExtensionContext 注册（业务工具化第四步把原先在
/// src/actions/ 里自注册的内置命令拆进了 ext.draw/ext.modify/ext.measure/ext.edit/
/// ext.view，doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节），扩展卸载时注销。
///
/// 命令只以字符串 ID 标识（形如 "ext.draw.line"，扩展 ID 加一段），由
/// `UIActionHandler::activateCommand` 按 ID 启动。原先供 keyconfig.xml 使用的
/// `DM::ActionType` 桥接在业务工具化第四步删除。
///
/// 命令行别名有两个来源：keyconfig.xml（`Commands`，以命令 ID 为键，分组可选，
/// "命令设置"对话框可改）与随 `CommandInfo` 注册在这里的别名；两者重名时
/// keyconfig.xml 优先。
///
/// 命令有两种注册类型（`CommandKind`，doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第二步第 2 项与第三步）：交互命令（工厂返回 `std::unique_ptr<IExclusiveCommand>`，
/// 由视图的命令总线运行）与即时命令（一个函数，不建命令对象、不占总线，没有打开
/// 图纸时也能执行）。原先的旧版 Action 类型在第四步随旧 Action 体系删除；叠在命令
/// 之上的临时视图工具（平移模式）随后也删除，平移只剩中键的导航手势。
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
class SelectionSet;
class QObject;
class QWidget;

/// @brief 构造命令所需的运行时环境。
///
/// 绝大多数命令只用 document/view，作用于选择集的即时命令用 selection（文档的选择集）；
/// sender 供图层命令透传触发的按钮。即时命令在没有打开图纸时 document/view/selection
/// 为空。entity/point 是命令要作用的实体与位置：
/// 选择层双击实体启动它的编辑命令（registerEntityEditor）时为双击的实体与位置，
/// 修改实体属性时为被修改的实体。原先供 ActionSelect 回调的 handler 字段随先选后建
/// 命令的迁移删除（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步）。
struct CommandContext
{
    DmDocument* document = nullptr;
    IDocumentView* view = nullptr;
    SelectionSet* selection = nullptr;
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

/// @brief 命令的注册类型
enum class CommandKind
{
    None,      ///< 未注册
    Exclusive, ///< 交互命令，由视图的命令总线运行
    Instant    ///< 即时命令
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
    /// @brief 命令行别名，大小写不敏感；与 keyconfig.xml 里的别名重名时 keyconfig.xml 优先。
    QStringList aliases;
    /// @brief 交互命令的选项条；为空表示该命令没有选项条。
    ExclusiveCommandOptionsFactory commandOptionsFactory;
    /// @brief 选项条容器的高度（像素）。几乎都是 23，样条的是 26（原先各选项条在对话框
    ///        工厂里写死，迁移时照原值登记）
    int commandOptionsHeight = 23;
    /// @brief 即时命令执行前如何处理正在运行的命令；只对即时命令有效。
    InstantInterrupt instantInterrupt = InstantInterrupt::EndUninterruptible;
};

/// @brief 命令注册表，以字符串 ID 为键。
/// @note 仅限 UI 主线程访问，无内部同步（与 PluginRegistry 的既有约定一致）。
class CommandRegistry
{
public:
    static CommandRegistry& instance();

    /// @brief 注册一个交互命令。
    /// @param id 稳定的点号命名空间字符串 ID（如 "ext.draw.line"），不能为空。
    /// @param factory 命令工厂，不能为空。
    /// @param info 说明、别名与选项条；别名与已注册命令的别名冲突时整体拒绝。
    /// @return 成功返回 true；id 已存在、别名冲突或参数非法返回 false，
    /// 失败时不留下任何部分注册的状态。
    bool registerExclusiveCommand(const QString& id, ExclusiveCommandFactory factory, CommandInfo info = {});

    /// @brief 注册一个即时命令，其余同 registerExclusiveCommand()。
    bool registerInstantCommand(const QString& id, InstantCommand command, CommandInfo info = {});

    /// @brief 注销一个命令，连同它的别名与双击编辑、属性编辑登记。
    /// @return id 未注册时返回 false。
    bool unregisterCommand(const QString& id);

    /// @brief 登记某类实体的双击编辑命令（如多行文字的就地编辑）：选择层双击这类实体时
    ///        按 ID 启动它，上下文的 entity/point 为双击的实体与位置。命令注销时登记随之删除
    /// @return 这类实体已有编辑命令，或 commandId 不是已注册的交互命令时返回 false
    bool registerEntityEditor(DM::EntityType type, const QString& commandId);
    /// @brief 这类实体的双击编辑命令；没有时返回空
    QString entityEditor(DM::EntityType type) const;

    /// @brief 登记某类实体的属性编辑命令："修改实体属性"命令选中这类实体时运行它，选择层
    ///        双击这类实体而没有双击编辑命令时也运行它；上下文的 entity 为要编辑的实体。
    ///        属性对话框登记为即时命令（不打断"修改实体属性"，可以接着点下一个实体），
    ///        非模态的属性面板（多行文字）登记为交互命令，由它接替。命令注销时登记随之删除
    /// @return 这类实体已有属性编辑命令，或 commandId 不是已注册的即时命令或交互命令时返回 false
    bool registerPropertyEditor(DM::EntityType type, const QString& commandId);
    /// @brief 这类实体的属性编辑命令；没有时返回空
    QString propertyEditor(DM::EntityType type) const;

    /// @brief 按类名登记某个自定义实体类（DM::EntityCustom，见 DmCustomEntity::className）的双击编辑命令，其余同按类型的重载
    bool registerEntityEditor(const QString& className, const QString& commandId);
    /// @brief 按类名登记某个自定义实体类的属性编辑命令，其余同按类型的重载
    bool registerPropertyEditor(const QString& className, const QString& commandId);

    /// @brief 实体的双击编辑命令：自定义实体按类名找，其余按类型；没有时返回空
    QString entityEditor(const DmEntity& entity) const;
    /// @brief 实体的属性编辑命令：自定义实体按类名找，其余按类型；没有时返回空
    QString propertyEditor(const DmEntity& entity) const;

    /// @brief id 是否已注册。
    bool hasCommand(const QString& id) const;

    /// @brief 命令的注册类型；未注册返回 CommandKind::None。
    CommandKind kind(const QString& id) const;

    /// @brief 按命令行别名查命令 ID（大小写不敏感）；未找到返回空串。
    QString commandForAlias(const QString& alias) const;

    /// @brief 全部已注册的别名（小写），供命令行自动补全。
    QStringList aliases() const;

    /// @brief 命令的说明；未注册或未提供时返回空串。
    QString description(const QString& id) const;

    /// @brief 交互命令的选项条工厂；未注册或未提供时返回空函数。
    ExclusiveCommandOptionsFactory commandOptionsFactory(const QString& id) const;

    /// @brief 选项条容器的高度；未注册时返回默认值 23。
    int commandOptionsHeight(const QString& id) const;

    /// @brief 即时命令执行前如何处理正在运行的命令；未注册时返回默认值。
    InstantInterrupt instantInterrupt(const QString& id) const;

    /// @brief 按字符串 ID 构造交互命令，并把 id 记到命令上；未注册、不是交互
    /// 命令或工厂返回空时返回空。
    std::unique_ptr<IExclusiveCommand> createCommand(const QString& id, const CommandContext& ctx) const;

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
        CommandInfo info;
    };

    /// @brief 各种注册共用：校验 id 与别名，登记条目
    bool addEntry(const QString& id, Entry entry);

    std::map<QString, Entry> m_commands;
    /// @brief 小写别名 -> 命令 ID
    std::map<QString, QString> m_aliases;
    /// @brief 实体类型 -> 双击编辑命令 ID
    std::map<DM::EntityType, QString> m_entityEditors;
    /// @brief 实体类型 -> 属性编辑命令 ID
    std::map<DM::EntityType, QString> m_propertyEditors;
    /// @brief 自定义实体的类名 -> 双击编辑命令 ID
    std::map<QString, QString> m_customEntityEditors;
    /// @brief 自定义实体的类名 -> 属性编辑命令 ID
    std::map<QString, QString> m_customPropertyEditors;
};

#endif  // COMMANDREGISTRY_H
