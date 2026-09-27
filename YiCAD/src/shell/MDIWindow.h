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
/// @brief 图纸窗口：持有一份打开的图纸，承载它的视图

#ifndef MDIWINDOW_H
#define MDIWINDOW_H

#include <memory>

#include <QMdiSubWindow>

class AppDocument;
class GuiDocumentView;
class DmDocument;
class IDocumentManager;
class QCloseEvent;

/// @brief 图纸窗口：持有一份打开的图纸，承载它的视图，处理文件对话框
class MDIWindow : public QMdiSubWindow
{
    Q_OBJECT

public:
    MDIWindow(const IDocumentManager& documents, QWidget* parent,
              Qt::WindowFlags wflags = Qt::WindowType::Widget);
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

    void slotZoomAuto();

public:
    /// @brief 获取文档视图
    /// @return 文档视图指针
    GuiDocumentView* getDocumentView() const;

    /// @brief 获取文档对象
    /// @return 文档对象指针
    DmDocument* getDocument() const;

signals:
    void signalClosing(MDIWindow*);

protected:
    void closeEvent(QCloseEvent*);

private:
    GuiDocumentView*             docView = nullptr;  ///< 文档视图
    std::unique_ptr<AppDocument> appDocument;        ///< 本窗口显示的图纸：文档与它的存盘策略
};

#endif
