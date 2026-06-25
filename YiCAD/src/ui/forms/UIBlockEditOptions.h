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

/// @file UIBlockEditOptions.h
/// @brief 块在位编辑选项栏
/// 显示正在编辑的块名称和"完成"按钮

#ifndef UIBLOCKEDITOPTIONS_H
#define UIBLOCKEDITOPTIONS_H

#include <QWidget>

class QLabel;
class QPushButton;
class ActionInterface;
class ActionBlocksEdit;

/// @class UIBlockEditOptions
/// @brief 块在位编辑选项栏
/// 显示正在编辑的块名称和"完成"按钮
class UIBlockEditOptions : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIBlockEditOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIBlockEditOptions() override;

public slots:
    /// @brief 设置当前 Action
    /// @param [in] a Action 接口指针
    void setAction(ActionInterface* a);

private slots:
    /// @brief 完成按钮点击槽
    void onCompleteClicked();

private:
    ActionBlocksEdit* m_action;          ///< 块编辑 Action 指针
    QLabel* m_label;                     ///< 当前编辑块名称标签
    QPushButton* m_completeButton;       ///< 完成按钮
};

#endif // UIBLOCKEDITOPTIONS_H
