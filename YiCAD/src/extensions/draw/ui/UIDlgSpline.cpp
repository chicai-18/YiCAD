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

/// @file UIDlgSpline.cpp
/// @brief 样条曲线属性编辑对话框

#include "UIDlgSpline.h"

#include <QPushButton>

#include "DmSpline.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "UIWidgetPen.h"
#include "UILayerBox.h"
#include "Math2d.h"
#include "Transaction.h"

UIDlgSpline::UIDlgSpline(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    this->setWindowIcon(QIcon(":/ribbon/logo.png"));
}

UIDlgSpline::~UIDlgSpline()
{
    // no need to delete child widgets, Qt does it all for us
}

void UIDlgSpline::languageChange()
{
    retranslateUi(this);
}

void UIDlgSpline::setSpline(DmSpline& e)
{
    m_pSpline = &e;
    wPen->setPen(m_pSpline->getPen(false), true, tr("Pen"));
    DmDocument* document = m_pSpline->getDocument();
    if (document != nullptr)
    {
        cbLayer->init(*(document->getLayerTable()), false);
    }
    DmLayer* lay = m_pSpline->getLayer(false);
    if (lay != nullptr)
    {
        cbLayer->setLayer(*lay);
    }

    QString s;
    s.setNum(m_pSpline->getDegree());
    cbDegree->setCurrentIndex(cbDegree->findText(s));

    cbClosed->setChecked(m_pSpline->isClosed());
}

void UIDlgSpline::updateSpline()
{
    Transaction t(tr("Modify spline").toStdString(), m_pSpline->getDocument());
    t.start();
    m_pSpline->getDocument()->getEntityTable()->startModify(m_pSpline);

    int k = m_pSpline->getDegree(); // TODO: 阶数暂不支持修改
    bool closeChanged = cbClosed->isChecked() != m_pSpline->isClosed();
    if (closeChanged)
    {
        bool isOriginClosed = m_pSpline->isClosed();
        bool isFit = m_pSpline->isByFit();
        // 拟合点
        if (isFit)
        {
            // 参考ActionDrawSplinePoints::fitPoints
            auto fitPts = m_pSpline->getFitPoints();
            if (isOriginClosed)
            {
                fitPts.erase(fitPts.end() - 1);
            }
            else
            {
                fitPts.emplace_back(fitPts.front());
            }
            m_pSpline->setFitPts(fitPts);
            m_pSpline->setClosed(cbClosed->isChecked());
            m_pSpline->fit();
            m_pSpline->update();
        }
        // 控制点
        else
        {
            auto ctrlPts = m_pSpline->getControlPoints();
            int n = m_pSpline->getNumberOfControlPoints() - 1;
            if (isOriginClosed)
            {
                // 移除追加环绕的k个控制点
                ctrlPts.erase(ctrlPts.end() - k, ctrlPts.end());
            }
            DmSpline::setControlPointsKnotsByClose(m_pSpline, cbClosed->isChecked(), ctrlPts);
        }
    }

    m_pSpline->setPen(wPen->getPen());
    m_pSpline->setLayer(cbLayer->currentText());
    m_pSpline->update();
    t.commit();
}
