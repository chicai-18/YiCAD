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

/// @file UICircleTan2Options.cpp
/// @brief 双切圆选项控件实现

#include "UICircleTan2Options.h"

#include "ActionDrawCircleTan2.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "Debug.h"

#include "ui_UICircleTan2Options.h"

/// @brief 构造 UICircleTan2Options
/// @param [in] parent 父窗口指针
/// @param [in] fl 窗口标志
UICircleTan2Options::UICircleTan2Options(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui::Ui_CircleTan2Options{})
{
    ui->setupUi(this);
}

/// @brief 析构函数，保存设置
UICircleTan2Options::~UICircleTan2Options()
{
    saveSettings();
    // no need to delete child widgets, Qt does it all for us
}

/// @brief 语言切换时刷新界面文本
void UICircleTan2Options::languageChange()
{
    ui->retranslateUi(this);
}

void UICircleTan2Options::saveSettings()
{
    DMSETTINGS->beginGroup("/Draw");
    DMSETTINGS->writeEntry("/CircleTan2Radius", ui->leRadius->text());
    DMSETTINGS->endGroup();
}

void UICircleTan2Options::setAction(ActionInterface* a, bool update)
{
    if (a && a->getEntityType() == DM::ActionDrawCircleTan2)
    {
        action = static_cast<ActionDrawCircleTan2*>(a);

        QString sr;
        if (update)
        {
            sr = QString("%1").arg(action->getRadius());
        }
        else
        {
            DMSETTINGS->beginGroup("/Draw");
            sr = DMSETTINGS->readEntry("/CircleTan2Radius", "1.0");
            DMSETTINGS->endGroup();
        }
        ui->leRadius->setText(sr);
    }
    else
    {
        action = nullptr;
    }
}

void UICircleTan2Options::updateRadius(const QString& r)
{
    if (action)
    {
        bool ok;
        double radius = Math2d::eval(r, &ok);
        if (ok)
        {
            action->setRadius(radius);
        }/*else{
            ui->leRadius->setText("10.0");
        }*/
    }
}
