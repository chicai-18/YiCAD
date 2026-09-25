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

/// @file ModifyReverseCommand.h
/// @brief 反向命令，取代原 ActionModifyReverse：反转选中直线、圆弧、椭圆、样条的方向

#ifndef MODIFYREVERSECOMMAND_H
#define MODIFYREVERSECOMMAND_H

#include <QCoreApplication>

#include "SelectFirstCommand.h"

/// @brief 反向命令：选择集就绪后立即执行并结束
class ModifyReverseCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyReverseCommand)

protected:
    /// @brief 反转选择集中实体的方向，在命令行报告数量后结束
    bool onSelectionReady() override;
};

#endif // MODIFYREVERSECOMMAND_H
