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

/// @file UIArcOptions.h
/// @brief 圆弧绘制选项控件

#ifndef UIARCOPTIONS_H
#define UIARCOPTIONS_H

#include <memory>
#include <QWidget>

class ActionDrawArc;
class ActionInterface;

namespace Ui
{
class Ui_ArcOptions;
}

class UIArcOptions : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIArcOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIArcOptions();

public slots:
    /// @brief 设置当前 Action
    /// @param [in] a Action 接口指针
    /// @param [in] update 是否从 Action 更新界面
    virtual void setAction(ActionInterface* a, bool update);

    /// @brief 方向切换槽
    /// @param [in] checked 是否选中
    void slotRdoToggled(bool checked);

protected:
    ActionDrawArc* action;                       ///< 圆弧绘制 Action 指针
    std::unique_ptr<Ui::Ui_ArcOptions> ui;       ///< UI 对象

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();

private:
    /// @brief 保存设置到持久化存储
    void saveSettings();
};

#endif // UIARCOPTIONS_H
