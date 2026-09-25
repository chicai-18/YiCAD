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

/// @file UIDlgPolyline.cpp
/// @brief 多段线属性编辑对话框

#include "UIDlgPolyline.h"

#include <QPushButton>

#include "DmPolyline.h"
#include "DmDocument.h"
#include "Transaction.h"

UIDlgPolyline::UIDlgPolyline(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

UIDlgPolyline::~UIDlgPolyline()
{
}

void UIDlgPolyline::languageChange()
{
    retranslateUi(this);
}

void UIDlgPolyline::setPolyline(DmPolyline& e)
{
    m_pPolyline = &e;
    wPen->setPen(m_pPolyline->getPen(false), true, tr("Pen"));
    DmDocument* document = m_pPolyline->getDocument();
    if (document != nullptr)
    {
        cbLayer->init(*(document->getLayerTable()), false);
    }
    DmLayer* lay = m_pPolyline->getLayer(false);
    if (lay != nullptr)
    {
        cbLayer->setLayer(*lay);
    }

    cbClosed->setChecked(m_pPolyline->isClosed());
}

void UIDlgPolyline::updatePolyline()
{
    Transaction t(tr("Modify polyline").toStdString(), m_pPolyline->getDocument());
    t.start();
    m_pPolyline->getDocument()->getEntityTable()->startModify(m_pPolyline);
    m_pPolyline->setClosed(cbClosed->isChecked());
    m_pPolyline->setPen(wPen->getPen());
    m_pPolyline->setLayer(cbLayer->currentText());
    m_pPolyline->update();
    t.commit();
}
