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

/// @file UIImageOptions.h
/// @brief 图片插入选项控件

#ifndef UIIMAGEOPTIONS_H
#define UIIMAGEOPTIONS_H

#include <memory>
#include <QWidget>

class ActionInterface;
class ActionDrawImage;
namespace Ui {
    class Ui_ImageOptions;
}

class UIImageOptions : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIImageOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIImageOptions();

public slots:
    /// @brief 设置关联的动作
    /// @param [in] a 动作接口指针
    /// @param [in] update 是否更新数据
    virtual void setAction(ActionInterface* a, bool update);

    virtual void updateData();
    virtual void updateDPI();
    virtual void updateFactor();

protected:
    ActionDrawImage* action = nullptr; ///< 关联的图片绘制动作

protected slots:
    virtual void languageChange();

private:
    void saveSettings();

    std::unique_ptr<Ui::Ui_ImageOptions> ui; ///< UI 对象
};

#endif // UIIMAGEOPTIONS_H
