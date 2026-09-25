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

/// @file UIDlgDefineAttribute.h
/// @brief 属性定义对话框

#ifndef UIDLGDEFINEATTRIBUTE_H
#define UIDLGDEFINEATTRIBUTE_H

#include "ui_UIDlgDefineAttribute.h"
#include "DmAttributeDefinition.h"

/// @brief 属性定义对话框
class UIDlgDefineAttribute : public QDialog, public Ui_UIDlgDefineAttribute
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIDlgDefineAttribute(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIDlgDefineAttribute();

protected:
    /// @brief 显示事件
    /// @param [in] ev 显示事件
    void showEvent(QShowEvent* ev);

public slots:
    /// @brief 设置属性定义
    /// @param [in,out] attr 属性定义对象引用
    /// @param [in] isNew 是否新建
    virtual void setAttributeDefinition(DmAttributeDefinition& attr, bool isNew);

    /// @brief 更新属性定义
    virtual void updateAttributeDefinition();

    /// @brief 样式变化槽
    virtual void slotStyleChanged();

    /// @brief 取消操作
    virtual void reject();

    /// @brief 确认操作
    virtual void accept();

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();

private:
    /// @brief 修改文字时，对齐方式修改，对齐点坐标调整
    void adjustAlignmentForModeChange();

    /// @brief 初始化连接
    void init();

    /// @brief 需要时保存设置
    void saveSettingIfNeed();

private:
    bool m_isDlgShow;                     ///< 窗体是否已显示
    bool m_isNew;                         ///< 是否新建
    bool m_saveSettings;                  ///< 是否保存设置
    DmAttributeDefinition* m_pAttrDef;    ///< 属性定义对象指针
};

#endif // !UIDLGDEFINEATTRIBUTE_H
