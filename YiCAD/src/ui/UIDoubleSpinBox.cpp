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

/// @file UIDoubleSpinBox.cpp
/// @brief 自定义双精度浮点数字输入控件，支持高精度小数输入

#include "UIDoubleSpinBox.h"

namespace
{
    constexpr double kDefaultStep = 0.0005;
    constexpr double kMaxValue = 1e10;
    constexpr double kMinValue = -1e10;
    constexpr int kDisplayPrecision = 15;
}

UIDoubleSpinBox::UIDoubleSpinBox(QWidget* parent /*= nullptr*/)
    : QDoubleSpinBox(parent)
{
    setDecimals(4);
    setSingleStep(kDefaultStep);
    setStepType(QAbstractSpinBox::DefaultStepType);
    setMaximum(kMaxValue);
    setMinimum(kMinValue);
}

QString UIDoubleSpinBox::textFromValue(double value) const
{
    //QString vStr = QLocale().toString(value, 'g', 10);
    QString vStr = QString::number(value, 'g', kDisplayPrecision);
    return vStr;
}
