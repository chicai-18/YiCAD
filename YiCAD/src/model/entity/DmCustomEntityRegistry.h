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

/// @file DmCustomEntityRegistry.h
/// @brief 自定义实体类的注册表（RENDER_PLAN.md 第 4.8.2 节）
///
/// 读盘按类名在这里找类：找到就建原实体，找不到就建代理实体（DmProxyEntity）。扩展经
/// IExtensionContext::registerEntityClass 登记，扩展关闭时注销；注销后已经在内存里的实体照常可用，
/// 之后读进来的才是代理。类型系统（MetaType）里的类型注册是全进程的、不注销，按类名建实例经它。

#ifndef DMCUSTOMENTITYREGISTRY_H
#define DMCUSTOMENTITYREGISTRY_H

#include <map>
#include <vector>

#include <QString>

#include "DmCustomEntity.h"

/// @brief 一个登记的自定义实体类
struct DmCustomEntityClass
{
    QString name;                               ///< 类名（MetaType 的类型名），如 "ext.sample.Pipe"
    Type type;                                  ///< 类型，建实例用
    std::uint32_t version = 0;                  ///< 数据版本（TYPESYSTEM_SOURCE_NAMED 的版本号）
    DmProxyFlags proxyFlags = DmProxyFlags::None; ///< 代理权限
    QString owner;                              ///< 登记它的扩展 ID
};

/// @brief 自定义实体类的注册表，见文件说明
class DmCustomEntityRegistry
{
public:
    static DmCustomEntityRegistry& instance();

    /// @brief 登记一个类
    /// @return 类型无效、不派生自 DmCustomEntity、不能建实例或同名的类已登记时返回 false
    bool registerClass(const DmCustomEntityClass& entityClass);

    /// @brief 注销一个类
    /// @return 没有登记时返回 false
    bool unregisterClass(const QString& name);

    /// @brief 按类名找；没有登记返回空
    const DmCustomEntityClass* find(const QString& name) const;

    /// @brief 按类名建实例；没有登记返回空
    DmCustomEntity* create(const QString& name) const;

    /// @brief 已登记的全部类名
    std::vector<QString> classNames() const;

    /// @brief 由 T 的类型注册得出登记信息：类型没初始化时先初始化（可重复调用），版本取 T 的版本号
    template <typename T>
    static DmCustomEntityClass describe(DmProxyFlags proxyFlags, const QString& owner)
    {
        if (T::getClassTypeId().isBad())
        {
            T::initialize();
        }
        std::vector<PAIR> revs;
        T::getRevId(revs);
        DmCustomEntityClass entityClass;
        entityClass.type = T::getClassTypeId();
        entityClass.name = QString::fromUtf8(entityClass.type.getName());
        entityClass.version = revs.empty() ? 0 : static_cast<std::uint32_t>(revs.back().second);
        entityClass.proxyFlags = proxyFlags;
        entityClass.owner = owner;
        return entityClass;
    }

private:
    std::map<QString, DmCustomEntityClass> m_classes;
};

#endif // DMCUSTOMENTITYREGISTRY_H
