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

/// @file DmDocumentTransfer.h
/// @brief 把实体带进另一份文档时，找出它引用的表项在目标文档里对应的条目
///
/// 实体引用所属文档的图层、线型、文字样式、标注样式与块。复制到剪贴板、从剪贴板粘贴时实体换了文档，
/// 这些引用要换成新文档的，否则来源图纸一关闭就悬空。规则：目标文档有同名条目就用它，不改动它；
/// 没有才复制一份进去，且只复制实体真正引用到的。实体经 DmEntity::transferTo 逐个改归，
/// 各实体类自己决定引用了哪些条目，本类只负责按名字找、按需复制。

#ifndef DMDOCUMENTTRANSFER_H
#define DMDOCUMENTTRANSFER_H

class DmBlock;
class DmDimensionStyle;
class DmDocument;
class DmLayer;
class DmLineType;
class DmTextStyle;

/// @brief 实体改归目标文档时解析引用的表项
class DmDocumentTransfer
{
public:
    /// @brief 目标文档没有同名条目时的做法
    enum class Missing
    {
        KeepSource,  ///< 不复制，仍用来源的条目；用于粘贴预览，预览不能改动目标文档
        AddDirect,   ///< 复制一份直接放进目标文档，不进撤销栈；用于剪贴板
        AddWithUndo, ///< 复制一份经命令放进目标文档，随调用方的事务撤销；用于粘贴
    };

    /// @param target 目标文档，须比本对象活得久
    /// @param missing 目标文档没有同名条目时的做法
    DmDocumentTransfer(DmDocument& target, Missing missing);

    /// @brief 目标文档
    DmDocument& target() const;

    /// @brief 图层在目标文档里对应的图层；复制时图层画笔的线型一并解析
    /// @return 来源为空时返回空
    DmLayer* layer(DmLayer* source);

    /// @brief 线型在目标文档里对应的线型
    /// @details DmLineTypeTable 的静态线型（ByLayer、ByBlock 等）不属于任何文档，原样返回
    DmLineType* lineType(DmLineType* source);

    /// @brief 文字样式在目标文档里对应的文字样式
    DmTextStyle* textStyle(DmTextStyle* source);

    /// @brief 标注样式在目标文档里对应的标注样式；复制时它的文字样式一并解析
    DmDimensionStyle* dimStyle(DmDimensionStyle* source);

    /// @brief 块定义在目标文档里对应的块；复制时块内图元逐个改归，嵌套的块随之复制
    DmBlock* block(DmBlock* source);

private:
    DmDocument& m_target; ///< 目标文档
    Missing m_missing;    ///< 目标文档没有同名条目时的做法
};

#endif // DMDOCUMENTTRANSFER_H
