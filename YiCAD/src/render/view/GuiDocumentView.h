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
/// @brief 文档的画布：经图形系统（GsView）画文档、预览与叠加层

#ifndef GUIDOCUMENTVIEW_H
#define GUIDOCUMENTVIEW_H

// GLEW 必须先于任何 gl.h 被包含（见 GL/glew.h 的 #error 保护），且这是整个翻译单元级别的约束：
// 下面的 <QOpenGLWidget> 经由 Qt 的 qopengl.h 间接拉入系统 GL/gl.h。本文件被很多文件包含，
// 其中有的随后还包含 GL 后端的头文件（GLRhiSurface.h 等，它们要 glew.h），所以在这里先包含它。
#include <GL/glew.h>

#include <QColor>
#include <QOpenGLWidget>
#include <QString>
#include <memory>

#include "DmDocumentListener.h"
#include "DmRect.h"
#include "IDocumentView.h"
#include "ISnapService.h"

class QMouseEvent;
class QKeyEvent;
class QCursor;
class QLabel;
class QTimer;
class DmDocument;
class DmEntityContainer;
class GLRhiWidgetSurface;
class GsModel;
class GsView;
class GuiCommandEvent;
class GuiGrid;
class IHiddenSource;
class IHighlightSource;
class ISelectionSource;

namespace yicad
{
class TimerCounter;
}

/// @brief 文档的画布
/// @details 经图形系统画（RENDER_PLAN.md 第 4.3 节）：文档模型 GsModel 由 AppDocument 持有、同一文档的视图共用
///          （没有注入时画布为自己的文档建一个），本画布的 GsView 持有相机、场景底图与每视图的状态；
///          预览容器是一个容器模型，画在叠加通道里；原点标记、选择框、光标、捕捉标记每帧填进叠加层。
///          相机（画布中心的世界坐标与每像素的世界长度，double）归本类。
///          本类只负责渲染与视图状态，不认识交互层的工具；鼠标、滚轮等输入由派生类 UIView
///          （application/view/UIView.h）接收并交给 ViewToolControl 分发，对应 DS 的 HQWidget 与 UIView 之分。
///          关联文档时注册为它的监听者，析构时注销，因此必须先于文档析构。
class GuiDocumentView : public QOpenGLWidget, public IDocumentView, public DmDocumentListener
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
    DmDocument* getDocument() const override;

    /// @brief 设置网格颜色
    void setGridColor(const QColor& c);
    /// @brief 设置辅网格颜色
    void setMetaGridColor(const QColor& c);
    /// @brief 设置选中颜色
    void setSelectedColor(const QColor& c);
    /// @brief 设置高亮颜色
    void setHighlightColor(const QColor& c);
    /// @brief 设置判断实体是否选中的来源（文档的选择集）；预览里没有选中的实体
    /// @param source 非持有指针，可为空（没有实体按选中绘制）；必须比本画布活得久或在释放前置空
    void setDocumentSelectionSource(const ISelectionSource* source);
    /// @brief 设置取要高亮的实体的来源（视图的高亮集）；预览不涉及高亮
    /// @param source 非持有指针，可为空（没有实体按高亮绘制）；必须比本画布活得久或在释放前置空
    void setDocumentHighlightSource(const IHighlightSource* source);
    /// @brief 设置取临时隐藏的实体的来源（视图的临时隐藏集，RENDER_PLAN.md 第 4.3.9 节）
    /// @param source 非持有指针，可为空；必须比本画布活得久或在释放前置空
    void setHiddenSource(const IHiddenSource* source);
    /// @brief 临时隐藏集变了：下一帧按它重画场景
    void specifyHiddenChanged();
    /// @brief 文档的图形模型（AppDocument 持有，同一文档的视图共用）；不设时画布为自己的文档建一个
    void setGraphicsModel(std::shared_ptr<GsModel> model);
    /// @brief 文档的图形模型；还没有文档时为空
    std::shared_ptr<GsModel> graphicsModel() const override;
    /// @brief 本画布的图形系统视图（测试用：看上一帧是否重画了场景底图）
    const GsView& graphicsView() const { return *m_gsView; }
    /// @brief 设置文档对象：从原文档注销监听，在新文档注册
    /// @param pDoc 文档对象指针，可为空
    void setDocument(DmDocument* pDoc);
    /// @brief 获取缩放因子
    /// @return 单位设备坐标对应的世界坐标
    DmVector getFactor() const override;

    /// @brief 结束全部命令（用户取消：Esc/空格未被接受、"结束全部命令"），并复位选择层
    /// @return 被否决时返回 false，什么也不改变（交互视图 UIView 按迁移计划 5.1 节
    ///         先征求命令同意）；本类没有交互层，总是返回 true
    virtual bool killAllActions();
    /// @brief 视图或文档关闭前结束全部命令，不能否决；本类没有交互层，什么也不做
    virtual void killAllActionsOnClose();
    /// @brief 本类没有交互层，返回 false
    bool hasActiveCommand() const override;
    /// @brief 本类没有交互层，返回空串
    QString activeCommandId() const override;
    /// @brief 发出选择变更信号
    void emitSelectedChanged() override;

    /// @brief 后退：相当于在当前命令中右键；本类没有交互层，什么也不做
    virtual void back();
    /// @brief 前进/确认：合成一次回车按下，交给 processKeyEvent()
    void enter();

    /// @brief 处理主窗口转交的按键（有文档时画布不直接接收键盘事件）
    /// @param e 按键事件；是否被接受由处理者设置在事件上
    /// @return 被交互层处理时返回 true；本类没有交互层，忽略事件并返回 false
    virtual bool processKeyEvent(QKeyEvent* e);

    /// @brief 处理命令行事件（坐标或文本）；本类没有交互层，不接受事件
    virtual void commandEvent(GuiCommandEvent* e);
    /// @brief 启用坐标输入
    void enableCoordinateInput() override;
    /// @brief 禁用坐标输入：命令行输入不再按坐标解析，整段交给命令（如输入文字内容时）
    void disableCoordinateInput() override;
    /// @brief 命令行坐标输入是否启用
    bool isCoordinateInputEnabled() const;

    virtual int getWidth() const;
    virtual int getHeight() const;
    /// @brief 刷新画布
    void redraw() override;
    /// @brief 设置画布背景色
    virtual void setBackground(const QColor& bg);
    /// @brief 设置鼠标光标类型
    void setMouseCursor(DM::CursorType c) override;
    /// @brief 直接设置 Qt 光标
    void setCursor(const QCursor& cursor) override;
    /// @brief 获取用于 Qt 信号槽连接的 QObject 视图
    QObject* asQObject() override;
    virtual DmVector getMousePosition() const;

    /// @brief 放大视图
    /// @param f 放大因子，默认 1.5
    /// @param center 缩放中心点
    void zoomIn(double f = 1.5, const DmVector& center = DmVector(false)) override;
    /// @brief 缩小视图
    /// @param f 缩小因子，默认 1.5
    /// @param center 缩放中心点
    void zoomOut(double f = 1.5, const DmVector& center = DmVector(false)) override;
    /// @brief 适屏显示
    void zoomAuto() override;
    /// @brief 直接设定视图：画布中心对应的世界坐标与比例
    /// @param center 画布中心的世界坐标
    /// @param unitsPerPixel 每像素的世界长度
    void setView(const DmVector& center, double unitsPerPixel);
    /// @brief 移动视图
    /// @param dx X 方向偏移
    /// @param dy Y 方向偏移
    void zoomPan(int dx, int dy) override;

    /// @brief 按当前相机算网格的间距与交点（捕捉网格用）；返回细网格的间距
    double updateGrid();

    /// @brief 设置选择框角点
    void setOverlayCorners(const DmVector& corner1, const DmVector& corner2) override;
    /// @brief 禁用选择框
    void disableOverlayBox() override;

    /// @brief 获取网格对象
    GuiGrid* getGrid() const override;

    /// @brief 设置默认捕捉模式
    void setDefaultSnapMode(SnapMode sm) override;
    SnapMode getDefaultSnapMode() const override;
    /// @brief 设置捕捉限制
    void setSnapRestriction(DM::SnapRestriction sr) override;
    DM::SnapRestriction getSnapRestriction() const;

    /// @brief 检查网格是否开启
    bool isGridOn() const;

    /// @brief 实际坐标转屏幕坐标
    DmVector toGui(DmVector v) const override;
    double toGuiX(double x) const;
    double toGuiY(double y) const;
    double toGuiDX(double d) const override;
    double toGuiDY(double d) const;

    /// @brief 屏幕坐标转实际坐标
    DmVector toGraph(DmVector v) const override;
    DmVector toGraph(int x, int y) const override;
    double toGraphX(int x) const override;
    double toGraphY(int y) const override;
    double toGraphDX(int d) const override;
    double toGraphDY(int d) const;

    /// @brief 锁定/解锁相对零点位置
    /// @param lock true 锁定，false 解锁
    void lockRelativeZero(bool lock);
    /// @return true 表示相对零点已锁定
    bool isRelativeZeroLocked() const;
    /// @return 相对零点坐标
    DmVector const& getRelativeZero() const override;

    void setRelativeZero(const DmVector& pos);
    void moveRelativeZero(const DmVector& pos) override;
    void hideRelativeZero(const bool isHide);

    void setOrthogonalZero(const DmVector& pos) override;
    DmVector const& getOrthogonalZero() const override;

    /// @return true 表示显示线宽（旧称草稿模式）
    bool isDraftMode() const;
    void setDraftMode(bool dm);
    /// @brief 按屏幕尺寸简化显示（RENDER_PLAN.md 第 4.3.10 节：小字画细条、密填充画实心、亚像素对象画点、小圆弧少画分段），
    ///        默认开；关掉时一律按完整几何画（对照与测试用）
    void setLevelOfDetail(bool on);
    /// @brief 每帧场景最多画多少个顶点（RENDER_PLAN.md 第 4.3.10 节的渐进绘制）；0 为按实测的 GPU 耗时定（默认）。
    ///        测试用固定的顶点数，结果与机器快慢无关
    void setSceneBudget(std::size_t vertices);
    /// @brief 场景底图画完了；没画完（超出这一帧的预算）时画布会接着画下一帧
    bool isSceneComplete() const;
    bool isCleanUp(void) const override;

    DmEntityContainer* getPreviewContainer() override;
    /// @brief 指示预览已修改：下一帧预览的模型整体重建
    void specifyPreviewModified() override;
    /// @brief 指示选择集已修改：下一帧只改对象状态与夹点
    void specifySelectChanged();
    /// @brief 指示高亮集已修改：下一帧只重画高亮，场景底图不作废（高亮集很大时除外）
    void specifyHighlightChanged();
    /// @brief 预览的整体变换：预览几何只生成一次，拖动只改变换（RENDER_PLAN.md 第 4.3.9 节）
    void setPreviewTransform(const GiTransform& transform) override;

    // ---- DmDocumentListener ----
    /// @brief 文档已修改：选中的实体可能动了，夹点跟着重取（几何由图形模型按变更集更新）
    void documentModified() override;
    /// @brief 文档请求重绘：同 redraw()
    void redrawRequested() override;
    /// @brief 进入、退出块编辑：图形模型自己换根，这里只重绘
    void paintContainerChanged(DmEntityContainer* container) override;

    /// @brief 获得视图范围（世界坐标）
    DmRect getViewRect() override;

    /// @brief 下一帧的耗时另记一份到指定计数器（埋点开启时），用于统计某个变化之后的首帧
    /// @param counter 进程内的计数器（yicad::counters 里的一个）；同一帧之前多次设置时以最后一次为准
    void setNextFrameCounter(yicad::TimerCounter& counter);

    void setStrDevice(const QString& strDevice);
    /// @brief 输入设备名称（"Mouse"/"Trackpad"），决定滚轮的解释方式
    const QString& getStrDevice() const;
    void setIsDrawCursor(const bool& isDrawCursor) override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    /// @brief 记录鼠标的世界坐标（绘制十字光标用）；派生类先调用本函数再分发
    void mouseMoveEvent(QMouseEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;

    /// @brief 当前捕捉结果，决定捕捉标记与捕捉提示；本类没有捕捉器，返回 SnapNone
    virtual SnapResultType currentSnapResult();
    /// @brief 当前捕捉点，与 currentSnapResult() 取自同一个捕捉器
    virtual DmVector currentSnapSpot();

    /// @brief 按 currentSnapResult() 显示或隐藏捕捉类型提示
    /// @param pos 鼠标的画布坐标，提示显示在它右下方
    void updateSnapTooltip(const QPoint& pos);

private slots:
    void hideSnapTooltip();

private:
    /// @brief 相机改了：重绘并发出 viewChanged（场景底图由 GsView 比对相机后作废）
    void cameraChanged();
    /// @brief 叠加层（原点标记、选择框、光标、捕捉标记）填进 GsView 的动态批次
    void fillOverlay();
    /// @brief 选中实体的夹点：参考点超过 100 个时一个也不画（与原先相同）
    std::vector<DmVector> collectGrips() const;

protected:
    DmDocument*                         pDocument;              ///< 文档实体容器
    bool                                m_isCoordinateInputEnabled = true; ///< 命令行坐标输入是否启用

    QColor                              background;             ///< 背景色
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
    bool                                m_levelOfDetail = true;  ///< 按屏幕尺寸简化显示
    std::size_t                         m_sceneBudget = 0;       ///< 每帧场景的顶点预算；0 为按实测
    double                              m_dMinScale;            ///< 视图放大至最大时单位像素允许的世界坐标尺寸

    DmVector                            relativeZero;           ///< 鼠标上一次捕捉的坐标
    DmVector                            orthogonalZero;         ///< 正交零点
    bool                                relativeZeroLocked;     ///< 相对零点是否锁定

    // 相机
    DmVector                            m_viewCenter = DmVector(0.0, 0.0); ///< 画布中心的世界坐标
    double                              m_unitsPerPixel = 1.0;  ///< 每（逻辑）像素的世界长度

    // 图形系统（RENDER_PLAN.md 第 4 阶段）
    std::shared_ptr<GsModel>            m_gsModel;              ///< 文档模型
    std::unique_ptr<GsModel>            m_gsPreview;            ///< 预览容器的模型
    std::unique_ptr<GsView>             m_gsView;               ///< 本画布的视图
    std::unique_ptr<GLRhiWidgetSurface> m_gsSurface;            ///< 画到本画布的表面
    const IHiddenSource*                m_pHiddenSource = nullptr; ///< 临时隐藏集
    bool                                m_gsGripsDirty = true;  ///< 选择集或文档变了，下一帧重新取夹点

    bool                                m_bIsCleanUp;           ///< 如果为 true 则清理 docView

    DmEntityContainer*                  m_pPreviewEntityContainer;  ///< 预览实体容器
    const ISelectionSource*             m_pDocumentSelection = nullptr; ///< 判断选中的来源
    const IHighlightSource*             m_pDocumentHighlight = nullptr; ///< 取高亮实体的来源

    DmVector                            m_currentMousePt;           ///< 当前鼠标位置（世界坐标）
    DM::CursorType                      m_eCursorType;              ///< 当前鼠标类型

    std::unique_ptr<QCursor>            m_selEntityCurcorStyle;     ///< 自定义选择实体时的鼠标样式

    QString                             m_strDevice;                ///< 输入设备名称

    bool                                m_isDrawCursor;             ///< 是否绘制光标
    bool                                m_isDrawOverlayBox = false; ///< 是否绘制选择框
    DmVector                            m_overlayCorner1;           ///< 选择框角点1
    DmVector                            m_overlayCorner2;           ///< 选择框角点2

    QLabel*                             m_snapTooltip = nullptr;    ///< 捕捉类型文字提示
    yicad::TimerCounter*                m_pNextFrameCounter = nullptr; ///< 下一帧的耗时另记一份的计数器，见 setNextFrameCounter()
    QTimer*                             m_snapTooltipTimer = nullptr; ///< 捕捉提示隐藏定时器

signals:
    void relative_zero_changed(const DmVector&);
    void xbutton1_released();
    void viewChanged();
    void selectedChanged();
};

#endif
