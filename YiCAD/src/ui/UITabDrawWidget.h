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

/// @file UITabDrawWidget.h
/// @brief 主窗体绘图选项卡控件

#ifndef UITABDRAWWIDGET_H
#define UITABDRAWWIDGET_H

#include <QWidget>
#include <QMdiArea>

#include "Datamodel.h"
#include "SAWindowButtonGroup.h"

class QToolButton;
class QPushButton;
class MDIWindow;
class DmDocument;
class UIActionHandler;
class UIBottomWindow;
class UICurrentActivePen;

// 单个选项卡信息
struct SingleTabDrawDataRibbon
{
	QString			name;					///< 选项卡名
	QString			fullPath;				///< 文件路径。如果文件存在，是全路径名。如果不存在，是相对路径名。
	int				number = 0;				///< 编号
	bool			isCurrent = false;		///< 是否为当前选中
	QWidget*		tabBack = nullptr;		///< 导航条底板
	QWidget*		theWidget = nullptr;	///< 这个选项卡底板
	QPushButton*	textBtn = nullptr;		///< 图纸名
	QPushButton*	closeDraw = nullptr;	///< 关闭按钮
	MDIWindow*		mdiWindow = nullptr;	///< 绘图画布
	bool			isPreviewPrint = false;	///< 是否为打印预览
};

/// @class UITabDrawWidget
/// @brief 主窗体绘图区域和选项卡
class UITabDrawWidget : public QWidget
{
	Q_OBJECT

public:
	UITabDrawWidget(QWidget* parent);
	~UITabDrawWidget();

public:
	/// @brief 创建图纸选项卡
	QWidget* createTabDrawWidget(QMdiArea* drawBackWidget, UIActionHandler* pActionHandler, UIBottomWindow* pBottomWidget);

	/// @brief 根据主窗口和Ribbon实际尺寸重排图纸选项卡、当前画笔栏和绘图区域
	void layoutWidgets(int contentTop, int windowWidth, int windowHeight, int bottomHeight);

	/// @brief 获取当前画布
	MDIWindow* getCurrentMdiWindow();

	/// @brief 获取当前选项卡信息
	SingleTabDrawDataRibbon* getCurrentTabDrawData();

	/// @brief 获得指定文档的选项卡信息
	SingleTabDrawDataRibbon* getTabDrawDataOfDocument(const DmDocument* document) const;

	/// @brief 获取所有选项卡列表
	std::vector<SingleTabDrawDataRibbon*>* getTabDrawList();

	/// @brief 获得打开的文档
	std::vector<DmDocument*> getDocuments() const;

	/// @brief 新建选项卡和绘图区域
	void newTabDraw(SingleTabDrawDataRibbon* newTab);

	/// @brief 重命名选项卡
	void renameTabDraw(SingleTabDrawDataRibbon* tab, const QString& fileName, const QString& fullFileName);

	/// @brief 新建默认绘图区域MdiWindow
	MDIWindow* createMdiWindow();

	/// @brief 获取选项卡底板
	QWidget* getBackTabDrawWidget();

	/// @brief 关闭所有选项卡
	void closeTabAll();

	UICurrentActivePen* getCurrentActivePen();

	/// @brief UICurrentActivePen更新事件
	void updateUICurrentActivePen();

	void swapBtnWidget(int currentIndex, int index);

	/// @brief 选项卡切换刷新菜单和左右栏ui
	void tabChangeEvent();

signals:
	void tabChangeSignals();

public:
	/// @brief 新建指定名字的图纸，名字为空则自动指定
	void slotFileNew(const QString& fileName);

	/// @brief 设置当前选项卡名字和提示
	void soltSetDrawingTabName(const QString& fileName);

	/// @brief 打开图纸
	void slotFileOpen();

	/// @brief 导出图片
	void slotFileExportImage();

	/// @brief 文件打印为PDF
	void slotFilePrintPDF();

	/// @brief 保存当前图纸
	void slotFileSave();

	/// @brief 图纸另存为
	void slotFileSaveAs();

	/// @brief 保存所有图纸
	void slotFileSaveAll();

	/// @brief 打印图纸
	void slotFilePrint();
	/// @brief 打印预览
	void slotFilePrintPreview();

	/// @brief 关闭所有图纸
	void slotFileCloseAll();

	/// @brief 显示关闭对话框
	int showCloseDialog(MDIWindow* w, bool showSaveAll = false);

	/// @brief 关闭选项卡
	void closeTab(SingleTabDrawDataRibbon* childTab);

private:
	/// @brief 打开图纸
	void slotFileOpen(const QString& fileName);

	/// @brief 导出图纸
	bool slotFileExport(const QString& name, const QString& format, QSize size, QSize borders, bool black, bool bw = true);

	/// @brief 保存图纸
	bool slotFileSave(MDIWindow* w, bool forceSaveAs = false);

	/// @brief 关闭图纸
	bool slotFileClosing(MDIWindow* pMdiWin);

	bool doSave(MDIWindow* w, bool forceSaveAs = false);
	void doClose(MDIWindow* w, bool activateNext = true);

	/// @brief 设置切换绘图区域
	void setChangeTabDrawArea();

	/// @brief 选项卡清空事件
	void tabClearEvent();

	/// @brief 根据当前选项卡栏宽度重排所有图纸选项卡
	void layoutTabButtons();

	/// @brief 更新指定标签页的样式表
	void updateTabStyleSheet(SingleTabDrawDataRibbon* tab);
private:
	// 鼠标跟随框
	virtual bool eventFilter(QObject* obj, QEvent* e);

public:
	int                                         m_btnIndex = 0;
	QWidget*                                    m_currentBtnWidget = nullptr;

private:
	QWidget*									m_pWidget = nullptr;
	MDIWindow*									m_currentMdiWindow = nullptr;		///< 当前绘图画布
	std::vector<SingleTabDrawDataRibbon*>*		m_pTabDrawList = nullptr;			///< 选项卡和绘图区域集合
	QWidget*									m_pBackTabDrawWidget = nullptr;		///< 选项卡底板
	QWidget*									m_pPenWidget = nullptr;				///< 当前画笔栏底板

	QMdiArea*									m_pDrawBackWidget = nullptr;		///< 绘图选项卡背板
	UIActionHandler*							m_pActionHandler = nullptr;			///< 槽实现类
	UIBottomWindow*								m_pBottomWidget = nullptr;			///< 底部状态栏

	UICurrentActivePen*							m_pCurrentActivePen = nullptr;		///< 当前文档激活的画笔
	bool                                        m_isPress = false;
	QPoint                                      m_startPos;
	QPoint                                      m_currentPos;
	QPoint                                      m_tmpPos;
};

#endif // UITABDRAWWIDGET_H
