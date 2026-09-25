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

/// @file UIDialogRunner.h
/// @brief 模态运行对话框的统一入口
///
/// 扩展自己构造对话框，经 UIDialogRunner::exec() 运行，而不是直接调用 QDialog::exec()。
/// 单测装入替身运行方式：不弹窗，记下弹出的是哪个对话框并给出结果（例如"取消"），
/// 这样"取消对话框时命令启动失败"之类的路径可以无头测试。

#ifndef UIDIALOGRUNNER_H
#define UIDIALOGRUNNER_H

#include <functional>

class QDialog;

/// @brief 模态运行对话框；运行方式可替换（单测用）
class UIDialogRunner
{
public:
    /// @brief 运行方式：对话框进来，QDialog::exec() 的结果出去
    using Runner = std::function<int(QDialog&)>;

    /// @brief 模态运行对话框
    /// @return QDialog::exec() 的结果；装了替身时返回替身的结果
    static int exec(QDialog& dialog);

    /// @brief 装入替身运行方式
    /// @param runner 为空时恢复 QDialog::exec()
    /// @return 原先的运行方式（为空表示原先是 QDialog::exec()），供调用方恢复
    static Runner setRunner(Runner runner);
};

#endif // UIDIALOGRUNNER_H
