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

/// @file DrawHatchCommand.h
/// @brief 填充命令 ext.hatch.draw，取代原 ActionDrawHatch：先在对话框里选图案，再在封闭区域
///        里单击生成填充，可连续填充。命令开始时有选中的实体就只在它们围成的区域里找，
///        否则在视图内的实体里找（视图变化后补充）

#ifndef DRAWHATCHCOMMAND_H
#define DRAWHATCHCOMMAND_H

#include <memory>

#include <QCoreApplication>
#include <QMetaObject>

#include "FindClosedRegion.h"
#include "PlaceCommand.h"

class DmVector;
class HatchData;

/// @brief 填充命令；交互由 DrawHatchTool 驱动
class DrawHatchCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawHatchCommand)

public:
    DrawHatchCommand();
    ~DrawHatchCommand() override;

    /// @brief 预览鼠标所在区域的填充
    void previewAt(const DmVector& pos);
    /// @brief 在单击处所在的区域生成填充；找不到区域时给出提示
    void commitAt(const DmVector& pos);

protected:
    /// @brief 弹出填充对话框；取消时启动失败
    std::unique_ptr<BasePlaceTool> createTool() override;

private:
    /// @brief 把视图内的实体加入区域查找（原 slotViewChanged）
    void addEntitiesInView();

    std::unique_ptr<HatchData> m_data;               ///< 填充数据
    FindClosedRegion::FindClosedRegion m_findMethod; ///< 封闭区域查找
    QMetaObject::Connection m_viewChanged;           ///< 与视图 viewChanged 信号的连接
};

#endif  // DRAWHATCHCOMMAND_H
