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

/// @file ModifySingleOffsetCommand.h
/// @brief 单个偏移命令 ext.modify.single_offset，取代原 ActionModifySingleOffset：选一个实体，
///        再点一下偏移的一侧，按选项条上的距离偏移出一个新实体，然后结束

#ifndef MODIFYSINGLEOFFSETCOMMAND_H
#define MODIFYSINGLEOFFSETCOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class DmEntity;
class DmVector;

/// @brief 单个偏移命令：持有偏移距离（选项条 UIModifyOffsetOptions 改写它）并提交偏移；
///        交互由 ModifySingleOffsetTool 驱动
class ModifySingleOffsetCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifySingleOffsetCommand)

public:
    // ---- 选项条（UIModifyOffsetOptions） ----

    /// @brief 偏移距离
    double distance() const { return m_distance; }
    void setDistance(double distance) { m_distance = distance; }

    // ---- 工具调用 ----

    /// @brief 预览 original 向 coord 一侧偏移的结果
    void previewOffset(DmEntity* original, const DmVector& coord);

    /// @brief 把 original 向 coord 一侧偏移出一个新实体（放在当前图层、用当前画笔），然后结束命令
    void commitOffset(DmEntity* original, const DmVector& coord);

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    double m_distance = 30.0; ///< 偏移距离；打开选项条时换成选项条上的值
};

#endif // MODIFYSINGLEOFFSETCOMMAND_H
