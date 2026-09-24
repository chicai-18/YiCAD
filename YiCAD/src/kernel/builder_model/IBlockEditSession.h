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

/// @file IBlockEditSession.h
/// @brief 块编辑选项条看到的块编辑模式
///
/// 块编辑选项条（UIBlockEditOptions）经 GuiDialogFactoryInterface::requestBlockEditOptions
/// 拿到它，显示正在编辑的块名，按"完成"时结束编辑。由块编辑模式实现；放在模型层，
/// 选项条与对话框工厂接口因此不必认识交互层的类型。

#ifndef IBLOCKEDITSESSION_H
#define IBLOCKEDITSESSION_H

#include <QString>

/// @brief 块编辑会话
class IBlockEditSession
{
public:
    virtual ~IBlockEditSession() = default;

    /// @brief 正在编辑的块名
    virtual QString blockName() const = 0;
    /// @brief 自进入编辑以来是否有修改
    virtual bool hasModifications() const = 0;
    /// @brief 结束编辑
    /// @param save 是否保存修改（更新块参照）
    virtual void completeEditing(bool save) = 0;
};

#endif // IBLOCKEDITSESSION_H
