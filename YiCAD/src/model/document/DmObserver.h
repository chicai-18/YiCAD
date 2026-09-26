/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmObserver.h
/// @brief 观察者模式接口，用于响应实体变更事件

#ifndef DMOBSERVER_H
#define DMOBSERVER_H

#include "Datamodel.h"

class DmEntity;

/// @brief 观察者抽象接口，响应实体变更通知
class DmObserver
{
public:
    DmObserver()
    {
    }

    virtual ~DmObserver() = default;

    /// @brief 响应实体变更
    /// @param ent 发生变更的实体指针
    virtual void respond(DmEntity* ent = nullptr) = 0;

    /// @brief 获取观察者类型
    /// @return 观察者类型枚举值
    virtual DM::ObserverType getObserverType() const = 0;

private:
};

#endif // !DMOBSERVER_H
