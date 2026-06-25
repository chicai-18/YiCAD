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

/// @file UIDlgText.cpp
/// @brief 文字实体属性编辑对话框

#include "UIDlgText.h"

#include <QPushButton>
#include <QTextCodec>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include "DmSystem.h"
#include "DmSettings.h"
#include "DmFont.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "Transaction.h"

/// @brief 构造 UIDlgText 对话框
/// 默认为非模态对话框，设置 modal 为 true 可构造模态对话框。
UIDlgText::UIDlgText(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , m_saveSettings(true)
    , m_pText(nullptr)
    , m_isNew(false)
    , m_isDlgShow(false)
{
    setModal(modal);
    setupUi(this);
    init();
}

/// @brief 销毁对象并释放资源
UIDlgText::~UIDlgText()
{
    saveSettingIfNeed();
    // no need to delete child widgets, Qt does it all for us
}

/// @brief 使用当前语言设置子控件的字符串
void UIDlgText::languageChange()
{
    retranslateUi(this);
}

void UIDlgText::init()
{
    connect(cbStyle, SIGNAL(styleChanged()), this, SLOT(slotStyleChanged()));
}

// set saveText to false, so, settings won't be saved during destroy, feature request#3445306
void UIDlgText::reject()
{
    m_saveSettings = false;
    QDialog::reject();
}

void UIDlgText::accept()
{
    if (leText->text().trimmed().isEmpty())
    {
        QMessageBox::critical(this, QObject::tr("Tips"), tr("Text can not be empty!"));
        return;
    }
    QDialog::accept();
}

void UIDlgText::adjustAlignmentForModeChange()
{
    if (m_pText != nullptr && !m_isNew)
    {
        ETextMode oldMode = m_pText->getDataConstPtr()->getTextMode();
        ETextMode newMode = cbAlignment->getAlignment();
        ETextHorzMode newHAlign;
        ETextVertMode newVAlign;
        cbAlignment->getAlignment(newHAlign, newVAlign);
        // 左对齐只有position有效，alignment为原点
        if (oldMode == ETextMode::kTextLeft && newMode != ETextMode::kTextLeft)
        {
            m_pText->setAlignment(m_pText->getPosition());
        }
        // 对于其他对齐转为左对齐的情况，DmText::update()中设置
    }
}

void UIDlgText::saveSettingIfNeed()
{
    if (m_isNew && m_saveSettings)
    {
        DMSETTINGS->beginGroup("/Draw");
        DMSETTINGS->writeEntry("/TextHeight", leHeight->text());
        ETextHorzMode hAlign;
        ETextVertMode vAlign;
        cbAlignment->getAlignment(hAlign, vAlign);
        DMSETTINGS->writeEntry("/TextAlignmentH", QString::number((int)hAlign));
        DMSETTINGS->writeEntry("/TextAlignmentV", QString::number((int)vAlign));
        DMSETTINGS->writeEntry("/TextStringT", leText->text());
        DMSETTINGS->writeEntry("/TextAngle", leAngle->text());
        DMSETTINGS->endGroup();
    }
}

/// @brief 设置此对话框表示的 m_pText 实体
void UIDlgText::setText(DmText& t, bool isNew)
{
    m_pText = &t;
    m_isNew = isNew;

    QString height;
    ETextHorzMode hAlign;
    ETextVertMode vAlign;
    QString str;
    QString angle;

    DmTextStyleTable* textStyleTable = m_pText->getDocument()->getTextStyleTable();
    cbStyle->init(textStyleTable);
    if (m_isNew)
    {
        DMSETTINGS->beginGroup("/Draw");
        height = DMSETTINGS->readEntry("/TextHeight", "1.0");
        hAlign = (ETextHorzMode)DMSETTINGS->readEntry("/TextAlignmentH", "0").toInt();
        vAlign = (ETextVertMode)DMSETTINGS->readEntry("/TextAlignmentV", "0").toInt();
        str = DMSETTINGS->readEntry("/TextStringT", "");
        angle = DMSETTINGS->readEntry("/TextAngle", "0");
        DMSETTINGS->endGroup();
    }
    else
    {
        height = QString("%1").arg(m_pText->getHeight());
        hAlign = m_pText->getHAlign();
        vAlign = m_pText->getVAlign();
        str = m_pText->getText();
        angle = QString("%1").arg(Math2d::rad2deg(m_pText->getAngle()));
        cbStyle->setStyle(m_pText->getStyle()->getName());
    }

    leHeight->setText(height);
    cbAlignment->setAlignment(hAlign, vAlign);
    leText->setText(str);
    leAngle->setText(angle);
    leText->setFocus();
    leText->selectAll();
    m_pText->setStyle(cbStyle->getStyle());
    adjustSize();
}

/// @brief 更新对话框表示的 m_pText 实体以匹配用户的选择
void UIDlgText::updateText()
{
    // 只在修改时需要事务，新建时事务在action中
    std::shared_ptr<Transaction> t;
    if (!m_isNew)
    {
        t = std::make_shared<Transaction>(tr("Modify text").toStdString(), m_pText->getDocument());
        t->start();
        m_pText->getDocument()->getEntityTable()->startModify(m_pText);
    }

    // 修改文字的对齐方式时，alignment的位置需要调整
    adjustAlignmentForModeChange();

    // 更新其他信息
    if (m_pText != nullptr)
    {
        DmTextStyle* style = cbStyle->getStyle();
        m_pText->setStyle(style);
        m_pText->setHeight(leHeight->text().toDouble());
        m_pText->setUpsideDown(style->getDataConstPtr()->isUpsideDown);
        m_pText->setReverseDirection(style->getDataConstPtr()->isReverseDirection);
        m_pText->setWidthFactor(style->getDataConstPtr()->widhFactor);
        m_pText->setSlashAngle(style->getDataConstPtr()->slashAngle);

        m_pText->setText(leText->text());
        ETextHorzMode hAlign;
        ETextVertMode vAlign;
        cbAlignment->getAlignment(hAlign, vAlign);
        m_pText->setHAlign(hAlign);
        m_pText->setVAlign(vAlign);
        m_pText->setAngle(Math2d::deg2rad(leAngle->text().toDouble()));
    }

    // 修改的情况update()；新建的情况外部调用update()，因为新建的点数据没有确定，对于"布满"的情况update()会修改宽度系数为0
    if (m_pText != nullptr && !m_isNew)
    {
        m_pText->update();
    }
    if (t)
    {
        t->commit();
    }
}

void UIDlgText::slotStyleChanged()
{
    constexpr double TOLERANCE = 1e-5;
    constexpr double DEFAULT_HEIGHT = 2.5;

    if (!m_isDlgShow)
    {
        return;
    }
    double defHeight = cbStyle->getStyle()->getData().defaultHeight;
    if (abs(defHeight) < TOLERANCE)
    {
        leHeight->setText(QString::number(DEFAULT_HEIGHT));
    }
    else
    {
        leHeight->setText(QString::number(defHeight));
    }
}

void UIDlgText::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    m_isDlgShow = true;
}
