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
/// @brief MDI文档窗口类实现，管理单个CAD文档的视图、文件I/O和子窗口

#include<iostream>
#include "MDIWindow.h"

#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QMessageBox>
#include <QFileInfo>
#include <QMdiArea>
#include <QPainter>

#include "DmDocument.h"
#include "DmSettings.h"
#include "UIExitDialog.h"
#include "UIFileDialog.h"
#include "DmBlockReference.h"
#include "DmMText.h"
#include "DmPen.h"
#include "GuiDocumentView.h"
#include "Debug.h"

int MDIWindow::idCounter = 0;

/// @brief MDIWindow构造函数
/// @param [in] doc 已有文档指针，若为nullptr则创建新文档
/// @param [in] parent 父窗口QMdiArea实例
/// @param [in] wflags 窗口标志
MDIWindow::MDIWindow(DmDocument* doc, QWidget* parent, Qt::WindowFlags wflags)
    : QMdiSubWindow(parent, wflags)
{
    setAttribute(Qt::WA_DeleteOnClose);

    if (doc == nullptr)
    {
        document = new DmDocument();
        document->initDoc();
        owner = true;
    }
    else
    {
        document = doc;
        owner = false;
    }

    docView = new GuiDocumentView(this, Qt::WindowFlags(), document);
    docView->setObjectName("documentview");

    setWidget(docView);

    id = idCounter++;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

/// @brief 析构函数，删除与此窗口关联的文档
MDIWindow::~MDIWindow()
{
    if (!(docView && docView->isCleanUp()))
    {
        if (owner == true && document)
        {
            delete document;
        }
        document = nullptr;
    }
}

GuiDocumentView* MDIWindow::getDocumentView() const
{
    return (docView) ? docView : nullptr;
}

int MDIWindow::getId() const
{
    return id;
}

GuiEventHandler* MDIWindow::getEventHandler() const
{
    if (docView)
    {
        return docView->getEventHandler();
    }
    else
    {
        return nullptr;
    }
}

void MDIWindow::setParentWindow(MDIWindow* p)
{
    parentWindow = p;
}

MDIWindow* MDIWindow::getParentWindow() const
{
    return parentWindow;
}

DmDocument* MDIWindow::getDocument() const
{
    return document;
}

/// @brief 将另一个MDI窗口添加到已知子窗口列表
/// @param [in] w 子窗口指针（可以是另一个视图或特定块的视图）
void MDIWindow::addChildWindow(MDIWindow* w)
{
    childWindows.append(w);
    w->setParentWindow(this);
}

/// @brief 移除子窗口
/// @param [in] w 待移除的子窗口指针
void MDIWindow::removeChildWindow(MDIWindow* w)
{
    if (childWindows.size() > 0)
    {
        if (childWindows.contains(w))
        {
            childWindows.removeAll(w);
        }
    }
}

QList<MDIWindow*>& MDIWindow::getChildWindows()
{
    return childWindows;
}

/// @brief 获取打印预览窗口
/// @return 打印预览窗口指针，若无则返回nullptr
MDIWindow* MDIWindow::getPrintPreview()
{
    for (auto w : childWindows)
    {
        if (w->getDocumentView()->isPrintPreview())
        {
            return w;
        }
    }
    return nullptr;
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

    if (document && !fileName.isEmpty())
    {
        ret = document->open(fileName);

        if (ret)
        {
            document->regenerate();
        }
    }

    return ret;
}

void MDIWindow::slotZoomAuto()
{
    if (docView)
    {
        if (docView->isPrintPreview())
        {
            // docView->zoomPage(); // TODO: 打印功能暂未实现
        }
        else
        {
            docView->zoomAuto();
        }
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

    if (document)
    {
        document->setDocumentView(docView);
        if (isAutoSave)
        {
            ret = document->save(true);
        }
        else
        {
            if (document->getFilename().isEmpty())
            {
                ret = slotFileSaveAs(cancelled);
            }
            else
            {
                QFileInfo info(document->getFilename());
                if (!info.isWritable())
                {
                    return false;
                }
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                ret = document->save();
                QApplication::restoreOverrideCursor();
            }
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
    if (document && !fn.isEmpty())
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        document->setDocumentView(docView);
        ret = document->saveAs(fn, formatType, true);
        QApplication::restoreOverrideCursor();
    }
    else
    {
        ret = true;
        cancelled = true;
    }

    return ret;
}

void MDIWindow::slotFilePrint()
{
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec())
    {
        QPainter painter;
        painter.begin(&printer);
        painter.end();
    }
}

/// @brief 流输出操作符，将MDI窗口信息输出到流
/// @param [in,out] os 输出流
/// @param [in] w MDI窗口引用
/// @return 输出流引用
std::ostream& operator << (std::ostream& os, MDIWindow& w)
{
    os << "MDIWindow[" << w.getId() << "]:\n";
    if (w.parentWindow)
    {
        os << "  parentWindow: " << w.parentWindow->getId() << "\n";
    }
    else
    {
        os << "  parentWindow: NULL\n";
    }
    int i = 0;
    for (auto p : w.childWindows)
    {
        os << "  childWindow[" << i++ << "]: " << p->getId() << "\n";
    }
    return os;
}

/// @brief 判断是否有子窗口
/// @return true 如果有子窗口
bool MDIWindow::has_children()
{
    return !childWindows.isEmpty();
}
