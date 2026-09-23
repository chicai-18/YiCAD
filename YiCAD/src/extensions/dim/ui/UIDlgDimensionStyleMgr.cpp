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

/// @file UIDlgDimensionStyleMgr.cpp
/// @brief 标注样式管理器对话框实现

#include "UIDlgDimensionStyleMgr.h"

#include <QPushButton>
#include <QCloseEvent>

UIDlgDimensionStyleMgr::UIDlgDimensionStyleMgr(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , m_pPreview(nullptr)
{
    setModal(modal);
    setupUi(this);
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));

    connect(lswDimStyles, SIGNAL(activeStyleChanged(QString)), this, SLOT(slotActiveDimStyleChanged(QString)));
    connect(lswDimStyles, SIGNAL(selectedStyleChanged()), this, SLOT(slotSelectedStyleChanged()));
    connect(lswDimStyles, SIGNAL(styleChanged()), this, SLOT(slotStyleChanged()));
    connect(btnNew, SIGNAL(released()), lswDimStyles, SLOT(slotNewDimStyle()));
    connect(btnActive, SIGNAL(released()), lswDimStyles, SLOT(slotSetActiveDimStyle()));
    connect(btnRename, SIGNAL(released()), lswDimStyles, SLOT(slotRenameDimStyle()));
    connect(btnModify, SIGNAL(released()), lswDimStyles, SLOT(slotModifyDimStyle()));
    connect(btnDelete, SIGNAL(released()), lswDimStyles, SLOT(slotDeleteDimStyle()));
}

UIDlgDimensionStyleMgr::~UIDlgDimensionStyleMgr()
{
    if (m_pPreview)
    {
        delete m_pPreview;
        m_pPreview = nullptr;
    }
}

void UIDlgDimensionStyleMgr::init(DmDimensionStyleTable* dimStyleTable, DmDocument* document)
{
    assert(dimStyleTable && document);
    m_pDimStyleTable = dimStyleTable;
    m_pDocument = document;
    lswDimStyles->init(dimStyleTable, document);
    lblCurDimStyle->setText(lswDimStyles->activeStyle()->getName());

    m_pPreview = new DmEntityContainer();
    preview->setContainer(m_pPreview);
}

void UIDlgDimensionStyleMgr::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    updatePreview();
}

void UIDlgDimensionStyleMgr::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updatePreview();
}

void UIDlgDimensionStyleMgr::slotActiveDimStyleChanged(QString style)
{
    lblCurDimStyle->setText(style);
}

void UIDlgDimensionStyleMgr::slotSelectedStyleChanged()
{
    btnDelete->setEnabled(lswDimStyles->canSelectedDelete());
    lPreviewDimStyle->setText(lswDimStyles->selectedStyle()->getName());
    updatePreview();
}

void UIDlgDimensionStyleMgr::slotStyleChanged()
{
    updatePreview();
}

void UIDlgDimensionStyleMgr::updatePreview()
{
    if (m_pPreview == nullptr)
    {
        return;
    }

    if (!preview->initialized())
    {
        return;
    }
    if (lswDimStyles->selectedStyle() == nullptr)
    {
        return;
    }
    lswDimStyles->selectedStyle()->getPreview(m_pPreview);
    preview->specifyModified();
    preview->zoomAuto();
}
