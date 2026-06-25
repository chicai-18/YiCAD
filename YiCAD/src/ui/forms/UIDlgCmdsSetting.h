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

/// @file UIDlgCmdsSetting.h
/// @brief 命令快捷键设置对话框

#ifndef UIDLGCMDSSETTING_H
#define UIDLGCMDSSETTING_H

#include "ui_UIDlgCmdsSetting.h"
#include "Datamodel.h"

class UIDlgCmdsSetting : public QDialog, public Ui::UIDlgCmdsSetting
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIDlgCmdsSetting(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 设置命令分组
    /// @param [in] group 分组名称
    void setGroup(const QString& group);

private:
    /// @brief 初始化数据
    void init();

    /// @brief 加载数据到界面
    void loadDataToUI();

private slots:
    /// @brief 确定按钮槽
    void slotOk();

    /// @brief 按钮点击槽
    /// @param [in] button 被点击的按钮指针
    void slotBtnClicked(QAbstractButton* button);

private:
    QString m_group;                                                   ///< 命令分组名称
    std::vector<std::tuple<DM::ActionType, QString, QStringList>> m_data; ///< 命令数据列表
};

#endif // UIDLGCMDSSETTING_H
