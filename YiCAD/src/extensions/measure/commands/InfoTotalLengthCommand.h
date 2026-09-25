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

/// @file InfoTotalLengthCommand.h
/// @brief 总长度命令：在命令行报告选择集的总长度，取代原 ActionInfoTotalLength

#ifndef INFOTOTALLENGTHCOMMAND_H
#define INFOTOTALLENGTHCOMMAND_H

#include <QCoreApplication>

#include "SelectFirstCommand.h"

/// @brief 总长度命令：先选后建，选择集就绪后报告并结束
class InfoTotalLengthCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(InfoTotalLengthCommand)

protected:
    /// @brief 报告选择集的总长度后结束
    bool onSelectionReady() override;
};

#endif // INFOTOTALLENGTHCOMMAND_H
