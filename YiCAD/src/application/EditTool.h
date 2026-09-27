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

/// @file EditTool.h
/// @brief 编辑工具：夹点编辑，移动选中实体的参考点
///
/// 从 `SelectTool` 拆出，对应 DS-master 的 `Application/Edit/EditTool`
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.6 节）。由交互视图 `UIView` 持有，与 DS-master 一样是
/// 业务工具：没有活动命令时由视图放在业务栈上（在编辑模式的工具之上），命令总线通知命令即将
/// 启动时移出、命令结束时放回（`UIView::onCommandStarting`/`onCommandFinished`）。注意与块编辑的
/// "编辑模式"（`IEditMode`，常驻在业务栈底部）不是一回事。
///
/// 交互与 AutoCAD 的夹点相同：左键按在选中实体的参考点（夹点）附近，这次按下就归本类；松开，
/// 或按住拖过阈值时夹点激活，参考点跟随鼠标，再单击落位；右键或 Esc 取消。是不是按在夹点上，
/// 按下时就能判定，所以本类与选择层不交接同一次手势：按在夹点上的按下选择层收不到，其余的按下
/// 本类不处理。DS-master 同样在按下时接管，但接管的是按在选中构件上的按下、移动整个选择集；
/// 这里单击线身是切换选中，所以只接管夹点，空闲态不能拖动整个实体，移动实体用移动命令。
///
/// 不处理的按下：
///   - `setEnabledQuery()` 告知不可用时：框选时点第二个角点（选择阶段有命令在运行，本类不在栈上）；
///   - Ctrl/Meta+左键：导航层的平移手势。
/// 中键与平移中的移动、释放一律让给导航层，平移结束后夹点照旧。
///
/// 启动命令时移出业务栈，`onDeactivate()` 取消激活的夹点；结束全部命令时由视图取消
/// （`UIView::killAllActions`）。

#ifndef EDITTOOL_H
#define EDITTOOL_H

#include <functional>
#include <optional>

#include "DmVector.h"
#include "IViewTool.h"

class DmDocument;
class IDocumentView;
class ISnapService;
class PanZoomTool;
class Preview;
class QKeyEvent;
class QMouseEvent;
class SelectionSet;

class EditTool : public IViewTool
{
public:
    /// @brief 内部状态
    enum Status
    {
        Neutral,  ///< 没有夹点
        Pressed,  ///< 左键按在夹点上，松开或拖过阈值时激活
        MovingRef ///< 夹点激活：参考点跟随鼠标，单击落位（原 `ActionDefault::MovingRef`）
    };

    /// @brief 查询此刻按在夹点上是否归本类（框选时点第二个角点时不归）
    using EnabledQuery = std::function<bool()>;

    /// @param doc 文档指针
    /// @param selection 文档的选择集，夹点取自其中的实体
    /// @param docView 文档视图指针
    /// @param snapService 非持有指针，与选择层共用视图的捕捉器
    /// @param preview 非持有指针，由视图持有的预览容器
    /// @param panTool 非持有指针，可为空；用于查询导航层是否正在平移中，为空时视为"从不平移"
    EditTool(DmDocument* doc, SelectionSet* selection, IDocumentView* docView, ISnapService* snapService,
             Preview* preview, PanZoomTool* panTool = nullptr);

    /// @brief 设置"此刻按在夹点上是否归本类"的查询，由视图装配时设置
    /// @param query 为空时视为总是可用
    void setEnabledQuery(EnabledQuery query) { m_enabledQuery = std::move(query); }

    /// @brief 取消按下或激活的夹点：清除预览，回到 Neutral
    void cancel();

    int getStatus() const { return m_status; }

    ViewToolResult mousePressEvent(QMouseEvent* e) override;
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override;
    ViewToolResult mouseMoveEvent(QMouseEvent* e) override;
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e) override;
    /// @brief Esc 取消夹点后仍返回 NotHandled：选择层接着清空选择，与拆分前一致
    ViewToolResult keyPressEvent(QKeyEvent* e) override;

    /// @brief 鼠标离开画布时清除预览；回到画布后下一次移动重新画出
    void leaveEvent() override;

    /// @brief 夹点激活时给出选择光标，否则没有偏好
    std::optional<DM::CursorType> getCursor() const override;

    void onDeactivate() override;

private:
    /// @brief 激活夹点：相对零点移到参考点，按鼠标位置画出预览
    void activate(QMouseEvent* e);

    /// @brief 按鼠标位置重画预览：选中实体的副本移动参考点，按住 Shift 时加一条引导线
    void updatePreview(QMouseEvent* e);

    /// @brief 在鼠标位置落位：生成撤销事务，回到 Neutral
    void commit(QMouseEvent* e);

    /// @brief 清除本类画的预览
    void clearPreview();

    bool isPanning() const;

    DmDocument* m_pDocument = nullptr;
    SelectionSet* m_selection = nullptr;
    IDocumentView* m_docView = nullptr;
    ISnapService* m_snapService = nullptr;
    Preview* m_preview = nullptr;
    PanZoomTool* m_panTool = nullptr;

    EnabledQuery m_enabledQuery; ///< 为空时视为总是可用

    int m_status = Neutral;
    bool m_hasPreview = false;
    DmVector m_pressPos; ///< `Pressed` 时左键按下的位置（世界坐标）
    DmVector m_base;     ///< 夹点对应的参考点
};

#endif // EDITTOOL_H
