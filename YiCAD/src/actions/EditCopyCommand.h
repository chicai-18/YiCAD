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

/// @file EditCopyCommand.h
/// @brief 复制到剪贴板、剪切命令，取代原 ActionEditCopy：指定参考点后复制或剪切选择集

#ifndef EDITCOPYCOMMAND_H
#define EDITCOPYCOMMAND_H

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class DmVector;

/// @brief 复制到剪贴板（edit.copy）与剪切（edit.cut）命令；交互由 EditCopyTool 驱动
class EditCopyCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(EditCopyCommand)

public:
    /// @param copy 为 true 时复制，为 false 时剪切
    explicit EditCopyCommand(bool copy);

    /// @brief 以参考点把选择集复制或剪切到剪贴板，然后结束命令
    void commitCopy(const DmVector& referencePoint);

protected:
    /// @brief 激活指定参考点的工具
    bool onSelectionReady() override;

private:
    bool m_copy; ///< true 为复制，false 为剪切
};

#endif // EDITCOPYCOMMAND_H
