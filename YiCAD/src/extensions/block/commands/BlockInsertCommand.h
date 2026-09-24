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

/// @file BlockInsertCommand.h
/// @brief 插入块命令 ext.block.insert，合并原 ActionBlockInsertPrepare 与 ActionBlocksInsert
///
/// 两个阶段：启动后在主窗口右侧弹出块列表（原"插入准备"），此时在画布上单击结束命令；
/// 在列表里点一个块就激活它并进入放置（原"插入"），可连续放置，右键回到选块。块列表
/// 在命令结束时关闭。放置阶段才有选项条（UIInsertOptions），选项在命令上。

#ifndef BLOCKINSERTCOMMAND_H
#define BLOCKINSERTCOMMAND_H

#include <memory>

#include <QCoreApplication>
#include <QPointer>

#include "PlaceCommand.h"

class DmBlock;
class DmBlockReferenceData;
class DmVector;
class QDialog;

/// @brief 插入块命令；交互由 BlockInsertTool 驱动
class BlockInsertCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(BlockInsertCommand)

public:
    BlockInsertCommand();
    ~BlockInsertCommand() override;

    // ---- 选项条（UIInsertOptions）与命令行 ----

    /// @brief 旋转角（弧度）
    double angle() const;
    void setAngle(double a);
    /// @brief 比例
    double factor() const;
    void setFactor(double f);
    /// @brief 阵列的列数、行数
    int columns() const;
    void setColumns(int c);
    int rows() const;
    void setRows(int r);
    /// @brief 阵列的列间距、行间距
    double columnSpacing() const;
    void setColumnSpacing(double cs);
    double rowSpacing() const;
    void setRowSpacing(double rs);

    // ---- 选块（块列表）----

    /// @brief 选定要插入的块：激活它，选项复位后开始放置；块里嵌套插入了自身时给出
    ///        提示，留在选块阶段
    void chooseBlock(DmBlock* block);
    /// @brief 放置阶段右键：回到选块，收起选项条
    void backToChoosing();
    /// @brief 正在放置的块；选块阶段为空
    DmBlock* block() const { return m_block; }
    /// @brief 关闭块列表（命令结束时）
    void closeBlockList();

    // ---- 预览与提交（工具调用）----

    /// @brief 预览插入在 pos 处的块参照（连同属性定义的默认文字）
    void previewInsert(const DmVector& pos);
    /// @brief 在 pos 处插入块参照；块有属性定义时先弹出属性对话框
    void commitInsert(const DmVector& pos);

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    /// @brief 弹出块列表
    bool onStarted() override;
    /// @brief 只在放置阶段显示选项条
    void showOptions() override;
    void hideOptions() override;

private:
    std::unique_ptr<DmBlockReferenceData> m_data; ///< 块参照数据（选项与插入点）
    DmBlock* m_block = nullptr;                   ///< 正在放置的块
    QPointer<QDialog> m_blockList;                ///< 块列表对话框
};

#endif  // BLOCKINSERTCOMMAND_H
