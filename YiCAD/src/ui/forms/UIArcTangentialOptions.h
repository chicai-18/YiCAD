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

/// @file UIArcTangentialOptions.h
/// @brief 相切圆弧选项控件

#ifndef UIARCTANGENTIALOPTIONS_H
#define UIARCTANGENTIALOPTIONS_H

#include <memory>
#include <QWidget>

class ActionInterface;
class ActionDrawArcTangential;

namespace Ui
{
class Ui_ArcTangentialOptions;
}

class UIArcTangentialOptions : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIArcTangentialOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIArcTangentialOptions();

public slots:
    /// @brief 设置当前 Action
    /// @param [in] a Action 接口指针
    /// @param [in] update 是否从 Action 更新界面
    virtual void setAction(ActionInterface* a, bool update);

    /// @brief 更新半径显示
    /// @param [in] s 半径字符串
    virtual void updateRadius(const QString& s);

    /// @brief 更新角度显示
    /// @param [in] s 角度字符串
    virtual void updateAngle(const QString& s);

    /// @brief 锁定角度切换槽
    /// @param [in] lock 是否锁定
    void slotLockAngle(bool lock);

    /// @brief 锁定半径切换槽
    /// @param [in] lock 是否锁定
    void slotLockRadius(bool lock);

protected:
    ActionDrawArcTangential* action;                     ///< 相切圆弧 Action 指针

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();

private slots:
    void on_leRadius_editingFinished();
    void on_leAngle_editingFinished();
    void on_rbRadius_clicked(bool checked);
    void on_rbAngle_clicked(bool checked);

private:
    std::unique_ptr<Ui::Ui_ArcTangentialOptions> ui;     ///< UI 对象
};

#endif // UIARCTANGENTIALOPTIONS_H
