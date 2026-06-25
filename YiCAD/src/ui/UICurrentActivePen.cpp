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

/// @file UICurrentActivePen.cpp
/// @brief 当前激活画笔控件，集成颜色、线宽和线型选择，修改当前文档和选中实体的笔属性

#include "UICurrentActivePen.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QAbstractItemView>

#include "UILineTypeBox.h"
#include "UIColorBox.h"
#include "UIWidthBox.h"
#include "ApplicationWindow.h"
#include "DmDocument.h"
#include "DocumentCmd.h"
#include "Transaction.h"

UICurrentActivePen::UICurrentActivePen(QWidget* parent)
	: QWidget(parent)
{
	setMinimumWidth(560);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

 	QLabel* penColor = new QLabel(tr("Color:"), this);
	penColor->setMinimumWidth(30);
	m_pCurrentColor = new UIColorBox(this);
	m_pCurrentColor->init(true);

	QLabel* penLineWidth = new QLabel(tr("LineWindth:"), this);
	penLineWidth->setMinimumWidth(30);
	m_pCurrentWidth = new UIWidthBox(this);
	m_pCurrentWidth->init(true);

	QLabel* penLineType = new QLabel(tr("LineType:"), this);
	penLineType->setMinimumWidth(30);
	m_pCurrentLineType = new UILineTypeBox(this);
	//m_pCurrentLineType->view()->setTextElideMode(Qt::ElideLeft);
	m_pCurrentLineType->init(true);

	QHBoxLayout* pLayout = new QHBoxLayout(this);
	pLayout->setContentsMargins(4, 0, 0, 0);
	pLayout->setSpacing(8);
	pLayout->addWidget(penColor);
	pLayout->addWidget(m_pCurrentColor);
	pLayout->addWidget(penLineWidth);
	pLayout->addWidget(m_pCurrentWidth);
	pLayout->addWidget(penLineType);
	pLayout->addWidget(m_pCurrentLineType);
	pLayout->setStretch(0, 1);
	pLayout->setStretch(1, 2);
	pLayout->setStretch(2, 1);
	pLayout->setStretch(3, 3);
	pLayout->setStretch(4, 1);
	pLayout->setStretch(5, 3);

	connect(m_pCurrentColor, SIGNAL(colorChanged(const DmColor&)), this, SLOT(slotSelectChanged()));
	connect(m_pCurrentWidth, SIGNAL(widthChanged(DM::LineWidth)), this, SLOT(slotSelectChanged()));
	connect(m_pCurrentLineType, SIGNAL(lineTypeChanged(DmLineType*)), this, SLOT(slotSelectChanged()));
}

UICurrentActivePen::~UICurrentActivePen()
{
}

void UICurrentActivePen::setPen(DmDocument* doc)
{
	auto activePen = doc->getActivePen();
	m_pCurrentColor->setColor(activePen.getColor());
	m_pCurrentWidth->setWidth(activePen.getWidth());
	m_pCurrentLineType->setLineType(activePen.getLineType(), doc);
}

DmLineType* UICurrentActivePen::getLineType()
{
	return m_pCurrentLineType->getLineType();
}

DM::LineWidth UICurrentActivePen::getLineWidth()
{
	return m_pCurrentWidth->getWidth();
}

DmColor UICurrentActivePen::getColor()
{
	return m_pCurrentColor->getColor();
}

void UICurrentActivePen::update(DmDocument* doc)
{
    DmPen pen = doc->getActivePen();
    m_pCurrentColor->setColor(pen.getColor());
    m_pCurrentWidth->setWidth(pen.getWidth());
	m_pCurrentLineType->updateLineTypeTable(doc);
}

void UICurrentActivePen::slotSelectChanged()
{
	//修改当前文档的当前pen
	DmDocument* doc = ApplicationWindow::getAppWindow()->getDocument();
	DmPen pen(getColor(), getLineWidth(), getLineType());
    DmPen originPen = doc->getActivePen();
    
    TransactionGroup tg(tr("Modify current pen").toStdString(), doc);
    tg.start();
    Transaction t(tr("Modify current pen").toStdString(), doc);
    t.start();
    ModifyDocPenCmd* cmd = new ModifyDocPenCmd(doc, pen, originPen);
    doc->getCmdManager()->addAndExecuteCmd(cmd);
    if (sender() == m_pCurrentLineType)
    {
        doc->getLineTypeTable()->activate(m_pCurrentLineType->getLineType());
    }
    t.commit();
    
	//修改当前选择实体的pen
	unsigned int selectedCount = doc->getEntityTable()->countSelect();
	if (selectedCount == 0)
	{
        tg.commit();
		return;
	}

    Transaction t2(tr("Modify current selected entities").toStdString(), doc);
    t2.start();
    EntityTable* entityTable = doc->getEntityTable();
	for (auto& e : *entityTable)
	{
		if (e->isSelected())
		{
            entityTable->startModify(e);
			e->setPen(pen);
			//对于简单实体（比如直线）不用update()。
			//但是对于文字（DmText）这种包含DmBlockReference的实体，里面的DmBlockReference的子实体颜色要从DmText及DmBlock获得，因此要update()
			e->update();
		}
	}
    t2.commit();
    tg.commit();
}
