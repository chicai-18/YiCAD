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

/// @file GuiCoordinateInput.cpp
/// @brief GuiCoordinateInput 的实现，逐分支从 GuiEventHandler::commandEvent 移来

#include "GuiCoordinateInput.h"

#include "Math2d.h"

namespace
{
/// @brief 按两个分量构造结果；任一分量求值失败即为语法错误
GuiCoordinateInput makeResult(bool ok1, bool ok2, const DmVector& position)
{
    GuiCoordinateInput result;
    if (ok1 && ok2)
    {
        result.status = GuiCoordinateInput::Status::Ok;
        result.position = position;
    }
    else
    {
        result.status = GuiCoordinateInput::Status::SyntaxError;
    }
    return result;
}
}  // namespace

GuiCoordinateInput GuiCoordinateInput::parse(const QString& cmd, const DmVector& relativeZero)
{
    bool ok1 = false;
    bool ok2 = false;

    // 直角坐标（绝对 "x,y"、相对 "@x,y"）
    if (cmd.contains(','))
    {
        const int commaPos = cmd.indexOf(',');
        if (cmd.at(0) != '@')
        {
            const double x = Math2d::eval(cmd.left(commaPos), &ok1);
            const double y = Math2d::eval(cmd.mid(commaPos + 1), &ok2);
            return makeResult(ok1, ok2, DmVector(x, y));
        }
        const double x = Math2d::eval(cmd.mid(1, commaPos - 1), &ok1);
        const double y = Math2d::eval(cmd.mid(commaPos + 1), &ok2);
        return makeResult(ok1, ok2, DmVector(x, y) + relativeZero);
    }

    // 极坐标（绝对 "r<a"、相对 "@r<a"），角度单位为度
    if (cmd.contains('<'))
    {
        const int anglePos = cmd.indexOf('<');
        if (cmd.at(0) != '@')
        {
            const double r = Math2d::eval(cmd.left(anglePos), &ok1);
            const double a = Math2d::eval(cmd.mid(anglePos + 1), &ok2);
            return makeResult(ok1, ok2, DmVector::polar(r, Math2d::deg2rad(a)));
        }
        const double r = Math2d::eval(cmd.mid(1, anglePos - 1), &ok1);
        const double a = Math2d::eval(cmd.mid(anglePos + 1), &ok2);
        return makeResult(ok1, ok2, DmVector::polar(r, Math2d::deg2rad(a)) + relativeZero);
    }

    return GuiCoordinateInput{};
}
