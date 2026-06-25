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

/// @file UIDlgEditAttributes.h
/// @brief 块属性编辑对话框，用于设置和获取块引用的属性值

#ifndef UIDLGEDITATTRIBUTES_H
#define UIDLGEDITATTRIBUTES_H

#include "ui_UIDlgEditAttributes.h"

class DmAttributeDefinition;
class DmAttribute;

class UIDlgEditAttributes : public QDialog, public Ui::UIDlgEditAttributes
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIDlgEditAttributes(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 设置对话框数据
    /// @param [in] blockName 块名称
    /// @param [in] attrDefs 属性定义列表
    void setData(const QString& blockName, const std::list<DmAttributeDefinition*>& attrDefs);

    /// @brief 获取用户修改后的属性列表
    /// @return 属性列表
    std::list<DmAttribute*> getAttributes() const;

private slots:
    void slotOk();
    void slotCancel();

private:
    std::vector<DmAttributeDefinition*> m_attrDefs; ///< 输入的属性定义
    std::list<DmAttribute*> m_attrs;                ///< 用来输出的属性
};

#endif // !UIDLGEDITATTRIBUTES_H
