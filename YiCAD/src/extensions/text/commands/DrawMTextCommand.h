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

/// @file DrawMTextCommand.h
/// @brief 多行文字命令，取代原 ActionDrawMText：新建（ext.text.mtext，先拉编辑框再编辑）与
///        就地编辑已有的多行文字（ext.text.edit_mtext，双击时由选择层启动）
///
/// 编辑时在画布上叠一个编辑框（MTextEditWidget），主窗口左上显示选项条
/// （UIMTextOptions），两者经 MTextEditContext 同步。在画布上按下提交并结束；编辑框里
/// 按 Esc 按用户选择提交或放弃。编辑中被外部结束（新命令、结束全部命令、关闭视图、
/// 撤销等即时命令）时弹出"Save the changes?"（5.1 节）。

#ifndef DRAWMTEXTCOMMAND_H
#define DRAWMTEXTCOMMAND_H

#include <memory>

#include <QCoreApplication>
#include <QPointer>

#include "DmVector.h"
#include "PlaceCommand.h"

class DmDocument;
class DmMText;
class MTextEditContext;
class MTextEditWidget;
class QWidget;
class TransactionGroup;
class UIMTextOptions;

/// @brief 多行文字命令；交互由 DrawMTextTool 驱动
class DrawMTextCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawMTextCommand)

public:
    /// @brief 新建多行文字
    DrawMTextCommand();
    /// @brief 就地编辑已有的多行文字
    /// @param originText 原文字（编辑期间隐藏，提交时写回）
    /// @param clickPt 双击的位置，编辑框按它放置光标
    DrawMTextCommand(DmMText* originText, const DmVector& clickPt);
    ~DrawMTextCommand() override;

    // ---- 编辑框与选项条（MTextEditWidget、UIMTextOptions）----

    /// @brief 编辑上下文；只在编辑时有效
    MTextEditContext* getContext();
    /// @brief 焦点交还编辑框（选项条改了设置之后）
    void focusEditWidget();
    /// @brief 文档
    DmDocument* getDocument() const { return document(); }

    // ---- 工具调用 ----

    /// @brief 预览编辑框的范围
    void previewBox(const DmVector& corner1, const DmVector& corner2);
    /// @brief 编辑框的两个角点定好后显示选项条与编辑框，进入编辑
    void startEditing(const DmVector& topLeft, const DmVector& bottomRight);
    /// @brief 编辑中在画布上按下：提交并结束
    void commitAndFinish();

    /// @brief 关闭编辑框与选项条，恢复十字光标（原 freeUI；工具结束时调用）
    void freeUI();

    /// @brief 编辑中不可打断：撤销、删除等即时命令先结束它
    bool isUninterruptible() const override { return true; }
    /// @brief 编辑中被外部结束时询问是否保存（是/否），随后提交或放弃；不否决
    bool onEndRequested(CommandEndReason reason) override;

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    /// @brief 就地编辑：直接进入编辑
    bool onStarted() override;

private:
    /// @brief 显示选项条与编辑框，开始事务组（原 initDisplayDialogs）
    void initDisplayDialogs();
    /// @brief 提交编辑结果（原 trigger()）
    void commit();
    /// @brief 放弃编辑（原 cancel()）
    void cancel();
    /// @brief 编辑框里按了 Esc（原 slotEscPressed）
    void onEscPressed(bool save);
    /// @brief 是否处于编辑中
    bool isEditing() const;

    const bool m_isModify;                        ///< 是否就地编辑已有的文字
    DmVector m_pos{false};                        ///< 编辑框左上角
    DmVector m_secPos{false};                     ///< 编辑框右下角
    DmVector m_clickPt{false};                    ///< 就地编辑时双击的位置
    DmMText* m_pOriginText = nullptr;             ///< 原文字，仅就地编辑时有效
    DmMText* m_pEditingText = nullptr;            ///< 正在编辑的文字
    std::shared_ptr<TransactionGroup> m_trans;    ///< 事务组
    std::unique_ptr<MTextEditContext> m_context;  ///< 编辑上下文
    QPointer<MTextEditWidget> m_pEditWidget;      ///< 编辑框
    QPointer<QWidget> m_pOptionBack;              ///< 选项条的底板
    bool m_done = false;                          ///< 已提交或放弃，只等结束
};

#endif  // DRAWMTEXTCOMMAND_H
