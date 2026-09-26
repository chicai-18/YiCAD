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

/// @file DmDocumentListener.h
/// @brief 文档监听接口：文档经它通知关心自身变化的对象（如画布）
///
/// 分层重组 S3 取代 DmDocument 原先持有的视图指针：Model 不再认识视图，
/// 一份文档可以有任意多个监听者。三个方法与原先对视图的调用一一对应，
/// 见 doc/LAYER_RESTRUCTURE_PLAN.md 7.2 节。

#ifndef DMDOCUMENTLISTENER_H
#define DMDOCUMENTLISTENER_H

class DmEntityContainer;

/// @brief 文档监听者
///
/// 文档不拥有监听者；监听者析构前必须调 DmDocument::removeListener() 注销。
/// 通知期间不得增删同一文档的监听者。
class DmDocumentListener
{
public:
    virtual ~DmDocumentListener() = default;

    /// @brief 文档内容已修改（实体、选中状态、画笔），绘制缓存需要更新
    virtual void documentModified() = 0;

    /// @brief 请求重绘
    virtual void redrawRequested() = 0;

    /// @brief 要绘制的实体容器已切换（进入或退出块编辑）
    /// @param container 此后要绘制的实体容器
    virtual void paintContainerChanged(DmEntityContainer* container) = 0;
};

#endif // DMDOCUMENTLISTENER_H
