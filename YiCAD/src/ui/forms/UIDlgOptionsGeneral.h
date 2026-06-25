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

/// @file UIDlgOptionsGeneral.h
/// @brief 系统设置窗体头文件

#ifndef UIDLGOPTIONSGENERAL_H
#define UIDLGOPTIONSGENERAL_H

#include "ui_UIDlgOptionsGeneral.h"

/// @brief 系统设置窗体
class UIDlgOptionsGeneral : public QDialog, public Ui::UIDlgOptionsGeneral
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态对话框
    /// @param [in] fl 窗口标志
    UIDlgOptionsGeneral(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgOptionsGeneral();

    static int current_tab; ///< 当前选项卡索引

    /// @brief 设置并弹出颜色选择对话框
    /// @param [in,out] combo 颜色下拉框，用于读取当前颜色和写入新颜色
    /// @param [in] custom 自定义颜色预设值
    void set_color(QComboBox* combo, QColor custom);

public slots:
    virtual void setRestartNeeded();

    /// @brief 确认并保存设置
    virtual void ok();

protected slots:
    virtual void languageChange();

private slots:
    void on_tabWidget_currentChanged(int index);

    void on_pb_background_clicked();

    void on_pb_grid_clicked();

    void on_pb_meta_clicked();

    void on_pb_selected_clicked();

    void on_pb_highlighted_clicked();

    void on_pb_clear_all_clicked();

    void on_pb_clear_geometry_clicked();

    void slotKeyboardModifyClicked();

    //void on_comboBox_currentIndexChanged(int index);

private:
    bool restartNeeded = false; ///< 是否需要重启应用

    void init();
    void initComboBox(QComboBox* cb, const QString& text);

};

#endif // UIDLGOPTIONSGENERAL_H
