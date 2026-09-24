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

/// @file UICloudLineOptions.h
/// @brief 云线选项控件

#ifndef UICLOUDLINEOPTIONS_H
#define UICLOUDLINEOPTIONS_H

#include <memory>
#include <QWidget>

class CloudLineCommand;
class IExclusiveCommand;

namespace Ui
{
class UICloudLineOptions;
}

class UICloudLineOptions : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UICloudLineOptions(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UICloudLineOptions();

public slots:
    /// @brief 设置当前命令
    /// @param [in] c 命令；不是云线命令时视为没有命令
    /// @param [in] update 是否从命令更新界面
    virtual void setCommand(IExclusiveCommand* c, bool update);

    /// @brief 撤销操作
    virtual void undo();

    /// @brief 更新最小长度
    /// @param [in] s 长度字符串
    virtual void updateMinLength(const QString& s);

    /// @brief 更新最大长度
    /// @param [in] s 长度字符串
    virtual void updateMaxLength(const QString& s);

    /// @brief 更新方向反转
    /// @param [in] reverse 反转状态值
    virtual void updateReverse(int reverse);

protected:
    CloudLineCommand* command = nullptr;             ///< 矩形、多边形或自由云线命令

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();

private:
    /// @brief 销毁时保存设置
    void destroy();

    std::unique_ptr<Ui::UICloudLineOptions> ui;       ///< UI 对象
};

#endif // UICLOUDLINEOPTIONS_H
