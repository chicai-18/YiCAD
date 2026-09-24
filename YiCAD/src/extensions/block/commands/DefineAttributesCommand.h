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

/// @file DefineAttributesCommand.h
/// @brief 定义属性命令 ext.block.define_attributes，取代原 ActionDefineAttributes：先在
///        对话框里填属性定义，再指定插入点（对齐、布满要两点），放下一个属性定义后结束

#ifndef DEFINEATTRIBUTESCOMMAND_H
#define DEFINEATTRIBUTESCOMMAND_H

#include <memory>

#include <QCoreApplication>

#include "PlaceCommand.h"

class AttributeDefinitionData;
class DmVector;
class TextData;

/// @brief 定义属性命令；交互由 DefineAttributesTool 驱动
class DefineAttributesCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DefineAttributesCommand)

public:
    DefineAttributesCommand();
    ~DefineAttributesCommand() override;

    /// @brief 对齐或布满方式要指定两点
    bool needsSecondPoint() const;

    /// @brief 预览放在 pos 处的属性定义（对齐、布满方式不预览文字）
    void previewText(const DmVector& pos);
    /// @brief 预览两点之间的连线（对齐、布满方式的第二点）
    void previewLine(const DmVector& first, const DmVector& second);
    /// @brief 放在 pos 处，然后结束命令
    void commitOnePoint(const DmVector& pos);
    /// @brief 以两点放置（对齐、布满方式），然后结束命令
    void commitTwoPoints(const DmVector& first, const DmVector& second);

protected:
    /// @brief 弹出属性定义对话框；取消时启动失败
    std::unique_ptr<BasePlaceTool> createTool() override;

private:
    /// @brief 一点方式下按对齐方式设置位置与对齐点（原 setDataWithOnePoint）
    void setDataWithOnePoint(const DmVector& pos);
    /// @brief 提交当前数据，然后结束命令
    void commit();

    std::unique_ptr<TextData> m_textData;                ///< 文字数据
    std::unique_ptr<AttributeDefinitionData> m_attrData; ///< 属性定义数据
};

#endif  // DEFINEATTRIBUTESCOMMAND_H
