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

/// @file UIDlgDimensionStyle.cpp
/// @brief 标注样式对话框实现

#include "UIDlgDimensionStyle.h"

#include "DmDimensionStyle.h"
#include "DmDocument.h"
#include "DmTextStyleTable.h"
#include "DmLine.h"

#include <QPushButton>

#include "DmDimension.h"
#include "Transaction.h"

UIDlgDimensionStyle::UIDlgDimensionStyle(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , m_pDocument(nullptr)
    , m_pStyle(nullptr)
    , m_pPreview(nullptr)
{
    setModal(modal);
    setupUi(this);
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

void UIDlgDimensionStyle::init(DmDimensionStyle* pStyle, DmDocument* pDocument)
{
    m_pDocument = pDocument;
    m_pStyle = pStyle;
    m_tempData = m_pStyle->getDataRef();
    m_originData = m_pStyle->getDataRef();
    setWindowTitle(tr("Dimension style: %1").arg(pStyle->getName()));

    cbColor->init(true);
    cbColorBoundary->init(true);
    cbLineType->init(true);
    cbLineTypeBoundary->init(true);
    cbLineWidth->init(true);
    cbLineWidthBoundary->init(true);
    cbTextColor->init(true);
    cbFillColor->init(true);
    cbTextStyle->init(m_pDocument->getTextStyleTable());
    if (m_tempData.textStyle() != nullptr)
    {
        cbTextStyle->setStyle(m_tempData.textStyle()->getName());
    }
    else
    {
        m_tempData.setTextStyle(cbTextStyle->getStyle());
    }
    connect(cbTextStyle, SIGNAL(styleChanged()), this, SLOT(slotTextStyleChanged()));
    tabWidget->setCurrentIndex(0);

    cbVerticalPos->addItem(tr("Mid"));
    cbVerticalPos->addItem(tr("Up"));
    cbVerticalPos->addItem(tr("Extern"));
    cbVerticalPos->addItem(tr("JIS"));
    cbVerticalPos->addItem(tr("Down"));

    cbHorizontalPos->addItem(tr("Mid"));
    cbHorizontalPos->addItem(tr("First boundary line"));
    cbHorizontalPos->addItem(tr("Second boundary line"));
    cbHorizontalPos->addItem(tr("Above first boundary line"));
    cbHorizontalPos->addItem(tr("Above second boundary line"));

    cbViewDirection->addItem(tr("Left to right"));
    cbViewDirection->addItem(tr("Right to left"));

    cbLinearUnitFormat->addItem(tr("Science"));
    cbLinearUnitFormat->addItem(tr("Decimal"));
    cbLinearUnitFormat->addItem(tr("Engineer"));
    cbLinearUnitFormat->addItem(tr("Architectural"));
    cbLinearUnitFormat->addItem(tr("Fraction"));
    cbLinearUnitFormat->addItem(tr("Windows"));
    //connect(cbLinearUnitFormat, SIGNAL(activated(int)), this, SLOT(slotLinearUnitFormatChanged(int)));	//手动才触发
    connect(cbLinearUnitFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(slotLinearUnitFormatChanged(int)));		//初始索引为0，如果setCurrentIndex也为0，则不会有响应
    //connect(cbLinearUnitFormat, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slotLinearUnitFormatChanged(const QString&)));
    cbFractionFormat->addItems({ tr("Horizontal"), tr("Diagonal"), tr("NoStack") });
    cbDecimalSaparator->addItems({ tr("Dot"), tr("Comma"), tr("Space") });
    cbAngleUnitFormat->addItems({ tr("Decimal degree"), tr("DMS"), tr("Gradians"), tr("Radians") });
    connect(cbAngleUnitFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(slotAngleUnitFormatChanged(int)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentTabChanged(int)));	//切换标签事件
    //connect(cbAngleUnitFormat, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slotAngleUnitFormatChanged(const QString&)));
    init();
}

void UIDlgDimensionStyle::init()
{
    //线
    connect(cbColor, SIGNAL(colorChanged(DmColor)), this, SLOT(slotColorChanged(DmColor)));
    connect(cbColorBoundary, SIGNAL(colorChanged(DmColor)), this, SLOT(slotColorChanged(DmColor)));
    connect(cbLineType, SIGNAL(lineTypeChanged(DM::LineType)), this, SLOT(slotLineTypeChanged(DM::LineType)));
    connect(cbLineTypeBoundary, SIGNAL(lineTypeChanged(DM::LineType)), this, SLOT(slotLineTypeChanged(DM::LineType)));
    connect(cbLineWidth, SIGNAL(widthChanged(DM::LineWidth)), this, SLOT(slotWidthChanged(DM::LineWidth)));
    connect(cbLineWidthBoundary, SIGNAL(widthChanged(DM::LineWidth)), this, SLOT(slotWidthChanged(DM::LineWidth)));
    connect(chkDimLine1, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(chkDimLine2, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(chkDimBoundaryLine1, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(chkDimBoundaryLine2, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(chkFixedLengthBoundryLine, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(sbExtendDimLine, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    connect(sbStartPtOffset, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    connect(sbFixedLengthBoundryLine, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    cbColor->setColor(m_tempData.dimLineColor());
    cbLineType->setLineType(m_tempData.dimLineType());
    cbLineWidth->setWidth(m_tempData.dimLineWidth());
    cbColorBoundary->setColor(m_tempData.boundLineColor());
    cbLineTypeBoundary->setLineType(m_tempData.boundLineType());
    cbLineWidthBoundary->setWidth(m_tempData.boundLineWidth());
    chkDimLine1->setChecked(m_tempData.hideDimLine1());
    chkDimLine2->setChecked(m_tempData.hideDimLine2());
    chkDimBoundaryLine1->setChecked(m_tempData.hideBoundLine1());
    chkDimBoundaryLine2->setChecked(m_tempData.hideBoundLine2());
    sbExtendDimLine->setValue(m_tempData.extendDimLine());
    sbStartPtOffset->setValue(m_tempData.startPtOffset());
    chkFixedLengthBoundryLine->setChecked(m_tempData.isFixedBoundLineLength());
    sbFixedLengthBoundryLine->setEnabled(m_tempData.isFixedBoundLineLength());
    sbFixedLengthBoundryLine->setValue(m_tempData.fixedBoundLineLength());
    //箭头
    cbFirstArrow->setArrowType(m_tempData.firstArrow());
    cbSecondArrow->setArrowType(m_tempData.secondArrow());
    cbLeaderLine->setArrowType(m_tempData.leaderArrow());
    sbArrowSize->setValue(m_tempData.arrowSize());
    connect(cbFirstArrow, SIGNAL(arrowChanged(DM::ArrowType)), this, SLOT(slotArrowChanged(DM::ArrowType)));
    connect(cbSecondArrow, SIGNAL(arrowChanged(DM::ArrowType)), this, SLOT(slotArrowChanged(DM::ArrowType)));
    connect(cbLeaderLine, SIGNAL(arrowChanged(DM::ArrowType)), this, SLOT(slotArrowChanged(DM::ArrowType)));
    connect(sbArrowSize, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    //文字
    cbTextColor->setColor(m_tempData.textColor());
    cbFillColor->setColor(m_tempData.textFillColor());
    sbTextHeight->setValue(m_tempData.textHeight());
    sbFractionScale->setValue(m_tempData.fractionHeightScale());
    chkDrawTextBoundary->setChecked(m_tempData.isDrawTextBoundary());
    cbVerticalPos->setCurrentIndex(static_cast<int>(m_tempData.textVerticalPos()));
    cbHorizontalPos->setCurrentIndex(static_cast<int>(m_tempData.textHorizontalPos()));
    cbViewDirection->setCurrentIndex(static_cast<int>(m_tempData.viewDirection()));
    sbOffsetFromDimensionLine->setValue(m_tempData.offsetFromDimLine());
    connect(cbTextColor, SIGNAL(colorChanged(DmColor)), this, SLOT(slotColorChanged(DmColor)));
    connect(cbFillColor, SIGNAL(colorChanged(DmColor)), this, SLOT(slotColorChanged(DmColor)));
    connect(chkDrawTextBoundary, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(sbTextHeight, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    connect(sbFractionScale, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    connect(sbOffsetFromDimensionLine, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    connect(cbVerticalPos, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboCurrentChanged(int)));
    connect(cbHorizontalPos, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboCurrentChanged(int)));
    connect(cbViewDirection, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboCurrentChanged(int)));
    //单位
    //cbLinearUnitFormat->setCurrentIndex(static_cast<int>(m_tempData.unitData.unitFormat));	//不一定会调用slotLinearUnitFormatChanged
    cbLinearUnitFormat->setCurrentIndex(static_cast<int>(m_tempData.unitFormat()));
    slotLinearUnitFormatChanged(static_cast<int>(m_tempData.unitFormat()));
    cbLinearPrecison->setCurrentIndex(static_cast<int>(m_tempData.precision()));
    sbRoundoff->setValue(m_tempData.roundOff());
    lePrefix->setText(m_tempData.prefix());
    lePostfix->setText(m_tempData.postfix());
    leMeasureFactor->setText(QLocale().toString(m_tempData.mesureUnitFactor(), 'g'));
    chkResetLinearPrefix->setChecked(m_tempData.resetPrefix());
    chkResetLinearPostfix->setChecked(m_tempData.resetPostfix());
    //cbAngleUnitFormat->setCurrentIndex(static_cast<int>(m_tempData.unitData.angleUnitFormat));
    slotAngleUnitFormatChanged(static_cast<int>(m_tempData.angleUnitFormat()));
    cbAngleUnitFormat->setCurrentIndex(static_cast<int>(m_tempData.angleUnitFormat()));
    cbAnglePrecision->setCurrentIndex(static_cast<int>(m_tempData.anglePrecision()));
    chkResetAnglePrefix->setChecked(m_tempData.resetAnglePrefix());
    chkResetAnglePostfix->setChecked(m_tempData.resetAnglePostfix());
    connect(chkResetAnglePrefix, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(chkResetAnglePostfix, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(chkResetLinearPrefix, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(chkResetLinearPostfix, SIGNAL(stateChanged(int)), this, SLOT(slotChkChanged(int)));
    connect(sbRoundoff, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxChanged(double)));
    connect(cbLinearPrecison, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboCurrentChanged(int)));
    connect(cbFractionFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboCurrentChanged(int)));
    connect(cbDecimalSaparator, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboCurrentChanged(int)));
    connect(cbAnglePrecision, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComboCurrentChanged(int)));
    connect(lePrefix, SIGNAL(textChanged(QString)), this, SLOT(slotTextChanged(QString)));
    connect(lePostfix, SIGNAL(textChanged(QString)), this, SLOT(slotTextChanged(QString)));
    connect(leMeasureFactor, SIGNAL(textChanged(QString)), this, SLOT(slotTextChanged(QString)));

    m_pPreview = new DmEntityContainer();
    previewLine->setContainer(m_pPreview);
    previewArrow->setContainer(m_pPreview);
    previewText->setContainer(m_pPreview);
    previewUnit->setContainer(m_pPreview);
}

QStringList UIDlgDimensionStyle::getPrecisionOfLinear(DmDimensionStyleUnitData::UnitFormat unitFormat) const
{
    QStringList items;
    switch (unitFormat)
    {
    case DmDimensionStyleUnitData::UnitFormat::Science:
        items.append({ "0E+01", "0.0E+01", "0.00E+01", "0.000E+01", "0.0000E+01", "0.00000E+01", "0.000000E+01", "0.0000000E+01", "0.00000000E+01" });
        break;
    case DmDimensionStyleUnitData::UnitFormat::Decimal:
        items.append({ "0", "0.0", "0.00", "0.000", "0.0000", "0.00000", "0.000000", "0.0000000", "0.00000000" });
        break;
    case DmDimensionStyleUnitData::UnitFormat::Engineer:
        items.append({ "0\'-0\"", "0\'-0.0\"", "0\'-0.00\"", "0\'-0.000\"", "0\'-0.0000\"", "0\'-0.00000\"", "0\'-0.000000\"", "0\'-0.0000000\"", "0\'-0.00000000\"" });
        break;
    case DmDimensionStyleUnitData::UnitFormat::Architectural:
        items.append({ "0\'-0\"", "0\'-1/2\"", "0\'-1/4\"", "0\'-1/8\"", "0\'-1/16\"", "0\'-1/32\"", "0\'-1/64\"", "0\'-1/128\"", "0\'-1/256\"" });
        break;
    case DmDimensionStyleUnitData::UnitFormat::Fraction:
        items.append({ "0 0", "0 1/2", "0 1/4", "0 1/8", "0 1/16", "0 1/32", "0 1/64", "0 1/128", "0 1/256" });
        break;
    case DmDimensionStyleUnitData::UnitFormat::Windows:
        items.append({ "0", "0.0", "0.00", "0.000", "0.0000", "0.00000", "0.000000", "0.0000000", "0.00000000" });
        break;
    default:
        break;
    }
    return items;
}

QStringList UIDlgDimensionStyle::getPrecisionOfAngle(DmDimensionStyleUnitData::AngleUnitFormat unitFormat) const
{
    QStringList items;
    switch (unitFormat)
    {
    case DmDimensionStyleUnitData::AngleUnitFormat::DecimalDegree:
        items.append({ "0", "0.0", "0.00", "0.000", "0.0000", "0.00000", "0.000000", "0.0000000", "0.00000000" });
        break;
    case DmDimensionStyleUnitData::AngleUnitFormat::DMS:
        items.append({ "0d", "0d00\'", "0d00\'", "0d00\'00\"", "0d00\'00\"", "0d00\'00.0\"", "0d00\'00.00\"", "0d00\'00.000\"", "0d00\'00.0000\"" });
        break;
    case DmDimensionStyleUnitData::AngleUnitFormat::Gradians:
        items.append({ "0g", "0.0g", "0.00g", "0.000g", "0.0000g", "0.00000g", "0.000000g", "0.0000000g", "0.00000000g" });
        break;
    case DmDimensionStyleUnitData::AngleUnitFormat::Radians:
        items.append({ "0r", "0.0r", "0.00r", "0.000r", "0.0000r", "0.00000r", "0.000000r", "0.0000000r", "0.00000000r" });
        break;
    default:
        break;
    }
    return items;
}

void UIDlgDimensionStyle::updatePreview()
{
    if (m_pPreview == nullptr)
    {
        return;
    }
    if (!previewLine->initialized() && !previewArrow->initialized()
        && !previewText->initialized() && !previewUnit->initialized())
    {
        return;
    }
    m_pStyle->updateData(m_tempData);
    //lswDimStyles->selectedStyle()->getPreview(m_pPreview.get());
    //preview->zoomAuto(false, true);
    m_pStyle->getPreview(m_pPreview);
    switch (tabWidget->currentIndex())
    {
    default:
    case 0:
        previewLine->specifyModified();
        previewLine->zoomAuto();
        break;
    case 1:
        previewArrow->specifyModified();
        previewArrow->zoomAuto();
        break;
    case 2:
        previewText->specifyModified();
        previewText->zoomAuto();
        break;
    case 3:
        previewUnit->specifyModified();
        previewUnit->zoomAuto();
        break;
    }
}

void UIDlgDimensionStyle::accept()
{
    auto styleInTable = m_pDocument->getDimStyleTable()->find(m_pStyle->getName());
    // 修改
    if (styleInTable)
    {
        Transaction t(tr("Modify dimension style").toStdString(), m_pDocument);
        t.start();
        m_pStyle->updateData(m_originData); //样式已修改，所以先还原，然后再启用修改，这样才能undo
        m_pDocument->getDimStyleTable()->startModify(m_pStyle);
        m_pStyle->updateData(m_tempData);
        t.commit();
    }
    // 新建
    else
    {
        Transaction t(tr("Add dimension style").toStdString(), m_pDocument);
        t.start();
        m_pStyle->updateData(m_tempData);
        m_pDocument->getDimStyleTable()->add(m_pStyle);
        t.commit();
    }

    QDialog::accept();
}

void UIDlgDimensionStyle::reject()
{
    m_pStyle->updateData(m_originData);
    QDialog::reject();
}

void UIDlgDimensionStyle::slotCurrentTabChanged(int index)
{
    updatePreview();
}

void UIDlgDimensionStyle::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    updatePreview();
}

void UIDlgDimensionStyle::resizeEvent(QResizeEvent* e)
{
    updatePreview();
}

void UIDlgDimensionStyle::slotColorChanged(DmColor color)
{
    if (sender() == cbColor)
    {
        m_tempData.setDimLineColor(color);
    }
    else if (sender() == cbColorBoundary)
    {
        m_tempData.setBoundLineColor(color);
    }
    else if (sender() == cbTextColor)
    {
        m_tempData.setTextColor(color);
    }
    else if (sender() == cbFillColor)
    {
        m_tempData.setTextFillColor(color);
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotLineTypeChanged(DmLineType* lineType)
{
    if (sender() == cbLineType)
    {
        m_tempData.setDimLineType(lineType);
    }
    else if (sender() == cbLineTypeBoundary)
    {
        m_tempData.setBoundLineType(lineType);
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotWidthChanged(DM::LineWidth width)
{
    if (sender() == cbLineWidth)
    {
        m_tempData.setDimLineWidth(width);
    }
    else if (sender() == cbLineWidthBoundary)
    {
        m_tempData.setBoundLineWidth(width);
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotChkChanged(int state)
{
    bool isChecked = (static_cast<Qt::CheckState>(state) == Qt::Checked) ? true : false;
    if (sender() == chkDimLine1)
    {
        m_tempData.setHideDimLine1(isChecked);
    }
    else if (sender() == chkDimLine2)
    {
        m_tempData.setHideDimLine2(isChecked);
    }
    else if (sender() == chkDimBoundaryLine1)
    {
        m_tempData.setHideBoundLine1(isChecked);
    }
    else if (sender() == chkDimBoundaryLine2)
    {
        m_tempData.setHideBoundLine2(isChecked);
    }
    else if (sender() == chkFixedLengthBoundryLine)
    {
        m_tempData.setIsFixedBoundLineLength(isChecked);
        sbFixedLengthBoundryLine->setEnabled(isChecked);
        sbFixedLengthBoundryLine->setValue(m_tempData.fixedBoundLineLength());
    }
    else if (sender() == chkDrawTextBoundary)
    {
        m_tempData.setIsDrawTextBoundary(isChecked);
    }
    else if (sender() == chkResetAnglePrefix)
    {
        m_tempData.setResetAnglePrefix(isChecked);
    }
    else if (sender() == chkResetAnglePostfix)
    {
        m_tempData.setResetAnglePostfix(isChecked);
    }
    else if (sender() == chkResetLinearPrefix)
    {
        m_tempData.setResetPrefix(isChecked);
    }
    else if (sender() == chkResetLinearPostfix)
    {
        m_tempData.setResetPostfix(isChecked);
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotSpinBoxChanged(double value)
{
    if (sender() == sbExtendDimLine)
    {
        m_tempData.setExtendDimLine(value);
    }
    else if (sender() == sbStartPtOffset)
    {
        m_tempData.setStartPtOffset(value);
    }
    else if (sender() == sbFixedLengthBoundryLine)
    {
        m_tempData.setFixedBoundLineLength(value);
    }
    else if (sender() == sbArrowSize)
    {
        m_tempData.setArrowSize(value);
    }
    else if (sender() == sbTextHeight)
    {
        m_tempData.setTextHeight(value);
    }
    else if (sender() == sbFractionScale)
    {
        m_tempData.setFractionHeightScale(value);
    }
    else if (sender() == sbOffsetFromDimensionLine)
    {
        m_tempData.setOffsetFromDimLine(value);
    }
    else if (sender() == sbRoundoff)
    {
        m_tempData.setRoundOff(value);
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotArrowChanged(DM::ArrowType arrowType)
{
    if (sender() == cbFirstArrow)
    {
        m_tempData.setFirstArrow(arrowType);
    }
    else if (sender() == cbSecondArrow)
    {
        m_tempData.setSecondArrow(arrowType);
    }
    else if (sender() == cbLeaderLine)
    {
        m_tempData.setLeaderArrow(arrowType);
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotComboCurrentChanged(int index)
{
    if (sender() == cbVerticalPos)
    {
        m_tempData.setTextVerticalPos(static_cast<DmDimensionStyleTextData::TextVerticalPos>(index));
    }
    else if (sender() == cbHorizontalPos)
    {
        m_tempData.setTextHorizontalPos(static_cast<DmDimensionStyleTextData::TextHorizontalPos>(index));
    }
    else if (sender() == cbViewDirection)
    {
        m_tempData.setViewDirection(static_cast<DmDimensionStyleTextData::ViewDirection>(index));
    }
    else if (sender() == cbLinearPrecison)
    {
        m_tempData.setPrecision(static_cast<DmDimensionStyleUnitData::Precision>(index));
    }
    else if (sender() == cbFractionFormat)
    {
        m_tempData.setFractionFormat(static_cast<DmDimensionStyleUnitData::FractionFormat>(index));
    }
    else if (sender() == cbDecimalSaparator)
    {
        m_tempData.setDecimalSaparator(static_cast<DmDimensionStyleUnitData::DecimalSaparator>(index));
    }
    else if (sender() == cbAnglePrecision)
    {
        m_tempData.setAnglePrecision(static_cast<DmDimensionStyleUnitData::Precision>(index));
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotTextStyleChanged()
{
    m_tempData.setTextStyle(cbTextStyle->getStyle());
    updatePreview();
}

void UIDlgDimensionStyle::slotLinearUnitFormatChanged(int idx)
{
    DmDimensionStyleUnitData::UnitFormat linearUnitFormat = static_cast<DmDimensionStyleUnitData::UnitFormat>(idx);
    cbLinearPrecison->clear();
    QStringList items = getPrecisionOfLinear(linearUnitFormat);
    cbLinearPrecison->addItems(items);
    m_tempData.setUnitFormat(linearUnitFormat);
    switch (linearUnitFormat)
    {
    case DmDimensionStyleUnitData::UnitFormat::Science:
        cbFractionFormat->setEnabled(false);
        cbDecimalSaparator->setEnabled(false);
        sbFractionScale->setEnabled(false);
        break;
    case DmDimensionStyleUnitData::UnitFormat::Decimal:
        cbFractionFormat->setEnabled(false);
        cbDecimalSaparator->setEnabled(true);
        cbDecimalSaparator->setCurrentIndex(static_cast<int>(m_tempData.decimalSaparator()));
        sbFractionScale->setEnabled(false);
        break;
    case DmDimensionStyleUnitData::UnitFormat::Engineer:
        cbFractionFormat->setEnabled(false);
        cbDecimalSaparator->setEnabled(false);
        sbFractionScale->setEnabled(false);
        break;
    case DmDimensionStyleUnitData::UnitFormat::Architectural:
        cbFractionFormat->setEnabled(true);
        cbFractionFormat->setCurrentIndex(static_cast<int>(m_tempData.fractionFormat()));
        cbDecimalSaparator->setEnabled(false);
        sbFractionScale->setEnabled(true);
        //sbFractionScale->setValue(m_tempData.textData.fractionHeightScale);
        break;
    case DmDimensionStyleUnitData::UnitFormat::Fraction:
        cbFractionFormat->setEnabled(true);
        cbFractionFormat->setCurrentIndex(static_cast<int>(m_tempData.fractionFormat()));
        cbDecimalSaparator->setEnabled(false);
        sbFractionScale->setEnabled(true);
        break;
    case DmDimensionStyleUnitData::UnitFormat::Windows:
        cbFractionFormat->setEnabled(false);
        cbDecimalSaparator->setEnabled(false);
        sbFractionScale->setEnabled(false);
        break;
    default:
        break;
    }
    updatePreview();
}

void UIDlgDimensionStyle::slotAngleUnitFormatChanged(int idx)
{
    cbAnglePrecision->clear();
    DmDimensionStyleUnitData::AngleUnitFormat angleUnitFormat = static_cast<DmDimensionStyleUnitData::AngleUnitFormat>(idx);
    m_tempData.setAngleUnitFormat(angleUnitFormat);
    QStringList items = getPrecisionOfAngle(angleUnitFormat);
    cbAnglePrecision->addItems(items);
    cbAnglePrecision->setCurrentIndex(static_cast<int>(m_tempData.anglePrecision()));
    updatePreview();
}

void UIDlgDimensionStyle::slotTextChanged(QString text)
{
    if (sender() == leMeasureFactor)
    {
        m_tempData.setMesureUnitFactor(text.toDouble());
    }
    else if (sender() == lePostfix)
    {
        m_tempData.setPostfix(text);
    }
    else if (sender() == lePrefix)
    {
        m_tempData.setPrefix(text);
    }
    updatePreview();
}
