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

/// @file UIExitDialog.cpp
/// @brief 退出/关闭文档确认对话框

#include "UIExitDialog.h"

#include <QMessageBox>
#include <QPushButton>

#include "ui_UIExitDialog.h"

/// @brief 构造 UIExitDialog 对话框
/// 默认为非模态对话框，设置 modal 为 true 可构造模态对话框。
UIExitDialog::UIExitDialog(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui::UIExitDialog())
{
    setModal(modal);
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));
    ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    ui->buttonBox->button(QDialogButtonBox::SaveAll)->setText(tr("Save All"));

    this->setWindowIcon(QIcon(":/ribbon/logo.png"));

    init();
}

UIExitDialog::~UIExitDialog() = default;

/// @brief 使用当前语言设置子控件的字符串
void UIExitDialog::languageChange()
{
    ui->retranslateUi(this);
}

void UIExitDialog::init()
{
    setShowSaveAll(false);
    // set dlg icon
    QMessageBox mb("", "", QMessageBox::Question, QMessageBox::Ok, Qt::NoButton, Qt::NoButton);
    ui->l_icon->setPixmap(mb.iconPixmap());
}

void UIExitDialog::clicked(QAbstractButton* button)
{
    QDialogButtonBox::StandardButton bt = ui->buttonBox->standardButton(button);
    switch (bt)
    {
    case QDialogButtonBox::Close:
        emit accept();
        break;
    case QDialogButtonBox::Save:
        slotSave();
        break;
    case QDialogButtonBox::SaveAll:
        slotSaveAll();
        break;
    default:
        emit reject();
        break;
    }
}

void UIExitDialog::setText(const QString& text)
{
    ui->lQuestion->setText(text);
}

void UIExitDialog::setTitle(const QString& text)
{
    setWindowTitle(text);
}

void UIExitDialog::setForce(bool force)
{
    QPushButton* bCancel = ui->buttonBox->button(QDialogButtonBox::Cancel);
    bCancel->setDisabled(force);
}

void UIExitDialog::setShowSaveAll(bool show)
{
    ui->buttonBox->button(QDialogButtonBox::SaveAll)->setVisible(show);
    QString close = show ? tr("Close All") : tr("Close");
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(close);
}

void UIExitDialog::slotSaveAll()
{
    done(3);
}

void UIExitDialog::slotSave()
{
    done(2);
}
