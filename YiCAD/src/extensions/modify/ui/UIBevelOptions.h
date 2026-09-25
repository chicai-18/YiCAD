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

/// @file UIBevelOptions.h
/// @brief 倒角选项控件

#ifndef UIBEVELOPTIONS_H
#define UIBEVELOPTIONS_H

#include <memory>
#include <QWidget>

class ModifyBevelCommand;
class IExclusiveCommand;

namespace Ui
{
class Ui_BevelOptions;
}

class UIBevelOptions : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIBevelOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIBevelOptions();

public slots:
    /// @brief 设置关联的命令
    /// @param [in] c 命令；类型不符时视为没有命令
    /// @param [in] update 是否从命令更新界面
    virtual void setCommand(IExclusiveCommand* c, bool update);

    /// @brief 将界面数据更新到命令
    virtual void updateData();

protected:
    ModifyBevelCommand* command = nullptr; ///< 命令
    std::unique_ptr<Ui::Ui_BevelOptions> ui;         ///< UI 对象

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();

    /// @brief 保存设置到持久化存储
    void saveSettings();
};

#endif // UIBEVELOPTIONS_H
