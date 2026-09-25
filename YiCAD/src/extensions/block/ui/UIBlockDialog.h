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

/// @file UIBlockDialog.h
/// @brief 块对话框

#ifndef UIBLOCKDIALOG_H
#define UIBLOCKDIALOG_H

#include "ui_UIBlockDialog.h"

class DmBlockTable;
struct DmBlockData;

class UIBlockDialog : public QDialog, public Ui::UIBlockDialog
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIBlockDialog(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIBlockDialog() = default;

    /// @brief 获取块数据
    /// @return 块数据
    virtual DmBlockData getBlockData();

public slots:
    /// @brief 设置块表
    /// @param [in] l 块表指针
    virtual void setBlockList(DmBlockTable* l);

    /// @brief 验证并接受
    virtual void validate();

    /// @brief 取消操作
    virtual void cancel();

protected:
    DmBlockTable* blockTable;    ///< 块表指针

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();
};

#endif // UIBLOCKDIALOG_H
