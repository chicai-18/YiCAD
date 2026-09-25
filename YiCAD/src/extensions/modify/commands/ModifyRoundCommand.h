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

/// @file ModifyRoundCommand.h
/// @brief 圆角命令 ext.modify.round，取代原 ActionModifyRound：选两个实体，按半径在两者之间
///        加圆角，可选同时修剪原实体

#ifndef MODIFYROUNDCOMMAND_H
#define MODIFYROUNDCOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

/// @brief 圆角命令：持有圆角选项（半径、是否修剪）；交互由 ModifyRoundTool 驱动
class ModifyRoundCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyRoundCommand)

public:
    // ---- 选项条（UIRoundOptions）与命令行 ----

    /// @brief 圆角半径
    double radius() const { return m_radius; }
    void setRadius(double r) { m_radius = r; }
    /// @brief 是否同时修剪原实体
    bool isTrimOn() const { return m_trim; }
    void setTrim(bool trim) { m_trim = trim; }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    double m_radius = 1.0; ///< 半径
    bool m_trim = true;    ///< 是否修剪
};

#endif // MODIFYROUNDCOMMAND_H
