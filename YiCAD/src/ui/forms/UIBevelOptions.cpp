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

/// @file UIBevelOptions.cpp
/// @brief 倒角选项控件实现

#include "UIBevelOptions.h"

#include "ActionModifyBevel.h"

#include "ui_UIBevelOptions.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "Debug.h"

/// @brief 构造 UIBevelOptions
/// @param [in] parent 父窗口指针
/// @param [in] fl 窗口标志
UIBevelOptions::UIBevelOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui::Ui_BevelOptions{})
{
    ui->setupUi(this);
}

/// @brief 析构函数，保存设置
UIBevelOptions::~UIBevelOptions()
{
    saveSettings();
}

/// @brief 语言切换时刷新界面文本
void UIBevelOptions::languageChange()
{
    ui->retranslateUi(this);
}

void UIBevelOptions::saveSettings()
{
    DMSETTINGS->beginGroup("/Modify");
    DMSETTINGS->writeEntry("/BevelLength1", ui->leLength1->text());
    DMSETTINGS->writeEntry("/BevelLength2", ui->leLength2->text());
    DMSETTINGS->writeEntry("/BevelTrim", static_cast<int>(ui->cbTrim->isChecked()));
    DMSETTINGS->endGroup();
}

void UIBevelOptions::setAction(ActionInterface* a, bool update)
{
    if (a && a->getEntityType() == DM::ActionModifyBevel)
    {
        action = static_cast<ActionModifyBevel*>(a);

        QString sd1;
        QString sd2;
        QString st;
        if (update)
        {
            sd1 = QString("%1").arg(action->getLength1());
            sd2 = QString("%1").arg(action->getLength2());
            st = QString("%1").arg(static_cast<int>(action->isTrimOn()));
        }
        else
        {
            DMSETTINGS->beginGroup("/Modify");
            sd1 = DMSETTINGS->readEntry("/BevelLength1", "1.0");
            sd2 = DMSETTINGS->readEntry("/BevelLength2", "1.0");
            st = DMSETTINGS->readEntry("/BevelTrim", "1");
            DMSETTINGS->endGroup();
        }
        ui->leLength1->setText(sd1);
        ui->leLength2->setText(sd2);
        ui->cbTrim->setChecked(st == "1");
    }
    else
    {
        action = nullptr;
    }
}

/// @brief 将界面数据更新到 Action
void UIBevelOptions::updateData()
{
    if (action)
    {
        action->setTrim(ui->cbTrim->isChecked());
        action->setLength1(Math2d::eval(ui->leLength1->text()));
        action->setLength2(Math2d::eval(ui->leLength2->text()));
    }
}
