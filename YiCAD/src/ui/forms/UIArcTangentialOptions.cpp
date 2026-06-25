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

/// @file UIArcTangentialOptions.cpp
/// @brief 相切圆弧选项控件实现

#include <cmath>

#include <QAction>
#include <QMessageBox>

#include "UIArcTangentialOptions.h"

#include "ActionDrawArcTangential.h"

#include "DmSettings.h"
#include "Math2d.h"
#include "Debug.h"

#include "ui_UIArcTangentialOptions.h"

/// @brief 构造 UIArcTangentialOptions
/// @param [in] parent 父窗口指针
/// @param [in] fl 窗口标志
UIArcTangentialOptions::UIArcTangentialOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , ui(new Ui::Ui_ArcTangentialOptions{})
{
    ui->setupUi(this);
    connect(ui->btnLockAngle, SIGNAL(toggled(bool)), this, SLOT(slotLockAngle(bool)));
    connect(ui->btnLockRadius, SIGNAL(toggled(bool)), this, SLOT(slotLockRadius(bool)));
}

/// @brief 析构函数
UIArcTangentialOptions::~UIArcTangentialOptions()
{
}

/// @brief 语言切换时刷新界面文本
void UIArcTangentialOptions::languageChange()
{
    ui->retranslateUi(this);
}

void UIArcTangentialOptions::setAction(ActionInterface* a, bool update)
{
    if (a && a->getEntityType() == DM::ActionDrawArcTangential)
    {
        action = static_cast<ActionDrawArcTangential*>(a);

        //半径
        double radius = 0.0;
        QString sr;
        if (action->isLockRadius())
        {
            radius = action->lockRadius();
            sr = QString("%1").arg(radius);
            ui->leRadius->setText(sr);
            ui->leRadius->setEnabled(false);
            ui->btnLockRadius->setChecked(true);
        }
        else
        {
            radius = action->getRadius();
            sr = QString("%1").arg(radius);
            ui->leRadius->setText(sr);
            ui->leRadius->setEnabled(true);
            ui->btnLockRadius->setChecked(false);
        }

        //角度
        double angle = 0.0;
        QString sa;
        if (action->isLockAngle())
        {
            angle = action->lockAngle();
            sa = QString("%1").arg(Math2d::rad2deg(angle));
            ui->leAngle->setText(sa);
            ui->leAngle->setEnabled(false);
            ui->btnLockAngle->setChecked(true);
        }
        else
        {
            angle = action->getAngle();
            sa = QString("%1").arg(Math2d::rad2deg(angle));
            ui->leAngle->setText(sa);
            ui->leAngle->setEnabled(true);
            ui->btnLockAngle->setChecked(false);
        }
    }
    else
    {
        action = nullptr;
    }
}

/// @brief 更新半径显示
/// @param [in] s 半径字符串
void UIArcTangentialOptions::updateRadius(const QString& s)
{
    ui->leRadius->setText(s);
}

/// @brief 更新角度显示
/// @param [in] s 角度字符串
void UIArcTangentialOptions::updateAngle(const QString& s)
{
    ui->leAngle->setText(s);
}

void UIArcTangentialOptions::slotLockAngle(bool lock)
{
    constexpr double FULL_CIRCLE_DEG = 360.0;
    constexpr double RAD_TO_DEG = M_PI / 180.0;

    if (lock)
    {
        bool ok = false;
        double val = ui->leAngle->text().toDouble(&ok);
        if (!ok || (val > FULL_CIRCLE_DEG && val < 0.0))
        {
            QMessageBox::critical(nullptr, tr("Tips"), tr("Make sure lock angle is between 0 and 360"), QMessageBox::StandardButton::Ok);
            ui->btnLockAngle->setChecked(false);
        }
        else
        {
            action->setLockAngle(val * RAD_TO_DEG);
            action->setIsLockAngle(true);
            ui->leAngle->setEnabled(false);
        }
    }
    else
    {
        action->setIsLockAngle(false);
        ui->leAngle->setEnabled(true);
    }
    action->updatePreview();
}

void UIArcTangentialOptions::slotLockRadius(bool lock)
{
    if (lock)
    {
        bool ok = false;
        double val = ui->leRadius->text().toDouble(&ok);
        if (!ok || val <= 0.0)
        {
            QMessageBox::critical(nullptr, tr("Tips"), tr("Make sure lock radius more than 0"), QMessageBox::StandardButton::Ok);
            ui->btnLockRadius->setChecked(false);
        }
        else
        {
            action->setLockRadius(val);
            action->setIsLockRadius(true);
            ui->leRadius->setEnabled(false);
        }
    }
    else
    {
        action->setLockRadius(false);
        ui->leRadius->setEnabled(true);
    }
    action->updatePreview();
}

void UIArcTangentialOptions::on_leRadius_editingFinished()
{
    //if (ui->rbRadius->isChecked()) {
    //	bool ok;
    //	double d = fabs(Math2d::eval(ui->leRadius->text(), &ok));
    //	if (!ok) return;
    //	if (d < DM_TOLERANCE) d = 1.0;
    //	//updateRadius(QString::number(d,'g',5));
    //	action->setRadius(d);
    //	action->setByRadius(true);
    //	ui->leRadius->setText(QString::number(d, 'g', 5));
    //}
}

void UIArcTangentialOptions::on_leAngle_editingFinished()
{
    //if (ui->rbAngle->isChecked()) {
    //	bool ok;
    //	double d = Math2d::correctAngle(Math2d::eval(ui->leAngle->text(), &ok) * M_PI / 180.);
    //	if (!ok) return;
    //	if (d < DM_TOLERANCE_ANGLE || d + DM_TOLERANCE_ANGLE > 2. * M_PI) d = M_PI; // can not do full circle
    //	action->setAngle(d);
    //	//updateAngle(QString::number(d*180./M_PI,'g',5));
    //	action->setByRadius(false);
    //	ui->leAngle->setText(QString::number(Math2d::rad2deg(d), 'g', 5));
    //}
}

void UIArcTangentialOptions::on_rbRadius_clicked(bool /*checked*/)
{
    //action->setByRadius(true);
    //action->setRadius(ui->leRadius->text().toDouble());
    //updateByRadius(true);
}

void UIArcTangentialOptions::on_rbAngle_clicked(bool /*checked*/)
{
    //action->setByRadius(false);
    //action->setAngle(ui->leAngle->text().toDouble() * M_PI / 180.);
    //updateByRadius(false);
}
