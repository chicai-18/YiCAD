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

/// @file ModifyScaleCommand.h
/// @brief 缩放命令，取代原 ActionModifyScale：指定基点，再指定或输入比例

#ifndef MODIFYSCALECOMMAND_H
#define MODIFYSCALECOMMAND_H

#include <memory>

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class CommandPreview;
class DmVector;

/// @brief 缩放命令；交互由 ModifyScaleTool 驱动
class ModifyScaleCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyScaleCommand)

public:
    ModifyScaleCommand();
    ~ModifyScaleCommand() override;

    /// @brief 由鼠标到基点的距离换算比例：选择集包围框宽高的较大值对应比例 2
    double factorAt(const DmVector& reference, const DmVector& mouse) const;
    /// @brief 预览选择集以基点按比例缩放
    void previewScale(const DmVector& reference, double factor);
    /// @brief 清除预览
    void clearPreview();
    /// @brief 把选择集以基点按比例缩放，然后结束命令
    void commitScale(const DmVector& reference, double factor);

protected:
    /// @brief 计算选择集包围框，激活缩放工具
    bool onSelectionReady() override;

private:
    std::unique_ptr<CommandPreview> m_preview;
    double m_boxRange = 0.0; ///< 选择集包围框宽高的较大值
};

#endif // MODIFYSCALECOMMAND_H
