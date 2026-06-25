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

/// @file UIDlgDimensionStyleNew.cpp
/// @brief 新建标注样式对话框实现

#include "UIDlgDimensionStyleNew.h"

#include "DmDocument.h"
#include "DmDimensionStyle.h"
#include "GuiDialogFactory.h"

#include <QPushButton>
#include <QMessageBox>

#include "Debug.h"

UIDlgDimensionStyleNew::UIDlgDimensionStyleNew(QWidget* parent /*= nullptr*/, bool modal /*= false*/, Qt::WindowFlags fl /*= 0*/)
    : QDialog(parent, fl)
    , ui(new Ui::UIDlgDimensionStyleNew())
    , m_pDocument(nullptr)
{
    setModal(modal);
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

UIDlgDimensionStyleNew::~UIDlgDimensionStyleNew()
{
}

void UIDlgDimensionStyleNew::init(DmDimensionStyle* tempStyle, DmDocument* document)
{
    ui->cbTemplateStyle->init(document->getDimStyleTable(), tempStyle->getName());
    m_pDocument = document;
    ui->leNewStyle->setText(QString("%1 %2").arg(tempStyle->getName()).arg(tr("copy")));
}

void UIDlgDimensionStyleNew::done(int r)
{
    //在关闭窗体前做验证。参考：https://www.qtcentre.org/threads/8048-Validate-Data-in-QDialog
    if (QDialog::Accepted == r)  // ok was pressed
    {
        QString newStyle = ui->leNewStyle->text();
        if (newStyle.trimmed().isEmpty())
        {
            QMessageBox::critical(this, tr("New dimension style"), tr("Please input valid dimension style name!"), QMessageBox::Close);
            return;
        }
        if (m_pDocument->getDimStyleTable()->find(newStyle))
        {
            QMessageBox::critical(this, tr("New dimension style"), tr("%0 is already exist !").arg(newStyle), QMessageBox::Close);
            return;
        }
        DmDimensionStyle* pNewStyle = new DmDimensionStyle(*ui->cbTemplateStyle->getStyle(), newStyle);
        pNewStyle->setDocument(m_pDocument);
        bool res = GUIDIALOGFACTORY->requestDimStyleModifyDialog(pNewStyle, m_pDocument);
        if (!res)
        {
            delete pNewStyle;
            pNewStyle = nullptr;
        }
        QDialog::done(r);
    }
    else    // cancel, close or exc was pressed
    {
        QDialog::done(r);
        return;
    }
}

bool UIDlgDimensionStyleNew::isInputValid()
{
    return false;
}
