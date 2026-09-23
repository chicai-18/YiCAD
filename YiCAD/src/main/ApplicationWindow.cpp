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

/// @file ApplicationWindow.cpp
/// @brief 应用程序主窗口实现，管理Ribbon界面、绘图区域、图层面板和事件分发

#include "ApplicationWindow.h"
#include <QFile>
#include <QTextEdit>
#include <QAbstractButton>
#include <QFileDialog>
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include <QPushButton>
#include <QMessageBox>
#include "SARibbonPannel.h"
#include "SARibbonToolButton.h"
#include <QAction>
#include <QMenu>
#include <QStatusBar>
#include <QDebug>
#include <QElapsedTimer>
#include <QRadioButton>
#include <QButtonGroup>
#include <QWidgetAction>
#include "SARibbonMenu.h"
#include "SARibbonComboBox.h"
#include "SARibbonLineEdit.h"
#include "SARibbonGallery.h"
#include "SARibbonCheckBox.h"
#include "SARibbonQuickAccessBar.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonApplicationButton.h"
#include "SARibbonCustomizeWidget.h"
#include "SAWindowButtonGroup.h"
#include <QCalendarWidget>
#include "SARibbonCustomizeDialog.h"
#include <QXmlStreamWriter>
#include <QTextStream>
#include <QFontComboBox>
#include <QLabel>
#include "SAFramelessHelper.h"
#include "CustomComboboxItem.h"
#include <QListWidget>
#include <QMdiArea>
#include <QString>
#include <QApplication>
#include <QToolBar>
#include <QDockWidget>
#include <QToolButton>
#include <QTabWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QVariant>
#include <QVector>

#include <functional>

#include "UITabDrawWidget.h"
#include "UIBottomWidget.h"
#include "UIActionHandler.h"
#include "UICommandWidget.h"
#include "UIBlockListWidget.h"
#include "UIBlockSaveAs.h"
#include "UIDialogFactory.h"
#include "UICurrentActivePen.h"

#include "CommandRegistry.h"
#include "ExtensionManager.h"
#include "IExtensionHost.h"
#include "UIRibbonManager.h"
#include "UIRibbonRegistry.h"

// 进程内扩展（src/extensions/<扩展>/，构建系统自动 glob 收集）。移除一个扩展：
// 删除其目录、这里的 #include，以及 registerExtensions() 里的 Register 一行。
#include "AIExtension.h"
#include "DimExtension.h"

#include "ActionLayersActivate.h"
#include "ActionLayersFreeze.h"
#include "ActionLayersLock.h"
#include "ActionLayersPrint.h"
#include "ActionLayersColor.h"
#include "ActionLayersDelete.h"

#include "MDIWindow.h"
#include "GuiDocumentView.h"
#include "DmSettings.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmColor.h"
#include "DmPen.h"
#include "GuiDialogFactory.h"
#include "UIDialogFactory.h"
#include "Selection.h"
#include "DmSystem.h"
#include "Debug.h"
#include "GuiDialogFactory.h"
#include "GuiEventHandler.h"
#include "Commands.h"
#include "DmFontList.h"

#include "HostApi.h"
#include "PluginManager.h"
#include "PluginRegistry.h"
#include "PluginUiAdapter.h"
#include "Fileio.h"

#include "DmLine.h"
#include "DmCircle.h"
#include "DmArc.h"
#include "DmSolid.h"
#include "DmPoint.h"
#include "DmEllipse.h"
#include "DmRay.h"
#include "DmXline.h"
#include "DmSpline.h"

/// @brief 将插件宿主服务限制在主窗口公开的文档和消息接口内。
class ApplicationPluginHostContext final : public PluginHostContext
{
public:
    explicit ApplicationPluginHostContext(ApplicationWindow& window) noexcept
        : m_window(window)
    {
    }

    void showPluginMessage(const QString& message) override
    {
        auto* commandWidget = m_window.getCmdWidget();
        if (commandWidget != nullptr && m_window.getMDIWindow() != nullptr)
        {
            commandWidget->appCmdTempText(message);
            return;
        }
        m_window.statusBar()->showMessage(message, 5000);
    }

    DmDocument* currentDocument() const noexcept override
    {
        return m_window.getDocument();
    }

    bool isDocumentOpen(const DmDocument* document) const noexcept override
    {
        const auto* tabs = m_window.getTabDrawWidget();
        return document != nullptr && tabs != nullptr &&
               tabs->getTabDrawDataOfDocument(document) != nullptr;
    }

    GuiDocumentView* documentView(
        const DmDocument* document) const noexcept override
    {
        auto* tabs = m_window.getTabDrawWidget();
        auto* tab = tabs == nullptr
            ? nullptr
            : tabs->getTabDrawDataOfDocument(document);
        return tab == nullptr || tab->mdiWindow == nullptr
            ? nullptr
            : tab->mdiWindow->getDocumentView();
    }

private:
    ApplicationWindow& m_window;
};

/// @brief ApplicationWindow 侧的扩展宿主服务。
///
/// 作为 ApplicationWindow 的成员，存活到 ExtensionManager::Shutdown() 之后。
/// 设置页入口与扩展的 Ribbon 条目都进同一个 Ribbon 注册表，扩展拿到的
/// Ribbon 入口按扩展 ID 限定命名空间。
///
/// 不放进匿名命名空间：要与 ApplicationWindow.h 里的前置声明是同一个类型
/// （与 ApplicationPluginHostContext 同一做法）。
class ApplicationWindowExtensionHost final : public IExtensionHost
{
public:
    ApplicationWindowExtensionHost(UIRibbonRegistry& ribbon, ApplicationWindow& window)
        : m_ribbon(ribbon), m_window(window)
    {
    }

    UIRibbonRegistrar& ribbonFor(std::string_view extensionId) override
    {
        auto& registrar = m_scopedRibbons[std::string(extensionId)];
        if (!registrar)
        {
            registrar = std::make_unique<UIRibbonScopedRegistrar>(m_ribbon, extensionId);
        }
        return *registrar;
    }

    QWidget* mainWindow() override { return &m_window; }
    DmDocument* currentDocument() const override { return m_window.getDocument(); }
    GuiDocumentView* currentDocumentView() const override { return m_window.getDocumentView(); }

    bool registerSettingsPage(const QString& id, const QString& title, const QString& iconPath,
                              std::function<void()> open) override
    {
        return m_ribbon.addAction(UIRibbonActionDef{
            .id = id,
            .panelId = UIRibbonIds::kPanelOptionsSettings,
            .text = title,
            .iconPath = iconPath.isEmpty() ? QStringLiteral(":/ribbon/options/settings.svg") : iconPath,
            .trigger = std::move(open),
        });
    }

    bool activateCommand(const QString& commandId) override
    {
        if (!CommandRegistry::instance().hasCommand(commandId))
        {
            return false;
        }
        m_window.getActionHandler()->activateCommand(commandId);
        return true;
    }

private:
    UIRibbonRegistry& m_ribbon;
    ApplicationWindow& m_window;
    std::map<std::string, std::unique_ptr<UIRibbonScopedRegistrar>> m_scopedRibbons;
};

// TODO: 以下宏为性能调试用函数式宏，无法直接转换为constexpr，建议后续改为内联函数
#define PRINT_COST_START()                                                                                             \
    QElapsedTimer __TMP_COST;                                                                                          \
    int __TMP_LASTTIMES = 0

#define PRINT_COST(STR)                                                                                                \
    do {                                                                                                               \
        int ___TMP_INT = __TMP_COST.elapsed();                                                                         \
        qDebug() << STR << " cost " << ___TMP_INT - __TMP_LASTTIMES << " ms (" << ___TMP_INT << ")";                   \
        __TMP_LASTTIMES = ___TMP_INT;                                                                                  \
    } while (0)

namespace
{
void applyLightThemeStyle(QWidget* rootWidget)
{
	QFile styleFile(QStringLiteral(":/styles/light.qss"));
	if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return;
	}

	const QString styleSheet = QString::fromUtf8(styleFile.readAll());
	qApp->setStyleSheet(styleSheet);
	if (rootWidget)
	{
		rootWidget->setStyleSheet(rootWidget->styleSheet() + QStringLiteral("\n") + styleSheet);
	}
}
}

ApplicationWindow* ApplicationWindow::appWindow = nullptr;

ApplicationWindow::ApplicationWindow(QWidget* par)
	: SARibbonMainWindow(par)
	, m_customizeWidget(nullptr)
	, m_pActionHandler(new UIActionHandler(this))
	, m_pLayerTable(nullptr)
	, m_pLayerTableWidget(nullptr)
	, m_pCurrentLayerItem(new ComboBoxData())
	, m_pLibraryList(nullptr)
{
	appWindow = this;
	PRINT_COST_START();
	SAFramelessHelper* helper = framelessHelper();
	helper->setRubberBandOnResize(false);
	setWindowTitle("YiCAD");
	// 底部状态栏
	m_pBottomWidget = new UIBottomWindow(this);

	// 添加绘图区域底板
	m_pDrawingArea = new QMdiArea(this);
	m_pDrawingArea->move(0, 185);
	m_pDrawingArea->setAutoFillBackground(true);

	// actions过程中弹出的配置框
	QWidget* pDialogBackWidget = new QWidget(this);
	pDialogBackWidget->move(1, 212);
	pDialogBackWidget->setObjectName("actionOptionsBackWidget");

	m_pDialogFactory = new UIDialogFactory(this, pDialogBackWidget);
	GuiDialogFactory::instance()->setFactoryObject(m_pDialogFactory);
	m_pDialogFactory->setActionHandle(m_pActionHandler);
	pDialogBackWidget->hide();

	// 绘图区域选项卡
	m_pTabDrawWidget = new UITabDrawWidget(this);
	m_pTabDrawWidget->createTabDrawWidget(m_pDrawingArea, m_pActionHandler, m_pBottomWidget);
	// 获取当前绘图画布
	m_pCurrentMdiWin = m_pTabDrawWidget->getCurrentMdiWindow();

	// ===========================ActionHandler参数设置======================
	m_pActionHandler->setMdiArea(m_pDrawingArea);
	m_pActionHandler->setMDIWindow(m_pCurrentMdiWin);
	m_pActionHandler->setUITabDrawWidget(m_pTabDrawWidget);
	
	// 底部状态栏
	m_pBottomWidget->createBottomWindow(m_pActionHandler, m_pCurrentMdiWin);
	UIDIALOGFACTORY->setBottomWidget(m_pBottomWidget);
	m_pActionHandler->setSnapToolBar(m_pBottomWidget->getSnapToolBar());

	// Command窗口
	m_cmdWin = new UICommandWidget(this, m_pTabDrawWidget);
	m_cmdWin->setActionHandler(m_pActionHandler);
	m_editLine = m_cmdWin->createTempEdit();
	// 将事件和状态链接起来
	UIDIALOGFACTORY->setCommandWidget(m_cmdWin);
	m_cmdWin->getCommandWidget()->setFixedWidth(400);
	m_cmdWin->getInfoWidget()->resize(400, 100);

	// 选项卡切换事件
	connect(m_pTabDrawWidget, SIGNAL(tabChangeSignals()), this, SLOT(slotsTabChangeEvent()));

	// 基类
	m_pRibbon = ribbonBar();
	applyLightThemeStyle(this);
	PRINT_COST("setCentralWidget & setWindowTitle");
	m_pRibbon->applicationButton()->hide();            // 隐藏父标签
	
	// 主窗口导航条
	SARibbonQuickAccessBar* quickAccessBar = m_pRibbon->quickAccessBar();
	createQuickAccessBar(quickAccessBar);
	PRINT_COST("add quick access bar");

	// Ribbon：内置类目先注册，扩展在 OnRegister 里接着注册（类目按注册顺序
	// 排列，扩展的设置页排在内置设置按钮之后），全部注册完再冻结并装配。
	m_ribbonRegistry = std::make_unique<UIRibbonRegistry>();
	registerBuiltinRibbon(*m_ribbonRegistry);
	registerExtensions();
	m_ribbonRegistry->finalize();
	m_ribbonManager = std::make_unique<UIRibbonManager>(
		*m_pRibbon, *m_ribbonRegistry,
		[this](const QString& commandId, QObject* source) { m_pActionHandler->activateCommand(commandId, source); },
		[this]() { return UIRibbonContext{getDocument()}; });
	m_ribbonManager->install();
	// 扩展命令的别名在 registerExtensions() 里才注册，补全列表要重建一次。
	m_cmdWin->refreshCompleter();
	PRINT_COST("register ribbon and extensions");

	// 主窗体最小尺寸
	this->setMinimumSize(900, 700);
	showMaximized();

	//Draw2d 作为缺省激活Tab
	if (SARibbonCategory* categoryDraw2d = m_ribbonManager->category(UIRibbonIds::kCategoryDraw2d))
	{
		ribbonBar()->setCurrentIndex(ribbonBar()->categoryIndex(categoryDraw2d));
	}

    /// @brief Ribbon、命令窗口和首个文档就绪后，接入唯一的新插件加载路径。
    m_pluginHostContext =
        std::make_unique<ApplicationPluginHostContext>(*this);
    m_pluginRegistry = std::make_unique<PluginRegistry>();
    m_pluginHostApi = std::make_unique<HostApi>(
        *m_pluginHostContext, *m_pluginRegistry);
    connect(m_pTabDrawWidget, &UITabDrawWidget::documentAboutToClose,
        this, [this](DmDocument* document) {
            if (m_pluginHostApi != nullptr)
            {
                m_pluginHostApi->rollbackImportsForDocument(document);
            }
        });
    m_pluginManager = std::make_unique<PluginManager>(
        *m_pluginHostApi, *m_pluginRegistry);
    m_pluginManager->loadAll();

    FileIO::instance()->setPluginRuntime(
        *m_pluginRegistry, *m_pluginManager, *m_pluginHostApi);

    m_pluginUiAdapter = std::make_unique<PluginUiAdapter>(
        *m_pRibbon, *m_pluginRegistry, *m_pActionHandler, *m_cmdWin);
    m_pluginUiAdapter->materialize();
}

/// @brief 注册进程内扩展并调用它们的 OnRegister（阶段4第二阶段，
/// doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4 §7.4任务③④）。
///
/// 每个扩展一行 Register，注册顺序即启动顺序、关闭的反序。
void ApplicationWindow::registerExtensions()
{
	m_extensionHost = std::make_unique<ApplicationWindowExtensionHost>(*m_ribbonRegistry, *this);
	ExtensionManager::instance().Register(std::make_unique<AIExtension>());
	ExtensionManager::instance().Register(std::make_unique<DimExtension>());
	ExtensionManager::instance().BootAll(*m_extensionHost);
}

void ApplicationWindow::onStyleClicked(int id)
{
	ribbonBar()->setRibbonStyle(static_cast<SARibbonBar::RibbonStyle>(id));
}

void ApplicationWindow::onActionCustomizeTriggered(bool b)
{
	Q_UNUSED(b);
	if (nullptr == m_customizeWidget)
	{
		m_customizeWidget = new SARibbonCustomizeWidget(this, this, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::Dialog);
		m_customizeWidget->setWindowModality(Qt::ApplicationModal);  //设置阻塞类型
		m_customizeWidget->setAttribute(Qt::WA_ShowModal, true);     //属性设置 true:模态 false:非模态
		m_customizeWidget->setupActionsManager(m_actMgr);
	}
	m_customizeWidget->show();
	m_customizeWidget->applys();
}

void ApplicationWindow::onActionCustomizeAndSaveTriggered(bool b)
{
	Q_UNUSED(b);
	SARibbonCustomizeDialog dlg(this);
	dlg.setupActionsManager(m_actMgr);
	dlg.fromXml("customize.xml");
	if (SARibbonCustomizeDialog::Accepted == dlg.exec())
	{
		dlg.applys();
		QByteArray str;
		QXmlStreamWriter xml(&str);
		xml.setAutoFormatting(true);
		xml.setAutoFormattingIndent(2);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)  // QXmlStreamWriter always encodes XML in UTF-8.
		xml.setCodec("utf-8");
#endif
		xml.writeStartDocument();
		bool isok = dlg.toXml(&xml);
		xml.writeEndDocument();
		if (isok)
		{
			QFile f("customize.xml");
			if (f.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
			{
				QTextStream s(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)  // QTextStream always encodes XML in UTF-8.
				s.setCodec("utf-8");
#endif
				s << str;
				s.flush();
			}
		}
	}
}

void ApplicationWindow::onActionRemoveAppBtnTriggered(bool b)
{
	if (b)
	{
		ribbonBar()->setApplicationButton(nullptr);
	}
	else
	{
		SARibbonApplicationButton* actionRemoveAppBtn = new SARibbonApplicationButton();
		actionRemoveAppBtn->setText(QObject::tr("File"));
		this->ribbonBar()->setApplicationButton(actionRemoveAppBtn);
	}
}

void ApplicationWindow::onActionUseQssTriggered()
{
	QFile f("ribbon.qss");
	if (!f.exists())
	{
		QString fdir = QFileDialog::getOpenFileName(this, QObject::tr("select qss file"));
		if (fdir.isEmpty())
		{
			return;
		}
		f.setFileName(fdir);
	}
	if (!f.open(QIODevice::ReadWrite))
	{
		return;
	}
	QString qss(f.readAll());
	this->ribbonBar()->setStyleSheet(qss);
}

void ApplicationWindow::onActionLoadCustomizeXmlFileTriggered()
{
	//只能调用一次
	static bool has_call = false;
	if (!has_call)
	{
		has_call = sa_apply_customize_from_xml_file("customize.xml", this, m_actMgr);
	}
}

void ApplicationWindow::onActionWindowFlagNormalButtonTriggered(bool b)
{
	if (b)
	{
		//最大最小关闭按钮都有
		Qt::WindowFlags f = windowFlags();
		f |= (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
		updateWindowFlag(f);
	}
	else
	{
		//由于已经处于frameless状态，这个最大最小设置是无效的
		// setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint&~Qt::WindowMinimizeButtonHint);
		Qt::WindowFlags f = windowFlags();
		f &= ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowCloseButtonHint;
		updateWindowFlag(f);
	}
}

void ApplicationWindow::closeEvent(QCloseEvent* event)
{
	m_pTabDrawWidget->slotFileCloseAll();
	if (m_pTabDrawWidget->getTabDrawList()->size() > 0)
	{
		event->ignore();
		return;
	}
	close();
}

void ApplicationWindow::keyPressEvent(QKeyEvent* e)
{
	if (e->matches(QKeySequence::Cut))
	{
		m_pActionHandler->activateCommand(QStringLiteral("edit.cut"));
	}
	else if (e->matches(QKeySequence::Copy))
	{
		m_pActionHandler->activateCommand(QStringLiteral("edit.copy"));
	}
	else if (e->matches(QKeySequence::Paste))
	{
		m_pActionHandler->activateCommand(QStringLiteral("edit.paste"));
	}
	else if (e->matches(QKeySequence::New))
	{
		m_pActionHandler->activateCommand(QStringLiteral("file.new"));
	}
	else if (e->matches(QKeySequence::Open))
	{
		m_pActionHandler->activateCommand(QStringLiteral("file.open"));
	}
	else if (e->matches(QKeySequence::Save))
	{
		m_pActionHandler->activateCommand(QStringLiteral("file.save"));
	}
	else if (e->matches(QKeySequence::Undo))
	{
		m_pActionHandler->slotEditUndo();
	}
	else if (e->matches(QKeySequence::Redo))
	{
		m_pActionHandler->activateCommand(QStringLiteral("edit.redo"));
	}
	else
	{
		switch (e->key())
		{
		case Qt::Key_Escape:    // ESC|空格 取消操作/取消选中
		case Qt::Key_Space:
		{
			GuiDocumentView* gv = m_pCurrentMdiWin->getDocumentView();
			GuiEventHandler* handle = gv->getEventHandler();
			handle->keyPressEvent(e);
			if (!e->isAccepted())
			{
				slotKillAllActions();
				//TODO. refactoring below code
				{
					m_editLine->hide();
					m_editLine->lower();
					setFocus();	
					m_cmdWin->getEditline()->setText("");
					m_cmdWin->getEdit()->setPlaceholderText(QObject::tr("command"));
				}
				e->accept();
			}
		}		
			break;

		case Qt::Key_Return:    // 大键盘回车
		case Qt::Key_Enter:     // 数字键回车
			slotEnter();
			e->accept();
			break;

		case Qt::Key_Delete:    // 键盘delete(todo:这里只能Tab+Del生效,QAction里也挂了一次)
		case Qt::Key_Backspace: // 退格(同上)
			slotDelete();
			e->accept();
			break;
		case Qt::Key_A:
		case Qt::Key_B:
		case Qt::Key_C:
		case Qt::Key_D:
		case Qt::Key_E:
		case Qt::Key_F:
		case Qt::Key_G:
		case Qt::Key_H:
		case Qt::Key_I:
		case Qt::Key_J:
		case Qt::Key_K:
		case Qt::Key_L:
		case Qt::Key_M:
		case Qt::Key_N:
		case Qt::Key_O:
		case Qt::Key_P:
		case Qt::Key_Q:
		case Qt::Key_R:
		case Qt::Key_S:
		case Qt::Key_T:
		case Qt::Key_U:
		case Qt::Key_V:
		case Qt::Key_W:
		case Qt::Key_X:
		case Qt::Key_Y:
		case Qt::Key_Z:
		case Qt::Key_0:
		case Qt::Key_1:
		case Qt::Key_2:
		case Qt::Key_3:
		case Qt::Key_4:
		case Qt::Key_5:
		case Qt::Key_6:
		case Qt::Key_7:
		case Qt::Key_8:
		case Qt::Key_9:
		case Qt::Key_Colon:
		case Qt::Key_Semicolon:
		case Qt::Key_Less:
		case Qt::Key_Greater:
		case Qt::Key_Question:
		case Qt::Key_BracketLeft:
		case Qt::Key_Backslash:
		case Qt::Key_BracketRight:
		case Qt::Key_Underscore:
		case Qt::Key_BraceLeft:
		case Qt::Key_BraceRight:
		case Qt::Key_AsciiTilde:
		case Qt::Key_Exclam:	//!
		case Qt::Key_At:		//@
		case Qt::Key_QuoteDbl:
		case Qt::Key_NumberSign:
		case Qt::Key_Dollar:	//$
		case Qt::Key_Percent:	//%
		case Qt::Key_Ampersand:	//&
		case Qt::Key_Apostrophe:
		case Qt::Key_ParenLeft:	//(
		case Qt::Key_ParenRight://)
		case Qt::Key_Asterisk:	//*
		case Qt::Key_Comma:		//,
		case Qt::Key_Period:	//.
		case Qt::Key_Slash:		///
        case Qt::Key_Plus:      // +号
        case Qt::Key_Equal:     // =号
        case Qt::Key_Minus:     // -号
			if (pointInPolygon(m_presentPos, m_pointVec))
			{
				m_editLine->raise();
				m_editLine->show();
				m_editLine->move(m_presentPos.x() + 10, m_presentPos.y() + 10);
				m_cmdWin->getEditline()->setFocus();
				m_cmdWin->getEditline()->setText(e->text());
			}
			break;
		default:
			break;
		}

		if (e->isAccepted())
		{
			return;
		}
	}

	QMainWindow::keyPressEvent(e);
}

void ApplicationWindow::slotKillAllActions()
{
	m_pCurrentMdiWin = m_pTabDrawWidget->getCurrentMdiWindow();
	GuiDocumentView* gv = m_pCurrentMdiWin->getDocumentView();
	if (gv && m_pCurrentMdiWin && m_pCurrentMdiWin->getDocument())
	{
		gv->killAllActions();

		Selection s(m_pCurrentMdiWin->getDocument(), gv);
		s.selectAll(false);
		m_pCurrentMdiWin->getDocumentView()->emitSelectedChanged();
		GUIDIALOGFACTORY->updateSelectionWidget(m_pCurrentMdiWin->getDocument()->getEntityTable()->countSelect());

		gv->redraw();
	}
}

void ApplicationWindow::slotEnter()
{
	m_pCurrentMdiWin = m_pTabDrawWidget->getCurrentMdiWindow();
	GuiDocumentView* docView = m_pCurrentMdiWin->getDocumentView();
	if (docView)
	{
		docView->enter();
	}
}

void ApplicationWindow::slotDelete()
{
	m_pActionHandler->activateCommand(QStringLiteral("modify.delete_no_select"));
}

void ApplicationWindow::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
	if (m_pTabDrawWidget == nullptr || m_pTabDrawWidget->getCurrentMdiWindow() == nullptr)
	{
		return;
	}

	const int bottomHeight = (m_pBottomWidget && m_pBottomWidget->getWidget()) ? m_pBottomWidget->getWidget()->height() : 0;
	const int contentTop = m_pRibbon ? m_pRibbon->geometry().bottom() + 1 : m_pDrawingArea->y();
	m_pTabDrawWidget->layoutWidgets(contentTop, this->width(), this->height(), bottomHeight);
	m_pCurrentMdiWin->resize(m_pDrawingArea->size());

	// cmd
	auto m_pCommandWidget = m_cmdWin->getCommandWidget();
	m_pCommandWidget->move((this->width() - m_cmdWin->getCommandWidget()->width()) * 0.5, this->height() - 65);
	m_cmdWin->getInfoWidget()->move((this->width() - m_cmdWin->getCommandWidget()->width()) * 0.5, this->height() - 165);
	m_cmdWin->getTempWidget()->move((this->width() - m_cmdWin->getCommandWidget()->width()) * 0.5, this->height() - 65 - m_cmdWin->getTempWidget()->height());

	// 状态栏
	m_pBottomWidget->getWidget()->setFixedWidth(this->width());
	m_pBottomWidget->getWidget()->move(0, this->height() - m_pBottomWidget->getWidget()->height());
	auto bottomWidget = m_pBottomWidget->getToolWidget();
	bottomWidget->move(this->width() - bottomWidget->width(), this->height() - bottomWidget->height() - m_pBottomWidget->getWidget()->height());
	auto comboBoxWiget = m_pBottomWidget->getComboBoxWidget();
	comboBoxWiget->move(this->width() - comboBoxWiget->width(), this->height() - comboBoxWiget->height() - m_pBottomWidget->getWidget()->height());
	//auto themecomboBoxWiget = m_pBottomWidget->getThemeComBoxWidget();
	//themecomboBoxWiget->move(this->width() - themecomboBoxWiget->width(), this->height() - themecomboBoxWiget->height() - m_pBottomWidget->getWidget()->height());
	auto viewportsWidget = m_pBottomWidget->getViewportsWidget();
	viewportsWidget->move(this->width() - viewportsWidget->width() - 110, this->height() - viewportsWidget->height() - m_pBottomWidget->getWidget()->height());

	m_pointVec.clear();
	QPoint p1 = m_pDrawingArea->pos();
	m_pointVec.emplace_back(p1);
	QPoint p2(p1.x(), p1.y() + m_pDrawingArea->height());
	m_pointVec.emplace_back(p2);
	QPoint p4 = m_pCommandWidget->pos();
	QPoint p3(p4.x(), p4.y() + m_pCommandWidget->height());
	m_pointVec.emplace_back(p3);
	m_pointVec.emplace_back(p4);
	QPoint p5(p4.x() + m_pCommandWidget->width(), p4.y());
	m_pointVec.emplace_back(p5);
	QPoint p6(p5.x(), p3.y());
	m_pointVec.emplace_back(p6);
	QPoint p7(p2.x() + m_pDrawingArea->width(), p2.y());
	m_pointVec.emplace_back(p7);
	QPoint p8(p7.x(), p1.y());
	m_pointVec.emplace_back(p8);

	m_snapWidgetVec.clear();
	p1 = bottomWidget->pos();
	m_snapWidgetVec.emplace_back(p1);
	p2 = {p1.x(), p1.y() + bottomWidget->height()};
	m_snapWidgetVec.emplace_back(p2);
	p3 = { p1.x() + bottomWidget->width(), p2.y() };
	m_snapWidgetVec.emplace_back(p3);
	p4 = { p3.x(),p1.y() };
	m_snapWidgetVec.emplace_back(p4);
}

void ApplicationWindow::updateLayerTable()
{
    // 清空
    m_pLayerTableWidget->clear();
    if (nullptr == m_pCurrentMdiWin)
    {
        return;
    }

	DmDocument* document = m_pCurrentMdiWin->getDocument();
	ComboBoxData* curData = nullptr;
	DmLayer* activeLayer = document->getLayerTable()->getActive();
	// 加载当前图纸图层
	auto layerList = document->getLayerTable();
    for (auto it = layerList->begin(); it != layerList->end(); ++it)
    {
        // 创建一个图层
        auto cbxData = newLayer(*it, m_pLayerTableWidget);
        if (*it == activeLayer)
        {
            curData = cbxData;
        }
    }

    //获得选择实体个数
    DmLayer* firstSelectedLayer = nullptr;
    int selectedNumType = 0;    //0表示无选择，1表示1个图层选择，2表示多于1个图层选择
    DmEntity* singleSelectedEnt = nullptr;
    for (auto ent : *document->getEntityTable())
    {
        if (ent->isSelected())
        {
            DmLayer* layer = ent->getLayer();
            if (firstSelectedLayer == nullptr)
            {
                firstSelectedLayer = layer;
                selectedNumType = 1;
                singleSelectedEnt = ent;
            }
            else if (firstSelectedLayer != layer)
            {
                selectedNumType = 2;
                break;
            }
        }
    }

    // 更新图层UI
    auto layerItems = getLayerComboboxItems();
    if (selectedNumType == 0 || selectedNumType == 1)
    {
        ComboBoxData* toDistplayLayerData = nullptr;
        DmLayer* displayLayer = nullptr;
        if (selectedNumType == 0)
        {
            displayLayer = activeLayer;
        }
        else
        {
            displayLayer = singleSelectedEnt->getLayer();
        }
        std::find_if(layerItems.begin(), layerItems.end(), [&toDistplayLayerData, displayLayer] (CustomComboboxItem* layerItem) {
            if (layerItem->getData()->getLayerName() == displayLayer->getName())
            {
                toDistplayLayerData = layerItem->getData();
                return true;
            }
            return false;
        });
        m_pCurrentLayerItem->show();
        m_pCurrentLayerItem->setByData(toDistplayLayerData);
    }
    else if (selectedNumType == 2)
    {
        m_pCurrentLayerItem->hide();
    }

    // 收起列表
    getLayerCombox()->view()->verticalScrollBar()->setSliderPosition(0);
    getLayerCombox()->hidePopup();
    getMDIWindow()->setFocus();
}

void ApplicationWindow::updateCurrentPenWidget()
{
	if (m_pCurrentMdiWin)
	{
		m_pTabDrawWidget->updateUICurrentActivePen();
	}
}

void ApplicationWindow::slotsTabChangeEvent()
{
	//当前没有打开文档，设置UI不可用。有打开文档时，设置UI可用
	MDIWindow* oldMdiWin = m_pCurrentMdiWin;
	m_pCurrentMdiWin = m_pTabDrawWidget->getCurrentMdiWindow();
	if (nullptr == m_pCurrentMdiWin)
	{
		enableButtons(false);
	}
	else if (nullptr == oldMdiWin)
	{
		enableButtons(true);
	}
	if (m_ribbonManager)
	{
		m_ribbonManager->evaluateActivation();
	}

	updateLayerTable();
	//updateViewportTable();
    updateCurrentPenWidget();
    updateUndoRedo();
}

QAction* ApplicationWindow::getRedoAction()
{
	return m_pActRedo;
}

QAction* ApplicationWindow::getUndoAction()
{
	return m_pActUndo;
}

void ApplicationWindow::setRedoEnable(bool enable)
{
	if (m_pActRedo)
	{
		m_pActRedo->setEnabled(enable);
	}
}

void ApplicationWindow::setUndoEnable(bool enable)
{
	if (m_pActUndo)
	{
		m_pActUndo->setEnabled(enable);
	}
}

QMdiArea const* ApplicationWindow::getMdiArea() const
{
	return m_pDrawingArea;
}

QMdiArea* ApplicationWindow::getMdiArea()
{
	return m_pDrawingArea;
}

const MDIWindow* ApplicationWindow::getMDIWindow() const
{
	return m_pTabDrawWidget->getCurrentMdiWindow();
}

MDIWindow* ApplicationWindow::getMDIWindow()
{
	return m_pTabDrawWidget->getCurrentMdiWindow();
}

const GuiDocumentView* ApplicationWindow::getDocumentView() const
{
	auto mdiwindow = getMDIWindow();
	if (mdiwindow)
	{
		return mdiwindow->getDocumentView();
	}
	return nullptr;
}

GuiDocumentView* ApplicationWindow::getDocumentView()
{
	auto mdiwindow = getMDIWindow();
	if (mdiwindow)
	{
		return mdiwindow->getDocumentView();
	}
	return nullptr;
}

const DmDocument* ApplicationWindow::getDocument() const
{
	auto mdiwindow = getMDIWindow();
	if (mdiwindow)
	{
		return mdiwindow->getDocument();
	}
	return nullptr;
}

DmDocument* ApplicationWindow::getDocument()
{
	auto mdiwindow = getMDIWindow();
	if (mdiwindow)
	{
		return mdiwindow->getDocument();
	}
	return nullptr;
}

UIActionHandler* ApplicationWindow::getActionHandler() const
{
	return m_pActionHandler;
}

UITabDrawWidget* ApplicationWindow::getTabDrawWidget() const
{
	return m_pTabDrawWidget;
}

std::vector<DmDocument*> ApplicationWindow::getDocuments() const
{
	return m_pTabDrawWidget->getDocuments();
}

UICommandWidget* ApplicationWindow::getCmdWidget() const
{
	return m_cmdWin;
}

UIBottomWindow* ApplicationWindow::getBottomWidget() const
{
	return m_pBottomWidget;
}

MDIWindow* ApplicationWindow::getWindowWithDoc(const DmDocument* doc)
{
	return m_pTabDrawWidget->getCurrentMdiWindow();
}

SARibbonComboBox* ApplicationWindow::getLayerCombox() const
{
	return m_pLayerTable;
}

QListWidget* ApplicationWindow::getLayerTableWidget() const
{
	return m_pLayerTableWidget;
}

ComboBoxData* ApplicationWindow::getCurrentLayerItem() const
{
	return m_pCurrentLayerItem;
}

QAction* ApplicationWindow::getActOnOff() const
{
	return m_pActOnOff;
}

QAction* ApplicationWindow::getActLock() const
{
	return m_pActLock;
}

std::vector<CustomComboboxItem*> ApplicationWindow::getLayerComboboxItems()
{
	std::vector<CustomComboboxItem*> res;
	res.reserve(m_pLayerTableWidget->count());
	for (int i = 0; i < m_pLayerTableWidget->count(); i++)
	{
		QListWidgetItem* item = m_pLayerTableWidget->item(i);
		CustomComboboxItem* customItem = static_cast<CustomComboboxItem*>(m_pLayerTableWidget->itemWidget(item));
		res.emplace_back(customItem);
	}
	return res;
}

ApplicationWindow::~ApplicationWindow()
{
    /// @brief 先关扩展（会触发 AIExtension::OnShutdown() 等），再关插件，
    /// 最后才删其它全局单例——与下面插件关闭的注释是同一个约束的延伸。
    ExtensionManager::instance().Shutdown();

    /// @brief 必须在任何窗口、文档和全局宿主服务销毁前关闭并卸载插件。
    FileIO::instance()->clearPluginRuntime();
    if (m_pluginManager)
    {
        m_pluginManager->shutdownAll();
    }
    m_pluginManager.reset();
    m_pluginUiAdapter.reset();
    m_pluginHostApi.reset();
    m_pluginRegistry.reset();
    m_pluginHostContext.reset();

	COMMANDS->deleteCommands();
	DMSETTINGS->deleteDmStettings();
	delete GuiDialogFactory::instance();
	DMSYSTEM->deleteDmSystem();
	DMFONTLIST->deleteFontList();
    DmLineTypeTable::deleteStaticLineTypes();
	DEBUG->deleteInstance();
	
	if (m_pBottomWidget)
	{
		delete m_pBottomWidget;
		m_pBottomWidget = nullptr;
	}

	if (m_pTabDrawWidget)
	{
		delete m_pTabDrawWidget;
		m_pTabDrawWidget = nullptr;
	}

	if (m_pDialogFactory)
	{
		delete m_pDialogFactory;
		m_pDialogFactory = nullptr;
	}

	if (m_pDrawingArea)
	{
		delete m_pDrawingArea;
		m_pDrawingArea = nullptr;
	}

	if (m_cmdWin)
	{
		delete m_cmdWin;
		m_cmdWin = nullptr;
	}

	if (m_pCurrentLayerItem)
	{
		delete m_pCurrentLayerItem;
		m_pCurrentLayerItem = nullptr;
	}

	Type::destruct();
}

bool ApplicationWindow::pointInPolygon(const QPoint point, const std::vector<QPoint> vec)
{
    int nCross = 0;
    auto nCount = vec.size();
    for (size_t i = 0; i < nCount; i++)
    {
        QPoint p1 = vec[i]; // 当前节点
        QPoint p2 = vec[(i + 1) % nCount]; // 下一个节点

        if (p1.y() == p2.y()) // p1p2 与 y=p0.y平行
        {
            continue;
        }

        if (point.y() < std::min(p1.y(), p2.y())) // 交点在p1p2延长线上
        {
            continue;
        }
        if (point.y() >= std::max(p1.y(), p2.y())) // 交点在p1p2延长线上
        {
            continue;
        }

        double x = static_cast<double>(point.y() - p1.y()) * static_cast<double>(p2.x() - p1.x()) / static_cast<double>(p2.y() - p1.y()) + p1.x();

        if (x > point.x())
        {
            nCross++; // 只统计单边交点
        }
    }

    // 单边交点为偶数，点在多边形之外 ---
    return (nCross % 2 == 1);
}

ApplicationWindow* ApplicationWindow::getAppWindow()
{
	return appWindow;
}

/// @brief 顶部导航条
void ApplicationWindow::createQuickAccessBar(SARibbonQuickAccessBar* quickAccessBar)
{
	auto actNew = createAction(QObject::tr("new"), ":/ribbon/file/new.svg", "new-quickbar");
	connect(actNew, &QAction::triggered, this, [this, actNew]() { m_pActionHandler->activateCommand(QStringLiteral("file.new"), actNew); });
	quickAccessBar->addAction(actNew);																		// 新建

	auto actOpen = createAction(QObject::tr("open"), ":/ribbon/file/open.svg", "open-quickbar");
	connect(actOpen, &QAction::triggered, this, [this](bool b) {
		Q_UNUSED(b);
		DMSYSTEM->setCurrentFormatType("ycd");
		m_pTabDrawWidget->slotFileOpen();
		m_pActionHandler->slotSetSnaps(m_pBottomWidget->getSnapToolBar()->getSnaps());
		});
	quickAccessBar->addAction(actOpen);																		// 打开

	auto actSave = createAction(QObject::tr("save"), ":/ribbon/file/save.svg", "save-quickbar");
	connect(actSave, &QAction::triggered, this, [this, actSave]() { m_pActionHandler->activateCommand(QStringLiteral("file.save"), actSave); });
	quickAccessBar->addAction(actSave);																		// 保存

	auto actSaveas = createAction(QObject::tr("save as"), ":/ribbon/file/save_as.svg", "saveas-quickbar");
	connect(actSaveas, &QAction::triggered, this, [this, actSaveas]() { m_pActionHandler->activateCommand(QStringLiteral("file.save_as"), actSaveas); });
	quickAccessBar->addAction(actSaveas);																	// 另存为
	quickAccessBar->addSeparator();																			// 分割条

	// undo/redo
	// 撤销
	m_pActUndo = createAction(QObject::tr("Undo"), ":/ribbon/undo.svg");
	// 撤销保留 slotEditUndo：没有打开的图纸时它直接返回，不构造 ActionEditUndo。
	connect(m_pActUndo, SIGNAL(triggered()), m_pActionHandler, SLOT(slotEditUndo()));
    m_pActUndo->setEnabled(false);
	quickAccessBar->addAction(m_pActUndo);																	// 回退

	// 重做
	m_pActRedo = createAction(QObject::tr("Redo"), ":/ribbon/redo.svg");
	connect(m_pActRedo, &QAction::triggered, this, [this]() { m_pActionHandler->activateCommand(QStringLiteral("edit.redo"), m_pActRedo); });
    m_pActRedo->setEnabled(false);
	quickAccessBar->addAction(m_pActRedo);																	// 重做
	quickAccessBar->addSeparator();                                                                         // 分割条
	//QMenu* m = new QMenu("Recent Files", this);                                                        // 历史文件 todo: 暂时屏蔽
	//m->setIcon(QIcon(":/ribbon/recent_files.svg"));
	//for (int i = 0; i < 10; ++i)
	//{
	//	m->addAction(createAction(QString("file%1").arg(i + 1), ":/ribbon/presentationFile.svg"));
	//}
	//quickAccessBar->addMenu(m);
}

QAction* ApplicationWindow::createAction(const QString& text, const QString& iconurl, const QString& objName)
{
	QAction* act = new QAction(this);
	act->setText(text);
	act->setIcon(QIcon(iconurl));
	act->setObjectName(objName);
	return act;
}

QAction* ApplicationWindow::createAction(const QString& text, const QString& iconurl)
{
	QAction* act = new QAction(this);
	act->setText(text);
	act->setIcon(QIcon(iconurl));
	act->setObjectName(text);
	return act;
}

QWidget* ApplicationWindow::createLayerTable(QWidget* parent)
{
	SARibbonButtonGroupWidget* allGroup = UIRibbonManager::createButtonGroup(parent, 2);

	// 图层列表
	m_pLayerTable = new SARibbonComboBox(allGroup);
	m_pLayerTable->setMinimumSize(200, 23);
	m_pLayerTable->setEditable(false);	// 禁止输入
	m_pLayerTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	
	m_pLayerTableWidget = new QListWidget(m_pLayerTable);
	m_pLayerTable->setModel(m_pLayerTableWidget->model());
	m_pLayerTable->setView(m_pLayerTableWidget);

	// 设置下拉列表当前显示数据
	auto currentLayer = m_pCurrentMdiWin->getDocument()->getLayerTable()->getActive();
	m_pCurrentLayerItem = initLayerComboboxItem(currentLayer, m_pLayerTable, true);

	// 加载当前图纸图层
	auto layerList = m_pCurrentMdiWin->getDocument()->getLayerTable();
    for (auto it = layerList->begin(); it != layerList->end(); ++it)
    {
        // 创建一个图层
        newLayer(*it, m_pLayerTableWidget);
    }
	allGroup->addWidget(m_pLayerTable);

	// 打开所有图层
	m_pActOnOff = createAction(QObject::tr("on all"), ":/ribbon/layer/layer_all_visible.svg");
	connect(m_pActOnOff, &QAction::triggered, this, [this]() { m_pActionHandler->activateCommand(QStringLiteral("layers.defreeze_all"), m_pActOnOff); });
	
	// 解锁所有图层
	m_pActLock = createAction(QObject::tr("unlock all"), ":/ribbon/layer/layer_all_unlock.svg");
	connect(m_pActLock, &QAction::triggered, this, [this]() { m_pActionHandler->activateCommand(QStringLiteral("layers.unlock_all"), m_pActLock); });
	
	// 新增图层
	QAction* actNewLayer = createAction(QObject::tr("new layer"), ":/ribbon/layer/add_layer.svg");
	connect(actNewLayer, &QAction::triggered, this, [this, actNewLayer]() { m_pActionHandler->activateCommand(QStringLiteral("layers.add"), actNewLayer); });

	//复制实体到图层
	QAction* actCopyLayer = createAction(QObject::tr("copy to layer"), ":/ribbon/layer/copy_entity_to_layer.svg");
	connect(actCopyLayer, &QAction::triggered, this, [this, actCopyLayer]() { m_pActionHandler->activateCommand(QStringLiteral("modify.copy_to_layer"), actCopyLayer); });

	// 修改图层
	QAction* actRenameLayer = createAction(QObject::tr("rename layer"), ":/ribbon/layer/rename_layer.svg");
	connect(actRenameLayer, &QAction::triggered, this, [this, actRenameLayer]() { m_pActionHandler->activateCommand(QStringLiteral("layers.rename"), actRenameLayer); });

	// 下面这排使用 Ribbon 按钮控件，但用自定义等分布局，确保图标尽量铺满且整行平铺。
	QWidget* layerActionRow = new QWidget(allGroup);
	layerActionRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	auto createLayerRibbonButton = [layerActionRow](QAction* action) {
		auto* button = new SARibbonToolButton(layerActionRow);
		button->setAutoRaise(true);
		button->setFocusPolicy(Qt::NoFocus);
		button->setButtonType(SARibbonToolButton::SmallButton);
		button->setToolButtonStyle(Qt::ToolButtonIconOnly);
		button->setDefaultAction(action);
		return button;
	};

	auto* btnOnOff = createLayerRibbonButton(m_pActOnOff);
	auto* btnLock = createLayerRibbonButton(m_pActLock);
	auto* btnNewLayer = createLayerRibbonButton(actNewLayer);
	auto* btnRenameLayer = createLayerRibbonButton(actRenameLayer);
	auto* btnCopyLayer = createLayerRibbonButton(actCopyLayer);

	auto* layerActionLayout = new QHBoxLayout(layerActionRow);
	layerActionLayout->setContentsMargins(0, 0, 0, 0);
	layerActionLayout->setSpacing(0);
	layerActionLayout->addStretch(1);
	layerActionLayout->addWidget(btnOnOff);
	layerActionLayout->addStretch(1);
	layerActionLayout->addWidget(btnLock);
	layerActionLayout->addStretch(1);
	layerActionLayout->addWidget(btnNewLayer);
	layerActionLayout->addStretch(1);
	layerActionLayout->addWidget(btnRenameLayer);
	layerActionLayout->addStretch(1);
	layerActionLayout->addWidget(btnCopyLayer);
	layerActionLayout->addStretch(1);

	allGroup->addWidget(layerActionRow);
	allGroup->setMinimumWidth(200);
	return allGroup;
}

ComboBoxData* ApplicationWindow::newLayer(DmLayer* layer, QListWidget* plistWidget)
{
	CustomComboboxItem* item = new CustomComboboxItem(plistWidget, layer);
	ComboBoxData* cbxData = initLayerComboboxItem(layer, item, false);
	item->setData(cbxData);
	QListWidgetItem* widgetItem = new QListWidgetItem(plistWidget);
	plistWidget->setItemWidget(widgetItem, item);

	// Reset after selection to avoid a blank area in the layer combo popup.
	connect(m_pLayerTable, QOverload<int>::of(&QComboBox::activated), m_pLayerTable, [this](int) {
		this->m_pLayerTable->view()->verticalScrollBar()->setSliderPosition(0);
	});
	return cbxData;
}

ComboBoxData* ApplicationWindow::initLayerComboboxItem(DmLayer* layer, QWidget* parent, bool isEditBox)
{
	auto docView = m_pCurrentMdiWin->getDocumentView();
	auto document = m_pCurrentMdiWin->getDocument();
	ComboBoxData* data = new ComboBoxData();
	// 显示、隐藏
	data->btnOn = new QToolButton(parent);
	data->setIsOn(!layer->isFrozen());
	connect(data->btnOn, &QAbstractButton::clicked, this, [this, source = data->btnOn]() { m_pActionHandler->activateCommand(QStringLiteral("layers.freeze"), source); });

	// 锁定、解锁
	data->btnLock = new QToolButton(parent);		
	data->setIsLock(layer->isLocked());
	connect(data->btnLock, &QAbstractButton::clicked, this, [this, source = data->btnLock]() { m_pActionHandler->activateCommand(QStringLiteral("layers.lock"), source); });

	// 打印、不打印
	data->btnPrint = new QToolButton(parent);
	data->setIsPrint(layer->isPrint());
	connect(data->btnPrint, &QAbstractButton::clicked, this, [this, source = data->btnPrint]() { m_pActionHandler->activateCommand(QStringLiteral("layers.print"), source); });

	// 颜色
	data->btnColor = new QToolButton(parent);
	DmColor layerColor = layer->getPen().getColor();
	data->setColor(QColor(layerColor.red(), layerColor.green(), layerColor.blue(), layerColor.alpha()));
	connect(data->btnColor, &QAbstractButton::clicked, this, [this, source = data->btnColor]() { m_pActionHandler->activateCommand(QStringLiteral("layers.color"), source); });

	// 名字
	data->labelName = new QPushButton(parent);
	data->labelName->setProperty("isLayerNameButton", true);
	data->labelName->setFlat(true);
	QString layerName = layer->getName();
	data->setLayerName(layerName);
	if (isEditBox)
	{
		QComboBox* cbx = dynamic_cast<QComboBox*>(parent);
		if (cbx)
		{
			connect(data->labelName, &QPushButton::clicked, cbx, [cbx] {
				cbx->showPopup(); // 展开列表
				});
		}
	}
	else
	{
		connect(data->labelName, &QAbstractButton::clicked, this, [this, source = data->labelName]() { m_pActionHandler->activateCommand(QStringLiteral("layers.activate"), source); });
	}

	// 删除
	if (!isEditBox)
	{
		data->btnDelete = new QToolButton(parent);
		data->btnDelete->setIcon(QIcon(":/ribbon/layer/delete_layer.svg"));
		connect(data->btnDelete, &QAbstractButton::clicked, this, [this, source = data->btnDelete]() { m_pActionHandler->activateCommand(QStringLiteral("layers.delete"), source); });
	}
	else
	{
		data->btnDelete = nullptr;
	}

	//添加到布局
	auto layout = new  QHBoxLayout(parent);
	layout->addWidget(data->btnOn);
	layout->addWidget(data->btnLock);
	layout->addWidget(data->btnPrint);
	layout->addWidget(data->btnColor);
	layout->addWidget(data->labelName);
	if (isEditBox)
	{
		layout->addStretch();
		layout->setSpacing(0);
		layout->setContentsMargins(2, 1, 20, 0);
	}
	else
	{
		layout->addStretch();
		layout->addWidget(data->btnDelete);
		layout->setSpacing(0);
		layout->setContentsMargins(2, 1, 2, 0);
	}
	parent->setLayout(layout);

	return data;
}

void ApplicationWindow::setDrawingTabName(const QString& fileName)
{
	m_pTabDrawWidget->soltSetDrawingTabName(fileName);
}

int ApplicationWindow::countRow(QPoint p, int row, int height) // 计算鼠标在哪一列和哪一行
{
    if (p.y() < 2)
    {
        return 10 + row;
    }
    else if (p.y() > height - 2)
    {
        return 30 + row;
    }
    else
    {
        return 20 + row;
    }
}

int ApplicationWindow::countLine(QPoint p, int width)
{
    if (p.x() < 2)
    {
        return 1;
    }
    else if (p.x() > width - 2)
    {
        return 3;
    }
    else
    {
        return 2;
    }
}

void ApplicationWindow::enableButtons(const bool enable)
{
	// Ribbon 按钮的可用状态由 m_ribbonManager 按注册时声明的可用条件重算
	// （见 slotsTabChangeEvent），这里只处理 Ribbon 之外的控件。
	m_pActRedo->setEnabled(enable);
	m_pActUndo->setEnabled(enable);
	for (auto act : ribbonBar()->quickAccessBar()->actions())
	{
		if (act->objectName() == "save-quickbar" || act->objectName() == "saveas-quickbar")
		{
			act->setEnabled(enable);
		}
	}

	//当前画笔
	if (nullptr != m_pTabDrawWidget)
	{
		m_pTabDrawWidget->getCurrentActivePen()->parentWidget()->setEnabled(enable);
	}

	//命令框
	if (nullptr != m_cmdWin)
	{
		m_cmdWin->getCommandWidget()->setEnabled(enable);
		m_cmdWin->getInfoWidget()->setEnabled(enable);
		m_cmdWin->getTempWidget()->setEnabled(enable);
		//m_cmdWin->getTipWidget()->setEnabled(enable);	这个getTipWidget()会创建
	}

	//状态栏
	if (nullptr != m_pBottomWidget)
	{
		m_pBottomWidget->setEnabled(enable);
	}
}

bool ApplicationWindow::eventFilter(QObject* obj, QEvent* e)
{
	// todo 暂时屏蔽
	//m_editLine->hide();

	auto classname = obj->metaObject()->className();
	QString objName = obj->objectName();
	QEvent::Type eventNow = e->type();
	if (QEvent::MouseMove == eventNow)
	{
		QMouseEvent* evev = dynamic_cast<QMouseEvent*>(e);
		if (classname == QStringLiteral("QWidgetWindow") && objName == "ApplicationWindowClassWindow")
		{			
			m_presentPos = evev->pos();
			QPoint movePoint = m_presentPos;
			QPoint maxPoint(m_presentPos.x() + m_editLine->width() + 10, m_presentPos.y() + m_editLine->height() + 10);
			if (!m_editLine->isHidden())
			{
				if (maxPoint.x() > m_pointVec[6].x() && maxPoint.y() < m_pointVec[6].y())
				{
					m_editLine->move(m_lineWidgetPos.x() + 10, movePoint.y() + 10);
					m_lineWidgetPos.setY(m_presentPos.y());
				}
				else if (maxPoint.y() > m_pointVec[6].y() && maxPoint.x() < m_pointVec[6].x())
				{
					m_editLine->move(movePoint.x() + 10, m_lineWidgetPos.y() + 10);
					m_lineWidgetPos.setX(m_presentPos.x());
				}
				else if (maxPoint.x() < m_pointVec[6].x() && maxPoint.y() < m_pointVec[6].y())
				{
					m_editLine->move(movePoint.x() + 10, movePoint.y() + 10);
					m_lineWidgetPos = m_presentPos;
				}
			}

			if (!pointInPolygon(m_presentPos, m_pointVec))
			{
				m_editLine->hide();
			}
			else
			{
				m_editLine->show();
				//m_cmdWin->m_editline->setFocus();
			}
		}	

		if (m_isPressed)
		{			
			m_isMove = true;
		}
	}
	else if (QEvent::MouseButtonPress == eventNow)
	{
		QMouseEvent* evev = dynamic_cast<QMouseEvent*>(e);
		if (obj != m_pBottomWidget->m_pUnitBtn && classname != QStringLiteral("QWidgetWindow"))
		{
			auto pBottomComboBoxWidget = m_pBottomWidget->getComboBoxWidget();
			if (!pBottomComboBoxWidget->isHidden())
			{
				pBottomComboBoxWidget->hide();
				pBottomComboBoxWidget->lower();
			}
		}

		if (obj != m_pBottomWidget->m_pViewBtn && classname != QStringLiteral("QWidgetWindow"))
		{
			auto viewportsWidget = m_pBottomWidget->getViewportsWidget();
			if (!viewportsWidget->isHidden())
			{
				viewportsWidget->hide();
				viewportsWidget->lower();
			}
		}

		//if (obj != m_pBottomWidget->m_pTheme && classname != QStringLiteral("QWidgetWindow"))
		//{
		//	auto pBottomComboBoxWidget = m_pBottomWidget->getThemeComBoxWidget();
		//	if (!pBottomComboBoxWidget->isHidden())
		//	{
		//		pBottomComboBoxWidget->hide();
		//		pBottomComboBoxWidget->lower();
		//	}
		//}

		if (obj != m_pBottomWidget->m_pSnapModeBtn && classname != QStringLiteral("QWidgetWindow"))
		{
			auto pBottomToolWidget = m_pBottomWidget->getToolWidget();
			if (!pBottomToolWidget->isHidden() && !pointInPolygon(m_presentPos, m_snapWidgetVec))
			{
				pBottomToolWidget->hide();
				pBottomToolWidget->lower();
			}
		}

		if (Qt::LeftButton == evev->button())
		{
			m_isPressed = true;
			m_startMovePos = evev->globalPos();
			m_quadrant = countRow(evev->pos(), countLine(evev->pos(), this->width()), this->height());

			//----------------------------------------------------

		}
		else if (Qt::RightButton == evev->button())
		{
			if (m_pCurrentMdiWin)
			{
				GuiDocumentView* gv = m_pCurrentMdiWin->getDocumentView();
				//gv->deleteRelativeZero();
				GuiEventHandler* handle = gv->getEventHandler();
				int actionNum = handle->getCurrentActionNum();
				if (actionNum == 0)
				{
					m_editLine->hide();
					m_editLine->lower();
					m_cmdWin->getEditline()->setText("");
				}
			}
		}
	}
	else if (QEvent::MouseButtonRelease == eventNow)
	{
		int ry = mapToGlobal(this->pos()).ry();

		//-------------------------------------------------------拖拉至顶部放大--------------------------------------------------------
		if (ry <= 3 && m_startMovePos.y() <= 55 && !this->isMaximized() && m_isMove == true && m_isDoubleClick == false)
		{
			this->showMaximized();
		}
		m_isMove = false;
		m_isPressed = false;
		m_isDoubleClick = false;
	}
	else if (QEvent::MouseButtonDblClick == eventNow)
	{
		QMouseEvent* evev = dynamic_cast<QMouseEvent*>(e);
		if (Qt::LeftButton == evev->button())
		{
			if (classname == QStringLiteral("SARibbonTabBar"))
			{
				return true;
			}
		}
		m_isDoubleClick = true;
	}

	//QEvent::Type eventNow = e->type();
	// 右键隐藏捕捉圆圈
	if (QEvent::MouseButtonPress == eventNow)
	{
		QMouseEvent* evev = dynamic_cast<QMouseEvent*>(e);
		if (Qt::RightButton == evev->button())
		{
			if (m_pCurrentMdiWin)
			{
				GuiDocumentView* gv = m_pCurrentMdiWin->getDocumentView();
				gv->hideRelativeZero(false);
			}
		}
	}

	return SARibbonMainWindow::eventFilter(obj, e);
}

void ApplicationWindow::updateUndoRedo()
{
    if (nullptr == m_pCurrentMdiWin)
    {
        return;
    }

    DmDocument* document = m_pCurrentMdiWin->getDocument();
    bool hasUndo = false;
    std::string undoName;
    bool hasRedo = false;
    std::string redoName;
    document->getCmdData(hasUndo, undoName, hasRedo, redoName);
    if (hasUndo)
    {
        m_pActUndo->setEnabled(true);
        m_pActUndo->setText(QObject::tr("Undo") + ": " + QString::fromStdString(undoName));
    }
    else
    {
        m_pActUndo->setEnabled(false);
        m_pActUndo->setText("");
    }
    if (hasRedo)
    {
        m_pActRedo->setEnabled(true);
        m_pActRedo->setText(QObject::tr("Redo") + ": " + QString::fromStdString(redoName));
    }
    else
    {
        m_pActRedo->setEnabled(false);
        m_pActRedo->setText("");
    }
}
