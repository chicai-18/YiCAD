/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file ModifyMoveCommand.h
/// @brief 移动命令，取代原 ActionModifyMove：指定参考点与目标点，移动选择集

#ifndef MODIFYMOVECOMMAND_H
#define MODIFYMOVECOMMAND_H

#include <memory>

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class CommandPreview;
class DmVector;

/// @brief 移动命令；交互由 ModifyMoveTool 驱动
class ModifyMoveCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyMoveCommand)

public:
    ModifyMoveCommand();
    ~ModifyMoveCommand() override;

    /// @brief 预览选择集从参考点移到目标点
    /// @param reference 参考点
    /// @param target 目标点
    /// @param showGuide 是否画出参考点到目标点的引导线（按住 Shift 时）
    void previewMove(const DmVector& reference, const DmVector& target, bool showGuide);
    /// @brief 清除预览
    void clearPreview();
    /// @brief 把选择集从参考点移到目标点，然后结束命令
    void commitMove(const DmVector& reference, const DmVector& target);

protected:
    /// @brief 激活移动工具
    bool onSelectionReady() override;

private:
    std::unique_ptr<CommandPreview> m_preview;
};

#endif // MODIFYMOVECOMMAND_H
