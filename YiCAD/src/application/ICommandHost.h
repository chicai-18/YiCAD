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

/// @file ICommandHost.h
/// @brief 命令的宿主：命令与命令总线经它取用所在视图的能力
///
/// 对应 DS 里 IExclusiveCommand::Activate(UIView*) 与 ExclusiveCommandBus(UIView*) 用到的
/// UIView。application/ 不能包含 kernel/interaction/（tools/check_layering.py），由交互视图
/// UIView 实现本接口，命令与总线只认识接口。

#ifndef ICOMMANDHOST_H
#define ICOMMANDHOST_H

#include "ISnapService.h"

class DmDocument;
class ExclusiveCommandBus;
class HighlightSet;
class IDocumentView;
class SelectionSet;
class ViewToolControl;

/// @brief 命令的宿主
class ICommandHost
{
public:
    virtual ~ICommandHost() = default;

    /// @brief 视图的文档
    virtual DmDocument* document() = 0;
    /// @brief 文档的选择集，同一文档的各个视图共用
    virtual SelectionSet* selection() = 0;
    /// @brief 视图的高亮集：命令进行中的拾取反馈，命令结束时由视图清空
    virtual HighlightSet* highlight() = 0;
    /// @brief 视图
    virtual IDocumentView* view() = 0;
    /// @brief 视图的工具控制器：命令在其业务栈上激活自己的工具
    virtual ViewToolControl* viewToolControl() = 0;
    /// @brief 视图的命令总线
    virtual ExclusiveCommandBus* commandBus() = 0;

    /// @brief 选择层进入选择阶段：按先选后做的约束完成点选与框选
    /// @param entityTypes 可选的实体类型；为空表示不限
    virtual void beginSelectionPhase(const EntityTypeList& entityTypes) = 0;
    /// @brief 选择层退出选择阶段
    virtual void endSelectionPhase() = 0;
};

#endif // ICOMMANDHOST_H
