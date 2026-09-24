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

/// @file DrawTextCommand.h
/// @brief 单行文字命令 ext.text.draw，取代原 ActionDrawText：先在对话框里填文字，再逐个
///        指定插入点（对齐、布满要两点），可连续放置；选项条可改文字与角度

#ifndef DRAWTEXTCOMMAND_H
#define DRAWTEXTCOMMAND_H

#include <memory>

#include <QCoreApplication>
#include <QString>

#include "PlaceCommand.h"

class DmVector;
class TextData;

/// @brief 单行文字命令；交互由 DrawTextTool 驱动
class DrawTextCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawTextCommand)

public:
    DrawTextCommand();
    ~DrawTextCommand() override;

    // ---- 选项条（UITextOptions）与命令行 ----

    /// @brief 文字内容
    QString text() const;
    void setText(const QString& t);
    /// @brief 旋转角（弧度）
    double angle() const;
    void setAngle(double a);

    // ---- 预览与提交（工具调用）----

    /// @brief 对齐或布满方式要指定两点
    bool needsSecondPoint() const;
    /// @brief 预览放在 pos 处的文字（对齐、布满方式不预览文字）
    void previewText(const DmVector& pos);
    /// @brief 预览两点之间的连线（对齐、布满方式的第二点）
    void previewLine(const DmVector& first, const DmVector& second);
    /// @brief 放在 pos 处，可继续放置
    void commitOnePoint(const DmVector& pos);
    /// @brief 以两点放置（对齐、布满方式），可继续放置
    void commitTwoPoints(const DmVector& first, const DmVector& second);

protected:
    /// @brief 弹出文字对话框；取消时启动失败
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    /// @brief 一点方式下按对齐方式设置位置与对齐点（原 setDataWithOnePoint）
    void setDataWithOnePoint(const DmVector& pos);
    void commit();

    std::unique_ptr<TextData> m_data; ///< 文字数据
};

#endif  // DRAWTEXTCOMMAND_H
