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

/// @file ModifyBevelCommand.h
/// @brief 倒角命令 ext.modify.bevel，取代原 ActionModifyBevel：选两个实体，按两段长度在交点处
///        倒角，可选同时修剪原实体

#ifndef MODIFYBEVELCOMMAND_H
#define MODIFYBEVELCOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

/// @brief 倒角命令：持有倒角选项（两段长度、是否修剪）；交互由 ModifyBevelTool 驱动
class ModifyBevelCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyBevelCommand)

public:
    // ---- 选项条（UIBevelOptions）与命令行 ----

    /// @brief 第一个实体上的倒角长度
    double length1() const { return m_length1; }
    void setLength1(double l1) { m_length1 = l1; }
    /// @brief 第二个实体上的倒角长度
    double length2() const { return m_length2; }
    void setLength2(double l2) { m_length2 = l2; }
    /// @brief 是否同时修剪原实体
    bool isTrimOn() const { return m_trim; }
    void setTrim(bool trim) { m_trim = trim; }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    double m_length1 = 1.0; ///< 长度 1
    double m_length2 = 1.0; ///< 长度 2
    bool m_trim = true;     ///< 是否修剪
};

#endif // MODIFYBEVELCOMMAND_H
