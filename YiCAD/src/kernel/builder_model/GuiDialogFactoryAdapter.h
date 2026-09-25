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

/// @file GuiDialogFactoryAdapter.h
/// @brief 对话框工厂默认适配器，提供空操作的回退实现

#ifndef GUIDIALOGFACTORYADAPTER_H
#define GUIDIALOGFACTORYADAPTER_H

#include "DmBlock.h"
#include "GuiDialogFactoryInterface.h"

class DmBlockTable;

/// @brief 对话框工厂接口的默认适配器
/// @details 当未设置实际工厂对象时使用，所有对话框操作返回默认值或空操作
class GuiDialogFactoryAdapter : public GuiDialogFactoryInterface
{
public:
    /// @brief 显示警告对话框（空操作）
    void requestWarningDialog(const QString&) override
    {
    }

    /// @brief 请求确认对话框
    /// @return 始终返回 false
    bool requestConfirmDialog(const QString&, const QString&) override
    {
        return false;
    }

    /// @brief 是/否/取消对话框
    /// @return 始终返回取消（不做任何改变）
    DialogAnswer requestYesNoCancelDialog(const QString&, const QString&) override
    {
        return DialogAnswer::Cancel;
    }

    /// @brief 获取当前活动文档
    /// @return 始终返回 nullptr
    DmDocument* requestActiveDocument() override
    {
        return nullptr;
    }

    /// @brief 请求未命名文档的默认名称
    /// @return 始终返回空字符串
    QString requestUntitledDocumentName(DmDocument*) override
    {
        return {};
    }

    /// @brief 请求文件导出
    /// @return 始终返回 false
    bool requestFileExport(DmDocument&, const QString&, const QString&) override
    {
        return false;
    }

    /// @brief 请求文件导入
    /// @return 始终返回 false
    bool requestFileImport(DmDocument&, const QString&) override
    {
        return false;
    }

    /// @brief 请求定义属性对话框
    /// @return 始终返回 false
    bool requestDefineAttributesDialog(DmAttributeDefinition* attrDef) override
    {
        return false;
    }

    /// @brief 请求操作选项（空操作）
    void requestCommandOptions(IExclusiveCommand*, bool, bool) override
    {
    }

    /// @brief 编辑模式的选项条（空操作）
    void requestEditModeOptions(const std::function<QWidget*(QWidget*)>&, bool) override
    {
    }

    /// @brief 请求捕捉中点选项（空操作）
    void requestSnapMiddleOptions(int&, bool) override
    {
    }

    /// @brief 请求修改实体对话框
    /// @return 始终返回 false
    bool requestModifyEntityDialog(DmEntity*) override
    {
        return false;
    }

    /// @brief 请求文字编辑对话框
    /// @return 始终返回 false
    bool requestTextDialog(DmText*) override
    {
        return false;
    }

    /// @brief 请求填充编辑对话框
    /// @return 始终返回 false
    bool requestHatchDialog(DmHatch*) override
    {
        return false;
    }

    /// @brief 更新坐标控件（空操作）
    void updateCoordinateWidget(const DmVector&, const DmVector&, bool = false) override
    {
    }

    /// @brief 更新鼠标提示控件（空操作）
    void updateMouseWidget(const QString&, const QString&) override
    {
    }

    /// @brief 更新选择数量控件（空操作）
    void updateSelectionWidget(int) override
    {
    }

    /// @brief 显示命令消息（空操作）
    void commandMessage(const QString&) override
    {
    }

    /// @brief 设置命令控件（空操作）
    void setCommandWidget(UICommandWidget*) override
    {
    }

    /// @brief 设置底部控件（空操作）
    void setBottomWidget(UIBottomWindow*) override
    {
    }
};

#endif
