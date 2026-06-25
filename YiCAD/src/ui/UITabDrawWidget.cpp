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

/// @file UITabDrawWidget.cpp
/// @brief 绘图选项卡控件，管理多个绘图画布的切换、新建、关闭、保存等操作

#include "UITabDrawWidget.h"

#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTabBar>
#include <QSvgGenerator>
#include <QPrinter>
#include <QPrintDialog>
#include <QHBoxLayout>

#include "DmDocument.h"
#include "GuiDocumentView.h"
#include "MDIWindow.h"
#include "UIActionHandler.h"
#include "DmPen.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmColor.h"
#include "UIBottomWidget.h"
#include "UIExitDialog.h"
#include "DmSettings.h"
#include "UIFileDialog.h"
#include "DmSystem.h"
#include "UIDialogFactory.h"
#include "Printing.h"
#include "GuiDialogFactory.h"
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QImageWriter>
#include <QPainter>
#include "UICurrentActivePen.h"
#include "DmBlockReference.h"

#include "Selection.h"
#include <set>
#include "qevent.h"

namespace
{
constexpr int kTabBarHeight = 25;
constexpr int kPenBarHeight = 28;
constexpr int kNewTabButtonWidth = 25;
constexpr int kDefaultTabWidth = 128;
constexpr int kMinimumTabWidth = 78;
constexpr int kTabCloseButtonSize = 20;
constexpr int kTabCloseIconSize = 12;
constexpr int kTabCloseButtonRightMargin = 4;
}

UITabDrawWidget::UITabDrawWidget(QWidget* parent)
	: m_pWidget(parent)
	, m_currentMdiWindow(nullptr)
	, m_pTabDrawList(new std::vector<SingleTabDrawDataRibbon*>())
	, m_pBackTabDrawWidget(nullptr)
	, m_pPenWidget(nullptr)
	, m_pDrawBackWidget(nullptr)
	, m_pActionHandler(nullptr)
	, m_pBottomWidget(nullptr)
{
}

UITabDrawWidget::~UITabDrawWidget()
{
	if (m_pTabDrawList)
	{
		m_pTabDrawList->clear();
		delete m_pTabDrawList;
		m_pTabDrawList = nullptr;
	}
}

QWidget* UITabDrawWidget::createTabDrawWidget(QMdiArea* drawBackWidget, UIActionHandler* pActionHandler, UIBottomWindow* pBottomWidget)
{
 	m_pDrawBackWidget = drawBackWidget;
	m_pActionHandler = pActionHandler;
	m_pBottomWidget = pBottomWidget;

	// 底板
	QWidget* tabDrawWidget = new QWidget(m_pWidget);
	m_pBackTabDrawWidget = tabDrawWidget;
	tabDrawWidget->move(1, 160);
	tabDrawWidget->resize(1920, kTabBarHeight);
	tabDrawWidget->setAutoFillBackground(true);
	tabDrawWidget->setObjectName("drawingTabBarWidget");

	// 选项卡
	SingleTabDrawDataRibbon* singleTabDraw = new SingleTabDrawDataRibbon();
	singleTabDraw->tabBack = tabDrawWidget;
	singleTabDraw->name = "Drawing1";
	singleTabDraw->fullPath = "Drawing1.ocd";
	singleTabDraw->number = 1;
	singleTabDraw->mdiWindow = createMdiWindow();
	singleTabDraw->isCurrent = true;
	m_currentMdiWindow = singleTabDraw->mdiWindow;
	newTabDraw(singleTabDraw);

	// 新增按钮
	QToolButton* newDraw = new QToolButton(tabDrawWidget);
	newDraw->setProperty("isDrawingTabNewButton", true);
	newDraw->resize(kNewTabButtonWidth, kTabBarHeight);
	newDraw->setAutoRaise(true);
	newDraw->setFocusPolicy(Qt::NoFocus);
	newDraw->setIcon(QIcon(":/ribbon/tabbar/new.svg"));
	newDraw->setIconSize(QSize(13, 13));
	connect(newDraw, SIGNAL(clicked()), m_pActionHandler, SLOT(slotFileNew()));

	// docView绘图区域的当前画笔栏
	m_pPenWidget = new QWidget(m_pWidget);
	m_pPenWidget->move(1, 186);
	m_pPenWidget->resize(700, kPenBarHeight);
	m_pPenWidget->setAutoFillBackground(true);
	QHBoxLayout* penLayout = new QHBoxLayout(m_pPenWidget);
	penLayout->setContentsMargins(0, 0, 0, 0);
	penLayout->setSpacing(0);
 	m_pCurrentActivePen = new UICurrentActivePen(m_pPenWidget);
	penLayout->addWidget(m_pCurrentActivePen, 0, Qt::AlignLeft | Qt::AlignVCenter);
	penLayout->addStretch();
	m_pCurrentActivePen->setPen(m_currentMdiWindow->getDocument());

	return tabDrawWidget;
}

void UITabDrawWidget::layoutWidgets(int contentTop, int windowWidth, int windowHeight, int bottomHeight)
{
	if (m_pBackTabDrawWidget == nullptr || m_pDrawBackWidget == nullptr)
	{
		return;
	}

	const int safeTop = qMax(0, contentTop);
	const int safeWidth = qMax(0, windowWidth);
	const int safeBottomHeight = qMax(0, bottomHeight);

	m_pBackTabDrawWidget->setGeometry(0, safeTop, safeWidth, kTabBarHeight);

	const int drawingTop = safeTop + kTabBarHeight;
	if (m_pPenWidget)
	{
		const int preferredPenWidth = m_pCurrentActivePen
			? qMax(m_pCurrentActivePen->minimumWidth(), m_pCurrentActivePen->sizeHint().width())
			: m_pPenWidget->sizeHint().width();
		const int penWidth = qMin(safeWidth, qMax(0, preferredPenWidth));
		m_pPenWidget->setGeometry(0, drawingTop, penWidth, kPenBarHeight);
	}

	const int drawingHeight = qMax(0, windowHeight - drawingTop - safeBottomHeight);
	m_pDrawBackWidget->setGeometry(0, drawingTop, safeWidth, drawingHeight);

	for (auto& tabData : *m_pTabDrawList)
	{
		if (tabData->mdiWindow)
		{
			tabData->mdiWindow->resize(m_pDrawBackWidget->size());
		}
	}

	layoutTabButtons();
	m_pBackTabDrawWidget->raise();
	if (m_pPenWidget)
	{
		m_pPenWidget->raise();
	}
}

MDIWindow* UITabDrawWidget::getCurrentMdiWindow()
{
	return m_currentMdiWindow;
}

SingleTabDrawDataRibbon* UITabDrawWidget::getCurrentTabDrawData()
{
	for (auto& item : *m_pTabDrawList)
	{
		if (m_currentMdiWindow == item->mdiWindow)
		{
			return item;
		}
	}

	return nullptr;
}

SingleTabDrawDataRibbon* UITabDrawWidget::getTabDrawDataOfDocument(const DmDocument* document) const
{
	for (auto& item : *m_pTabDrawList)
	{
		if (item->mdiWindow->getDocument() == document)
		{
			return item;
		}
	}
	return nullptr;
}


std::vector<SingleTabDrawDataRibbon*>* UITabDrawWidget::getTabDrawList()
{
	return m_pTabDrawList;
}

std::vector<DmDocument*> UITabDrawWidget::getDocuments() const
{
	std::vector<DmDocument*> docs;
	for (auto doc : *m_pTabDrawList)
	{
		docs.emplace_back(doc->mdiWindow->getDocument());
	}
	return docs;
}

void UITabDrawWidget::newTabDraw(SingleTabDrawDataRibbon* newTab)
{
	// 选项卡
	QWidget* drawWidget = new QWidget(newTab->tabBack);
	drawWidget->setProperty("isDrawingTab", true);
	drawWidget->resize(kDefaultTabWidth, kTabBarHeight);
	drawWidget->move(static_cast<int>(m_pTabDrawList->size()) * kDefaultTabWidth + kNewTabButtonWidth, 0);
	m_currentBtnWidget = drawWidget;
	m_currentPos = drawWidget->pos();
	if (newTab->isCurrent)
	{
		m_currentMdiWindow = newTab->mdiWindow;
	}

	drawWidget->show();
	newTab->theWidget = drawWidget;

	// 选项卡名字
	QPushButton* textBtn = new QPushButton(drawWidget);
	textBtn->setProperty("isDrawingTabText", true);
	textBtn->setFlat(true);
	textBtn->setFocusPolicy(Qt::NoFocus);
	const int initialCloseX = kDefaultTabWidth - kTabCloseButtonSize - kTabCloseButtonRightMargin;
	textBtn->resize(initialCloseX, kTabBarHeight);
	textBtn->show();
	// 超长的文字自动显示省略号
	QFontMetrics elideFont(textBtn->font());
	textBtn->setText(elideFont.elidedText(newTab->name, Qt::ElideMiddle, textBtn->width())); //省略号显示在右边
	textBtn->setToolTip(newTab->fullPath);
	connect(textBtn, &QPushButton::clicked, drawWidget, [this, textBtn, drawWidget] {
		// 设置其他按钮为不选中
		for (int i = 0; i < m_pTabDrawList->size(); ++i)
		{
			SingleTabDrawDataRibbon* tabData = (*m_pTabDrawList)[i];
			if (tabData->textBtn == textBtn)
			{
				if (false == tabData->isCurrent)
				{
					tabData->isCurrent = true;

					// 调整当前画布大小
					tabData->mdiWindow->resize(m_pDrawBackWidget->width(), m_pDrawBackWidget->height());
					// 显示画布
					tabData->mdiWindow->show();
					// 将这个画布设为当前
					m_currentMdiWindow = tabData->mdiWindow;
					setChangeTabDrawArea();
				}
			}
			else
			{
				tabData->isCurrent = false;

				// 画布隐藏
				tabData->mdiWindow->hide();
			}

			// 刷新stylesheet
			updateTabStyleSheet(tabData);
		}
		m_pActionHandler->slotSetSnaps(m_pBottomWidget->getSnapToolBar()->getSnaps());
	});
	newTab->textBtn = textBtn;
	textBtn->installEventFilter(this);
	// 关闭按钮
	QPushButton* closeDraw = new QPushButton(drawWidget);
	closeDraw->setProperty("isTabCloseButton", true);
	closeDraw->setFlat(true);
	closeDraw->setFocusPolicy(Qt::NoFocus);
	closeDraw->setIcon(QIcon(":/ribbon/tabbar/close.svg"));
	closeDraw->setIconSize(QSize(kTabCloseIconSize, kTabCloseIconSize));
	closeDraw->resize(kTabCloseButtonSize, kTabCloseButtonSize);
	closeDraw->move(initialCloseX, (kTabBarHeight - kTabCloseButtonSize) / 2);
	closeDraw->show();
	connect(closeDraw, &QPushButton::clicked, newTab->tabBack, [this, newTab, drawWidget, closeDraw] {

		QList<MDIWindow*> childMdi = newTab->mdiWindow->getChildWindows();
		for (auto ite = childMdi.begin(); ite != childMdi.end(); ite++)
		{
			for (int i = 0; i < m_pTabDrawList->size(); ++i)
			{
				SingleTabDrawDataRibbon* tabData = (*m_pTabDrawList)[i];
				if (tabData->mdiWindow == (*ite))
				{
					closeTab(tabData);
				}
			}
		}
		closeTab(newTab);
		layoutTabButtons();
	});
	newTab->closeDraw = closeDraw;

	// 初始化画布
	newTab->mdiWindow->resize(m_pDrawBackWidget->width(), m_pDrawBackWidget->height());
	newTab->mdiWindow->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);

	if (newTab->isCurrent)
	{
		// 设置其他按钮为不选中
		for (int i = 0; i < m_pTabDrawList->size(); ++i)
		{
			SingleTabDrawDataRibbon* tabData = (*m_pTabDrawList)[i];
			if (tabData->isCurrent)
			{
				tabData->isCurrent = false;
				// 刷新stylesheet
				updateTabStyleSheet(tabData);
			}

			// 设置不选中的画布隐藏
			tabData->mdiWindow->hide();
		}
	}
	// 刷新stylesheet
	updateTabStyleSheet(newTab);
	m_pTabDrawList->emplace_back(newTab);
	m_btnIndex = static_cast<int>((*m_pTabDrawList).size()) - 1;
	layoutTabButtons();
}

void UITabDrawWidget::renameTabDraw(SingleTabDrawDataRibbon* tab, const QString& fileName, const QString& fullFileName)
{
	QString copyFileName = fileName;
	int index;
	index = copyFileName.lastIndexOf(".");
	copyFileName.truncate(index);
	tab->name = copyFileName;
	tab->fullPath = fullFileName;

	// 过长名字显示省略号
	QFontMetrics elideFont(tab->textBtn->font());
	tab->textBtn->setText(elideFont.elidedText(copyFileName, Qt::ElideMiddle, tab->textBtn->width()));
	tab->textBtn->setToolTip(fullFileName);
}

void UITabDrawWidget::closeTab(SingleTabDrawDataRibbon* newTab)
{
	bool isCancel = slotFileClosing(newTab->mdiWindow);
	if (isCancel)
	{
		return;
	}

	// 移动受影响的选项卡
	int numb = -1;
	for (int i = 0; i < m_pTabDrawList->size(); ++i)
	{
		SingleTabDrawDataRibbon* tabData = (*m_pTabDrawList)[i];
		if (tabData->closeDraw == newTab->closeDraw)
		{
			numb = i;
		}

		if (numb != -1 && i > numb)
		{
			auto pos_x = tabData->theWidget->pos().x();
			auto width = tabData->theWidget->width();
			tabData->theWidget->move(pos_x - width, 0);
		}
	}

	// 如果关闭的是当前选中的选项卡清空当前画布
	if ((*m_pTabDrawList)[numb]->isCurrent)
	{
		m_currentMdiWindow = nullptr;
	}

	// 删除数组对应元素
	if (numb != -1)
	{
		// 释放资源
		delete newTab->mdiWindow;
		delete newTab->textBtn;
		delete newTab->closeDraw;
		newTab->theWidget->hide();
		if (m_pTabDrawList->size() == 1)
		{
			delete newTab->theWidget;
		}
		delete newTab;

		(*m_pTabDrawList).erase((*m_pTabDrawList).begin() + numb);
	}

	// 如果还有选项卡,选中状态移动到前一个选项卡
	if (m_currentMdiWindow == nullptr && (*m_pTabDrawList).size() > 0 && numb != -1)
	{
		if (numb == 0)
		{
			numb = 1;
		}

		SingleTabDrawDataRibbon* preTabData = (*m_pTabDrawList)[(size_t)numb - 1];
		preTabData->isCurrent = true;
		updateTabStyleSheet(preTabData);
		// 调整当前画布大小
		preTabData->mdiWindow->resize(m_pDrawBackWidget->width(), m_pDrawBackWidget->height());
		// 显示画布
		preTabData->mdiWindow->show();
		// 将这个画布设为当前
		m_currentMdiWindow = (*m_pTabDrawList)[(size_t)numb - 1]->mdiWindow;
		setChangeTabDrawArea();
	}
	else if (m_currentMdiWindow == nullptr && (*m_pTabDrawList).size() == 0)
	{
		// 底部状态栏置灰
		tabClearEvent();
	}
	tabChangeEvent();
}

MDIWindow* UITabDrawWidget::createMdiWindow()
{
	MDIWindow* w = new MDIWindow(nullptr, m_pDrawBackWidget, Qt::WindowType::Widget);
	GuiDocumentView* view = w->getDocumentView();
	connect(view, SIGNAL(selectedChanged()), m_pActionHandler, SLOT(slotSecectedChanged()));
    connect(view, SIGNAL(selectedChanged()), ApplicationWindow::getAppWindow(), SLOT(updateLayerTable()));
	m_pActionHandler->set_view(view);
	m_pActionHandler->set_document(w->getDocument());
	DmDocument* document = w->getDocument();

    connect(document->getCmdManager(), SIGNAL(cmdChanged()), ApplicationWindow::getAppWindow(), SLOT(updateUndoRedo()));
    connect(document->getCmdManager(), SIGNAL(layerModified()), ApplicationWindow::getAppWindow(), SLOT(updateLayerTable()));
    connect(document->getCmdManager(), SIGNAL(activePenModified()), ApplicationWindow::getAppWindow(), SLOT(updateCurrentPenWidget()));

    return w;
}

QWidget* UITabDrawWidget::getBackTabDrawWidget()
{
	return m_pBackTabDrawWidget;
}

void UITabDrawWidget::closeTabAll()
{
	slotFileCloseAll();

	// 顶部/左右菜单栏关闭或置灰
	tabClearEvent();
}

UICurrentActivePen* UITabDrawWidget::getCurrentActivePen()
{
	return m_pCurrentActivePen;
}

void UITabDrawWidget::slotFileNew(const QString& fileName)
{
	SingleTabDrawDataRibbon* addSingleTabDraw = new SingleTabDrawDataRibbon();
	addSingleTabDraw->tabBack = m_pBackTabDrawWidget;

	// 为空表示新建文档
	if (fileName.isEmpty())
	{
		addSingleTabDraw->name = "Drawing" + QString::number(m_pTabDrawList->size() + 1);
		addSingleTabDraw->fullPath = addSingleTabDraw->name;
	}
	// 不为空表示（打开文件前）新建文档（后面填入数据）
	else
	{
		QFileInfo fileInfo(fileName);
		if (fileInfo.exists())
		{
			QString name = fileInfo.fileName();
			int index = name.lastIndexOf(".");
			name.truncate(index);
			addSingleTabDraw->name = name;
		}
		else
		{
			addSingleTabDraw->name = fileName;
		}
		addSingleTabDraw->fullPath = fileName;
	}
	addSingleTabDraw->number = m_pTabDrawList->size();
	addSingleTabDraw->mdiWindow = createMdiWindow();
	addSingleTabDraw->isCurrent = true;
	m_currentMdiWindow = addSingleTabDraw->mdiWindow;

	newTabDraw(addSingleTabDraw);
	tabChangeEvent();
	m_currentMdiWindow->show();
	m_pActionHandler->slotSetSnaps(m_pBottomWidget->getSnapToolBar()->getSnaps());
	m_pCurrentActivePen->setPen(m_currentMdiWindow->getDocument());
}

void UITabDrawWidget::soltSetDrawingTabName(const QString& fileName)
{
	QFileInfo file(fileName);
	auto name = file.fileName();
	QString copyName = name;
	int index = copyName.lastIndexOf(".");
	copyName.truncate(index);
	for (int i = 0; i < m_pTabDrawList->size(); ++i)
	{
		SingleTabDrawDataRibbon* tabData = (*m_pTabDrawList)[i];
		if (tabData->isCurrent == true)
		{
			tabData->name = copyName;
			tabData->textBtn->setText(copyName);

			// 超长的文字自动显示省略号
			QFontMetrics elideFont(tabData->textBtn->font());
			tabData->textBtn->setText(elideFont.elidedText(tabData->textBtn->text(), Qt::ElideMiddle, tabData->textBtn->width())); //省略号显示在右边
			tabData->textBtn->setToolTip(fileName);
		}
	}
}

void UITabDrawWidget::slotFileOpen()
{
	UIFileDialog dlg(this);
 	QString fileName = dlg.getOpenFile();
	slotFileOpen(fileName);
	tabChangeEvent();
}

void UITabDrawWidget::slotFileExportImage()
{
	MDIWindow* w = m_currentMdiWindow;
	QString fn;
	if (w)
	{
		auto view = w->getDocumentView();
		view->setIsDrawCursor(false);
		view->update();

		// read default settings:
		DMSETTINGS->beginGroup("/Export");
		QString defDir = DMSETTINGS->readEntry("/ExportImage", DMSYSTEM->getHomeDir());
		QString defFilter = DMSETTINGS->readEntry("/ExportImageFilter", QString("%1 (%2)(*.%2)").arg(UIDialogFactory::extToFormat("png")).arg("png"));
		DMSETTINGS->endGroup();

		bool cancel = false;

		QStringList filters;
		QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();

		for (QString format : supportedImageFormats)
		{
			format = format.toLower();
			QString st;
			if (format == "jpeg" || format == "tiff")
			{
				// Don't add the aliases
			}
			else
			{
				st = QString("%1 (%2)(*.%2)")
					.arg(UIDialogFactory::extToFormat(format))
					.arg(format);
			}
			if (st.length() > 0)
			{
				filters.push_back(st);
			}
		}
		// revise list of filters
		filters.removeDuplicates();
		filters.sort();

		// set dialog options: filters, mode, accept, directory, filename
		QFileDialog fileDlg(this, tr("Export as"));

		fileDlg.setNameFilters(filters);
		fileDlg.setFileMode(QFileDialog::AnyFile);
		fileDlg.selectNameFilter(defFilter);
		fileDlg.setAcceptMode(QFileDialog::AcceptSave);
		fileDlg.setDirectory(defDir);
		fn = QFileInfo(w->getDocument()->getFilename()).baseName();
		if (nullptr == fn)
		{
			fn = "unnamed";
		}
		fileDlg.selectFile(fn);

		if (fileDlg.exec() == QDialog::Accepted)
		{
			QStringList files = fileDlg.selectedFiles();
			if (!files.isEmpty())
			{
				fn = files[0];
			}
			cancel = false;
		}
		else
		{
			cancel = true;
		}

		// store new default settings:
		if (!cancel)
		{
			DMSETTINGS->beginGroup("/Export");
			DMSETTINGS->writeEntry("/ExportImage", QFileInfo(fn).absolutePath());
			DMSETTINGS->writeEntry("/ExportImageFilter", fileDlg.selectedNameFilter());
			DMSETTINGS->endGroup();

			// find out extension:
			QString filter = fileDlg.selectedNameFilter();
			QString format = "";
			int i = filter.indexOf("(*.");
			if (i != -1)
			{
				int i2 = filter.indexOf(QRegExp("[) ]"), i);
				format = filter.mid(i + 3, i2 - (i + 3));
				format = format.toUpper();
			}

			// append extension to file:
			if (!QFileInfo(fn).fileName().contains("."))
			{
				fn.push_back("." + format.toLower());
			}

			slotFileExport(fn, format, QSize(), QSize(), 0, 0);
		}

		view->setIsDrawCursor(true);
	}
}

void UITabDrawWidget::slotFilePrintPDF()
{
	slotFilePrint();
}

void UITabDrawWidget::slotFileSave()
{
	if (slotFileSave(m_currentMdiWindow))
	{
		// 更新最近文件菜单
		// recentFiles->updateRecentFilesMenu();
	}
}

void UITabDrawWidget::slotFileSaveAs()
{
	if (slotFileSave(m_currentMdiWindow, true))
	{
		// 更新最近文件菜单
		// recentFiles->updateRecentFilesMenu();
	}
}

void UITabDrawWidget::slotFileSaveAll()
{
	for (auto item : *m_pTabDrawList)
	{
		auto w = item->mdiWindow;
		auto result = true;
		if (w && w->getDocument()->isModified())
		{
			slotFileSave(w);
			if (!result)
			{
				//statusBar()->showMessage(tr("Save All cancelled"), 2000);
				break;
			}
		}
	}

	// 更新最近打开文件栏
	//recentFiles->updateRecentFilesMenu();
}

void UITabDrawWidget::slotFileCloseAll()
{
	if (m_pTabDrawList)
	{
		for (auto it = m_pTabDrawList->size(); it > 0; --it)
		{
			QList<MDIWindow*> childMdi = (*m_pTabDrawList)[it-1]->mdiWindow->getChildWindows();
			for (auto ite = childMdi.begin(); ite != childMdi.end(); ite++)
			{
				for (int i = 0; i < m_pTabDrawList->size(); ++i)
				{
					if ((*m_pTabDrawList)[i]->mdiWindow == (*ite))
					{
						closeTab((*m_pTabDrawList)[i]);
						it--;
					}
				}
			}
			closeTab((*m_pTabDrawList)[it - 1]);
		}
	}
}

int UITabDrawWidget::showCloseDialog(MDIWindow* w, bool showSaveAll)
{
	UIExitDialog dlg(this);
	dlg.setShowSaveAll(showSaveAll);
	dlg.setTitle(QObject::tr("Closing Drawing"));
	if (w && w->getDocument()->isModified())
	{
		QString fn = w->getDocument()->getFilename();
		if (fn.isEmpty())
		{
			fn = w->windowTitle();
		}
		else if (fn.length() > 50)
		{
			fn = QString("%1...%2").arg(fn.left(24)).arg(fn.right(24));
		}

		dlg.setText(QObject::tr("Save changes to the following item?\n%1").arg(fn));
		return dlg.exec();
	}
	return -1; // should never get here; please send only modified documents
}

void UITabDrawWidget::slotFileOpen(const QString& fileName)
{
	QSettings settings;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// 检查文件是否已经打开，如果是则切换到现有标签页
	QFileInfo openFileInfo(fileName);
	QString openFilePath = openFileInfo.canonicalFilePath();
	if (!openFilePath.isEmpty())
	{
		for (int i = 0; i < m_pTabDrawList->size(); ++i)
		{
			SingleTabDrawDataRibbon* tabData = (*m_pTabDrawList)[i];
			QString existingPath = QFileInfo(tabData->fullPath).canonicalFilePath();
			if (!existingPath.isEmpty() && existingPath == openFilePath)
			{
				// 文件已打开，切换到该标签页
				for (int j = 0; j < m_pTabDrawList->size(); ++j)
				{
					SingleTabDrawDataRibbon* td = (*m_pTabDrawList)[j];
					td->isCurrent = (td == tabData);
					if (td->isCurrent)
					{
						td->mdiWindow->resize(m_pDrawBackWidget->width(), m_pDrawBackWidget->height());
						td->mdiWindow->show();
						m_currentMdiWindow = td->mdiWindow;
					}
					else
					{
						td->mdiWindow->hide();
					}
					updateTabStyleSheet(td);
				}
				setChangeTabDrawArea();
				m_pActionHandler->slotSetSnaps(m_pBottomWidget->getSnapToolBar()->getSnaps());
				QApplication::restoreOverrideCursor();
				return;
			}
		}
	}

	SingleTabDrawDataRibbon* addSingleTabDraw = nullptr;
	bool success = false;	//是否导入成功


	if (QFileInfo(fileName).exists())
	{
		// 新建文档窗体
		addSingleTabDraw = new SingleTabDrawDataRibbon();
		addSingleTabDraw->tabBack = m_pBackTabDrawWidget;
		addSingleTabDraw->name = "Drawing" + QString::number(m_pTabDrawList->size());
		addSingleTabDraw->fullPath = addSingleTabDraw->name;
		addSingleTabDraw->number = m_pTabDrawList->size();
		addSingleTabDraw->mdiWindow = createMdiWindow();
		addSingleTabDraw->isCurrent = true;
		m_currentMdiWindow = addSingleTabDraw->mdiWindow;
		newTabDraw(addSingleTabDraw);
		addSingleTabDraw->textBtn->setToolTip(fileName);
		MDIWindow* w = addSingleTabDraw->mdiWindow;
		w->show();

		auto document = w->getDocument();
		bool res = w->slotFileOpen(fileName);
		if (res)
		{
			success = true;
			w->getDocumentView()->zoomAuto();
		}
		if (!success)
		{
			closeTab(addSingleTabDraw);
			QApplication::restoreOverrideCursor();
			return;
		}

		// 更新最近的文件菜单
		//recentFiles->add(fileName);
		//openedFiles.push_back(fileName);

		// 设置主窗体标题
		QFileInfo info = QFileInfo(fileName);
		auto simpFileName = info.fileName();
		//m_pTitleContent->setText(fileName); // todo:先不跟主窗体名称关联处理过于复杂
		// 设置选项卡名
		auto tabData = getCurrentTabDrawData();
		int index;
		index = simpFileName.lastIndexOf(".");
		simpFileName.truncate(index);
		// 超长的文字自动显示省略号
		QFontMetrics elideFont(tabData->textBtn->font());
		tabData->textBtn->setText(elideFont.elidedText(simpFileName, Qt::ElideMiddle, tabData->textBtn->width()));
		tabData->fullPath = fileName;
		tabData->textBtn->setToolTip(tabData->fullPath);


		DMSETTINGS->beginGroup("/CADPreferences");
		if (DMSETTINGS->readNumEntry("/AutoZoomDrawing"))
		{
			w->getDocumentView()->zoomAuto();
		}
		DMSETTINGS->endGroup();

		if (settings.value("Appearance/DraftMode", 0).toBool())
		{
			QString draft_string = " [" + tr("Draft Mode") + "]";
			w->getDocumentView()->setDraftMode(true);
			w->getDocumentView()->redraw();
			QString title = w->windowTitle();
			w->setWindowTitle(title + draft_string);
		}
	}
	QApplication::restoreOverrideCursor();

	if (addSingleTabDraw && success)
	{
		//重命名选项卡（可能打开的备份文件）。代码放到此处是因为QApplication::setOverrideCursor()调用之后设置无效
		QString fullFileName = addSingleTabDraw->mdiWindow->getDocument()->getFilename();
		QFileInfo finfo(fullFileName);
		QString fileName = finfo.fileName();
		renameTabDraw(addSingleTabDraw, fileName, fullFileName);
	}
}

bool UITabDrawWidget::slotFileExport(const QString& name, const QString& format, QSize size, QSize borders, bool black, bool bw)
{
	MDIWindow* w = m_currentMdiWindow;
	if (w == nullptr)
	{
		return false;
	}

	DmDocument* document = w->getDocument();
	if (document == nullptr)
	{
		return false;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	bool ret = false;

	// set vars for normal pictures and vectors (svg)
	QPixmap picture = QPixmap::grabWidget(w->getDocumentView(), w->getDocumentView()->rect());

	// end the picture output
	if (format.toLower() != "svg")
	{
		QImageWriter iio;
		QImage img = picture.toImage();
		iio.setFileName(name);
		iio.setFormat(format.toLatin1());
		if (iio.write(img))
		{
			ret = true;
		}
	}
	QApplication::restoreOverrideCursor();

	if (ret)
	{
		//statusBar()->showMessage(tr("Export complete"), 2000);
	}
	else
	{
		//statusBar()->showMessage(tr("Export failed!"), 2000);
	}

	return ret;
}

bool UITabDrawWidget::slotFileSave(MDIWindow* w, bool forceSaveAs)
{
	QString name, msg;
	bool cancelled;
	if (!w)
	{
		return false;
	}

	if (w->getDocument()->isModified() || forceSaveAs)
	{
		name = w->getDocument()->getFilename();

		msg = name.isEmpty() ? tr("Saving drawing...") : tr("Saving drawing: %1").arg(name);
		//statusBar()->showMessage(msg);
		bool res = forceSaveAs ? w->slotFileSaveAs(cancelled) : w->slotFileSave(cancelled);
		if (res)
		{
			if (cancelled)
			{
				//statusBar()->showMessage(tr("Save cancelled"), 2000);
				return false;
			}
			name = w->getDocument()->getFilename();
			msg = tr("Saved drawing: %1").arg(name);
			//statusBar()->showMessage(msg, 2000);

			QFileInfo info = QFileInfo(name);
			auto fileName = info.fileName();

			w->setWindowTitle(fileName + "[*]");
			if (w->getDocumentView()->isDraftMode())
			{
				w->setWindowTitle(w->windowTitle() + " [" + tr("Draft Mode") + "]");
			}

			// 设置选项卡名
			auto tabData = getCurrentTabDrawData();
			renameTabDraw(tabData, fileName, name);
		}
		else
		{
			msg = tr("Cannot save the file ") + w->getDocument()->getFilename()
				+ tr(" , please check the filename and permissions.");
			//statusBar()->showMessage(msg, 2000);
			//commandWidget->appendHistory(msg);
			return slotFileSave(w, true);
		}
	}
	return true;
}

void UITabDrawWidget::slotFilePrint()
{
	MDIWindow* w = m_currentMdiWindow;
	if (w == nullptr)
	{
		return;
	}

	DmDocument* document = w->getDocument();
	if (document == nullptr)
	{
		return;
	}

	QString strDefaultFile("");
	DMSETTINGS->beginGroup("/Print");
	strDefaultFile = DMSETTINGS->readEntry("/FileName", "");
	DMSETTINGS->endGroup();

	QFileInfo infDefaultFile(strDefaultFile);
	QFileDialog fileDlg(this, tr("Export as PDF"));
	QString defFilter("PDF files (*.pdf)");
	QStringList filters;

	filters << defFilter << "Any files (*)";

	fileDlg.setNameFilters(filters);
	fileDlg.setFileMode(QFileDialog::AnyFile);
	fileDlg.selectNameFilter(defFilter);
	fileDlg.setAcceptMode(QFileDialog::AcceptSave);
	fileDlg.setDefaultSuffix("pdf");
	fileDlg.setDirectory(infDefaultFile.dir().path());

	if (QDialog::Accepted == fileDlg.exec())
	{
		QStringList files = fileDlg.selectedFiles();
		if (!files.isEmpty())
		{
			if (!files[0].endsWith(R"(.pdf)", Qt::CaseInsensitive))
			{
				files[0] = files[0] + ".pdf";
			}

			auto gvPreview = new GuiDocumentView();
			gvPreview->setDocument(w->getDocument());
			gvPreview->zoomAuto();
			gvPreview->show();
			//gvPreview->exportView(files[0].toStdString().c_str());
			gvPreview->hide();

			delete gvPreview;
			gvPreview = nullptr;
		}
	}

	// statusBar()->showMessage(tr("Printing complete"), 2000);
}

void UITabDrawWidget::slotFilePrintPreview()
{
	auto currentTab = getCurrentTabDrawData();

	// 获取预览mdi
	if (!m_currentMdiWindow)
	{
		return;
	}
	if (!m_currentMdiWindow->getDocumentView()->isPrintPreview())
	{
		QSettings settings;
		//generate a new print preview

		MDIWindow* w = new MDIWindow(m_currentMdiWindow->getDocument(), m_pDrawBackWidget, Qt::WindowFlags());
		m_pDrawBackWidget->addSubWindow(w);
		m_currentMdiWindow->addChildWindow(w);

		w->setWindowTitle(tr("Print preview for %1").arg(m_currentMdiWindow->windowTitle()));
		GuiDocumentView* docView = w->getDocumentView();
		docView->setStrDevice(settings.value("Hardware/Device", "Mouse").toString());
		docView->setPrintPreview(true);
		docView->setBackground(QColor(255, 255, 255));
		
		// 创建一个预览mdiwindow
		SingleTabDrawDataRibbon* addSingleTabDraw = new SingleTabDrawDataRibbon();
		addSingleTabDraw->tabBack = m_pBackTabDrawWidget;
		addSingleTabDraw->name = currentTab->name + QObject::tr("Print Preview");
		addSingleTabDraw->number = m_pTabDrawList->size();
		addSingleTabDraw->mdiWindow = w;
		addSingleTabDraw->isCurrent = true;
		addSingleTabDraw->isPreviewPrint = true;
		m_currentMdiWindow = addSingleTabDraw->mdiWindow;
		m_currentMdiWindow->show();

		newTabDraw(addSingleTabDraw);
	}
}

bool UITabDrawWidget::slotFileClosing(MDIWindow* pMdiWin)
{
	bool cancel = false;
	bool hasParent = pMdiWin->getParentWindow() != nullptr;
	if (pMdiWin && pMdiWin->getDocument()->isModified() && !hasParent)
	{
		switch (showCloseDialog(pMdiWin))
		{
		case UIExitDialog::Save:
			cancel = !doSave(pMdiWin);
			break;
		case UIExitDialog::Cancel:
			cancel = true;
			break;
		}
	}
	if (!cancel)
	{
		doClose(pMdiWin);
	}

	return cancel;
}

bool UITabDrawWidget::doSave(MDIWindow* w, bool forceSaveAs)
{
	QString name, msg;
	bool cancelled;
	if (!w)
	{
		return false;
	}

	if (w->getDocument()->isModified() || forceSaveAs)
	{
		name = w->getDocument()->getFilename();
		if (name.isEmpty())
		{
			//doActivate(w); // show the user the drawing for save as
		}
		msg = name.isEmpty() ? tr("Saving drawing...") : tr("Saving drawing: %1").arg(name);
		//statusBar()->showMessage(msg);
		bool res = forceSaveAs ? w->slotFileSaveAs(cancelled) : w->slotFileSave(cancelled);
		if (res)
		{
			if (cancelled)
			{
				//statusBar()->showMessage(tr("Save cancelled"), 2000);
				return false;
			}
			name = w->getDocument()->getFilename();
			msg = tr("Saved drawing: %1").arg(name);
			//statusBar()->showMessage(msg, 2000);
			//commandWidget->appendHistory(msg);
		}
		else
		{
			msg = tr("Cannot save the file ") + w->getDocument()->getFilename() + tr(" , please check the filename and permissions.");
			//statusBar()->showMessage(msg, 2000);
			//commandWidget->appendHistory(msg);
			return doSave(w, true);
		}
	}
	return true;
}

void UITabDrawWidget::doClose(MDIWindow* w, bool activateNext)
{
	w->getDocumentView()->killAllActions();
	MDIWindow* parentWindow = w->getParentWindow();
	if (parentWindow)
	{
		parentWindow->removeChildWindow(w);
	}

	for (auto child : w->getChildWindows()) // block editors and print previews; just force these closed
	{
		doClose(child, false); // they belong to the document (changes already saved there)
	}
	w->getChildWindows().clear();
}

void UITabDrawWidget::setChangeTabDrawArea()
{
	GuiDocumentView* view = m_currentMdiWindow->getDocumentView();
	m_pActionHandler->set_view(view);
	m_pActionHandler->set_document(m_currentMdiWindow->getDocument());
	m_pCurrentActivePen->setPen(m_currentMdiWindow->getDocument());

	tabChangeEvent();
}

void UITabDrawWidget::tabClearEvent()
{
	// 底部状态栏置灰
	m_pBottomWidget->getWidget()->setEnabled(false);
	m_pActionHandler->set_view(nullptr);
	m_pActionHandler->set_document(nullptr);
}

void UITabDrawWidget::layoutTabButtons()
{
	if (m_pBackTabDrawWidget == nullptr || m_pTabDrawList == nullptr || m_pTabDrawList->empty())
	{
		return;
	}

	const int tabCount = static_cast<int>(m_pTabDrawList->size());
	const int availableWidth = qMax(0, m_pBackTabDrawWidget->width() - kNewTabButtonWidth);
	int tabWidth = kDefaultTabWidth;
	if (tabCount * kDefaultTabWidth > availableWidth)
	{
		tabWidth = tabCount > 0 ? availableWidth / tabCount : kDefaultTabWidth;
	}
	tabWidth = qMax(kMinimumTabWidth, tabWidth);

	for (int i = 0; i < tabCount; ++i)
	{
		SingleTabDrawDataRibbon* tabData = (*m_pTabDrawList)[i];
		if (tabData == nullptr || tabData->theWidget == nullptr)
		{
			continue;
		}

		tabData->theWidget->resize(tabWidth, kTabBarHeight);
		tabData->theWidget->move(i * tabWidth + kNewTabButtonWidth, 0);

		if (m_currentBtnWidget == tabData->theWidget)
		{
			m_currentPos = tabData->theWidget->pos();
		}

		if (tabData->textBtn)
		{
			const int textWidth = qMax(0, tabWidth - kTabCloseButtonSize - kTabCloseButtonRightMargin);
			tabData->textBtn->resize(textWidth, kTabBarHeight);
			QFontMetrics elideFont(tabData->textBtn->font());
			tabData->textBtn->setText(elideFont.elidedText(tabData->name, Qt::ElideMiddle, tabData->textBtn->width()));
		}

		if (tabData->closeDraw)
		{
			const int closeX = qMax(0, tabWidth - kTabCloseButtonSize - kTabCloseButtonRightMargin);
			tabData->closeDraw->setIconSize(QSize(kTabCloseIconSize, kTabCloseIconSize));
			tabData->closeDraw->resize(kTabCloseButtonSize, kTabCloseButtonSize);
			tabData->closeDraw->move(closeX, (kTabBarHeight - kTabCloseButtonSize) / 2);
			tabData->closeDraw->raise();
		}
	}
}

void UITabDrawWidget::updateTabStyleSheet(SingleTabDrawDataRibbon* tabData)
{
	if (tabData->isCurrent)
	{
		tabData->theWidget->setProperty("isCurrentTab", true);
	}
	else
	{
		tabData->theWidget->setProperty("isCurrentTab", false);
	}
	tabData->theWidget->style()->unpolish(tabData->theWidget);
	tabData->theWidget->style()->polish(tabData->theWidget);
	tabData->textBtn->style()->unpolish(tabData->textBtn);
	tabData->textBtn->style()->polish(tabData->textBtn);
	tabData->closeDraw->style()->unpolish(tabData->closeDraw);
	tabData->closeDraw->style()->polish(tabData->closeDraw);
}

void UITabDrawWidget::updateUICurrentActivePen()
{
	m_pCurrentActivePen->update(m_currentMdiWindow->getDocument());
}

bool UITabDrawWidget::eventFilter(QObject* obj, QEvent* e)
{
	QEvent::Type eventNow = e->type();
	auto classname = obj->metaObject()->className();
	QString objName = obj->objectName();
	if (QEvent::MouseButtonPress == eventNow)
	{
		QMouseEvent* evev = dynamic_cast<QMouseEvent*>(e);
		QPushButton* btn = dynamic_cast<QPushButton*>(obj);

		if (Qt::LeftButton == evev->button())
		{
			for (int i = 0; i < m_pTabDrawList->size(); ++i)
			{
				if ((*m_pTabDrawList)[i]->textBtn == btn)
				{
					m_currentBtnWidget = (*m_pTabDrawList)[i]->theWidget;
					m_currentPos = (*m_pTabDrawList)[i]->theWidget->pos();
					m_btnIndex = i;
				}
			}

			m_isPress = true;
			m_startPos = evev->globalPos();
			m_tmpPos = m_currentPos;
		}
	}
	else if (QEvent::MouseButtonRelease == eventNow)
	{
		m_isPress = false;
		m_currentBtnWidget->move(m_currentPos);
	}
	else if (QEvent::MouseMove == eventNow)
	{
		QMouseEvent* evev = dynamic_cast<QMouseEvent*>(e);
		if (true == m_isPress)
		{
			QPoint movePoint = evev->globalPos() - m_startPos;
			m_currentBtnWidget->move(m_tmpPos.x() + movePoint.x(), 0);
			double left_right = m_currentPos.x() - m_currentBtnWidget->pos().x();
			if (left_right < 0 && (*m_pTabDrawList).size() > m_btnIndex + 1)
			{
				double dist = std::abs(m_currentBtnWidget->x() - (*m_pTabDrawList)[m_btnIndex + 1]->theWidget->x());
				if (dist < m_currentBtnWidget->width() / 2)
				{
					swapBtnWidget(m_btnIndex, m_btnIndex + 1);
				}				
			}
			else if (left_right > 0 && m_btnIndex > 0)
			{
				double dist = std::abs(m_currentBtnWidget->x() - (*m_pTabDrawList)[m_btnIndex - 1]->theWidget->x());
				if (dist < m_currentBtnWidget->width() / 2)
				{
					swapBtnWidget(m_btnIndex, m_btnIndex - 1);
				}			
			}
		}
	}
	return QWidget::eventFilter(obj, e);
}

void UITabDrawWidget::swapBtnWidget(int currentIndex, int index)
{
	QPoint pos = (*m_pTabDrawList)[index]->theWidget->pos();
	(*m_pTabDrawList)[index]->theWidget->move(m_currentPos);

	SingleTabDrawDataRibbon* ribbon = (*m_pTabDrawList)[currentIndex];
	(*m_pTabDrawList)[currentIndex] = (*m_pTabDrawList)[index];
	(*m_pTabDrawList)[index] = ribbon;
	
	m_currentPos = pos;
	m_btnIndex = index;
}

void UITabDrawWidget::tabChangeEvent()
{
	emit tabChangeSignals(); // 注册信号

	if (m_pBottomWidget)
	{
		// 底部状态栏取消置灰
		m_pBottomWidget->getWidget()->setEnabled(true);
		m_pBottomWidget->redrawBottomWidget(m_pActionHandler, m_currentMdiWindow);
	}

	// todo: 新painer事否考虑类似处理？
	//if (m_currentMdiWindow)
	//{
	//	m_currentMdiWindow->getDocument()->getCacheDrawData()->setUpdateDrawEnts();
	//	m_currentMdiWindow->getDocument()->getCacheDrawData()->setUpdateSelcetedEnts();
	//	m_currentMdiWindow->getDocument()->getCacheDrawData()->setUpdateHighlightedEnts();
	//}
}
