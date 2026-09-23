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

/// @file IDocumentView.h
/// @brief Action 与 Snapper 实际依赖的文档视图能力接口
///
/// 从具体的 GuiDocumentView（OpenGL 画布控件）中抽出 Action 体系真正
/// 需要的一组能力：坐标变换、重绘请求、光标设置、预览容器访问、相对零点、
/// 视口矩形等。GuiDocumentView 实现本接口；actions/ 与 kernel/actions/
/// 只依赖本接口，不再需要包含拖有 OpenGL/Qt 具体渲染细节的
/// GuiDocumentView.h。见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段1。

#ifndef IDOCUMENTVIEW_H
#define IDOCUMENTVIEW_H

#include "DmRect.h"
#include "Snapper.h"

class QCursor;
class QObject;
class ActionInterface;
class DmDocument;
class DmEntityContainer;
class GuiEventHandler;
class GuiGrid;

/// @brief Action / Snapper 视角下的文档视图接口
class IDocumentView
{
public:
    virtual ~IDocumentView() = default;

    /// @brief 刷新画布
    virtual void redraw() = 0;
    /// @brief 设置鼠标光标类型
    virtual void setMouseCursor(DM::CursorType c) = 0;
    /// @brief 直接设置 Qt 光标（脱离 setMouseCursor 的自绘光标语义时使用）
    virtual void setCursor(const QCursor& cursor) = 0;
    /// @brief 获取用于 Qt 信号槽连接的 QObject 视图
    virtual QObject* asQObject() = 0;

    /// @brief 放大视图
    virtual void zoomIn(double f = 1.5, const DmVector& center = DmVector(false)) = 0;
    /// @brief 缩小视图
    virtual void zoomOut(double f = 1.5, const DmVector& center = DmVector(false)) = 0;
    /// @brief 适屏显示
    virtual void zoomAuto() = 0;
    /// @brief 移动视图
    virtual void zoomPan(int dx, int dy) = 0;

    /// @brief 设置选择框角点
    virtual void setOverlayCorners(const DmVector& corner1, const DmVector& corner2) = 0;
    /// @brief 禁用选择框
    virtual void disableOverlayBox() = 0;

    /// @brief 获取网格对象
    virtual GuiGrid* getGrid() const = 0;

    /// @brief 设置默认捕捉模式
    virtual void setDefaultSnapMode(SnapMode sm) = 0;
    virtual SnapMode getDefaultSnapMode() const = 0;
    /// @brief 设置捕捉限制
    virtual void setSnapRestriction(DM::SnapRestriction sr) = 0;

    /// @brief 实际坐标转屏幕坐标
    virtual DmVector toGui(DmVector v) const = 0;
    virtual double toGuiDX(double d) const = 0;

    /// @brief 屏幕坐标转实际坐标
    virtual DmVector toGraph(DmVector v) const = 0;
    virtual DmVector toGraph(int x, int y) const = 0;
    virtual double toGraphX(int x) const = 0;
    virtual double toGraphY(int y) const = 0;
    virtual double toGraphDX(int d) const = 0;

    /// @return 相对零点坐标
    virtual DmVector const& getRelativeZero() const = 0;
    virtual void moveRelativeZero(const DmVector& pos) = 0;

    virtual void setOrthogonalZero(const DmVector& pos) = 0;
    virtual DmVector const& getOrthogonalZero() const = 0;

    virtual GuiEventHandler* getEventHandler() const = 0;

    virtual bool isCleanUp() const = 0;

    virtual DmEntityContainer* getOverlayContainer(DM::OverlayDocument position) = 0;
    virtual DmEntityContainer* getPreviewContainer() = 0;
    /// @brief 指示预览已修改
    virtual void specifyPreviewModified() = 0;
    /// @brief 指示文档已修改
    virtual void specifyDocumentModified() = 0;
    /// @brief 指定预览模型矩阵的偏移量
    virtual void setPreviewModelOffset(const DmVector& offset) = 0;

    /// @brief 获得视图范围（世界坐标）
    virtual DmRect getViewRect() = 0;

    virtual void setIsDrawCursor(const bool& isDrawCursor) = 0;

    /// @brief 设置当前操作
    virtual void setCurrentAction(ActionInterface* action) = 0;
    /// @brief 获取当前操作
    virtual ActionInterface* getCurrentAction() = 0;
    /// @brief 终止选择类操作
    virtual void killSelectActions() = 0;
    /// @brief 发出选择变更信号
    virtual void emitSelectedChanged() = 0;

    /// @brief 启用坐标输入
    virtual void enableCoordinateInput() = 0;
    /// @brief 禁用坐标输入
    virtual void disableCoordinateInput() = 0;

    /// @brief 获取关联的文档对象
    virtual DmDocument* getDocument() const = 0;
    /// @brief 获取缩放因子
    virtual DmVector getFactor() const = 0;
};

#endif // IDOCUMENTVIEW_H
