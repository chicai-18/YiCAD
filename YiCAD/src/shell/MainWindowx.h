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

/// @file MainWindowx.h
/// @brief 窗体基类头文件
/// @details 提供窗口停靠区域控制功能

#ifndef MAINWINDOWX_H
#define MAINWINDOWX_H

#include <QMainWindow>

/// @brief 窗体基类，提供停靠区域显示/隐藏控制功能
class MainWindowX : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindowX(QWidget* parent = nullptr);

    /// @brief 按标题排序停靠控件列表
    /// @param [in,out] list 待排序的停靠控件列表
    void sortWidgetsByTitle(QList<QDockWidget*>& list);

    /// @brief 按标题排序工具栏列表
    /// @param [in,out] list 待排序的工具栏列表
    void sortWidgetsByTitle(QList<QToolBar*>& list);

public slots:
    /// @brief 切换右侧停靠区域显示/隐藏
    /// @param [in] state true=显示，false=隐藏
    void toggleRightDockArea(bool state);

    /// @brief 切换左侧停靠区域显示/隐藏
    /// @param [in] state true=显示，false=隐藏
    void toggleLeftDockArea(bool state);

    /// @brief 切换顶部停靠区域显示/隐藏
    /// @param [in] state true=显示，false=隐藏
    void toggleTopDockArea(bool state);

    /// @brief 切换底部停靠区域显示/隐藏
    /// @param [in] state true=显示，false=隐藏
    void toggleBottomDockArea(bool state);

    /// @brief 切换浮动停靠控件显示/隐藏
    /// @param [in] state true=显示，false=隐藏
    void toggleFloatingDockwidgets(bool state);
};

#endif  // MAINWINDOWX_H
