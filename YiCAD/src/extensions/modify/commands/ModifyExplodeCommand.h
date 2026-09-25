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

/// @file ModifyExplodeCommand.h
/// @brief 分解命令，取代原 ActionModifyExplode
///
/// 把块参照、通用容器、多段线、多行文字分解为独立实体；选择集就绪后立即执行并结束，
/// 没有画布交互。

#ifndef MODIFYEXPLODECOMMAND_H
#define MODIFYEXPLODECOMMAND_H

#include <vector>

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class DmEntity;
class DmMText;

/// @brief 分解命令
class ModifyExplodeCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyExplodeCommand)

protected:
    /// @brief 分解选择集后结束
    bool onSelectionReady() override;

private:
    /// @brief 执行分解操作
    /// @param [in] remove 是否在分解后删除原实体
    /// @return 分解成功返回true，否则返回false
    bool explode(const bool remove);

    /// @brief 将多行文字分解为单行文字
    /// @param [in] text 多行文字实体指针
    /// @param [out] addList 分解后产生的单行文字实体列表
    /// @return 分解成功返回true，否则返回false
    bool explodeMTextIntoLetters(DmMText* text, std::vector<DmEntity*>& addList);
};

#endif // MODIFYEXPLODECOMMAND_H
