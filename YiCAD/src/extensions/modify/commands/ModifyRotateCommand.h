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

/// @file ModifyRotateCommand.h
/// @brief 旋转命令，取代原 ActionModifyRotate：指定旋转中心，再指定或输入角度

#ifndef MODIFYROTATECOMMAND_H
#define MODIFYROTATECOMMAND_H

#include <memory>

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class CommandPreview;
class DmVector;

/// @brief 旋转命令；交互由 ModifyRotateTool 驱动
class ModifyRotateCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyRotateCommand)

public:
    ModifyRotateCommand();
    ~ModifyRotateCommand() override;

    /// @brief 预览选择集绕中心旋转到给定角度
    /// @param center 旋转中心
    /// @param angle 角度（弧度）
    void previewRotate(const DmVector& center, double angle);
    /// @brief 清除预览
    void clearPreview();
    /// @brief 把选择集绕中心旋转给定角度，然后结束命令
    /// @param center 旋转中心
    /// @param angle 角度（弧度）
    void commitRotate(const DmVector& center, double angle);

protected:
    /// @brief 激活旋转工具
    bool onSelectionReady() override;

private:
    std::unique_ptr<CommandPreview> m_preview;
};

#endif // MODIFYROTATECOMMAND_H
