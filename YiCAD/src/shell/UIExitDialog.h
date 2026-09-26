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

/// @file UIExitDialog.h
/// @brief 退出/关闭文档确认对话框

#ifndef UIEXITDIALOG_H
#define UIEXITDIALOG_H

#include <memory>
#include <QDialog>

class QAbstractButton;

namespace Ui
{
    class UIExitDialog;
}

class UIExitDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIExitDialog(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIExitDialog();

    /// @brief 对话框结果枚举
    enum ExitDialogResult
    {
        Cancel,
        Close,
        Save,
        SaveAll
    };

public slots:
    /// @brief 设置提示文本
    /// @param [in] text 提示文本
    virtual void setText(const QString& text);

    /// @brief 设置对话框标题
    /// @param [in] text 标题文本
    virtual void setTitle(const QString& text);

    /// @brief 设置是否强制（禁用取消按钮）
    /// @param [in] force 是否强制
    virtual void setForce(bool force);

    /// @brief 设置是否显示"全部保存"按钮
    /// @param [in] show 是否显示
    virtual void setShowSaveAll(bool show);

    virtual void slotSaveAll();
    virtual void slotSave();
    virtual void clicked(QAbstractButton* button);

protected slots:
    virtual void languageChange();

private:
    void init();

    std::unique_ptr<Ui::UIExitDialog> ui; ///< UI 对象
};

#endif // UIEXITDIALOG_H
