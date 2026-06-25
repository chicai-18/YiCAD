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

/// @file UIImageOptions.cpp
/// @brief 图片插入选项控件

#include "UIImageOptions.h"

#include "ActionInterface.h"
#include "ActionDrawImage.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "Debug.h"
#include "ui_UIImageOptions.h"

UIImageOptions::UIImageOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui::Ui_ImageOptions)
{
    ui->setupUi(this);
}

/// @brief 销毁对象并释放资源
UIImageOptions::~UIImageOptions()
{
    saveSettings();
}

/// @brief 使用当前语言设置子控件的字符串
void UIImageOptions::languageChange()
{
    ui->retranslateUi(this);
}

void UIImageOptions::saveSettings()
{
    DMSETTINGS->beginGroup("/Image");
    DMSETTINGS->writeEntry("/ImageAngle", ui->leAngle->text());
    DMSETTINGS->writeEntry("/ImageFactor", ui->leFactor->text());
    DMSETTINGS->endGroup();
}

void UIImageOptions::setAction(ActionInterface* a, bool update)
{
    if (a != nullptr && a->getEntityType() == DM::ActionDrawImage)
    {
        action = static_cast<ActionDrawImage*>(a);

        QString sAngle;
        QString sFactor;
        if (update)
        {
            sAngle = QString("%1").arg(Math2d::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
        }
        else
        {
            DMSETTINGS->beginGroup("/Image");
            sAngle = DMSETTINGS->readEntry("/ImageAngle", "0.0");
            sFactor = DMSETTINGS->readEntry("/ImageFactor", "1.0");
            DMSETTINGS->endGroup();
        }
        ui->leAngle->setText(sAngle);
        ui->leFactor->setText(sFactor);
        updateData();
        updateFactor();
    }
    else
    {
        action = nullptr;
    }
}

void UIImageOptions::updateData()
{
    if (action != nullptr)
    {
        action->setAngle(Math2d::deg2rad(Math2d::eval(ui->leAngle->text())));
    }
}

void UIImageOptions::updateDPI()
{
    if (action != nullptr)
    {
        double f = action->dpiToScale(Math2d::eval(ui->leDPI->text()));
        ui->leFactor->blockSignals(true);
        ui->leFactor->setText(QString::number(f));
        ui->leFactor->blockSignals(false);
        action->setFactor(f);
    }
}

void UIImageOptions::updateFactor()
{
    if (action != nullptr)
    {
        double f = Math2d::eval(ui->leFactor->text());
        double dpi = action->scaleToDpi(f);
        ui->leDPI->blockSignals(true);
        ui->leDPI->setText(QString::number(dpi));
        ui->leDPI->blockSignals(false);
        action->setFactor(f);
    }
}
