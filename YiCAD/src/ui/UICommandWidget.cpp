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

/// @file UICommandWidget.cpp
/// @brief 命令行输入控件，包含命令编辑栏、信息面板、临时提示面板和命令自动补全功能

#include "UICommandWidget.h"
#include <QPropertyAnimation>
#include <QStringListModel>
#include <QIcon>
#include "QPalette"
#include "UIActionHandler.h"
#include "DmVector.h"
#include "DmDocument.h"
#include "Commands.h"
#include "QCompleter"
#include "QAbstractItemView"
#include <QPushButton>
#include "GuiDocumentView.h"
#include "GuiEventHandler.h"


UICommandWidget::UICommandWidget(QWidget* parent, UITabDrawWidget* tabDrawWidget)
 	:QWidget(parent)
	, m_pWidget(parent)
	, m_pTabDrawWidget(tabDrawWidget)
	, m_pActionHandler(nullptr)
{
	this->lower();
	m_cmdWin = new QWidget(m_pWidget);
	m_infoWin = new QWidget(m_pWidget);
	m_pCmdTempWin = new QWidget(m_pWidget);
	m_infoTextEdit = new QTextEdit(m_infoWin);
	m_pTempTextEdit = new QTextEdit(m_pCmdTempWin);

	//-------------------------------------------------输入栏------------------------------------------------------------------
	m_cmdWin->setAutoFillBackground(true);

	//--------------------------------------------------输入栏图标-----------------------------------------------------------
	QToolButton* setBtn = new QToolButton(m_cmdWin);
	setBtn->resize(20, 20);
	setBtn->setIconSize(QSize(20, 20));
	setBtn->move(5, 5);
	setBtn->setIcon(QIcon(":/ribbon/cmd/cmd.svg"));
	setBtn->setToolTip(tr("Command"));
	setBtn->setStyleSheet("border: none lightgrey;");

	//----------------------------------------------------信息板-----------------------------------------------------------------
	m_infoWin->lower();
	m_infoWin->hide();
	m_infoTextEdit->resize(400, 100);
	m_infoTextEdit->setFixedSize(400, 100);
	m_infoTextEdit->setStyleSheet("border:none lightgrey");
	m_infoTextEdit->setReadOnly(true);

	//----------------------------------------------------临时信息板----------------------------------------------------------------
	m_pTempTextEdit->lower();
	m_pTempTextEdit->setReadOnly(true);
	m_pTempTextEdit->resize(400, 50);
	m_pTempTextEdit->document()->setMaximumBlockCount(3);

	//----------------------------------------------------------------------------------------------------------------------------------
	createLineEdit();
	createTriangleBtn();

	m_pCmdTempWin->lower();
	m_pCmdTempWin->hide();
}

UICommandWidget::~UICommandWidget()
{
	if (editWidget)
	{
		delete editWidget;
		editWidget = nullptr;
	}

	if (m_pCompleter)
	{
		delete m_pCompleter;
		m_pCompleter = nullptr;
	}
}

void UICommandWidget::createTriangleBtn()
{
	QToolButton* btn = new QToolButton(m_cmdWin);
	btn->resize(20, 20);
	btn->move(375, 5);
	btn->setAutoFillBackground(true);
	btn->setIcon(QIcon(":/ribbon/cmd/arrow_up.svg"));
	btn->setIconSize(QSize(20, 20));
	btn->setToolTip(tr("Show command history"));
	//btn->setStyleSheet("border: none lightgrey;");
	//btn->setArrowType(Qt::ArrowType::UpArrow);
	connect(btn, SIGNAL(clicked()), this, SLOT(btnShowTextEdit()));
}

void UICommandWidget::createLineEdit()
{
	m_pEdit = new QLineEdit(m_cmdWin);
	m_pEdit->resize(345, 20);
	m_pEdit->move(30, 5);
	m_pEdit->setPlaceholderText(tr("Command"));
	m_pEdit->setAutoFillBackground(true);
	connect(m_pEdit, SIGNAL(returnPressed()), this, SLOT(pressShowTextEdit()));
}

QWidget* UICommandWidget::getCommandWidget()
{
	return m_cmdWin;
}

QWidget* UICommandWidget::getInfoWidget()
{
	return m_infoWin;
}

QWidget* UICommandWidget::getTempWidget()
{
	return m_pCmdTempWin;
}

void UICommandWidget::appCmdTempText(const QString text)
{
	if (!text.isEmpty())
	{
		MDIWindow* mdiWindow = m_pTabDrawWidget->getCurrentMdiWindow();
		GuiDocumentView* gv = mdiWindow->getDocumentView();
		GuiEventHandler* handle = gv->getEventHandler();
		int actionNum = handle->getCurrentActionNum();

		QString displayText = text;
		ActionInterface* action = handle->getCurrentAction();
		if (action)
		{
			DM::ActionType actionType = action->getEntityType();
			if (actionType != DM::ActionNone)
			{
				QString desc = COMMANDS->description(actionType);
				if (!desc.isEmpty())
				{
					displayText = QString("[%1] %2").arg(desc, text);
				}
			}
		}

		if (displayText == m_pLastText)
		{
			return;
		}

		if (actionNum == 0)
		{
			editWidget->show();
			editWidget->raise();
		}

		m_editline->clearFocus();
		m_editline->setPlaceholderText(displayText);
		m_pEdit->setPlaceholderText(m_editline->text() + ":" + displayText);
		m_cmdInfo = displayText;
		m_pCmdTempWin->raise();
		m_pCmdTempWin->show();
		m_infoWin->hide();

		m_pAnimation.reset(new QPropertyAnimation(m_pCmdTempWin, "windowOpacity"));
		m_pAnimation->setDuration(5000);
		m_pAnimation->setStartValue(1);
		m_pAnimation->setEndValue(0);
		m_pAnimation->start();
		m_pAnimation->setEasingCurve(QEasingCurve::OutBounce);  // 缓和曲线风格
		connect(m_pAnimation.get(), SIGNAL(finished()), m_pCmdTempWin, SLOT(lower()));

		m_pLastText = displayText;
		m_pTempTextEdit->append(displayText);
		m_infoTextEdit->append(displayText);
		QTextDocument* document = m_pTempTextEdit->document();
		int Area = document->size().width() * document->size().height();
		int NewHeight = Area / m_pTempTextEdit->width();

		m_pTempTextEdit->setFixedHeight(NewHeight + 5);

		m_pCmdTempWin->move((m_pCmdTempWin->parentWidget()->width() - m_cmdWin->width()) * 0.5, m_pCmdTempWin->parentWidget()->height() - 58 - NewHeight - 5);
		m_pCmdTempWin->resize(m_pTempTextEdit->width(), m_pTempTextEdit->height());
	}
}

void UICommandWidget::setActionHandler(UIActionHandler* pActionHandler)
{
	m_pActionHandler = pActionHandler;
}

void UICommandWidget::pressShowTextEdit()
{
	m_LineText = m_pEdit->text();
	if (m_LineText.isEmpty())
	{
		return;
	}
	m_infoTextEdit->append(m_LineText);
	createTempWin(m_pEdit);
	m_pEdit->setText("");
}

void UICommandWidget::pressShowLineEdit()
{
	if (!m_editline->hasFocus())
	{
		return;
	}
	m_LineText = m_editline->text();
	if (m_LineText.isEmpty())
	{
		return;
	}
	m_infoTextEdit->append(m_LineText);
	createTempWin(m_editline);
	m_editline->clear();
}

void UICommandWidget::btnShowTextEdit()
{
	if (m_infoWin->isHidden())
	{
		m_infoWin->raise();
		m_infoWin->show();
	}
	else
	{
		m_infoWin->lower();
		m_infoWin->hide();
	}
}

void UICommandWidget::createTempWin(QLineEdit* e)
{
	m_pCmdTempWin->raise();
	m_pCmdTempWin->show();
	m_infoWin->hide();
	m_pAnimation.reset(new QPropertyAnimation(m_pCmdTempWin, "windowOpacity"));
	m_pAnimation->setDuration(5000);
	m_pAnimation->setStartValue(1);
	m_pAnimation->setEndValue(0);
	m_pAnimation->start();
	m_pAnimation->setEasingCurve(QEasingCurve::OutBounce);  // 缓和曲线风格
	connect(m_pAnimation.get(), SIGNAL(finished()), m_pCmdTempWin, SLOT(lower()));

	m_LineText = e->text();
	if (m_LineText.isEmpty())
	{
		return;
	}

	m_pTempTextEdit->append(m_LineText);
	m_pCmdTempWin->show();
	QTextDocument* document = m_pTempTextEdit->document();
	int Area = document->size().width() * document->size().height();
	int NewHeight = Area / m_pTempTextEdit->width();

	m_pTempTextEdit->setFixedHeight(NewHeight + 5);
	m_pCmdTempWin->move((m_pCmdTempWin->parentWidget()->width() - m_cmdWin->width()) * 0.5, m_pCmdTempWin->parentWidget()->height() - 58 - NewHeight - 5);
	m_pCmdTempWin->resize(m_pTempTextEdit->width(), m_pTempTextEdit->height());

	if (m_pActionHandler != nullptr)
	{
		bool isAction = false;
		isAction = m_pActionHandler->command(m_LineText);
		if (!isAction && !(m_LineText.contains(',') || m_LineText.at(0) == '@'))
		{
			appCmdTempText(tr("Unknown command"));
		}
	}
}

void UICommandWidget::setCompleterStrings(const QStringList& strs)
{
	m_completerStrings = strs;
	updateCompleterModel();
}

void UICommandWidget::setExternalCommandStrings(const QStringList& commands)
{
	m_externalCommandStrings = commands;
	updateCompleterModel();
}

void UICommandWidget::updateCompleterModel()
{
	if (!m_pCompleter)
	{
		return;
	}

	auto* model = qobject_cast<QStringListModel*>(m_pCompleter->model());
	if (!model)
	{
		return;
	}

	QStringList commands = m_completerStrings;
	commands.append(m_externalCommandStrings);
	commands.removeDuplicates();
	model->setStringList(commands);
}

QWidget* UICommandWidget::createTempEdit()
{
	editWidget = new QWidget(m_pWidget);
	editWidget->setAutoFillBackground(true);
	m_editline = new QLineEdit(editWidget);
	m_editline->resize(200, 25);
	m_editline->setAutoFillBackground(true);
	connect(m_editline, SIGNAL(returnPressed()), this, SLOT(pressShowLineEdit()));

	std::map<QString, DM::ActionType> cmdTranslation = COMMANDS->getActionCommands();
	m_completerStrings.clear();
	for (auto ite = cmdTranslation.begin(); ite != cmdTranslation.end(); ite++)
	{
		m_completerStrings.push_back(ite->first);
	}
	m_pCompleter = new QCompleter();
	QStringList commands = m_completerStrings;
	commands.append(m_externalCommandStrings);
	commands.removeDuplicates();
	QStringListModel* model = new QStringListModel(commands,m_pCompleter);
	m_pCompleter->setModel(model);
	m_pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
	m_pCompleter->setCompletionMode(QCompleter::PopupCompletion);
	m_editline->setCompleter(m_pCompleter);
	editWidget->resize(200, 25);
	editWidget->lower();
	editWidget->hide();
	return editWidget;
}

QWidget* UICommandWidget::getTipWidget()
{
	m_pTipWidget = new QWidget(m_pWidget);
	m_pTipWidget->setAutoFillBackground(true);

	m_pTipWin = new QTextEdit(m_pTipWidget);
	m_pTipWin->resize(150, 100);
	m_pTipWin->setFixedSize(150, 100);
	m_pTipWin->setStyleSheet("border:none lightgrey");
	m_pTipWin->setReadOnly(true);

	m_pTipWidget->resize(150, 100);
	m_pTipWidget->hide();
	m_pTipWidget->lower();
	return m_pTipWidget;
}

void UICommandWidget::doNothing()
{
	m_editline->setText("");
	return;
}

QLineEdit* UICommandWidget::getEditline()
{
	return m_editline;
}

QLineEdit* UICommandWidget::getEdit()
{
	return m_pEdit;
}

QTextEdit* UICommandWidget::getTipWin()
{
	return m_pTipWin;
}
