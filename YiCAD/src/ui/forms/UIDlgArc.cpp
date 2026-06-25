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

/// @file UIDlgArc.cpp
/// @brief 圆弧属性对话框实现

#include "UIDlgArc.h"

#include <QPushButton>

#include "DmArc.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "Transaction.h"

UIDlgArc::UIDlgArc(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

UIDlgArc::~UIDlgArc()
{
    // no need to delete child widgets, Qt does it all for us
}

/// @brief 语言切换时刷新界面文本
void UIDlgArc::languageChange()
{
    retranslateUi(this);
}

void UIDlgArc::setArc(DmArc& a)
{
    arc = &a;
    wPen->setPen(arc->getPen(false), true, tr("Pen"));

    DmDocument* document = arc->getDocument();
    if (document)
    {
        cbLayer->init(*(document->getLayerTable()), false);
    }

    DmLayer* lay = arc->getLayer(false);
    if (lay)
    {
        cbLayer->setLayer(*lay);
    }

    QString s;
    s.setNum(arc->getCenter().x);
    leCenterX->setText(s);
    s.setNum(arc->getCenter().y);
    leCenterY->setText(s);
    s.setNum(arc->getRadius());
    leRadius->setText(s);
    s.setNum(Math2d::rad2deg(arc->getStartAngle()));
    leAngle1->setText(s);
    s.setNum(Math2d::rad2deg(arc->getEndAngle()));
    leAngle2->setText(s);
    s.setNum(arc->getNormal().x);
    leNormalX->setText(s);
    s.setNum(arc->getNormal().y);
    leNormalY->setText(s);
    s.setNum(arc->getNormal().z);
    leNormalZ->setText(s);
}

void UIDlgArc::updateArc()
{
    Transaction t(tr("Modify arc").toStdString(), arc->getDocument());
    t.start();
    arc->getDocument()->getEntityTable()->startModify(arc);
    arc->setCenter(DmVector(Math2d::eval(leCenterX->text()), Math2d::eval(leCenterY->text())));
    arc->setRadius(Math2d::eval(leRadius->text()));
    arc->setStartAngle(Math2d::deg2rad(Math2d::eval(leAngle1->text())));
    arc->setEndAngle(Math2d::deg2rad(Math2d::eval(leAngle2->text())));
    double normalX = Math2d::eval(leNormalX->text());
    double normalY = Math2d::eval(leNormalY->text());
    double normalZ = Math2d::eval(leNormalZ->text());
    arc->setNormal(DmVector(normalX, normalY, normalZ));
    arc->setPen(wPen->getPen());
    arc->setLayer(cbLayer->currentText());
    arc->update();
    arc->calculateBorders();
    t.commit();
}
