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

/// @file UIArcOptions.cpp
/// @brief 圆弧绘制选项控件实现

#include "UIArcOptions.h"

#include "ActionDrawArc.h"
#include "DmSettings.h"
#include "Debug.h"
#include "ui_UIArcOptions.h"

/// @brief 构造 UIArcOptions
/// @param [in] parent 父窗口指针
/// @param [in] fl 窗口标志
UIArcOptions::UIArcOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui::Ui_ArcOptions{})
{
    ui->setupUi(this);
    connect(ui->rbPos, SIGNAL(toggled(bool)), this, SLOT(slotRdoToggled(bool)));
}

/// @brief 析构函数，保存设置
UIArcOptions::~UIArcOptions()
{
    saveSettings();
}

void UIArcOptions::slotRdoToggled(bool checked)
{
    if (action)
    {
        action->setClockwise(ui->rbNeg->isChecked());
    }
}

/// @brief 语言切换时刷新界面文本
void UIArcOptions::languageChange()
{
    ui->retranslateUi(this);
}

void UIArcOptions::saveSettings()
{
    DMSETTINGS->beginGroup("/Draw");
    DMSETTINGS->writeEntry("/ArcClockwise", static_cast<int>(ui->rbNeg->isChecked()));
    DMSETTINGS->endGroup();
}

void UIArcOptions::setAction(ActionInterface* a, bool update)
{
    if (a && a->getEntityType() == DM::ActionDrawArc)
    {
        action = static_cast<ActionDrawArc*>(a);

        bool clockwise;
        if (update)
        {
            clockwise = action->isClockwise();
        }
        else
        {
            DMSETTINGS->beginGroup("/Draw");
            clockwise = DMSETTINGS->readNumEntry("/ArcClockwise", 0);
            DMSETTINGS->endGroup();
            action->setClockwise(clockwise);
        }
        ui->rbNeg->setChecked(clockwise);
    }
    else
    {
        action = nullptr;
    }
}
