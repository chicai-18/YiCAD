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

/// @file UIDlgDimensionStyleNew.h
/// @brief 新建标注样式对话框

#ifndef UIDLGDIMENSIONSTYLENEW_H
#define UIDLGDIMENSIONSTYLENEW_H

#include "ui_UIDlgDimensionStyleNew.h"
#include <memory>

namespace Ui
{
class UIDlgDimensionStyleNew;
}

class DmDocument;
class DmDimensionStyle;

class UIDlgDimensionStyleNew : public QDialog
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIDlgDimensionStyleNew(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIDlgDimensionStyleNew();

    /// @brief 初始化对话框
    /// @param [in] tempStyle 模板样式指针
    /// @param [in] document 文档指针
    void init(DmDimensionStyle* tempStyle, DmDocument* document);

public slots:
    /// @brief 对话框结果处理
    /// @param [in] r 结果代码
    virtual void done(int r);

private:
    /// @brief 校验输入是否有效
    /// @return 有效返回 true
    bool isInputValid();

private:
    std::unique_ptr<Ui::UIDlgDimensionStyleNew> ui;  ///< UI 对象
    DmDocument* m_pDocument;                         ///< 文档指针
};

#endif
