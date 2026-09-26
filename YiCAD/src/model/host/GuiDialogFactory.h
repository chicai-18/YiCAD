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

/// @file GuiDialogFactory.h
/// @brief 对话框工厂类，管理对话框的创建和显示

#ifndef GUIDIALOGFACTORY_H
#define GUIDIALOGFACTORY_H

#include "GuiDialogFactoryAdapter.h"

class GuiDialogFactoryInterface;

/// @brief 便捷宏，用于获取对话框工厂对象
#define GUIDIALOGFACTORY GuiDialogFactory::instance()->getFactoryObject()

/// @brief 对话框工厂
/// @details 提供创建和显示对话框的接口，使用单例模式
class GuiDialogFactory
{
private:
    GuiDialogFactory();

public:
    virtual ~GuiDialogFactory() = default;

    /// @brief 获取单例实例
    /// @return 对话框工厂的唯一实例
    static GuiDialogFactory* instance();

    /// @brief 设置实际的工厂对象
    /// @param fo 实现了 GuiDialogFactoryInterface 的工厂对象
    void setFactoryObject(GuiDialogFactoryInterface* fo);

    /// @brief 获取工厂对象
    /// @return 工厂对象，如果未设置则返回默认适配器
    GuiDialogFactoryInterface* getFactoryObject();

    /// @brief 显示命令消息
    /// @param m 要显示的消息文本
    void commandMessage(const QString& m);

private:
    GuiDialogFactoryInterface*  m_pFactoryObject;     ///< 实际的工厂对象
    GuiDialogFactoryAdapter     m_factoryAdapter;      ///< 默认适配器
};

#endif
