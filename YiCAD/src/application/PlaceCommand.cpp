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

/// @file PlaceCommand.cpp
/// @brief PlaceCommand 的实现

#include "PlaceCommand.h"

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "ViewToolControl.h"

PlaceCommand::PlaceCommand() = default;

PlaceCommand::~PlaceCommand() = default;

bool PlaceCommand::onActivate()
{
    m_preview = std::make_unique<CommandPreview>(selection(), view());
    m_tool = createTool();
    if (!m_tool)
    {
        m_preview.reset();
        return false;
    }
    m_tool->setPreview(m_preview.get());
    // 原 PreviewActionInterface::init()：启动时清除一次预览（视图共用的预览容器）
    m_preview->clear();
    showOptions();
    viewToolControl()->activate(m_tool.get());
    if (!onStarted())
    {
        // 启动失败的命令不会再经 onDeactivate() 结束
        viewToolControl()->deactivate(m_tool.get());
        m_tool->finishSession();
        hideOptions();
        m_preview->clear();
        return false;
    }
    return true;
}

void PlaceCommand::onDeactivate()
{
    // 与原 Action 结束时一样：清除预览、挂起并结束捕捉会话，收起选项条
    viewToolControl()->deactivate(m_tool.get());
    m_tool->finishSession();
    hideOptions();
    m_preview->clear();
}

ISnapService* PlaceCommand::snapService() const
{
    return m_tool ? m_tool->snapper() : nullptr;
}
