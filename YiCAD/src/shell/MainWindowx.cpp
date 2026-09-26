/*
**********************************************************************************
** Copyright (C) 2015 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/


/// @file MainWindowx.cpp
/// @brief 窗体基类实现文件
/// @details 实现窗口停靠区域的显示/隐藏控制功能

#include "MainWindowx.h"

#include <QDockWidget>
#include <QToolBar>
#include <algorithm>

namespace Sorting
{
    /// @brief 按窗口标题比较两个控件
    /// @param [in] left 左侧控件
    /// @param [in] right 右侧控件
    /// @return true 如果左侧标题小于右侧标题
    bool byWindowTitle(QWidget* left, QWidget* right)
    {
        return left->windowTitle() < right->windowTitle();
    }
}

/// @brief 构造函数
/// @param [in] parent 父窗口指针
MainWindowX::MainWindowX(QWidget* parent)
    : QMainWindow(parent)
{
}

/// @brief 按标题排序停靠控件列表
/// @param [in,out] list 待排序的停靠控件列表
void MainWindowX::sortWidgetsByTitle(QList<QDockWidget*>& list)
{
    std::sort(list.begin(), list.end(), Sorting::byWindowTitle);
}

/// @brief 按标题排序工具栏列表
/// @param [in,out] list 待排序的工具栏列表
void MainWindowX::sortWidgetsByTitle(QList<QToolBar*>& list)
{
    std::sort(list.begin(), list.end(), Sorting::byWindowTitle);
}

/// @brief 切换左侧停靠区域显示/隐藏
/// @param [in] state true=显示，false=隐藏
void MainWindowX::toggleLeftDockArea(bool state)
{
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::LeftDockWidgetArea && !dw->isFloating())
        {
            dw->setVisible(state);
        }
    }
}

/// @brief 切换右侧停靠区域显示/隐藏
/// @param [in] state true=显示，false=隐藏
void MainWindowX::toggleRightDockArea(bool state)
{
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::RightDockWidgetArea && !dw->isFloating())
        {
            dw->setVisible(state);
        }
    }
}

/// @brief 切换顶部停靠区域显示/隐藏
/// @param [in] state true=显示，false=隐藏
void MainWindowX::toggleTopDockArea(bool state)
{
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::TopDockWidgetArea && !dw->isFloating())
        {
            dw->setVisible(state);
        }
    }
}

/// @brief 切换底部停靠区域显示/隐藏
/// @param [in] state true=显示，false=隐藏
void MainWindowX::toggleBottomDockArea(bool state)
{
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dockWidgetArea(dw) == Qt::BottomDockWidgetArea && !dw->isFloating())
        {
            dw->setVisible(state);
        }
    }
}

/// @brief 切换浮动停靠控件显示/隐藏
/// @param [in] state true=显示，false=隐藏
void MainWindowX::toggleFloatingDockwidgets(bool state)
{
    foreach (QDockWidget* dw, findChildren<QDockWidget*>())
    {
        if (dw->isFloating())
        {
            dw->setVisible(state);
        }
    }
}
