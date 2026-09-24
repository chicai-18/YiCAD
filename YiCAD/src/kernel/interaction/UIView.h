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

/// @file UIView.h
/// @brief 交互视图：在画布 GuiDocumentView 之上装配交互层
///
/// 对应 DS 的 UIView（DimX/Source/View/UIView.h）。DS 把渲染控件 HQWidget 与
/// 交互组装 UIView 拆成基类与派生类，本类沿用这一拆法：GuiDocumentView 是
/// 渲染层（YiCadRender）的画布，不认识交互层的具体类型；本类持有
/// ViewToolControl 与导航（PanZoomTool）、选择（SelectTool）、业务
/// （LegacyActionTool）三层工具，接收画布的 Qt 输入事件交给 ViewToolControl
/// 分发。第二步的命令总线也由本类持有（doc/COMMAND_TOOL_MIGRATION_PLAN.md）。
///
/// 放在 kernel/interaction/（YiCadInteraction 分区），不和画布同在
/// kernel/view/（YiCadRender 分区）：一个目录归一个分区，渲染层不能依赖交互层。
/// DS 把 HQWidget 与 UIView 同放 View/，是因为它不按目录分库。
///
/// 命令与工具只能经 IDocumentView/GuiDocumentView 认识视图，不能反过来依赖
/// 本类：内核禁止包含 UI* 头文件（tools/check_layering.py），白名单只放行
/// UIView.cpp 包含自身头文件。这一点与 DS 不同：DS 的 EditTool、
/// ExclusiveCommandBus 以 UIView* 构造。
///
/// 旧版 Action 栈 GuiEventHandler 仍由基类持有：IDocumentView 的
/// getEventHandler()/setCurrentAction()/getCurrentAction() 要求画布实现它们，
/// 第四步随 GuiEventHandler 一起删除。

#ifndef UIVIEW_H
#define UIVIEW_H

#include <memory>

#include "GuiDocumentView.h"

class LegacyActionTool;
class PanZoomTool;
class Preview;
class SelectTool;
class ViewToolControl;

/// @brief 交互视图：画布加交互层工具栈
class UIView : public GuiDocumentView
{
    Q_OBJECT

public:
    /// @param parent 父控件
    /// @param fl 窗口标志
    /// @param doc 关联的文档；为空时不建选择层
    UIView(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags(), DmDocument* doc = nullptr);
    ~UIView() override;

    /// @brief 主窗口转交的按键，经 ViewToolControl 分发
    /// @return 某一层工具处理了该事件时返回 true
    bool processKeyEvent(QKeyEvent* e) override;

    /// @brief 设置默认捕捉模式，并同步给选择层的捕捉器
    void setDefaultSnapMode(SnapMode sm) override;
    /// @brief 设置捕捉限制，并同步给选择层的捕捉器
    void setSnapRestriction(DM::SnapRestriction sr) override;

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void tabletEvent(QTabletEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void enterEvent(QEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

    /// @brief 有业务 Action 时取它的捕捉器，空闲态取选择层的
    SnapResultType currentSnapResult() override;
    /// @brief 与 currentSnapResult() 取自同一个捕捉器
    DmVector currentSnapSpot() override;

private:
    // 注意声明顺序：成员按声明的逆序析构。m_pViewToolControl 最后声明、最先
    // 析构，析构时向各层工具发 onDeactivate()，被它引用的工具此时都还在；
    // 工具之间的裸指针（LegacyActionTool/SelectTool 引用 PanZoomTool，
    // SelectTool 引用捕捉器与预览容器）也按被引用者在前排列。全部成员都在
    // 基类析构之前析构，基类持有的预览容器与 GuiEventHandler 这时仍然有效。
    std::unique_ptr<PanZoomTool>        m_pPanZoomTool;         ///< 导航层：中键/Ctrl+左键平移
    std::unique_ptr<LegacyActionTool>   m_pLegacyActionTool;    ///< 业务层：包装基类的 GuiEventHandler
    std::unique_ptr<Snapper>            m_pSelectSnapper;       ///< 选择层的捕捉器，空闲态的捕捉提示也读它
    std::unique_ptr<Preview>            m_pSelectPreview;       ///< 选择层拖动实体时的预览容器
    std::unique_ptr<SelectTool>         m_pSelectTool;          ///< 选择层；没有文档时为空
    std::unique_ptr<ViewToolControl>    m_pViewToolControl;     ///< 交互层工具控制器
};

#endif // UIVIEW_H
