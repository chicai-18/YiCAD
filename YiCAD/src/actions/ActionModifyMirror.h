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


/// @file ActionModifyMirror.h
/// @brief 镜像修改操作——处理用户鼠标事件以实现实体镜像功能

#ifndef ACTIONMODIFYMIRROR_H
#define ACTIONMODIFYMIRROR_H

#include "PreviewActionInterface.h"

class MirrorData;

/// @brief 镜像修改操作类，处理用户鼠标事件实现实体镜像功能
class ActionModifyMirror : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 镜像操作状态枚举
    enum Status
    {
        SetAxisPoint1, ///< 设置镜像轴第一点
        SetAxisPoint2, ///< 设置镜像轴第二点
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyMirror(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionModifyMirror() override;

    /// @brief 初始化数据（从配置恢复参数）
    void initData();

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值
    void init(int status = 0) override;

    /// @brief 执行镜像操作，对所有选中的实体进行镜像变换
    void trigger() override;

    /// @brief 处理坐标事件，根据状态设置镜像轴点并执行镜像
    /// @param [in] e 坐标事件指针
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 处理鼠标移动事件，更新镜像轴点位置
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件，确认镜像轴点或回退操作
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理命令事件，支持 Y/N 切换复制/删除原图模式
    /// @param [in] e 命令事件指针
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 更新鼠标按钮提示信息
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标为十字光标
    void updateMouseCursor() override;

    /// @brief 禁用拷贝构造
    ActionModifyMirror(const ActionModifyMirror&) = delete;

    /// @brief 禁用拷贝赋值
    ActionModifyMirror& operator=(const ActionModifyMirror&) = delete;

private:
    struct Points;
    std::unique_ptr<Points> pPoints; ///< 镜像操作数据点
};

#endif
