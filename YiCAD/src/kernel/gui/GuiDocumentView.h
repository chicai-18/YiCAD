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

/// @file GuiDocumentView.h
/// @brief 文档画布类，使用 OpenGL 渲染四层画布

#ifndef GUIDOCUMENTVIEW_H
#define GUIDOCUMENTVIEW_H

#define GL_GLEXT_PROTOTYPES

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <QColor>
#include <QMap>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QString>
#include <QToolButton>
#include <memory>

#include "PainterCreator.h"

#include "DmRect.h"
#include "Snapper.h"
#include "CustomComboboxItem.h"

class QMouseEvent;
class QKeyEvent;
class QCursor;
class QLabel;
class QTimer;
class ActionInterface;
class DmCachePainter;
class DmDocument;
class DmEntityContainer;
class GuiEventHandler;
class GuiCommandEvent;
class GuiGrid;

namespace opengl
{
class GLPainter;
}

/// @brief 文档的画布
/// @details 包括4层：背景层，文档层，预览层，前景层
class GuiDocumentView : public QOpenGLWidget
{
    Q_OBJECT

public:
    /// @brief 构造文档画布
    /// @param parent 父控件
    /// @param fl 窗口标志
    /// @param doc 关联的文档对象
    GuiDocumentView(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags(), DmDocument* doc = 0);
    virtual ~GuiDocumentView();

    /// @brief 清理资源
    void cleanUp();

    /// @brief 获取关联的文档对象
    /// @return 文档对象指针，如果无效则返回 nullptr
    DmDocument* getDocument() const;

    /// @brief 设置网格颜色
    void setGridColor(const QColor& c);
    /// @brief 设置辅网格颜色
    void setMetaGridColor(const QColor& c);
    /// @brief 设置选中颜色
    void setSelectedColor(const QColor& c);
    /// @brief 设置高亮颜色
    void setHighlightColor(const QColor& c);
    /// @brief 设置文档对象
    /// @param pDoc 文档对象指针
    void setDocument(DmDocument* pDoc);
    /// @brief 获取缩放因子
    /// @return 单位设备坐标对应的世界坐标
    DmVector getFactor() const;

    /// @brief 设置默认操作
    void setDefaultAction(ActionInterface* action);
    /// @brief 获取默认操作
    ActionInterface* getDefaultAction();
    /// @brief 设置当前操作
    void setCurrentAction(ActionInterface* action);
    /// @brief 获取当前操作
    ActionInterface* getCurrentAction();

    /// @brief 终止选择类操作
    void killSelectActions();
    /// @brief 终止所有操作
    void killAllActions();
    /// @brief 发出选择变更信号
    void emitSelectedChanged();

    /// @brief 后退
    void back();
    /// @brief 前进/确认
    void enter();

    /// @brief 处理命令事件
    void commandEvent(GuiCommandEvent* e);
    /// @brief 启用坐标输入
    void enableCoordinateInput();
    /// @brief 禁用坐标输入
    void disableCoordinateInput();

    virtual int getWidth() const;
    virtual int getHeight() const;
    /// @brief 刷新画布
    virtual void redraw();
    /// @brief 设置画布背景色
    virtual void setBackground(const QColor& bg);
    /// @brief 设置鼠标光标类型
    virtual void setMouseCursor(DM::CursorType c);
    virtual DmVector getMousePosition() const;

    /// @brief 放大视图
    /// @param f 放大因子，默认 1.5
    /// @param center 缩放中心点
    void zoomIn(double f = 1.5, const DmVector& center = DmVector(false));
    /// @brief 缩小视图
    /// @param f 缩小因子，默认 1.5
    /// @param center 缩放中心点
    void zoomOut(double f = 1.5, const DmVector& center = DmVector(false));
    /// @brief 适屏显示
    void zoomAuto();
    /// @brief 移动视图
    /// @param dx X 方向偏移
    /// @param dy Y 方向偏移
    void zoomPan(int dx, int dy);

    void drawBackgroundLayer();
    void drawDocumentLayer();
    void drawPreviewLayer();
    void drawForegroundLayer();

    /// @brief 绘制原点坐标
    void drawAbsoluteZero();
    /// @brief 绘制背景栅格
    void drawGridLine();
    /// @brief 绘制鼠标十字光标
    void drawCursor();
    /// @brief 绘制捕捉点标识
    void drawSnapIndicator();
    /// @brief 设置选择框角点
    void setOverlayCorners(const DmVector& corner1, const DmVector& corner2);
    /// @brief 绘制选择框
    void drawOverlayBox();
    /// @brief 禁用选择框
    void disableOverlayBox();

    /// @brief 获取网格对象
    GuiGrid* getGrid() const;

    /// @brief 设置默认捕捉模式
    void setDefaultSnapMode(SnapMode sm);
    SnapMode getDefaultSnapMode() const;
    /// @brief 设置捕捉限制
    void setSnapRestriction(DM::SnapRestriction sr);
    DM::SnapRestriction getSnapRestriction() const;

    /// @brief 检查网格是否开启
    bool isGridOn() const;

    /// @brief 实际坐标转屏幕坐标
    DmVector toGui(DmVector v) const;
    double toGuiX(double x) const;
    double toGuiY(double y) const;
    double toGuiDX(double d) const;
    double toGuiDY(double d) const;

    /// @brief 屏幕坐标转实际坐标
    DmVector toGraph(DmVector v) const;
    DmVector toGraph(int x, int y) const;
    double toGraphX(int x) const;
    double toGraphY(int y) const;
    double toGraphDX(int d) const;
    double toGraphDY(int d) const;

    /// @brief 锁定/解锁相对零点位置
    /// @param lock true 锁定，false 解锁
    void lockRelativeZero(bool lock);
    /// @return true 表示相对零点已锁定
    bool isRelativeZeroLocked() const;
    /// @return 相对零点坐标
    DmVector const& getRelativeZero() const;

    void setRelativeZero(const DmVector& pos);
    void moveRelativeZero(const DmVector& pos);
    void hideRelativeZero(const bool isHide);

    void setOrthogonalZero(const DmVector& pos);
    DmVector const& getOrthogonalZero() const;

    GuiEventHandler* getEventHandler() const;

    /// @brief 启用或禁用打印预览
    void setPrintPreview(bool pv);
    /// @return true 表示当前为打印预览视图
    bool isPrintPreview() const;

    /// @brief 启用或禁用打印
    void setPrinting(bool p);
    /// @return true 表示当前为打印视图
    bool isPrinting() const;

    /// @return true 表示草稿模式（线宽为 1 像素，无样式缩放）
    bool isDraftMode() const;
    void setDraftMode(bool dm);
    bool isCleanUp(void) const;

    virtual DmEntityContainer* getOverlayContainer(DM::OverlayDocument position);
    DmEntityContainer* getPreviewContainer();
    /// @brief 指示预览已修改
    void specifyPreviewModified();
    /// @brief 指示文档已修改
    void specifyDocumentModified();
    /// @brief 指示选择已修改
    void specifySelectChanged();
    /// @brief 指定预览模型矩阵的偏移量
    void setPreviewModelOffset(const DmVector& offset);
    /// @brief 切换文档画笔的实体容器（用于块编辑）
    void setDocumentPainterContainer(DmEntityContainer* container);

    /// @brief 获得视图范围（世界坐标）
    DmRect getViewRect();

    void setStrDevice(const QString& strDevice);
    void setIsDrawCursor(const bool& isDrawCursor);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void tabletEvent(QTabletEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void enterEvent(QEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private slots:
    void hideSnapTooltip();

private:
    void createPainters(unsigned int width, unsigned int height);
    void deletePainters();

protected:
    DmDocument*                         pDocument;              ///< 文档实体容器
    GuiEventHandler*                    eventHandler;           ///< 事件处理器

    QColor                              background;             ///< 背景色
    QColor                              foreground;             ///< 前景色
    QColor                              gridColor;              ///< 网格主线色
    QColor                              metaGridColor;          ///< 网格辅线色
    QColor                              selectedColor;          ///< 选中实体颜色
    QColor                              highlightColor;         ///< 高亮实体颜色
    std::unique_ptr<GuiGrid>            grid;                   ///< 背景网格

    SnapMode                            defaultSnapMode;        ///< 当前默认捕捉模式
    DM::SnapRestriction                 defaultSnapRes;         ///< 当前默认捕捉限制

    bool                                isSmoothScrolling;      ///< 当前是否正在高分辨率下滚动鼠标中键

private:
    bool                                draftMode;              ///< 是否显示线宽
    double                              m_dMinScale;            ///< 视图放大至最大时单位像素允许的世界坐标尺寸

    DmVector                            relativeZero;           ///< 鼠标上一次捕捉的坐标
    DmVector                            orthogonalZero;         ///< 正交零点
    bool                                relativeZeroLocked;     ///< 相对零点是否锁定
    bool                                printPreview;           ///< 打印预览标志
    bool                                printing;               ///< 仅在打印时激活

    QMap<int, DmEntityContainer*>       m_overlayEntities;      ///< 交互时的前景实体集（TODO 删除）

    bool                                m_bIsCleanUp;           ///< 如果为 true 则清理 docView

    DmEntityContainer*                  m_pPreviewEntityContainer;  ///< 预览实体容器
    opengl::GLPainter*                  m_pBackgroundPainter;       ///< 背景画笔
    DmCachePainter*                     m_pDocumentPainter;         ///< 文档画笔
    DmCachePainter*                     m_pPreviewPainter;          ///< 预览画笔
    opengl::GLPainter*                  m_pForegroundPainter;       ///< 前景画笔

    DmVector                            m_currentMousePt;           ///< 当前鼠标位置（世界坐标）
    DM::CursorType                      m_eCursorType;              ///< 当前鼠标类型

    std::unique_ptr<QCursor>            m_selEntityCurcorStyle;     ///< 自定义选择实体时的鼠标样式

    QString                             m_strDevice;                ///< 输入设备名称

    bool                                m_isDrawCursor;             ///< 是否绘制光标
    bool                                m_isDrawOverlayBox;         ///< 是否绘制选择框
    DmVector                            m_overlayCorner1;           ///< 选择框角点1
    DmVector                            m_overlayCorner2;           ///< 选择框角点2

    QLabel*                             m_snapTooltip = nullptr;    ///< 捕捉类型文字提示
    QTimer*                             m_snapTooltipTimer = nullptr; ///< 捕捉提示隐藏定时器

signals:
    void relative_zero_changed(const DmVector&);
    void xbutton1_released();
    void viewChanged();
    void selectedChanged();
};

#endif
