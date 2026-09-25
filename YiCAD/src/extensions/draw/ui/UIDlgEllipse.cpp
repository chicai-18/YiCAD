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

/// @file UIDlgEllipse.cpp
/// @brief 椭圆/椭圆弧属性编辑对话框

#include "UIDlgEllipse.h"

#include <QPushButton>

#include "DmEllipse.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "Transaction.h"

UIDlgEllipse::UIDlgEllipse(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

UIDlgEllipse::~UIDlgEllipse()
{
}

void UIDlgEllipse::languageChange()
{
    retranslateUi(this);
}

void UIDlgEllipse::setEllipse(DmEllipse& e)
{
    m_pEllipse = &e;
    wPen->setPen(m_pEllipse->getPen(false), true, tr("Pen"));
    DmDocument* document = m_pEllipse->getDocument();
    if (document)
    {
        cbLayer->init(*(document->getLayerTable()), false);
    }
    DmLayer* lay = m_pEllipse->getLayer(false);
    if (lay)
    {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(m_pEllipse->getCenter().x);
    leCenterX->setText(s);
    s.setNum(m_pEllipse->getCenter().y);
    leCenterY->setText(s);
    s.setNum(m_pEllipse->getMajorP().magnitude());
    leMajor->setText(s);
    s.setNum(m_pEllipse->getMajorP().magnitude() * m_pEllipse->getRatio());
    leMinor->setText(s);
    s.setNum(Math2d::rad2deg(m_pEllipse->getMajorP().angle()));
    leRotation->setText(s);
    s.setNum(Math2d::rad2deg(m_pEllipse->getStartAngle()));
    leAngle1->setText(s);
    s.setNum(Math2d::rad2deg(m_pEllipse->getEndAngle()));
    leAngle2->setText(s);
    s.setNum(m_pEllipse->getNormal().x);
    leNormalX->setText(s);
    s.setNum(m_pEllipse->getNormal().y);
    leNormalY->setText(s);
    s.setNum(m_pEllipse->getNormal().z);
    leNormalZ->setText(s);
}

void UIDlgEllipse::updateEllipse()
{
    constexpr double SMALL_TOLERANCE = 1.0e-6;
    constexpr double DEFAULT_RATIO = 1.0;

    Transaction t(tr("Modify ellipse").toStdString(), m_pEllipse->getDocument());
    t.start();
    m_pEllipse->getDocument()->getEntityTable()->startModify(m_pEllipse);
    m_pEllipse->setCenter(DmVector(Math2d::eval(leCenterX->text()), Math2d::eval(leCenterY->text())));
    DmVector v = DmVector::polar(Math2d::eval(leMajor->text()), Math2d::deg2rad(Math2d::eval(leRotation->text())));
    m_pEllipse->setMajorP(v);
    if (Math2d::eval(leMajor->text()) > SMALL_TOLERANCE)
    {
        m_pEllipse->setRatio(Math2d::eval(leMinor->text()) / Math2d::eval(leMajor->text()));
    }
    else
    {
        m_pEllipse->setRatio(DEFAULT_RATIO);
    }
    m_pEllipse->setStartAngle(Math2d::deg2rad(Math2d::eval(leAngle1->text())));
    m_pEllipse->setEndAngle(Math2d::deg2rad(Math2d::eval(leAngle2->text())));
    double normalX = Math2d::eval(leNormalX->text());
    double normalY = Math2d::eval(leNormalY->text());
    double normalZ = Math2d::eval(leNormalZ->text());
    m_pEllipse->setNormal(DmVector(normalX, normalY, normalZ));
    m_pEllipse->setPen(wPen->getPen());
    m_pEllipse->setLayer(cbLayer->currentText());
    m_pEllipse->calculateBorders();
    m_pEllipse->update();
    t.commit();
}
