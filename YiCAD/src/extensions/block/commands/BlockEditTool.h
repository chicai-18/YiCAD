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

/// @file BlockEditTool.h
/// @brief 块编辑模式，取代原 ActionBlocksEdit
///
/// 块编辑是"编辑模式"而不是命令（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 5 节"命令并存"）：
/// 进入后本工具常驻在业务栈底部，右键询问是否保存并退出，其余鼠标与按键事件让给
/// 选择层，块内的点选、框选、拖动与原先一致；模式里启动的命令与旧版 Action 叠在它
/// 上面，结束后回到块编辑。模式由视图的命令总线持有（ExclusiveCommandBus::enterEditMode），
/// 由编辑块命令（BlocksEditCommand）进入，撤销/重做后由 UIActionHandler 重新进入或退出。
///
/// 退出的三条路：
///   - 右键、选项条"完成"：用户选择保存或放弃后退出；
///   - 结束全部命令（Esc/空格未被接受、Ribbon 的结束全部、排他的旧 Action）：弹出与
///     右键相同的对话框，取消即否决（2026-09-24 确认，迁移计划 9.2 节）；
///   - 视图关闭、撤销/重做离开了块编辑：只收起界面，不改动文档（与原先一致）。

#ifndef BLOCKEDITTOOL_H
#define BLOCKEDITTOOL_H

#include <QCoreApplication>
#include <QString>

#include "IEditMode.h"

class DmBlock;
class DmBlockReference;
class DmDocument;
class ICommandHost;
class IDocumentView;

/// @brief 块编辑模式
class BlockEditTool : public IEditMode
{
    Q_DECLARE_TR_FUNCTIONS(BlockEditTool)

public:
    /// @param host 所在视图；模式退出自己时经它的命令总线请求
    explicit BlockEditTool(ICommandHost& host);
    ~BlockEditTool() override;

    /// @brief 正常进入的第一步：确定要编辑的块（块里嵌套了别的块时让用户选择）
    /// @param blockRef 选中的块参照
    /// @return 用户取消或块定义不存在时返回 false（已在命令行说明）
    bool prepare(DmBlockReference* blockRef);
    /// @brief 正常进入的第二步：取消块参照的选中，以 BlockEditEnterCmd 事务进入块编辑，
    ///        适屏显示块内容
    /// @note 在模式交给总线之后调用：事务触发的撤销栈变化通知要能看到模式已经存在
    void beginEditing(DmBlockReference* blockRef);
    /// @brief 撤销/重做后重新进入：文档已处于块编辑，只恢复界面
    void reenter(DmBlock* block);

    // ---- 块编辑选项条（UIBlockEditOptions）----

    /// @brief 正在编辑的块名
    QString blockName() const { return m_blockName; }
    /// @brief 自进入编辑以来是否有修改
    bool hasModifications() const;
    /// @brief 记下保存或放弃，请求退出模式（在事件处理或选项条的按钮里调用都安全）
    void completeEditing(bool save);

    // ---- IEditMode ----

    /// @brief 结束全部命令时弹出与右键相同的"是否保存"对话框，取消即否决；视图关闭时不问
    bool onEndRequested(CommandEndReason reason) override;
    /// @brief 按决定保存或放弃后退出块编辑，收起选项条
    void onExit() override;
    void suspendMode() override;
    void resumeMode() override;

    // ---- IViewTool ----

    /// @brief 鼠标按下让给选择层
    ViewToolResult mousePressEvent(QMouseEvent* e) override;
    /// @brief 右键询问是否保存并退出；其余让给选择层与导航层
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override;
    ViewToolResult mouseMoveEvent(QMouseEvent* e) override;
    /// @brief 双击到此为止：块编辑中双击实体无反应（清单 B5，既有）
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e) override;
    /// @brief 按键让给选择层（Esc 清空块内选择）
    ViewToolResult keyPressEvent(QKeyEvent* e) override;
    ViewToolResult keyReleaseEvent(QKeyEvent* e) override;
    /// @brief 命令行坐标被接受但不起作用（原先由块编辑 Action 接受）
    ViewToolResult coordinateEvent(const DmVector& pos) override;
    /// @brief 回到画布时刷新提示（模式没有被叠住时）
    void enterEvent() override;

private:
    /// @brief 退出时怎么处理文档
    enum class ExitDecision
    {
        LeaveAsIs, ///< 不改动文档：视图关闭、撤销/重做已离开块编辑
        Save,      ///< 保存修改并退出
        Discard    ///< 放弃修改并退出
    };

    /// @brief 弹出"是否保存并退出"对话框，记下决定
    /// @return 用户取消时返回 false
    bool askSaveAndExit();
    void updateHints() const;
    /// @brief 在宿主的选项条区域显示或收起块编辑选项条（UIBlockEditOptions）
    void showOptions(bool on);

    ICommandHost& m_host;
    DmDocument* m_document = nullptr;
    IDocumentView* m_view = nullptr;
    QString m_blockName;              ///< 编辑的块名称
    size_t m_undoCountAtEnter = 0;    ///< 进入编辑时的 undo 栈深度（用于检测修改）
    ExitDecision m_exitDecision = ExitDecision::LeaveAsIs;
    bool m_suspended = true;          ///< 命令或旧版 Action 叠在上面；resumeMode() 之前也是
};

#endif // BLOCKEDITTOOL_H
