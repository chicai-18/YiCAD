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

/// @file GuiDocumentView.cpp
/// @brief 文档画布类实现，包含 OpenGL 渲染和事件处理

#include "GuiDocumentView.h"

#include <climits>
#include <cmath>

#include <QApplication>
#include <QAction>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QtAlgorithms>
#include <QNativeGestureEvent>
#include <QTimer>
#include <QLabel>
#include <QToolButton>

#include "DmLine.h"
#include "DmEntityContainer.h"
#include "DmDocument.h"
#include "GuiGrid.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmBlockReference.h"
#include "DmSettings.h"
#include "DmLayer.h"
#include "Math2d.h"
#include "Debug.h"
#include "ScopedTimer.h"
#include "DmColor.h"

#ifdef Q_OS_WIN32
#define CURSOR_SIZE 16
#else
#define CURSOR_SIZE 15
#endif

#include <optional>
#include <set>
#include <utility>
#include "QString"
#include "RhiFrameStats.h"
#include "GLRhiDevice.h"
#include "GLRhiSurface.h"
#include "GsModel.h"
#include "GsView.h"
#include "IHiddenSource.h"
#include "IHighlightSource.h"
#include "ISelectionSource.h"

GuiDocumentView::GuiDocumentView(QWidget* parent, Qt::WindowFlags f, DmDocument* doc)
    : QOpenGLWidget(parent, f)
    , pDocument(nullptr)
    , background(30, 30, 30, 255)
    , gridColor(50, 55, 72, 255)
    , metaGridColor(73, 79, 105, 255)
    , grid(new GuiGrid())
    , defaultSnapMode(SnapMode())
    , defaultSnapRes(DM::RestrictNothing)
    , isSmoothScrolling(false)
    , draftMode(false)
    , m_dMinScale(0.0001)
    , relativeZero(DmVector(false))
    , orthogonalZero(DmVector(false))
    , relativeZeroLocked(false)
    , m_gsView(std::make_unique<GsView>())
    , m_bIsCleanUp(false)
    , m_pPreviewEntityContainer(new DmEntityContainer())
    , m_currentMousePt(DmVector(false))
    , m_eCursorType(DM::CadCursor)
    , m_selEntityCurcorStyle(new QCursor(QPixmap(":/ribbon/cursor_style/select_entity.svg"), CURSOR_SIZE, CURSOR_SIZE))
    , m_strDevice("Mouse")
    , m_isDrawCursor(true)
    , m_snapTooltip(nullptr)
    , m_snapTooltipTimer(nullptr)
{
    setMouseTracking(true);

    if (doc)
    {
        setDocument(doc);
    }

    DMSETTINGS->beginGroup("Colors");
    setBackground(QColor(DMSETTINGS->readEntry("/background", Colors::BACKGROUND)));
    setGridColor(QColor(DMSETTINGS->readEntry("/grid", Colors::GRID)));
    setMetaGridColor(QColor(DMSETTINGS->readEntry("/meta_grid", Colors::META_GRID)));
    setSelectedColor(QColor(DMSETTINGS->readEntry("/select", Colors::SELECT)));
    setHighlightColor(QColor(DMSETTINGS->readEntry("/highlight", Colors::HIGHLIGHT)));
    DMSETTINGS->endGroup();

    const int MULTISAMPLE_COUNT = 4;
    // 与程序的其他 GL 上下文同一格式（4.3 core），否则共享组里的上下文格式不一（RENDER_PLAN.md 第 4.7.3 节）
    QSurfaceFormat format = GLRhiDevice::surfaceFormat();
    format.setSamples(MULTISAMPLE_COUNT);    // 设置多重采样的采样点数
    setFormat(format);
    // 此处原有一行 glEnable(GL_MULTISAMPLE)，已删除（P12）：构造函数里还没有
    // current context，该调用不会生效。删除是安全的——GL_MULTISAMPLE 的默认值
    // 本就是 GL_TRUE，上面 setFormat 请求的 4 重采样已经使多重采样生效。

    // 捕捉类型文字提示
    m_snapTooltip = new QLabel(this);
    m_snapTooltip->setWindowFlags(Qt::ToolTip);
    m_snapTooltip->setStyleSheet("QLabel { background-color: #333333; color: #FFD700; border: 1px solid #888888; padding: 4px 10px; font-size: 16px; font-weight: bold; }");
    m_snapTooltip->hide();

    m_snapTooltipTimer = new QTimer(this);
    m_snapTooltipTimer->setSingleShot(true);
    connect(m_snapTooltipTimer, &QTimer::timeout, this, &GuiDocumentView::hideSnapTooltip);

    // 图形系统：画到本画布的表面与预览容器的模型（文档模型由 AppDocument 注入或第一帧时自建）
    m_gsSurface = std::make_unique<GLRhiWidgetSurface>(*this);
    m_gsPreview = std::make_unique<GsModel>(m_pPreviewEntityContainer);
    m_gsView->setTransient(m_gsPreview.get());
}

GuiDocumentView::~GuiDocumentView()
{
    // 图形系统的资源放进设备的延迟释放队列，不需要当前上下文
    m_gsView->setTransient(nullptr);
    m_gsView->release();
    m_gsPreview.reset();
    m_gsModel.reset();
    setDocument(nullptr);
    cleanUp();

    if (m_pPreviewEntityContainer)
    {
        delete m_pPreviewEntityContainer;
        m_pPreviewEntityContainer = nullptr;
    }
}

/// @brief 必须由派生类的析构函数调用
void GuiDocumentView::cleanUp()
{
    m_bIsCleanUp = true;
}

/// @brief 设置文档对象，并把本画布从原文档的监听者中移到新文档
void GuiDocumentView::setDocument(DmDocument* pDoc)
{
    if (pDocument)
    {
        pDocument->removeListener(this);
    }
    if (pDoc != pDocument && m_gsModel)
    {
        m_gsView->setModel(nullptr, true);
        m_gsModel.reset();
    }
    this->pDocument = pDoc;
    if (pDocument)
    {
        pDocument->addListener(this);
    }
}

/// @brief 检查网格是否开启
/// @return true 表示网格已打开
bool GuiDocumentView::isGridOn() const
{
    if (pDocument)
    {
        return pDocument->isGridOn();
    }
    return true;
}

/// @brief 终止所有操作；本类没有交互层
bool GuiDocumentView::killAllActions()
{
    return true;
}

/// @brief 视图关闭前终止所有操作；本类没有交互层
void GuiDocumentView::killAllActionsOnClose()
{
}

bool GuiDocumentView::hasActiveCommand() const
{
    return false;
}

QString GuiDocumentView::activeCommandId() const
{
    return QString();
}

/// @brief 发出选择变更信号
/// @details 原先还标记文档已修改，让文档画笔整图重建；调用方都是改了选择集之后调这里，
///          选择集自己的变化通知已让画笔重建选中组（RENDER_PLAN.md 1.1 步、第 4.9 节）
void GuiDocumentView::emitSelectedChanged()
{
    emit selectedChanged();
}

/// @brief 在当前命令中后退；本类没有交互层
void GuiDocumentView::back()
{
}

/// @brief 前进/确认当前操作：合成一次回车按下，交给 processKeyEvent()
void GuiDocumentView::enter()
{
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    processKeyEvent(&e);
}

bool GuiDocumentView::processKeyEvent(QKeyEvent* e)
{
    e->ignore();
    return false;
}

/// @brief 处理命令事件（由命令行 UI 调用）；本类没有交互层，不接受事件
void GuiDocumentView::commandEvent(GuiCommandEvent*)
{
}

/// @brief 启用命令行坐标输入
void GuiDocumentView::enableCoordinateInput()
{
    m_isCoordinateInputEnabled = true;
}

/// @brief 禁用命令行坐标输入
void GuiDocumentView::disableCoordinateInput()
{
    m_isCoordinateInputEnabled = false;
}

bool GuiDocumentView::isCoordinateInputEnabled() const
{
    return m_isCoordinateInputEnabled;
}

int GuiDocumentView::getWidth() const
{
    return width();
}

int GuiDocumentView::getHeight() const
{
    return height();
}

void GuiDocumentView::redraw()
{
    update();
}

/// @brief 放大视图
/// @param f 放大因子
/// @param center 缩放中心
void GuiDocumentView::zoomIn(double f, const DmVector& center)
{
    const double MIN_ZOOM_FACTOR = 1.0e-6;
    if (f < MIN_ZOOM_FACTOR)
    {
        return;
    }

    // 设置画布最小放缩比例
    if (getFactor().x < m_dMinScale && f < 1)
    {
        return;
    }

    // 以 c 为不动点缩放：c 在屏幕上的位置不变。原先算了 c 却把 center 交给画笔，没给缩放中心时
    // （缩放命令）绕世界原点缩放；现在按 c（鼠标在画布上时取鼠标，否则取画布中心）
    DmVector c = center;
    if (!c.valid)
    {
        c = getMousePosition();
    }
    const double newScale = m_unitsPerPixel * f;
    m_viewCenter = DmVector(c.x - (c.x - m_viewCenter.x) * f, c.y - (c.y - m_viewCenter.y) * f);
    m_unitsPerPixel = newScale;
    cameraChanged();
}

/// @brief 缩小视图
/// @param f 缩小因子
/// @param center 缩放中心
void GuiDocumentView::zoomOut(double f, const DmVector& center)
{
    const double MIN_ZOOM_FACTOR = 1.0e-6;
    if (f < MIN_ZOOM_FACTOR)
    {
        return;
    }
    zoomIn(1 / f, center);
}

/// @brief 适屏显示
void GuiDocumentView::zoomAuto()
{
    if (pDocument)
    {
        if (getWidth() == 0 || getHeight() == 0)
        {
            return;
        }
        double sx = 0.0;
        double sy = 0.0;
        pDocument->getEntityTable()->updateContainer();
        auto container = pDocument->getEntityTable()->getEntityContainer();
        DmVector max = container->getMax();
        DmVector min = container->getMin();
        DmVector center = (max + min) / 2.0;
        auto const dV = max - min;
        sx = std::max(dV.x, 0.);
        sy = std::max(dV.y, 0.);

        double fx = 1., fy = 1.;
        if (sx > DM_TOLERANCE && sy > DM_TOLERANCE)
        {
            fx = sx / getWidth();
            fy = sy / getHeight();
        }
        else
        {
            return;
        }
        fx = fy = std::max(fx, fy);

        setView(center, fx);  // 重绘并发出 viewChanged
        return;
    }
    emit viewChanged();
}

void GuiDocumentView::setView(const DmVector& center, double unitsPerPixel)
{
    m_viewCenter = DmVector(center.x, center.y);
    m_unitsPerPixel = unitsPerPixel;
    cameraChanged();
}

void GuiDocumentView::cameraChanged()
{
    redraw();
    emit viewChanged();
}

/// @brief 平移视图
/// @param dx X 方向像素偏移
/// @param dy Y 方向像素偏移
void GuiDocumentView::zoomPan(int dx, int dy)
{
    double dx_world = toGraphDX(dx);
    double dy_world = toGraphDY(dy);
    m_viewCenter = DmVector(m_viewCenter.x - dx_world, m_viewCenter.y - dy_world);
    cameraChanged();
}

double GuiDocumentView::updateGrid()
{
    const double MIN_GRID_SPACING_INIT = 20.;  // 最小网格间距
    const double MIN_DISTANCE_LOWER = 10.0;      // 距离下限
    const double MIN_DISTANCE_UPPER = 100.0;     // 距离上限
    const double SCALE_FACTOR = 10.0;            // 缩放调整因子
    const double THRESHOLD_10 = 10.0;
    const double THRESHOLD_20 = 20.0;
    const double THRESHOLD_50 = 50.0;
    const double NUM_MINOR_LINES = 5.;

    if (!grid)
    {
        return 0.0;
    }

    // 间距：与原先 drawGridLine 相同，屏幕上 10 到 100 像素之间取 10、20、50、100 的整数倍
    DmVector zeroCorner = toGraph(DmVector(0., 0.));
    DmVector gridSPacing = toGraph(DmVector(MIN_GRID_SPACING_INIT, MIN_GRID_SPACING_INIT));
    double minDistancePoints = gridSPacing.x - zeroCorner.x;
    double factor = 1.0;
    while (minDistancePoints < MIN_DISTANCE_LOWER && minDistancePoints > 0.0)
    {
        minDistancePoints *= SCALE_FACTOR;
        factor = factor * SCALE_FACTOR;
    }
    while (minDistancePoints > MIN_DISTANCE_UPPER)
    {
        minDistancePoints = minDistancePoints / SCALE_FACTOR;
        factor = factor / SCALE_FACTOR;
    }
    double gridSize;
    if (minDistancePoints < THRESHOLD_10)
    {
        gridSize = (THRESHOLD_10 / factor);
    }
    else if (minDistancePoints < THRESHOLD_20)
    {
        gridSize = (THRESHOLD_20 / factor);
    }
    else if (minDistancePoints < THRESHOLD_50)
    {
        gridSize = (THRESHOLD_50 / factor);
    }
    else
    {
        gridSize = (MIN_DISTANCE_UPPER / factor);
    }
    grid->setCellVector(DmVector(gridSize, gridSize));

    // 交点（捕捉网格用）：细线与粗线的交点，同原先
    const geo::Area updateRect = { {toGraph(DmVector(0,0))},{toGraph(DmVector(getWidth(), getHeight()))} };
    std::vector<DmVector> points;
    double left = updateRect.minP().x - fmod(updateRect.minP().x, gridSize);
    double top = updateRect.maxP().y - fmod(updateRect.maxP().y, gridSize);
    grid->setBaseGrid(DmVector(left, updateRect.minP().y - fmod(updateRect.minP().y, gridSize)));
    for (double x = left; x < updateRect.maxP().x; x += gridSize)
    {
        for (double y = top; y > updateRect.minP().y; y -= gridSize)
        {
            points.emplace_back(DmVector(x, y));
        }
    }
    const double major = gridSize * static_cast<int>(NUM_MINOR_LINES);
    left = updateRect.minP().x - fmod(updateRect.minP().x, major);
    top = updateRect.maxP().y - fmod(updateRect.maxP().y, major);
    for (double x = left; x < updateRect.maxP().x; x += major)
    {
        for (double y = top; y > updateRect.minP().y; y -= major)
        {
            points.emplace_back(DmVector(x, y));
        }
    }
    grid->setPoints(points);
    return gridSize;
}

void GuiDocumentView::hideSnapTooltip()
{
    if (m_snapTooltip)
        m_snapTooltip->hide();
}

void GuiDocumentView::setOverlayCorners(const DmVector& corner1, const DmVector& corner2)
{
    m_overlayCorner1 = corner1;
    m_overlayCorner2 = corner2;
    m_isDrawOverlayBox = true;
}

void GuiDocumentView::disableOverlayBox()
{
    m_isDrawOverlayBox = false;
}

DM::SnapRestriction GuiDocumentView::getSnapRestriction() const
{
    return defaultSnapRes;
}

SnapMode GuiDocumentView::getDefaultSnapMode() const
{
    return defaultSnapMode;
}

/// @brief 设置默认捕捉模式（用于新创建的命令）；UIView 另外同步给选择层与活动命令
void GuiDocumentView::setDefaultSnapMode(SnapMode sm)
{
    defaultSnapMode = sm;
}

/// @brief 设置捕捉限制（如正交）；UIView 另外同步给选择层与活动命令
void GuiDocumentView::setSnapRestriction(DM::SnapRestriction sr)
{
    defaultSnapRes = sr;
}

/// @brief 将实际坐标转为屏幕坐标
DmVector GuiDocumentView::toGui(DmVector v) const
{
    return DmVector(toGuiX(v.x), toGuiY(v.y));
}

/// @brief 将实际 X 坐标转为屏幕 X 坐标
double GuiDocumentView::toGuiX(double x) const
{
    // 与旧画笔的 user_to_device 相同：画布中心对着相机中心，y 向下
    return getWidth() / 2.0 + (x - m_viewCenter.x) / m_unitsPerPixel;
}

/// @brief 将实际 Y 坐标转为屏幕 Y 坐标
double GuiDocumentView::toGuiY(double y) const
{
    return getHeight() / 2.0 - (y - m_viewCenter.y) / m_unitsPerPixel;
}

/// @brief 将实际距离转为屏幕距离
double GuiDocumentView::toGuiDX(double d) const
{
    return toGuiX(d) - toGuiX(0);
}

double GuiDocumentView::toGuiDY(double d) const
{
    return std::fabs(toGuiY(d) - toGuiY(0));
}

/// @brief 将屏幕坐标转换为实际坐标
DmVector GuiDocumentView::toGraph(DmVector v) const
{
    return toGraph(Math2d::round(v.x), Math2d::round(v.y));
}

/// @brief 将屏幕坐标转换为实际坐标
DmVector GuiDocumentView::toGraph(int x, int y) const
{
    return DmVector(toGraphX(x), toGraphY(y));
}

/// @brief 将屏幕坐标 X 转换为实际坐标 X
double GuiDocumentView::toGraphX(int x) const
{
    return m_viewCenter.x + (x - getWidth() / 2.0) * m_unitsPerPixel;
}

/// @brief 将屏幕坐标 Y 转换为实际坐标 Y
double GuiDocumentView::toGraphY(int y) const
{
    return m_viewCenter.y + (getHeight() / 2.0 - y) * m_unitsPerPixel;
}

/// @brief 将屏幕坐标距离 X 转换为实际坐标距离 X
double GuiDocumentView::toGraphDX(int d) const
{
    return toGraphX(d) - toGraphX(0);
}

/// @brief 将屏幕坐标距离 Y 转换为实际坐标距离 Y
double GuiDocumentView::toGraphDY(int d) const
{
    return toGraphY(d) - toGraphY(0);
}

/// @brief 设置相对零点坐标（如果未锁定），不删除/重绘点
void GuiDocumentView::setRelativeZero(const DmVector& pos)
{
    if (relativeZeroLocked == false)
    {
        relativeZero = pos;
        orthogonalZero = pos;
        emit relative_zero_changed(pos);
    }
}

void GuiDocumentView::setOrthogonalZero(const DmVector& pos)
{
    orthogonalZero = pos;
}

DmVector const& GuiDocumentView::getOrthogonalZero() const
{
    return orthogonalZero;
}

/// @brief 设置相对零点坐标，删除旧位置并在屏幕上绘制新位置
void GuiDocumentView::moveRelativeZero(const DmVector& pos)
{
    setRelativeZero(pos);
    redraw();
}

void GuiDocumentView::hideRelativeZero(const bool isHide)
{
    relativeZero.valid = isHide;
}

DmEntityContainer* GuiDocumentView::getPreviewContainer()
{
    return m_pPreviewEntityContainer;
}

void GuiDocumentView::specifyPreviewModified()
{
    m_gsPreview->invalidate();
}

void GuiDocumentView::specifySelectChanged()
{
    if (m_gsModel)
    {
        m_gsModel->selectionChanged();
    }
    m_gsGripsDirty = true;
}

void GuiDocumentView::specifyHighlightChanged()
{
    m_gsView->setHighlighted(m_pDocumentHighlight ? m_pDocumentHighlight->highlightedEntities()
                                                  : std::vector<DmEntity*>());
}

void GuiDocumentView::setHiddenSource(const IHiddenSource* source)
{
    m_pHiddenSource = source;
    specifyHiddenChanged();
}

void GuiDocumentView::specifyHiddenChanged()
{
    m_gsView->setHidden(m_pHiddenSource ? m_pHiddenSource->hiddenEntities() : std::vector<DmEntity*>());
    redraw();
}

void GuiDocumentView::setGraphicsModel(std::shared_ptr<GsModel> model)
{
    m_gsModel = std::move(model);
    if (m_gsModel && m_pDocumentSelection)
    {
        m_gsModel->setSelectionSource(m_pDocumentSelection);
    }
    m_gsView->setModel(m_gsModel, true);
}

std::shared_ptr<GsModel> GuiDocumentView::graphicsModel() const
{
    if (!m_gsModel && pDocument)
    {
        // 没有注入（测试、独立的画布）：为自己的文档建一个
        const_cast<GuiDocumentView*>(this)->setGraphicsModel(std::make_shared<GsModel>(*pDocument));
    }
    return m_gsModel;
}

void GuiDocumentView::setPreviewTransform(const GiTransform& transform)
{
    m_gsPreview->setRootTransform(transform);
}

void GuiDocumentView::documentModified()
{
    // 几何由图形模型按变更集更新；选中的实体可能动了，夹点跟着重取
    m_gsGripsDirty = true;
}

void GuiDocumentView::redrawRequested()
{
    redraw();
}

void GuiDocumentView::paintContainerChanged(DmEntityContainer*)
{
    // 图形模型自己是文档的监听者，块编辑时换根（GsModel::paintContainerChanged）
    m_gsGripsDirty = true;
    redraw();
}

DmRect GuiDocumentView::getViewRect()
{
    return DmRect(toGraph(0, 0), toGraph(getWidth(), getHeight()));
}

void GuiDocumentView::setNextFrameCounter(yicad::TimerCounter& counter)
{
    m_pNextFrameCounter = &counter;
}

GuiGrid* GuiDocumentView::getGrid() const
{
    return grid.get();
}

SnapResultType GuiDocumentView::currentSnapResult()
{
    return SnapResultType::SnapNone;
}

DmVector GuiDocumentView::currentSnapSpot()
{
    return DmVector(false);
}

void GuiDocumentView::setBackground(const QColor& bg)
{
    background = bg;
}

/// @brief 设置鼠标光标类型
void GuiDocumentView::setMouseCursor(DM::CursorType c)
{
    switch (c)
    {
    default:
    case DM::ArrowCursor:
        setCursor(Qt::BlankCursor); // 屏蔽Qt默认光标
        m_eCursorType = DM::ArrowCursor;
        break;
    case DM::UpArrowCursor:
        setCursor(Qt::UpArrowCursor);
        m_eCursorType = DM::UpArrowCursor;
        break;
    case DM::CrossCursor:
        setCursor(Qt::CrossCursor);
        m_eCursorType = DM::CrossCursor;
        break;
    case DM::WaitCursor:
        setCursor(Qt::WaitCursor);
        m_eCursorType = DM::WaitCursor;
        break;
    case DM::IbeamCursor:
        setCursor(Qt::IBeamCursor);
        m_eCursorType = DM::IbeamCursor;
        break;
    case DM::SizeVerCursor:
        setCursor(Qt::SizeVerCursor);
        m_eCursorType = DM::SizeVerCursor;
        break;
    case DM::SizeHorCursor:
        setCursor(Qt::SizeHorCursor);
        m_eCursorType = DM::SizeHorCursor;
        break;
    case DM::SizeBDiagCursor:
        setCursor(Qt::SizeBDiagCursor);
        m_eCursorType = DM::SizeBDiagCursor;
        break;
    case DM::SizeFDiagCursor:
        setCursor(Qt::SizeFDiagCursor);
        m_eCursorType = DM::SizeFDiagCursor;
        break;
    case DM::SizeAllCursor:
        setCursor(Qt::SizeAllCursor);
        m_eCursorType = DM::SizeAllCursor;
        break;
    case DM::BlankCursor:
        setCursor(Qt::BlankCursor);
        m_eCursorType = DM::BlankCursor;
        break;
    case DM::SplitVCursor:
        setCursor(Qt::SplitVCursor);
        m_eCursorType = DM::SplitVCursor;
        break;
    case DM::SplitHCursor:
        setCursor(Qt::SplitHCursor);
        m_eCursorType = DM::SplitHCursor;
        break;
    case DM::PointingHandCursor: // 拖拽画布时的手爪状态
        setCursor(Qt::PointingHandCursor);
        m_eCursorType = DM::PointingHandCursor;
        break;
    case DM::ForbiddenCursor:
        setCursor(Qt::ForbiddenCursor);
        m_eCursorType = DM::ForbiddenCursor;
        break;
    case DM::WhatsThisCursor:
        setCursor(Qt::WhatsThisCursor);
        m_eCursorType = DM::WhatsThisCursor;
        break;
    case DM::OpenHandCursor:
        setCursor(Qt::OpenHandCursor);
        m_eCursorType = DM::OpenHandCursor;
        break;
    case DM::ClosedHandCursor:
        setCursor(Qt::ClosedHandCursor);
        m_eCursorType = DM::ClosedHandCursor;
        break;
    case DM::CadCursor: // 绘制交互时 拾取坐标点状态
        setCursor(Qt::BlankCursor);
        m_eCursorType = DM::CadCursor;
        break;
    case DM::DelCursor:
        setCursor(Qt::BlankCursor); // 删除实体
        m_eCursorType = DM::DelCursor;
        break;
    case DM::SelectCursor:  // 选取实体状态
        setCursor(*m_selEntityCurcorStyle);
        m_eCursorType = DM::SelectCursor;
        break;
    case DM::MagnifierCursor:   // 放大镜
        setCursor(Qt::BlankCursor);
        m_eCursorType = DM::MagnifierCursor;
        break;
    case DM::MovingHandCursor:
        setCursor(Qt::BlankCursor);
        m_eCursorType = DM::MovingHandCursor;
        break;
    }
}

void GuiDocumentView::setCursor(const QCursor& cursor)
{
    QWidget::setCursor(cursor);
}

QObject* GuiDocumentView::asQObject()
{
    return this;
}

/// @brief 获取鼠标在文档中的位置
DmVector GuiDocumentView::getMousePosition() const
{
    // 获取鼠标位置
    QPoint vp = mapFromGlobal(QCursor::pos());
    // 如果鼠标不在widget上，返回widget中心位置
    if (!rect().contains(vp))
    {
        vp = QPoint(width() / 2, height() / 2);
    }
    return toGraph(vp.x(), vp.y());
}

void GuiDocumentView::setGridColor(const QColor& c)
{
    gridColor = c;
}

void GuiDocumentView::setMetaGridColor(const QColor& c)
{
    metaGridColor = c;
}

void GuiDocumentView::setSelectedColor(const QColor& c)
{
    selectedColor = c;
}

void GuiDocumentView::setHighlightColor(const QColor& c)
{
    // 高亮画在叠加通道，场景底图不作废（GsView 比对显示设置时只在高亮走状态位图时看高亮色）
    highlightColor = c;
}

void GuiDocumentView::setDocumentSelectionSource(const ISelectionSource* source)
{
    m_pDocumentSelection = source;
    m_gsGripsDirty = true;
    if (m_gsModel && source)
    {
        // 文档模型由同一文档的视图共用，选择集也是文档的：只设不清（画布释放时不把共用模型的来源置空）
        m_gsModel->setSelectionSource(source);
    }
}

void GuiDocumentView::setDocumentHighlightSource(const IHighlightSource* source)
{
    m_pDocumentHighlight = source;
    m_gsView->setHighlighted(source ? source->highlightedEntities() : std::vector<DmEntity*>());
}

DmDocument* GuiDocumentView::getDocument() const
{
    return pDocument;
}

DmVector GuiDocumentView::getFactor() const
{
    return DmVector(m_unitsPerPixel, m_unitsPerPixel);
}

void GuiDocumentView::lockRelativeZero(bool lock)
{
    relativeZeroLocked = lock;
}

bool GuiDocumentView::isRelativeZeroLocked() const
{
    return relativeZeroLocked;
}

DmVector const& GuiDocumentView::getRelativeZero() const
{
    return relativeZero;
}

bool GuiDocumentView::isDraftMode() const
{
    return draftMode;
}

void GuiDocumentView::setDraftMode(bool dm)
{
    draftMode = dm;
}

bool GuiDocumentView::isCleanUp(void) const
{
    return m_bIsCleanUp;
}

void GuiDocumentView::initializeGL()
{
    // 图形系统在第一帧取设备（GsDevice::acquire）；上下文重建时视图的目标与缓冲照常可用（资源在共享组里）
}

void GuiDocumentView::paintGL()
{
    // 帧耗时埋点。默认关闭，开启方式见 ScopedTimer.h；
    // 此前这里是每帧一次 std::cout，既污染帧耗时又用 system_clock 测时长（P11）。
    YICAD_SCOPED_TIMER(yicad::counters::paintGL());
    // 某个变化之后的首帧另记一份，见 setNextFrameCounter()
    std::optional<yicad::ScopedTimer> nextFrameTimer;
    yicad::TimerCounter* nextFrameCounter = std::exchange(m_pNextFrameCounter, nullptr);
    if (nextFrameCounter && yicad::Profiler::isEnabled())
    {
        nextFrameTimer.emplace(*nextFrameCounter);
    }
    RhiFrameStats::beginFrame();

    graphicsModel();  // 没有注入时为自己的文档建一个

    GsViewStyle style;
    style.background = background;
    style.selected = selectedColor;
    style.highlight = highlightColor;
    style.grid = gridColor;
    style.metaGrid = metaGridColor;
    style.lineWidths = draftMode;
    style.gridOn = isGridOn();
    style.gridSpacing = style.gridOn ? updateGrid() : 0.0;
    m_gsView->setStyle(style);
    m_gsView->setCamera(m_viewCenter, m_unitsPerPixel);
    if (m_gsGripsDirty)
    {
        m_gsView->setGrips(collectGrips());
        m_gsGripsDirty = false;
    }
    fillOverlay();
    m_gsView->render(*m_gsSurface, devicePixelRatioF());

    RhiFrameStats::endFrame();
}

void GuiDocumentView::resizeGL(int, int)
{
    // 场景底图按表面尺寸由 GsView 重建
}

void GuiDocumentView::mouseMoveEvent(QMouseEvent* e)
{
    m_currentMousePt = toGraph(DmVector(e->pos().x(), e->pos().y()));
    e->accept();
}

void GuiDocumentView::updateSnapTooltip(const QPoint& pos)
{
    SnapResultType snapResult = currentSnapResult();
    if (snapResult != SnapResultType::SnapNone)
    {
        QString text;
        switch (snapResult)
        {
        case SnapResultType::SnapEndpoint:    text = tr("Endpoint"); break;
        case SnapResultType::SnapCenter:      text = tr("Center"); break;
        case SnapResultType::SnapMiddle:      text = tr("Middle"); break;
        case SnapResultType::SnapIntersection: text = tr("Intersection"); break;
        case SnapResultType::SnapOnEntity:    text = tr("On Entity"); break;
        case SnapResultType::SnapSubsection:  text = tr("Subsection"); break;
        case SnapResultType::SnapGrid:        text = tr("Grid"); break;
        default: break;
        }
        if (!text.isEmpty())
        {
            m_snapTooltip->setText(text);
            m_snapTooltip->adjustSize();
            QPoint globalPos = mapToGlobal(pos + QPoint(15, 15));
            m_snapTooltip->move(globalPos);
            m_snapTooltip->show();
            m_snapTooltipTimer->start(2000);
        }
    }
    else
    {
        m_snapTooltip->hide();
        m_snapTooltipTimer->stop();
    }
}

void GuiDocumentView::focusOutEvent(QFocusEvent* e)
{
    QWidget::focusOutEvent(e);
}

void GuiDocumentView::setStrDevice(const QString& strDevice)
{
    m_strDevice = strDevice;
}

const QString& GuiDocumentView::getStrDevice() const
{
    return m_strDevice;
}

void GuiDocumentView::setIsDrawCursor(const bool& isDrawCursor)
{
    m_isDrawCursor = isDrawCursor;
}

// ---------------------------------------------------------------------------
// 叠加层与夹点
// ---------------------------------------------------------------------------

std::vector<DmVector> GuiDocumentView::collectGrips() const
{
    // 与原先旧渲染器的 DmCachePainter::cacheSelectedPoints 相同：参考点超过 100 个就一个也不画
    constexpr std::size_t kMaxSelectedPoints = 100;
    std::vector<DmVector> grips;
    // 每个实体至少一个参考点：选中的实体多于上限时不必取出它们（全选几十万个实体时这一步原先要上百毫秒）
    if (!m_pDocumentSelection || m_pDocumentSelection->hasMoreThan(kMaxSelectedPoints))
    {
        return grips;
    }
    for (DmEntity* e : m_pDocumentSelection->selectedEntities())
    {
        for (const DmVector& pt : e->getRefPoints())
        {
            grips.push_back(pt);
        }
        if (grips.size() > kMaxSelectedPoints)
        {
            return {};
        }
    }
    return grips;
}

void GuiDocumentView::fillOverlay()
{
    GsOverlay& overlay = m_gsView->overlay();
    overlay.clear();

    // 原点标记（同 drawAbsoluteZero）：长度单位 zr 为 20 像素，世界坐标的 y 向上即像素的 y 向下
    {
        const DmVector o = toGui(DmVector(0.0, 0.0));
        constexpr double zr = 20.0;
        auto seg = [&](double x0, double y0, double x1, double y1, const QColor& c) {
            overlay.line(o.x + x0 * zr, o.y - y0 * zr, o.x + x1 * zr, o.y - y1 * zr, 1.0, c);
        };
        const QColor blue(0, 0, 255);
        const QColor red(255, 0, 0);
        const QColor green(0, 255, 0);
        seg(-0.2, 0.2, 0.2, 0.2, blue);
        seg(0.2, 0.2, 0.2, -0.2, blue);
        seg(0.2, -0.2, -0.2, -0.2, blue);
        seg(-0.2, -0.2, -0.2, 0.2, blue);
        seg(0.0, 0.0, 2.0, 0.0, red);
        seg(0.0, 0.0, 0.0, 2.0, green);
        seg(-0.15, 2.8, 0.0, 2.5, green);
        seg(0.15, 2.8, 0.0, 2.5, green);
        seg(0.0, 2.5, 0.0, 2.2, green);
        seg(2.2, 0.3, 2.5, -0.3, red);
        seg(2.2, -0.3, 2.5, 0.3, red);
    }

    // 选择框（同 drawOverlayBox）：从右往左为交叉选，绿色、虚线边；从左往右为窗选，蓝色、实线边
    if (m_isDrawOverlayBox)
    {
        const DmVector a = toGui(m_overlayCorner1);
        const DmVector b = toGui(m_overlayCorner2);
        const double x0 = std::min(a.x, b.x);
        const double x1 = std::max(a.x, b.x);
        const double y0 = std::min(a.y, b.y);
        const double y1 = std::max(a.y, b.y);
        const bool crossing = m_overlayCorner1.x > m_overlayCorner2.x;
        overlay.fillRect(x0, y0, x1, y1, crossing ? QColor::fromRgbF(0.1f, 0.45f, 0.2f, 0.6f)
                                                  : QColor::fromRgbF(0.1f, 0.22f, 0.55f, 0.7f));
        const QColor white(255, 255, 255);
        overlay.line(x0, y0, x0, y1, 1.0, white, crossing);
        overlay.line(x0, y1, x1, y1, 1.0, white, crossing);
        overlay.line(x1, y1, x1, y0, 1.0, white, crossing);
        overlay.line(x1, y0, x0, y0, 1.0, white, crossing);
    }

    // 光标（同 drawCursor）：十字线 60 像素，空闲时中心加一个方块
    if (m_isDrawCursor && (m_eCursorType == DM::CadCursor || m_eCursorType == DM::ArrowCursor))
    {
        const DmVector p = toGui(m_currentMousePt);
        constexpr double zr = 60.0;
        constexpr double tr = zr * 0.08;
        const QColor white(255, 255, 255);
        overlay.line(p.x - zr, p.y, p.x + zr, p.y, 1.0, white);
        overlay.line(p.x, p.y - zr, p.x, p.y + zr, 1.0, white);
        if (m_eCursorType == DM::ArrowCursor)
        {
            overlay.line(p.x - tr, p.y - tr, p.x + tr, p.y - tr, 1.0, white);
            overlay.line(p.x + tr, p.y - tr, p.x + tr, p.y + tr, 1.0, white);
            overlay.line(p.x + tr, p.y + tr, p.x - tr, p.y + tr, 1.0, white);
            overlay.line(p.x - tr, p.y + tr, p.x - tr, p.y - tr, 1.0, white);
        }
    }

    // 捕捉标记（同 drawSnapIndicator）：半边长 12 像素，金色，线宽 1.5
    const SnapResultType snapResult = currentSnapResult();
    const DmVector snapSpot = snapResult == SnapResultType::SnapNone ? DmVector(false) : currentSnapSpot();
    if (snapSpot.valid)
    {
        const DmVector c = toGui(snapSpot);
        constexpr double hs = 12.0;
        const QColor gold = QColor::fromRgbF(1.0f, 0.85f, 0.0f, 1.0f);
        auto seg = [&](double x0, double y0, double x1, double y1) {
            overlay.line(c.x + x0, c.y - y0, c.x + x1, c.y - y1, 1.5, gold);
        };
        switch (snapResult)
        {
        case SnapResultType::SnapEndpoint:
            seg(-hs, hs, hs, hs);
            seg(hs, hs, hs, -hs);
            seg(hs, -hs, -hs, -hs);
            seg(-hs, -hs, -hs, hs);
            break;
        case SnapResultType::SnapCenter:
        {
            constexpr int kSegments = 12;
            for (int i = 0; i < kSegments; ++i)
            {
                const double a1 = 2.0 * M_PI * i / kSegments;
                const double a2 = 2.0 * M_PI * (i + 1) / kSegments;
                seg(hs * std::cos(a1), hs * std::sin(a1), hs * std::cos(a2), hs * std::sin(a2));
            }
            break;
        }
        case SnapResultType::SnapMiddle:
        {
            const double h = hs * 1.2;
            const double w = hs * 1.2;
            seg(0.0, h, -w, -h * 0.6);
            seg(-w, -h * 0.6, w, -h * 0.6);
            seg(w, -h * 0.6, 0.0, h);
            break;
        }
        case SnapResultType::SnapIntersection:
        {
            const double d = hs * 1.2;
            seg(-d, -d, d, d);
            seg(d, -d, -d, d);
            break;
        }
        case SnapResultType::SnapOnEntity:
        case SnapResultType::SnapSubsection:
            seg(0.0, hs, hs * 0.7, 0.0);
            seg(hs * 0.7, 0.0, 0.0, -hs);
            seg(0.0, -hs, -hs * 0.7, 0.0);
            seg(-hs * 0.7, 0.0, 0.0, hs);
            break;
        case SnapResultType::SnapGrid:
            seg(0.0, -hs, 0.0, hs);
            seg(-hs, 0.0, hs, 0.0);
            break;
        default:
            break;
        }
    }
}

