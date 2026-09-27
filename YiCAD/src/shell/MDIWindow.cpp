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

/// @file MDIWindow.cpp
/// @brief 图纸窗口的实现：承载视图，处理打开、保存、另存为的文件对话框与等待光标

#include "MDIWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QFileInfo>

#include "AppDocument.h"
#include "DmDocument.h"
#include "DocumentFileService.h"
#include "SelectionSet.h"
#include "UIFileDialog.h"
#include "UIView.h"

/// @brief MDIWindow构造函数，新建一份空白图纸
/// @param [in] documents 宿主管理的打开图纸，交给文档文件服务取未命名文档的名字；必须比本窗口活得久
/// @param [in] parent 父窗口QMdiArea实例
/// @param [in] wflags 窗口标志
MDIWindow::MDIWindow(const IDocumentManager& documents, QWidget* parent, Qt::WindowFlags wflags)
    : QMdiSubWindow(parent, wflags)
    , appDocument(std::make_unique<AppDocument>(documents))
{
    setAttribute(Qt::WA_DeleteOnClose);

    docView = new UIView(this, Qt::WindowFlags(), appDocument.get());
    docView->setObjectName("documentview");

    setWidget(docView);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

/// @brief 析构函数，先释放视图，再释放图纸
///
/// 视图是文档的监听者，析构时从文档注销，还引用图纸的选择集，必须先于图纸释放。它是本窗口的子控件，
/// 不在这里删就要等基类析构时才释放，那时文档已经删除。图纸内部先释放选择集与文档文件服务、再释放文档。
MDIWindow::~MDIWindow()
{
    delete docView;
    docView = nullptr;
    appDocument.reset();
}

GuiDocumentView* MDIWindow::getDocumentView() const
{
    return (docView) ? docView : nullptr;
}

DmDocument* MDIWindow::getDocument() const
{
    return &appDocument->document();
}

SelectionSet* MDIWindow::getSelection() const
{
    return &appDocument->selection();
}

/// @brief 关闭事件处理（由Qt在用户关闭此MDI窗口时调用）
/// @param [in] ce 关闭事件
void MDIWindow::closeEvent(QCloseEvent* ce)
{
    emit(signalClosing(this));
    ce->accept();
}

/// @brief 在此MDI窗口中打开指定文件
/// @param [in] fileName 文件路径
/// @return true 如果打开成功
bool MDIWindow::slotFileOpen(const QString& fileName)
{
    bool ret = false;

    if (!fileName.isEmpty())
    {
        ret = appDocument->fileService().open(fileName);

        if (ret)
        {
            appDocument->document().regenerate();
        }
    }

    return ret;
}

void MDIWindow::slotZoomAuto()
{
    if (docView)
    {
        docView->zoomAuto();
    }
}

/// @brief 保存当前文件
/// @param [out] cancelled 用户是否取消操作
/// @param [in] isAutoSave 是否为自动保存操作
/// @return true 如果保存成功
bool MDIWindow::slotFileSave(bool& cancelled, bool isAutoSave)
{
    bool ret = false;
    cancelled = false;

    if (isAutoSave)
    {
        ret = appDocument->fileService().save(true);
    }
    else
    {
        const QString fileName = appDocument->document().getFilename();
        if (fileName.isEmpty())
        {
            ret = slotFileSaveAs(cancelled);
        }
        else
        {
            QFileInfo info(fileName);
            if (!info.isWritable())
            {
                return false;
            }
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            ret = appDocument->fileService().save();
            QApplication::restoreOverrideCursor();
        }
    }

    return ret;
}

/// @brief 另存为当前文件，弹出对话框让用户选择新文件名和格式
/// @param [out] cancelled 用户是否取消操作
/// @return true 如果保存成功或用户取消
bool MDIWindow::slotFileSaveAs(bool& cancelled)
{
    bool ret = false;
    cancelled = false;

    UIFileDialog dlg(this);
    QString formatType;
    QString fn = dlg.getSaveFile(formatType);
    if (!fn.isEmpty())
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        ret = appDocument->fileService().saveAs(fn, formatType, true);
        QApplication::restoreOverrideCursor();
    }
    else
    {
        ret = true;
        cancelled = true;
    }

    return ret;
}
