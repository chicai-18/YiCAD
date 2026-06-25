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

/// @file GuiDialogFactory.cpp
/// @brief 对话框工厂类实现

#include "GuiDialogFactory.h"

// 私有构造函数
GuiDialogFactory::GuiDialogFactory()
    : m_pFactoryObject(nullptr)
{
}

/// @brief 获取单例实例
/// @return 对话框工厂的唯一实例
GuiDialogFactory* GuiDialogFactory::instance()
{
    static GuiDialogFactory* uniqueInstance = new GuiDialogFactory{};
    return uniqueInstance;
}

/// @brief 设置实际的工厂对象
/// @param fo 实现了 GuiDialogFactoryInterface 的工厂对象
void GuiDialogFactory::setFactoryObject(GuiDialogFactoryInterface* fo)
{
    m_pFactoryObject = fo;
}

/// @brief 获取工厂对象
/// @return 工厂对象（永不为 nullptr），如果未设置则返回默认适配器
GuiDialogFactoryInterface* GuiDialogFactory::getFactoryObject()
{
    return m_pFactoryObject ? m_pFactoryObject : &m_factoryAdapter;
}

/// @brief 显示命令消息
/// @param m 要显示的消息文本
void GuiDialogFactory::commandMessage(const QString& m)
{
    if (m_pFactoryObject)
    {
        m_pFactoryObject->commandMessage(m);
    }
}
