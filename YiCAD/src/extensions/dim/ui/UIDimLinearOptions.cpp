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

/// @file UIDimLinearOptions.cpp
/// @brief 线性标注选项控件实现

#include "UIDimLinearOptions.h"

#include "DmSettings.h"
#include "Math2d.h"
#include "Debug.h"

#include "ui_UIDimLinearOptions.h"
#include "DimCommands.h"

/// @brief 构造 UIDimLinearOptions
/// @param [in] parent 父窗口指针
/// @param [in] fl 窗口标志
UIDimLinearOptions::UIDimLinearOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui::Ui_DimLinearOptions{})
{
    ui->setupUi(this);
}

/// @brief 析构函数，保存设置
UIDimLinearOptions::~UIDimLinearOptions()
{
    saveSettings();
}

/// @brief 语言切换时刷新界面文本
void UIDimLinearOptions::languageChange()
{
    ui->retranslateUi(this);
}

void UIDimLinearOptions::saveSettings()
{
    DMSETTINGS->beginGroup("/Dimension");
    DMSETTINGS->writeEntry("/Angle", ui->leAngle->text());
    DMSETTINGS->endGroup();
}

void UIDimLinearOptions::setCommand(IExclusiveCommand* c, bool update)
{
    command = dynamic_cast<DimLinearCommand*>(c);
    if (command)
    {
        QString sa;
        if (!update)
        {
            sa = QString("%1").arg(Math2d::rad2deg(command->angle()));
        }
        else
        {
            DMSETTINGS->beginGroup("/Dimension");
            sa = DMSETTINGS->readEntry("/Angle", "0.0");
            DMSETTINGS->endGroup();
        }
        ui->leAngle->setText(sa);
    }
}

void UIDimLinearOptions::updateAngle(const QString& a)
{
    if (command)
    {
        command->setAngle(Math2d::deg2rad(Math2d::eval(a)));
    }
}
