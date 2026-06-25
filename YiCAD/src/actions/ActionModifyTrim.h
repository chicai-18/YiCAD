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


/// @file ActionModifyTrim.h
/// @brief 修剪实体交互命令头文件

#ifndef ACTIONMODIFYTRIM_H
#define ACTIONMODIFYTRIM_H

#include "PreviewActionInterface.h"
#include <vector>

/// @brief 修剪实体交互命令
///
/// 处理用户交互事件，实现实体的修剪操作。
/// 先选择限制边界实体，再选择需要修剪的实体部分。
class ActionModifyTrim : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 命令状态枚举
    enum Status
    {
        ChooseLimitEntity, ///< 选择限制边界实体
        ChooseTrimEntity   ///< 选择要修剪的实体
    };

public:
    ActionModifyTrim(DmDocument* doc, GuiDocumentView* docView);
    ~ActionModifyTrim() override;

    /// @brief 初始化命令状态
    /// @param [in] status 初始状态，默认为0（ChooseLimitEntity）
    void init(int status = 0) override;

    /// @brief 完成命令，取消高亮并清理状态
    /// @param [in] updateTB 是否更新工具栏
    void finish(bool updateTB = true) override;

    /// @brief 执行修剪操作
    void trigger() override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件对象
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件对象
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 处理键盘按键事件
    /// @param [in] e 键盘事件对象
    void keyPressEvent(QKeyEvent* e) override;

private:
    std::vector<DmEntity*> m_seleltedEnts;  ///< 选择的实体，作为求交的边界
    DmEntity* m_entToTrim;                  ///< 待修剪的实体
    DmEntity* m_entUnderCursor;             ///< 当前鼠标下的实体
    DmVector m_trimPt;                      ///< 确认修剪时，鼠标点下的位置

    /// @brief 取消限制边界实体的高亮状态
    void unhighlightLimitingEntity();
};

#endif
