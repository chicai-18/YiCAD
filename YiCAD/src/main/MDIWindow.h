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

/// @file MDIWindow.h
/// @brief MDI文档窗口类，封装单个CAD文档及其视图

#ifndef MDIWINDOW_H
#define MDIWINDOW_H

#include <QList>
#include <QMdiSubWindow>

#include "Datamodel.h"

class GuiDocumentView;
class DmDocument;
class DmPen;
class QMdiArea;
class GuiEventHandler;
class QCloseEvent;

/// @brief 文档窗体类，包含文档和视口
class MDIWindow : public QMdiSubWindow
{
    Q_OBJECT

public:
    MDIWindow(DmDocument* doc, QWidget* parent, Qt::WindowFlags wflags = Qt::WindowType::Widget);
    ~MDIWindow();

public slots:
    /// @brief 打开文件
    /// @param [in] fileName 文件路径
    /// @return true 如果打开成功
    bool slotFileOpen(const QString& fileName);

    /// @brief 保存文件
    /// @param [out] cancelled 用户是否取消操作
    /// @param [in] isAutoSave 是否自动保存
    /// @return true 如果保存成功
    bool slotFileSave(bool& cancelled, bool isAutoSave = false);

    /// @brief 另存为文件
    /// @param [out] cancelled 用户是否取消操作
    /// @return true 如果保存成功
    bool slotFileSaveAs(bool& cancelled);

    void slotFilePrint();
    void slotZoomAuto();

public:
    /// @brief 获取文档视图
    /// @return 文档视图指针
    GuiDocumentView* getDocumentView() const;

    /// @brief 获取文档对象
    /// @return 文档对象指针
    DmDocument* getDocument() const;

    /// @brief 获取当前事件处理器
    /// @return 事件处理器指针
    GuiEventHandler* getEventHandler() const;

    /// @brief 添加子窗口
    /// @param [in] w 子窗口指针
    void addChildWindow(MDIWindow* w);

    /// @brief 移除子窗口
    /// @param [in] w 子窗口指针
    void removeChildWindow(MDIWindow* w);

    /// @brief 获取子窗口列表
    /// @return 子窗口列表引用
    QList<MDIWindow*>& getChildWindows();

    /// @brief 获取打印预览窗口
    /// @return 打印预览窗口指针
    MDIWindow* getPrintPreview();

    /// @brief 设置父窗口（当本窗口关闭时需要通知的窗口）
    /// @param [in] p 父窗口指针
    void setParentWindow(MDIWindow* p);

    /// @brief 获取父窗口
    /// @return 父窗口指针
    MDIWindow* getParentWindow() const;

    /// @brief 获取MDI窗口ID
    /// @return 窗口ID
    int getId() const;

    friend std::ostream& operator<<(std::ostream& os, MDIWindow& w);

    /// @brief 判断是否有子窗口
    /// @return true 如果有子窗口
    bool has_children();

signals:
    void signalClosing(MDIWindow*);

protected:
    void closeEvent(QCloseEvent*);

private:
    int                     id = 0;                     ///< 窗口ID
    static int              idCounter;                  ///< ID计数器
    GuiDocumentView*        docView = nullptr;          ///< 文档视图
    DmDocument*             document = nullptr;         ///< 关联的文档对象
    bool                    owner = false;              ///< 窗口是否拥有文档的所有权
    QList<MDIWindow*>       childWindows;               ///< 已知子窗口列表（显示同一图纸的块）
    MDIWindow*              parentWindow = nullptr;     ///< 父窗口指针（需要知道本窗口是否关闭）
};

#endif
