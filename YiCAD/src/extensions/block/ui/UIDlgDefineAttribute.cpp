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

/// @file UIDlgDefineAttribute.cpp
/// @brief 属性定义对话框实现

#include <QMessageBox>

#include "UIDlgDefineAttribute.h"
#include "DmTextStyle.h"
#include "DmSettings.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "Transaction.h"

UIDlgDefineAttribute::UIDlgDefineAttribute(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , m_pAttrDef(nullptr)
    , m_isNew(false)
    , m_isDlgShow(false)
{
    setModal(modal);
    setupUi(this);
    init();
}

UIDlgDefineAttribute::~UIDlgDefineAttribute()
{
    saveSettingIfNeed();
}

void UIDlgDefineAttribute::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    m_isDlgShow = true;
}

void UIDlgDefineAttribute::init()
{
    connect(cbStyle, SIGNAL(styleChanged()), this, SLOT(slotStyleChanged()));
}

void UIDlgDefineAttribute::saveSettingIfNeed()
{
    if (m_isNew && m_saveSettings)
    {
        DMSETTINGS->beginGroup("/Block");
        DMSETTINGS->writeEntry("/AttributeHeight", leHeight->text());
        ETextHorzMode hAlign;
        ETextVertMode vAlign;
        cbAlignment->getAlignment(hAlign, vAlign);
        DMSETTINGS->writeEntry("/TextAlignmentH", QString::number(static_cast<int>(hAlign)));
        DMSETTINGS->writeEntry("/TextAlignmentV", QString::number(static_cast<int>(vAlign)));
        DMSETTINGS->writeEntry("/AttributeAngle", leAngle->text());
        DMSETTINGS->endGroup();
    }
}

void UIDlgDefineAttribute::setAttributeDefinition(DmAttributeDefinition& t, bool isNew)
{
    m_pAttrDef = &t;
    this->m_isNew = isNew;

    QString tag;
    QString prompt;
    QString defValue;
    QString height;
    ETextHorzMode hAlign;
    ETextVertMode vAlign;
    QString angle;

    if (m_isNew)
    {
        DMSETTINGS->beginGroup("/Block");
        height = DMSETTINGS->readEntry("/AttributeHeight", "1.0");
        hAlign = static_cast<ETextHorzMode>(DMSETTINGS->readEntry("/TextAlignmentH", "0").toInt());
        vAlign = static_cast<ETextVertMode>(DMSETTINGS->readEntry("/TextAlignmentV", "0").toInt());
        angle = DMSETTINGS->readEntry("/AttributeAngle", "0");
        DMSETTINGS->endGroup();
    }
    else
    {
        height = QString("%1").arg(m_pAttrDef->getHeight());
        hAlign = m_pAttrDef->getHAlign();
        vAlign = m_pAttrDef->getVAlign();
        angle = QString("%1").arg(Math2d::rad2deg(m_pAttrDef->getAngle()));
        tag = m_pAttrDef->getTag();
        prompt = m_pAttrDef->getPrompt();
        defValue = m_pAttrDef->getText();
    }

    leTag->setText(tag);
    lePrompt->setText(prompt);
    leDefault->setText(defValue);
    leHeight->setText(height);
    cbAlignment->setAlignment(hAlign, vAlign);
    leAngle->setText(angle);

    DmTextStyleTable* textStyleTable = m_pAttrDef->getDocument()->getTextStyleTable();
    cbStyle->init(textStyleTable);
    m_pAttrDef->setStyle(cbStyle->getStyle());
    adjustSize();
}

void UIDlgDefineAttribute::updateAttributeDefinition()
{
    // 只在修改时需要事务，新建时事务在action中
    std::shared_ptr<Transaction> t;
    if (!m_isNew)
    {
        t = std::make_shared<Transaction>(tr("Modify attribute definition").toStdString(), m_pAttrDef->getDocument());
        t->start();
        m_pAttrDef->getDocument()->getEntityTable()->startModify(m_pAttrDef);
    }

    //修改文字的对齐方式时，alignment的位置需要调整
    adjustAlignmentForModeChange();
    if (m_pAttrDef)
    {
        DmTextStyle* style = cbStyle->getStyle();
        m_pAttrDef->setStyle(style);
        m_pAttrDef->setHeight(leHeight->text().toDouble());
        m_pAttrDef->setUpsideDown(style->getDataConstPtr()->isUpsideDown);
        m_pAttrDef->setReverseDirection(style->getDataConstPtr()->isReverseDirection);
        m_pAttrDef->setWidthFactor(style->getDataConstPtr()->widhFactor);
        m_pAttrDef->setSlashAngle(style->getDataConstPtr()->slashAngle);

        m_pAttrDef->setTag(leTag->text());
        m_pAttrDef->setPrompt(lePrompt->text());
        m_pAttrDef->setText(leDefault->text());
        ETextHorzMode hAlign;
        ETextVertMode vAlign;
        cbAlignment->getAlignment(hAlign, vAlign);
        m_pAttrDef->setHAlign(hAlign);
        m_pAttrDef->setVAlign(vAlign);
        m_pAttrDef->setAngle(Math2d::deg2rad(leAngle->text().toDouble()));
    }
    if (m_pAttrDef && !m_isNew)
    {
        m_pAttrDef->update();
    }
    if (t)
    {
        t->commit();
    }
}

void UIDlgDefineAttribute::slotStyleChanged()
{
    if (!m_isDlgShow)
    {
        return;
    }

    constexpr double TOLERANCE = 1e-5;

    double defHeight = cbStyle->getStyle()->getData().defaultHeight;
    if (abs(defHeight) < TOLERANCE)
    {
        leHeight->setText("2.5");
    }
    else
    {
        leHeight->setText(QString::number(defHeight));
    }
}

void UIDlgDefineAttribute::reject()
{
    m_saveSettings = false;
    QDialog::reject();
}

void UIDlgDefineAttribute::accept()
{
    if (leTag->text().trimmed().isEmpty())
    {
        QMessageBox::critical(this, QObject::tr("Tips"), tr("Tag can not be empty!"));
        return;
    }
    QDialog::accept();
}

void UIDlgDefineAttribute::adjustAlignmentForModeChange()
{
    if (m_pAttrDef && !m_isNew)
    {
        ETextMode oldMode = m_pAttrDef->getDataConstPtr()->getTextMode();
        ETextMode newMode = cbAlignment->getAlignment();
        ETextHorzMode newHAlign;
        ETextVertMode newVAlign;
        cbAlignment->getAlignment(newHAlign, newVAlign);

        //左对齐只有position有效，alignment为原点
        if (oldMode == ETextMode::kTextLeft && newMode != ETextMode::kTextLeft)
        {
            m_pAttrDef->setAlignment(m_pAttrDef->getPosition());
        }

        //对于其他对齐转为左对齐的情况，DmText::update()中设置
    }
}

void UIDlgDefineAttribute::languageChange()
{
    retranslateUi(this);
}
