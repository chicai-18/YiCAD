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


/// @file ActionDrawHatch.h
/// @brief 填充图案（Hatch）绘制交互动作类

#ifndef ACTIONDRAWHATCH_H
#define ACTIONDRAWHATCH_H

#include "PreviewActionInterface.h"
#include "FindClosedRegion.h"

class HatchData;
class DmHatch;
class UIDlgHatchOptions;

/// @brief 通过点选封闭区域来创建填充图案
class ActionDrawHatch : public PreviewActionInterface
{
    Q_OBJECT

    /// @brief 动作状态枚举
    enum Status
    {
        ShowDialog, ///< 显示填充对话框
        PrepareAdd  ///< 准备添加（鼠标移动预览状态）
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    /// @param isModify 是否为修改模式
    ActionDrawHatch(DmDocument* doc, GuiDocumentView* docView, bool isModify);
    ~ActionDrawHatch();

    void init(int status = 0) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    /// @brief 用选中的实体初始化封闭区域查找算法
    void initFindMethodForSelectedEntities();

private slots:
    /// @brief 视图变化槽函数
    void slotViewChanged();

private:
    std::unique_ptr<HatchData> m_pData;              ///< 填充数据
    FindClosedRegion::FindClosedRegion m_findMethod; ///< 封闭区域查找算法
    DmVector m_pickPoint;                            ///< 鼠标点击位置
    bool m_hasSelectEntity{ false };                 ///< 命令开始时是否已选择实体
};

#endif
