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

/// @file UIBlockDialog.cpp
/// @brief 块对话框实现

#include "UIBlockDialog.h"

#include <QMessageBox>
#include <QPushButton>

#include "DmBlockTable.h"
#include "DmBlock.h"
#include "Debug.h"

UIBlockDialog::UIBlockDialog(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

/// @brief 语言切换时刷新界面文本
void UIBlockDialog::languageChange()
{
    retranslateUi(this);
}

void UIBlockDialog::setBlockList(DmBlockTable* l)
{
    blockTable = l;
    if (blockTable)
    {
        DmBlock* block = blockTable->getActive();
        if (block)
        {
            leName->setText(block->getName());
        }
    }
}

DmBlockData UIBlockDialog::getBlockData()
{
    return DmBlockData(leName->text(), DmVector(0.0, 0.0), false);
}

void UIBlockDialog::validate()
{
    QString name = leName->text();

    if (!name.isEmpty())
    {
        if (blockTable && !blockTable->find(name))
        {
            accept();
        }
        else
        {
            QMessageBox::warning(this, tr("Renaming Block"), tr("Could not name block. A block named \"%1\" already exists.").arg(leName->text()), QMessageBox::Ok, Qt::NoButton);
        }
    }
}

void UIBlockDialog::cancel()
{
    leName->setText("");
    reject();
}
