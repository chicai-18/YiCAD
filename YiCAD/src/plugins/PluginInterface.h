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

/// @file PluginInterface.h
/// @brief 插件接口定义，包含插件项、分组、菜单位置及插件主接口类

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QtPlugin>
#include <QIcon>

class Document_Interface;
class Database_Interface;

/// @brief 菜单项类型枚举
enum class EItemType
{
    None = 0,
    Action = 1,
    Button = 2,
};

/// @brief 菜单的一个项，描述插件在界面上的单个操作入口
class PluginItem
{
public:
    /// @brief 构造插件菜单项
    /// @param itemName 项名称
    /// @param type 项类型
    /// @param icon 项图标
    PluginItem(const QString itemName, const EItemType type, const QIcon icon)
    {
        this->itemName = itemName;
        this->itemType = type;
        this->itemIcon = icon;
    }

    ~PluginItem() = default;

    QString     itemName;   ///< 项名
    EItemType   itemType;   ///< 项类型
    QIcon       itemIcon;   ///< 图标
};

/// @brief 选项卡下的分组，包含一组相关的菜单项
class PluginGroup
{
public:
    /// @brief 构造插件分组
    /// @param groupName 分组名
    /// @param menuItems 该分组下的项集合
    PluginGroup(const QString groupName, const QList<PluginItem> menuItems)
    {
        this->groupName = groupName;
        this->menuItems = menuItems;
    }

    ~PluginGroup() = default;

    QString             groupName;   ///< 分组名
    QList<PluginItem>   menuItems;   ///< 该分组下的项集合
};

/// @brief 插件的菜单入口，描述插件在界面上的一个选项卡及其内容
class PluginMenuLocation
{
public:
    /// @brief 构造插件菜单位置
    /// @param menuEntryActionName 选项卡名
    /// @param menuGroups 选项卡下的分组集合
    /// @param isLoad 是否加载插件
    PluginMenuLocation(const QString menuEntryActionName, const QList<PluginGroup> menuGroups, const bool isLoad)
    {
        this->menuEntryActionName = menuEntryActionName;
        this->menuGroups = menuGroups;
        this->isLoad = isLoad;
    }

    QString             menuEntryActionName;   ///< 选项卡名
    bool                isLoad;                 ///< 是否加载插件
    QList<PluginGroup>  menuGroups;             ///< 选项卡下的分组集合
};

/// @brief 插件功能描述，包含插件的菜单入口点集合
class PluginCapabilities
{
public:
    QList<PluginMenuLocation> menuEntryPoints;   ///< 菜单入口点集合
};

/// @brief 插件接口类，所有插件必须实现此接口
class PluginInterface
{
public:
    virtual ~PluginInterface() = default;

    /// @brief 设置数据库接口
    /// @param pDb 数据库接口指针
    virtual void setDatabase(Database_Interface* pDb) = 0;

    /// @brief 获取插件名称
    /// @return 插件名称
    virtual QString name() const = 0;

    /// @brief 获取插件功能描述
    /// @return 插件功能描述对象
    virtual PluginCapabilities getCapabilities() const = 0;

    /// @brief 执行插件命令
    /// @param doc 文档接口指针
    /// @param parent 父窗口指针
    /// @param cmd 命令字符串
    virtual void execComm(Document_Interface* doc, QWidget* parent, QString cmd) = 0;
};

#define DOCUMENT_INTERFACE_IID "org.YiCAD.PluginInterface/1.0"

Q_DECLARE_INTERFACE(PluginInterface, DOCUMENT_INTERFACE_IID)

#endif
