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

/// @file BasePlaceTool.h
/// @brief 放置工具的通用基类：交互状态、捕捉会话、按键提示与光标
///
/// 放置工具（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 1 节）管事件：交互状态机、
/// 捕捉、光标、按键提示，把用户输入翻译成对所属命令的调用；预览与提交归命令。
/// 本类是原 PreviewActionInterface 里与事件相关的那一半：
///   - 状态：setStatus() 只在变化时刷新提示，restart() 相当于原 init(status)
///     （连同清除预览，与原 PreviewActionInterface::init 一致）；
///   - 捕捉会话：构造时 init，离开画布或被停用时 suspend（连同清除预览），
///     回到画布或被激活时刷新提示、resume、重绘预览，结束时 finish 并复位
///     正交零点；
///   - 事件归属与原先经 LegacyActionTool 转发时一致：中键按下、按着中键的移动
///     （平移中）与中键释放让给导航层；其余鼠标事件与双击到此为止；按键默认不接受
///     （Esc/空格由主窗口结束全部命令），也不再下传。
/// 光标经 getCursor() 参与 ViewToolControl 的仲裁，不再直接设置。

#ifndef BASEPLACETOOL_H
#define BASEPLACETOOL_H

#include <memory>

#include "IViewTool.h"

class BaseExclusiveCommand;
class CommandPreview;
class DmDocument;
class IDocumentView;
class ISnapService;
class Snapper;

/// @brief 放置工具的通用基类
class BasePlaceTool : public IViewTool
{
public:
    /// @param command 所属命令，退回第一步之前时请求它结束
    /// @param doc 文档
    /// @param view 视图
    BasePlaceTool(BaseExclusiveCommand& command, DmDocument* doc, IDocumentView* view);
    ~BasePlaceTool() override;

    BasePlaceTool(const BasePlaceTool&) = delete;
    BasePlaceTool& operator=(const BasePlaceTool&) = delete;

    /// @brief 当前交互状态
    int status() const { return m_status; }
    /// @brief 工具的捕捉器；命令的 snapService() 返回它
    ISnapService* snapper() const;

    /// @brief 挂起与恢复时连带清除、重绘的预览（命令持有）；可为空
    void setPreview(CommandPreview* preview) { m_preview = preview; }

    /// @brief 结束捕捉会话：先调用 onFinish()，再清除捕捉标记、复位正交零点
    ///        （原 Action 的 finish() 覆盖与 ActionInterface::finish）
    void finishSession();

    ViewToolResult mousePressEvent(QMouseEvent* e) final;
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) final;
    ViewToolResult mouseMoveEvent(QMouseEvent* e) final;
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e) final;
    ViewToolResult keyPressEvent(QKeyEvent* e) final;
    ViewToolResult keyReleaseEvent(QKeyEvent* e) final;
    ViewToolResult coordinateEvent(const DmVector& pos) final;
    /// @return 子类接受了文本时返回 Handled，否则 NotHandled（文本被当作新命令）
    ViewToolResult commandEvent(GuiCommandEvent* e) final;

    /// @brief 回到画布：刷新提示，恢复捕捉标记，重绘预览（原 resume()）
    void enterEvent() override;
    /// @brief 离开画布：挂起捕捉，清除预览（原 suspend()）
    void leaveEvent() override;
    /// @brief 被激活（命令开始或旧 Action 结束后恢复）：同 enterEvent()
    void onActivate() override;
    /// @brief 被停用（命令结束或旧 Action 叠上来）：同 leaveEvent()
    void onDeactivate() override;

    /// @brief 默认十字光标
    std::optional<DM::CursorType> getCursor() const override;

protected:
    /// @brief 改变状态，只在变化时刷新提示（原 ActionInterface::setStatus）
    void setStatus(int status);
    /// @brief 回到某一状态：清除预览，改变状态，重新初始化捕捉器
    ///        （原 PreviewActionInterface::init(status)，status ≥ 0）
    void restart(int status);
    /// @brief 右键退回上一步（原 init(getStatus() - 1)）：已在第一步时结束命令
    void stepBack();
    /// @brief 结束时是否复位正交零点：原 ActionInterface::finish 只在 Action 类型
    ///        不是 ActionNone 时复位（剪切/复制到剪贴板没有设置类型，不复位）
    void setResetsOrthogonalOnFinish(bool resets) { m_resetsOrthogonal = resets; }
    /// @brief 正交限制下结束命令（取代已删除的 Snapper::finishOrthogonal：它经 getCurrentAction()
    ///        结束当前 Action，对命令无效）
    void finishIfOrthogonal();

    BaseExclusiveCommand& command() const { return m_command; }
    DmDocument* document() const { return m_document; }
    IDocumentView* view() const { return m_view; }

    /// @brief 按当前状态刷新按键提示
    virtual void updateHints() = 0;
    virtual void onMousePress(QMouseEvent*) {}
    virtual void onMouseRelease(QMouseEvent*) {}
    virtual void onMouseMove(QMouseEvent*) {}
    virtual void onCoordinate(const DmVector&) {}
    /// @brief 命令行文本；使用后 accept() 该事件
    virtual void onCommand(GuiCommandEvent*) {}
    /// @brief 双击；默认什么也不做
    virtual void onMouseDoubleClick(QMouseEvent*) {}
    /// @brief 按键；默认不接受
    virtual void onKeyPress(QKeyEvent* e);
    /// @brief 按键释放；默认不接受
    virtual void onKeyRelease(QKeyEvent* e);
    /// @brief 命令结束时（捕捉会话结束之前）调用：取消高亮等收尾（原 Action 的 finish() 覆盖）
    virtual void onFinish() {}

private:
    BaseExclusiveCommand& m_command;
    DmDocument* m_document = nullptr;
    IDocumentView* m_view = nullptr;
    std::unique_ptr<Snapper> m_snapper;
    CommandPreview* m_preview = nullptr;
    int m_status = 0;
    bool m_resetsOrthogonal = true;
};

#endif // BASEPLACETOOL_H
