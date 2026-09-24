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

/// @file TransientViewTool.h
/// @brief 临时视图工具的基类：不占命令总线，叠在业务栈顶
///
/// doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 5 节"命令并存"的例外①：视图工具（平移）
/// 作为临时工具叠在业务栈顶，结束后其下的命令照常继续。对应原 Action 体系里
/// isViewAction() 的 Action：它压在旧 Action 栈顶，挂起前一个、结束后恢复前一个。
///
/// 由视图（UIView）持有：启动时挂起其下的各层（旧 Action、命令或编辑模式、选择层），
/// 进入/离开画布只通知它，结束时恢复其下各层；启动命令、启动旧版 Action、结束全部
/// 命令与视图关闭时结束它。经 CommandRegistry::registerViewTool() 注册。

#ifndef TRANSIENTVIEWTOOL_H
#define TRANSIENTVIEWTOOL_H

#include <functional>
#include <utility>

#include <QString>

#include "IViewTool.h"

/// @brief 临时视图工具的基类
class TransientViewTool : public IViewTool
{
public:
    /// @brief 命令 ID（CommandRegistry 里的字符串 ID），命令行提示的 "[说明]" 前缀用
    const QString& commandId() const { return m_commandId; }
    /// @brief 记录命令 ID，只由 CommandRegistry::createViewTool 调用
    void setCommandId(const QString& commandId) { m_commandId = commandId; }

    /// @brief 视图启动工具时登记：工具请求结束自己时调用它
    void setFinishHandler(std::function<void()> handler) { m_finishHandler = std::move(handler); }

protected:
    /// @brief 请求结束自己
    /// @details 视图在这次分发返回后才结束并销毁工具，调用之后仍可安全访问成员
    void finish()
    {
        if (m_finishHandler)
        {
            m_finishHandler();
        }
    }

private:
    QString m_commandId;
    std::function<void()> m_finishHandler;
};

#endif // TRANSIENTVIEWTOOL_H
