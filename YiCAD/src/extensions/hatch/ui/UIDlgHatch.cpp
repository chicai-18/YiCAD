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

/// @file UIDlgHatch.cpp
/// @brief 填充图案属性编辑对话框

#include "UIDlgHatch.h"

#include <QPushButton>
#include <QButtonGroup>

#include "DmSettings.h"
#include "DmLine.h"
#include "DmHatch.h"
#include "DmPatternList.h"
#include "DmPattern.h"
#include "Math2d.h"
#include "Transaction.h"
#include "DmDocument.h"

UIDlgHatch::UIDlgHatch(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , m_saveSettings(true)
{
    setModal(modal);
    setupUi(this);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    init();
}

void UIDlgHatch::languageChange()
{
    retranslateUi(this);
}

void UIDlgHatch::init()
{
    m_pPattern = nullptr;
    m_pHatch = nullptr;
    m_isNew = false;

    m_pPreview = new DmEntityContainer();
    gvPreview->setContainer(m_pPreview);

    cbPattern->init();
}

void UIDlgHatch::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    gvPreview->zoomAuto();
}

UIDlgHatch::~UIDlgHatch()
{
    if (m_pPreview != nullptr)
    {
        delete m_pPreview;
        m_pPreview = nullptr;
    }
    saveSettingIfNeed();
}

void UIDlgHatch::setHatch(DmHatch& h, bool isNew)
{
    m_pHatch = &h;
    m_isNew = isNew;

    // read defaults from config file:
    if (m_isNew)
    {
        DMSETTINGS->beginGroup("/Draw");
        QString solid = DMSETTINGS->readEntry("/HatchSolid", "0");
        QString pat = DMSETTINGS->readEntry("/HatchPattern", "ANGLE");
        QString scale = DMSETTINGS->readEntry("/HatchScale", "1.0");
        QString angle = DMSETTINGS->readEntry("/HatchAngle", "0.0");
        DMSETTINGS->endGroup();

        cbSolid->setChecked(solid == "1");
        setPattern(pat);
        leScale->setText(scale);
        leAngle->setText(angle);
    }
    // initialize dialog based on given m_pHatch:
    else
    {
        cbSolid->setChecked(m_pHatch->isSolid());
        setPattern(m_pHatch->getPattern());
        QString s;
        s.setNum(m_pHatch->getScale());
        leScale->setText(s);
        s.setNum(Math2d::rad2deg(m_pHatch->getAngle()));
        leAngle->setText(s);
    }
}

void UIDlgHatch::updateHatch()
{
    // 只在修改时需要事务，新建时事务在action中
    std::shared_ptr<Transaction> t;
    if (!m_isNew)
    {
        t = std::make_shared<Transaction>(tr("Modify hatch").toStdString(), m_pHatch->getDocument());
        t->start();
        m_pHatch->getDocument()->getEntityTable()->startModify(m_pHatch);
    }
    m_pHatch->setSolid(cbSolid->isChecked());
    m_pHatch->setScale(Math2d::eval(leScale->text()));
    m_pHatch->setAngle(Math2d::deg2rad(Math2d::eval(leAngle->text())));
    if (m_pPattern != nullptr)
    {
        m_pHatch->getDataRef().setPattern(*m_pPattern);
    }
    if (t)
    {
        t->commit();
    }
}

void UIDlgHatch::setPattern(const QString& p)
{
    if (!DMPATTERNLIST->contains(p))
    {
        cbPattern->addItem(p);
    }
    cbPattern->setCurrentIndex(cbPattern->findText(p));
    m_pPattern = cbPattern->getPattern();
}

void UIDlgHatch::resizeEvent(QResizeEvent*)
{
    updatePreview();
}

void UIDlgHatch::updatePreview()
{
    constexpr double RANGE_MARGIN_RATIO = 5.0;
    constexpr double RANGE_SCALE_RATIO = 10.0;
    constexpr double DEFAULT_RANGE = 100.0;

    if (m_pPreview == nullptr || m_pHatch == nullptr)
    {
        return;
    }

    if (cbSolid->isChecked())
    {
        m_pPattern = nullptr;
    }
    else
    {
        QString patName = cbPattern->currentText();
        m_pPattern = cbPattern->getPattern();
    }

    // 计算预览的范围
    DmVector min(false), max(false);
    if (m_pPattern != nullptr)
    {
        bool ret = m_pPattern->getMaxRange(min, max);
        if (ret)
        {
            DmVector delta = max - min;
            double range = std::max(delta.x, delta.y);
            min -= DmVector(range / RANGE_MARGIN_RATIO, range / RANGE_MARGIN_RATIO);
            max = min + DmVector(range * RANGE_SCALE_RATIO, range * RANGE_SCALE_RATIO);
        }
    }
    if (!min.valid)
    {
        min = DmVector(0.0, 0.0);
        max = DmVector(DEFAULT_RANGE, DEFAULT_RANGE);
    }

    DmEntityContainerPtr loop(new DmEntityContainer(nullptr));
    loop->setPen(DmPen(DM::FlagInvalid));
    // 逆时针四条直线
    loop->addRectangle(min, max);
    bool isSolid = cbSolid->isChecked();
    double scale = Math2d::eval(leScale->text(), 1.0);
    double angle = Math2d::deg2rad(Math2d::eval(leAngle->text(), 0.0));

    m_pPreview->clear();
    DmHatch* prevHatch = new DmHatch(m_pPreview, HatchData(isSolid, scale, angle, m_pPattern));
    prevHatch->setDocument(m_pHatch->getDocument());
    if (!m_isNew)
    {
        prevHatch->setPen(m_pHatch->getPen());
        prevHatch->setLayer(m_pHatch->getLayer(false));
    }

    DmRegionPtr region(new DmRegion());
    RegionData d(loop, {});
    region->setData(d);
    region->update();
    prevHatch->setBoundary(region);
    prevHatch->update();
    m_pPreview->addEntity(prevHatch);
    gvPreview->specifyModified();
    gvPreview->zoomAuto();
}

void UIDlgHatch::reject()
{
    m_saveSettings = false;
    QDialog::reject();
}

void UIDlgHatch::accept()
{
    QDialog::accept();
}

void UIDlgHatch::saveSettingIfNeed()
{
    if (m_isNew && m_saveSettings)
    {
        DMSETTINGS->beginGroup("/Draw");
        DMSETTINGS->writeEntry("/HatchSolid", cbSolid->isChecked());
        DMSETTINGS->writeEntry("/HatchPattern", cbPattern->currentText());
        DMSETTINGS->writeEntry("/HatchScale", leScale->text());
        DMSETTINGS->writeEntry("/HatchAngle", leAngle->text());
        DMSETTINGS->endGroup();
    }
}
