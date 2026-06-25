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

/// @file UIDlgOptionsDrawing.cpp
/// @brief 绘图选项设置对话框

#include "UIDlgOptionsDrawing.h"

#include <iostream>
#include <cfloat>
#include <QPushButton>
#include <QMessageBox>
#include "DmDocument.h"
#include "DmSettings.h"
#include "DmFont.h"
#include "Debug.h"

namespace
{
    int current_tab = 0;
}

UIDlgOptionsDrawing::UIDlgOptionsDrawing(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , document(nullptr)
{
    setModal(modal);
    setupUi(this);
    tabWidget->setCurrentIndex(current_tab);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    init();
}

/// @brief Destroys the object and frees any allocated resources
UIDlgOptionsDrawing::~UIDlgOptionsDrawing()
{
}

/// @brief Sets the strings of the subwidgets using the current language.
void UIDlgOptionsDrawing::languageChange()
{
    retranslateUi(this);
}

void UIDlgOptionsDrawing::init()
{
    document = nullptr;

    // precision list:
    for (int i = 0; i <= 8; i++)
    {
        listPrec1 << QString("%1").arg(0.0, 0, 'f', i);
    }

    // Main drawing unit:
    for (int i = DM::None; i < DM::LastUnit; i++)
    {
        cbUnit->addItem(DmUnits::unitToString(static_cast<DM::Unit>(i)));
    }

    // init units combobox:
    QStringList unitList;
    unitList << tr("Scientific") << tr("Decimal") << tr("Engineering") << tr("Architectural") << tr("Fractional") << tr("Architectural (metric)");
    cbLengthFormat->insertItems(0, unitList);

    // init angle units combobox:
    QStringList aunitList;
    aunitList << tr("Decimal Degrees") << tr("Deg/min/sec") << tr("Gradians") << tr("Radians") << tr("Surveyor's units");
    cbAngleFormat->insertItems(0, aunitList);
}

/// @brief Sets the document and updates the GUI to match the drawing.
void UIDlgOptionsDrawing::setDocument(DmDocument* g)
{
    document = g;

    if (document == nullptr)
    {
        std::cout << " UIDlgOptionsDrawing::setDocument(nullptr)\n";
        return;
    }

    // main drawing unit:
    int insunits = document->getVariableInt("$INSUNITS", 0);
    cbUnit->setCurrentIndex(cbUnit->findText(DmUnits::unitToString((DM::Unit)insunits)));

    // units / length format:
    int lunits = document->getVariableInt("$LUNITS", 2);
    cbLengthFormat->setCurrentIndex(lunits - 1);

    // units length precision:
    int luprec = document->getVariableInt("$LUPREC", 4);
    updateCBLengthPrecision(cbLengthFormat, cbLengthPrecision);
    cbLengthPrecision->setCurrentIndex(luprec);

    // units / angle format:
    int aunits = document->getVariableInt("$AUNITS", 0);
    cbAngleFormat->setCurrentIndex(aunits);

    // units angle precision:
    int auprec = document->getVariableInt("$AUPREC", 2);
    updateCBAnglePrecision(cbAngleFormat, cbAnglePrecision);
    cbAnglePrecision->setCurrentIndex(auprec);

    updateUnitLabels();
}

/// @brief Called when OK is clicked.
void UIDlgOptionsDrawing::validate()
{
    DM::LinearFormat f = (DM::LinearFormat)cbLengthFormat->currentIndex();
    if (f == DM::Engineering || f == DM::Architectural)
    {
        if (DmUnits::stringToUnit(cbUnit->currentText()) != DM::Inch)
        {
            QMessageBox::warning(this, tr("Options"), tr("For the length formats 'Engineering' and 'Architectural', the unit must be set to Inch."),
                QMessageBox::Ok, Qt::NoButton);
            return;
        }
    }
    if (f == DM::ArchitecturalMetric)
    {
        if (DmUnits::stringToUnit(cbUnit->currentText()) != DM::Meter)
        {
            QMessageBox::warning(this, tr("Options"),
                tr("For the length format 'Architectural (metric)', the unit must be set to Meter."), QMessageBox::Ok, Qt::NoButton);
            return;
        }
    }

    if (document != nullptr)
    {
        // units:
        DM::Unit unit = static_cast<DM::Unit>(cbUnit->currentIndex());
        document->setUnit(unit);

        document->addVariable("$LUNITS", cbLengthFormat->currentIndex() + 1, 70);
        document->addVariable("$LUPREC", cbLengthPrecision->currentIndex(), 70);
        document->addVariable("$AUNITS", cbAngleFormat->currentIndex(), 70);
        document->addVariable("$AUPREC", cbAnglePrecision->currentIndex(), 70);

    }
    accept();
}

/// @brief Updates the length precision combobox
void UIDlgOptionsDrawing::updateLengthPrecision()
{
    updateCBLengthPrecision(cbLengthFormat, cbLengthPrecision);
}

/// @brief Updates the length precision combobox
void UIDlgOptionsDrawing::updateCBLengthPrecision(QComboBox* f, QComboBox* p)
{
    int index = p->currentIndex();
    p->clear();

    switch (f->currentIndex())
    {
        // scientific
    case 0:
        p->addItem("0E+01");
        p->addItem("0.0E+01");
        p->addItem("0.00E+01");
        p->addItem("0.000E+01");
        p->addItem("0.0000E+01");
        p->addItem("0.00000E+01");
        p->addItem("0.000000E+01");
        p->addItem("0.0000000E+01");
        p->addItem("0.00000000E+01");
        break;

        // decimal
        // (0, 0.1, 0.01, ...)
    case 1:
        p->insertItems(0, listPrec1);
        break;

        // architectural:
    case 3:
        p->addItem("0'-0\"");
        p->addItem("0'-0 1/2\"");
        p->addItem("0'-0 1/4\"");
        p->addItem("0'-0 1/8\"");
        p->addItem("0'-0 1/16\"");
        p->addItem("0'-0 1/32\"");
        p->addItem("0'-0 1/64\"");
        p->addItem("0'-0 1/128\"");
        break;

        // engineering:
    case 2:
        p->addItem("0'-0\"");
        p->addItem("0'-0.0\"");
        p->addItem("0'-0.00\"");
        p->addItem("0'-0.000\"");
        p->addItem("0'-0.0000\"");
        p->addItem("0'-0.00000\"");
        p->addItem("0'-0.000000\"");
        p->addItem("0'-0.0000000\"");
        p->addItem("0'-0.00000000\"");
        break;

        // fractional
    case 4:
        p->addItem("0");
        p->addItem("0 1/2");
        p->addItem("0 1/4");
        p->addItem("0 1/8");
        p->addItem("0 1/16");
        p->addItem("0 1/32");
        p->addItem("0 1/64");
        p->addItem("0 1/128");
        break;

        // architectural metric
    case 5:
        p->insertItems(0, listPrec1);
        break;

    default:
        break;
    }

    p->setCurrentIndex(index);
}

/// @brief Updates the angle precision combobox
void UIDlgOptionsDrawing::updateAnglePrecision()
{
    updateCBAnglePrecision(cbAngleFormat, cbAnglePrecision);
}

/// @brief Updates the angle precision combobox
void UIDlgOptionsDrawing::updateCBAnglePrecision(QComboBox* u, QComboBox* p)
{
    constexpr int UNICODE_DEGREE = 0xB0;

    int index = p->currentIndex();
    p->clear();

    switch (u->currentIndex())
    {
        // decimal degrees:
    case 0:
        p->insertItems(0, listPrec1);
        break;

        // deg/min/sec:
    case 1:
        p->addItem(QString("0%1").arg(QChar(UNICODE_DEGREE)));
        p->addItem(QString("0%100'").arg(QChar(UNICODE_DEGREE)));
        p->addItem(QString("0%100'00\"").arg(QChar(UNICODE_DEGREE)));
        p->addItem(QString("0%100'00.0\"").arg(QChar(UNICODE_DEGREE)));
        p->addItem(QString("0%100'00.00\"").arg(QChar(UNICODE_DEGREE)));
        p->addItem(QString("0%100'00.000\"").arg(QChar(UNICODE_DEGREE)));
        p->addItem(QString("0%100'00.0000\"").arg(QChar(UNICODE_DEGREE)));
        break;

        // gradians:
    case 2:
        p->addItem("0g");
        p->addItem("0.0g");
        p->addItem("0.00g");
        p->addItem("0.000g");
        p->addItem("0.0000g");
        p->addItem("0.00000g");
        p->addItem("0.000000g");
        p->addItem("0.0000000g");
        p->addItem("0.00000000g");
        break;

        // radians:
    case 3:
        p->addItem("0r");
        p->addItem("0.0r");
        p->addItem("0.00r");
        p->addItem("0.000r");
        p->addItem("0.0000r");
        p->addItem("0.00000r");
        p->addItem("0.000000r");
        p->addItem("0.0000000r");
        p->addItem("0.00000000r");
        break;

        // surveyor's units:
    case 4:
        p->addItem("N 0d E");
        p->addItem("N 0d00' E");
        p->addItem("N 0d00'00\" E");
        p->addItem("N 0d00'00.0\" E");
        p->addItem("N 0d00'00.00\" E");
        p->addItem("N 0d00'00.000\" E");
        p->addItem("N 0d00'00.0000\" E");
        break;

    default:
        break;
    }

    p->setCurrentIndex(index);
}

/// @brief Updates the preview of unit display.
void UIDlgOptionsDrawing::updatePreview()
{
    QString prev;
    prev = DmUnits::formatLinear(3.14159265, static_cast<DM::Unit>(cbUnit->currentIndex()),
        static_cast<DM::LinearFormat>(cbLengthFormat->currentIndex()), cbLengthPrecision->currentIndex());
    lLinear->setText(prev);

    prev = DmUnits::formatAngle(1.570796325, static_cast<DM::AngleFormat>(cbAngleFormat->currentIndex()), cbAnglePrecision->currentIndex());
    lAngular->setText(prev);
}

/// @brief Updates all unit labels that depend on the global unit.
void UIDlgOptionsDrawing::updateUnitLabels()
{
    DM::Unit u = (DM::Unit)cbUnit->currentIndex();
    QString sign = DmUnits::unitToSign(u);
}

void UIDlgOptionsDrawing::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
}

void UIDlgOptionsDrawing::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
}

void UIDlgOptionsDrawing::on_tabWidget_currentChanged(int index)
{
    current_tab = index;
}
