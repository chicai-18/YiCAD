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

/// @file UIDlgImage.cpp
/// @brief 图片属性编辑对话框

#include "UIDlgImage.h"

#include <QPushButton>

#include "DmImage.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "Transaction.h"

UIDlgImage::UIDlgImage(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

UIDlgImage::~UIDlgImage()
{
    delete val;
    val = nullptr;
}

void UIDlgImage::languageChange()
{
    retranslateUi(this);
}

void UIDlgImage::setImage(DmImage& e)
{
    m_pImage = &e;
    val = new QDoubleValidator(leScale);
    wPen->setPen(m_pImage->getPen(false), true, tr("Pen"));
    DmDocument* document = m_pImage->getDocument();
    if (document != nullptr)
    {
        cbLayer->init(*(document->getLayerTable()), false);
    }
    DmLayer* lay = m_pImage->getLayer(false);
    if (lay != nullptr)
    {
        cbLayer->setLayer(*lay);
    }
    leInsertX->setValidator(val);
    leInsertY->setValidator(val);
    leWidth->setValidator(val);
    leHeight->setValidator(val);
    leScale->setValidator(val);
    leAngle->setValidator(val);
    m_dScale = m_pImage->getUVector().magnitude();
    leInsertX->setText(QString("%1").arg(m_pImage->getInsertionPoint().x));
    leInsertY->setText(QString("%1").arg(m_pImage->getInsertionPoint().y));
    leWidth->setText(QString("%1").arg(m_pImage->getImageWidth()));
    leHeight->setText(QString("%1").arg(m_pImage->getImageHeight()));
    leScale->setText(QString("%1").arg(m_dScale));
    leAngle->setText(QString("%1").arg(Math2d::rad2deg(m_pImage->getUVector().angle())));
    lePath->setText(m_pImage->getFile());
    leSize->setText(QString("%1 x %2").arg(m_pImage->getWidth()).arg(m_pImage->getHeight()));
    DM::Unit unit = DM::None;
    if (m_pImage->getDocument() != nullptr)
    {
        unit = m_pImage->getDocument()->getUnit();
    }
    leDPI->setText(QString("%1").arg(DmUnits::scaleToDpi(m_dScale, unit)));
}

void UIDlgImage::changeWidth()
{
    double width = leWidth->text().toDouble();
    m_dScale = width / m_pImage->getWidth();
    leHeight->setText(QString("%1").arg(m_pImage->getHeight() * m_dScale));
    leScale->setText(QString("%1").arg(m_dScale));
}

void UIDlgImage::changeHeight()
{
    double height = leHeight->text().toDouble();
    m_dScale = height / m_pImage->getHeight();
    leWidth->setText(QString("%1").arg(m_pImage->getWidth() * m_dScale));
    leScale->setText(QString("%1").arg(m_dScale));
}

void UIDlgImage::changeScale()
{
    m_dScale = leScale->text().toDouble();
    leWidth->setText(QString("%1").arg(m_pImage->getWidth() * m_dScale));
    leHeight->setText(QString("%1").arg(m_pImage->getHeight() * m_dScale));
    DM::Unit unit = DM::None;
    if (m_pImage->getDocument() != nullptr)
    {
        unit = m_pImage->getDocument()->getUnit();
    }
    leDPI->setText(QString("%1").arg(DmUnits::scaleToDpi(m_dScale, unit)));
}

void UIDlgImage::changeDPI()
{
    DM::Unit unit = DM::None;
    if (m_pImage->getDocument() != nullptr)
    {
        unit = m_pImage->getDocument()->getUnit();
    }
    m_dScale = DmUnits::dpiToScale(leDPI->text().toDouble(), unit);
    leScale->setText(QString("%1").arg(m_dScale));
    leWidth->setText(QString("%1").arg(m_pImage->getWidth() * m_dScale));
    leHeight->setText(QString("%1").arg(m_pImage->getHeight() * m_dScale));
}

void UIDlgImage::updateImage()
{
    Transaction t(tr("Modify image").toStdString(), m_pImage->getDocument());
    t.start();
    m_pImage->getDocument()->getEntityTable()->startModify(m_pImage);
    m_pImage->setPen(wPen->getPen());
    m_pImage->setLayer(cbLayer->currentText());
    m_pImage->setInsertionPoint(DmVector(leInsertX->text().toDouble(), leInsertY->text().toDouble()));
    double orgScale = m_pImage->getUVector().magnitude();
    m_dScale /= orgScale;
    double orgAngle = m_pImage->getUVector().angle();
    double angle = Math2d::deg2rad(leAngle->text().toDouble());
    m_pImage->scale(m_pImage->getInsertionPoint(), DmVector(m_dScale, m_dScale));
    m_pImage->rotateAngle(m_pImage->getInsertionPoint(), angle - orgAngle);

    m_pImage->update();
    t.commit();
}
