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

#include<climits>
#include<cmath>
#include <iostream>
#include <unordered_map>

#include <QApplication>
#include <QDesktopWidget>
#include <QAction>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QtAlgorithms>
#include <QNativeGestureEvent>
#include <QTimer>
#include <QLabel>
#include <QToolButton>

#include "DmLine.h"
#include "DmCachePainter.h"
#include "DmEntityContainer.h"
#include "GuiEventHandler.h"
#include "DmDocument.h"
#include "GuiGrid.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmBlockReference.h"
#include "DmSettings.h"
#include "GuiDialogFactory.h"
#include "DmLayer.h"
#include "Math2d.h"
#include "Debug.h"
#include "DmColor.h"
#include "ActionZoomIn.h"
#include "ActionZoomPan.h"
#include "ActionModifyDelete.h"
#include "ActionSelectSingle.h"
#include "ActionDefault.h"

#ifdef Q_OS_WIN32
#define CURSOR_SIZE 16
#else
#define CURSOR_SIZE 15
#endif

#include <set>
#include "QString"

GuiDocumentView::GuiDocumentView(QWidget* parent, Qt::WindowFlags f, DmDocument* doc)
    : QOpenGLWidget(parent, f)
    , pDocument(nullptr)
    , eventHandler(new GuiEventHandler(this))
    , background(30, 30, 30, 255)
    , foreground(30, 30, 30, 255)
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
    , printPreview(false)
    , printing(false)
    , m_overlayEntities(QMap<int, DmEntityContainer*>())
    , m_pPreviewEntityContainer(new DmEntityContainer())
    , m_bIsCleanUp(false)
    , m_pBackgroundPainter(nullptr)
    , m_pDocumentPainter(nullptr)
    , m_pForegroundPainter(nullptr)
    , m_pPreviewPainter(nullptr)
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
        doc->setDocumentView(this);
        setDefaultAction(new ActionDefault(doc, this));
    }

    DMSETTINGS->beginGroup("Colors");
    setBackground(QColor(DMSETTINGS->readEntry("/background", Colors::BACKGROUND)));
    setGridColor(QColor(DMSETTINGS->readEntry("/grid", Colors::GRID)));
    setMetaGridColor(QColor(DMSETTINGS->readEntry("/meta_grid", Colors::META_GRID)));
    setSelectedColor(QColor(DMSETTINGS->readEntry("/select", Colors::SELECT)));
    setHighlightColor(QColor(DMSETTINGS->readEntry("/highlight", Colors::HIGHLIGHT)));
    DMSETTINGS->endGroup();

    const int MULTISAMPLE_COUNT = 4;
    QSurfaceFormat format;
    format.setSamples(MULTISAMPLE_COUNT);    // 设置多重采样的采样点数
    setFormat(format);
    glEnable(GL_MULTISAMPLE);

    // 捕捉类型文字提示
    m_snapTooltip = new QLabel(this);
    m_snapTooltip->setWindowFlags(Qt::ToolTip);
    m_snapTooltip->setStyleSheet("QLabel { background-color: #333333; color: #FFD700; border: 1px solid #888888; padding: 4px 10px; font-size: 16px; font-weight: bold; }");
    m_snapTooltip->hide();

    m_snapTooltipTimer = new QTimer(this);
    m_snapTooltipTimer->setSingleShot(true);
    connect(m_snapTooltipTimer, &QTimer::timeout, this, &GuiDocumentView::hideSnapTooltip);
}

GuiDocumentView::~GuiDocumentView()
{
    cleanUp();
    qDeleteAll(m_overlayEntities);
    deletePainters();

    if (eventHandler)
    {
        delete eventHandler;
        eventHandler = nullptr;
    }
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

/// @brief 设置文档对象
void GuiDocumentView::setDocument(DmDocument* pDoc)
{
    this->pDocument = pDoc;
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

/// @brief 获取默认操作
/// @return 当前操作或 nullptr
ActionInterface* GuiDocumentView::getDefaultAction()
{
    if (eventHandler)
    {
        return eventHandler->getDefaultAction();
    }
    else
    {
        return nullptr;
    }
}

/// @brief 设置事件处理器的默认操作
void GuiDocumentView::setDefaultAction(ActionInterface* action)
{
    if (eventHandler)
    {
        eventHandler->setDefaultAction(action);
    }
}

/// @brief 获取当前操作
/// @return 当前操作或 nullptr
ActionInterface* GuiDocumentView::getCurrentAction()
{
    if (eventHandler)
    {
        return eventHandler->getCurrentAction();
    }
    else
    {
        return nullptr;
    }
}

/// @brief 设置事件处理器的当前操作
void GuiDocumentView::setCurrentAction(ActionInterface* action)
{
    if (eventHandler)
    {
        eventHandler->setCurrentAction(action);
    }
}

/// @brief 终止选择类操作
void GuiDocumentView::killSelectActions()
{
    if (eventHandler)
    {
        eventHandler->killSelectActions();
    }
}

/// @brief 终止所有操作
void GuiDocumentView::killAllActions()
{
    if (eventHandler)
    {
        eventHandler->killAllActions();
    }
}

/// @brief 发出选择变更信号
void GuiDocumentView::emitSelectedChanged()
{
    specifyDocumentModified();
    emit selectedChanged();
}

/// @brief 在菜单或当前操作中后退
void GuiDocumentView::back()
{
    if (eventHandler && eventHandler->hasAction())
    {
        eventHandler->back();
    }
}

/// @brief 前进/确认当前操作
void GuiDocumentView::enter()
{
    if (eventHandler && eventHandler->hasAction())
    {
        eventHandler->enter();
    }
}

/// @brief 处理命令事件（由命令行 UI 调用）
void GuiDocumentView::commandEvent(GuiCommandEvent* e)
{
    if (eventHandler)
    {
        eventHandler->commandEvent(e);
    }
}

/// @brief 启用命令行坐标输入
void GuiDocumentView::enableCoordinateInput()
{
    if (eventHandler)
    {
        eventHandler->enableCoordinateInput();
    }
}

/// @brief 禁用命令行坐标输入
void GuiDocumentView::disableCoordinateInput()
{
    if (eventHandler)
    {
        eventHandler->disableCoordinateInput();
    }
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

    DmVector c = center;
    if (!c.valid)
    {
        c = getMousePosition();
    }
    m_pBackgroundPainter->scale(f, center.x, center.y);
    m_pPreviewPainter->scale(f, center.x, center.y);
    m_pDocumentPainter->scale(f, center.x, center.y);
    m_pForegroundPainter->scale(f, center.x, center.y);
    redraw();
    emit viewChanged();
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

        if (m_pBackgroundPainter)
        {
            m_pBackgroundPainter->setViewPosition(center.x, center.y);
            m_pBackgroundPainter->setScale(fx);
        }
        if (m_pDocumentPainter)
        {
            m_pDocumentPainter->setViewPosition(center.x, center.y);
            m_pDocumentPainter->setScale(fx);
        }
        if (m_pPreviewPainter)
        {
            m_pPreviewPainter->setViewPosition(center.x, center.y);
            m_pPreviewPainter->setScale(fx);
        }
        if (m_pForegroundPainter)
        {
            m_pForegroundPainter->setViewPosition(center.x, center.y);
            m_pForegroundPainter->setScale(fx);
        }

        redraw();
    }
    emit viewChanged();
}

/// @brief 平移视图
/// @param dx X 方向像素偏移
/// @param dy Y 方向像素偏移
void GuiDocumentView::zoomPan(int dx, int dy)
{
    double dx_world = toGraphDX(dx);
    double dy_world = toGraphDY(dy);
    m_pBackgroundPainter->translateView(dx_world, dy_world);
    m_pPreviewPainter->translateView(dx_world, dy_world);
    m_pDocumentPainter->translateView(dx_world, dy_world);
    m_pForegroundPainter->translateView(dx_world, dy_world);
    redraw();
    emit viewChanged();
}

void GuiDocumentView::drawBackgroundLayer()
{
    // 绘制背景色
    double backgroundColor_r = background.red() / 255.;
    double backgroundColor_g = background.green() / 255.;
    double backgroundColor_b = background.blue() / 255.;
    m_pBackgroundPainter->source_rgb(backgroundColor_r, backgroundColor_g, backgroundColor_b);
    double min_x = 0;
    double min_y = 0;
    double min_x_user = 0;
    double min_y_user = 0;
    m_pBackgroundPainter->device_to_user(min_x, min_y, &min_x_user, &min_y_user);
    double max_x = getWidth();
    double max_y = getHeight();
    double max_x_user = 0;
    double max_y_user = 0;
    m_pBackgroundPainter->device_to_user(max_x, max_y, &max_x_user, &max_y_user);
    m_pBackgroundPainter->rectangle(min_x_user, min_y_user, max_x_user - min_x_user, min_y_user - max_y_user);
    m_pBackgroundPainter->setFill(true);
    m_pBackgroundPainter->stroke();

    // 打印预览
    if (isPrintPreview())
    {
        // drawPaper(m_pBackgroundPainter); // TODO : 打印功能暂时未实现
    }

    if (!isPrintPreview())
    {
        // 背景栅格
        drawGridLine();
    }
}

void GuiDocumentView::drawDocumentLayer()
{
    m_pDocumentPainter->draw();
}

void GuiDocumentView::drawPreviewLayer()
{
    m_pPreviewPainter->draw();
}

void GuiDocumentView::drawForegroundLayer()
{
    // 原点坐标显示
    drawAbsoluteZero();
    // 选择框
    drawOverlayBox();
    // 光标
    drawCursor();
    // 捕捉点标识
    drawSnapIndicator();
}

void GuiDocumentView::drawAbsoluteZero()
{
    const double zr = toGraphDX(20);
    auto vp = DmVector(0, 0);

    m_pForegroundPainter->lineWidth(1.0);

    // 十字矩形
    m_pForegroundPainter->source_rgba(0., 0., 1., 1.);
    m_pForegroundPainter->move_to(vp.x - zr * 0.2, vp.y + zr * 0.2);
    m_pForegroundPainter->line_to(vp.x + zr * 0.2, vp.y + zr * 0.2);
    m_pForegroundPainter->move_to(vp.x + zr * 0.2, vp.y + zr * 0.2);
    m_pForegroundPainter->line_to(vp.x + zr * 0.2, vp.y - zr * 0.2);
    m_pForegroundPainter->move_to(vp.x + zr * 0.2, vp.y - zr * 0.2);
    m_pForegroundPainter->line_to(vp.x - zr * 0.2, vp.y - zr * 0.2);
    m_pForegroundPainter->move_to(vp.x - zr * 0.2, vp.y - zr * 0.2);
    m_pForegroundPainter->line_to(vp.x - zr * 0.2, vp.y + zr * 0.2);
    m_pForegroundPainter->stroke();

    // 十字横线
    m_pForegroundPainter->source_rgba(1., 0., 0., 1.);
    m_pForegroundPainter->move_to(vp.x, vp.y);
    m_pForegroundPainter->line_to(vp.x + 2. * zr, vp.y);
    m_pForegroundPainter->stroke();

    // 十字纵线
    m_pForegroundPainter->source_rgba(0., 1., 0., 1.);
    m_pForegroundPainter->move_to(vp.x, vp.y);
    m_pForegroundPainter->line_to(vp.x, vp.y + 2. * zr);
    m_pForegroundPainter->stroke();

    // y轴指示文字
    m_pForegroundPainter->source_rgb(0., 1., 0.);
    m_pForegroundPainter->move_to(vp.x - zr * 0.15, vp.y + 2.8 * zr);
    m_pForegroundPainter->line_to(vp.x, vp.y + 2.5 * zr);
    m_pForegroundPainter->stroke();
    m_pForegroundPainter->move_to(vp.x + zr * 0.15, vp.y + 2.8 * zr);
    m_pForegroundPainter->line_to(vp.x, vp.y + 2.5 * zr);
    m_pForegroundPainter->stroke();
    m_pForegroundPainter->move_to(vp.x, vp.y + 2.5 * zr);
    m_pForegroundPainter->line_to(vp.x, vp.y + 2.2 * zr);
    m_pForegroundPainter->stroke();

    // x轴指示文字
    m_pForegroundPainter->source_rgb(1., 0., 0.);
    m_pForegroundPainter->move_to(vp.x + zr * 2.2, vp.y + 0.3 * zr);
    m_pForegroundPainter->line_to(vp.x + zr * 2.5, vp.y - 0.3 * zr);
    m_pForegroundPainter->stroke();
    m_pForegroundPainter->move_to(vp.x + zr * 2.2, vp.y - 0.3 * zr);
    m_pForegroundPainter->line_to(vp.x + zr * 2.5, vp.y + 0.3 * zr);
    m_pForegroundPainter->stroke();
}

void GuiDocumentView::drawGridLine()
{
    const double MIN_GRID_SPACING_INIT = 20.;  // 最小网格间距
    const double NUM_MINOR_LINES = 5.;           // 最小格子数
    const double MIN_DISTANCE_LOWER = 10.0;      // 距离下限
    const double MIN_DISTANCE_UPPER = 100.0;     // 距离上限
    const double SCALE_FACTOR = 10.0;            // 缩放调整因子
    const double THRESHOLD_10 = 10.0;
    const double THRESHOLD_20 = 20.0;
    const double THRESHOLD_50 = 50.0;

    if (!grid || !isGridOn())
    {
        return;
    }

    std::vector<DmVector> points; // 记录栅格交点

    const geo::Area updateRect = { {toGraph(DmVector(0,0))},{toGraph(DmVector(getWidth(), getHeight()))} };

    m_pBackgroundPainter->lineWidth(1.0);
    m_pBackgroundPainter->setFill(false);

    DmVector zeroCorner = toGraph(DmVector(0., 0.));
    double iMinimumGridSpacing = MIN_GRID_SPACING_INIT;
    DmVector gridSPacing = toGraph(DmVector(iMinimumGridSpacing, iMinimumGridSpacing));

    // 距离始终在10到100之间
    double minDistancePoints = gridSPacing.x - zeroCorner.x;
    double factor = 1.0;

    while (minDistancePoints < MIN_DISTANCE_LOWER)
    {
        minDistancePoints *= SCALE_FACTOR;
        factor = factor * SCALE_FACTOR;
    }

    while (minDistancePoints > MIN_DISTANCE_UPPER)
    {
        minDistancePoints = minDistancePoints / SCALE_FACTOR;
        factor = factor / SCALE_FACTOR;
    }

    // 栅格距离
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

    // 绘制网格主线
    double majorColor_r = gridColor.red() / 255.;
    double majorColor_g = gridColor.green() / 255.;
    double majorColor_b = gridColor.blue() / 255.;
    double majorColor_a = gridColor.alpha() / 255.;
    m_pBackgroundPainter->source_rgba(majorColor_r, majorColor_g, majorColor_b, majorColor_a);

    double left = updateRect.minP().x - fmod(updateRect.minP().x, gridSize);
    double top = updateRect.maxP().y - fmod(updateRect.maxP().y, gridSize);
    grid->setBaseGrid(DmVector(left, updateRect.minP().y - fmod(updateRect.minP().y, gridSize)));

    for (double x = left; x < updateRect.maxP().x; x += gridSize)
    {
        m_pBackgroundPainter->move_to(x, updateRect.maxP().y);
        m_pBackgroundPainter->line_to(x, updateRect.minP().y);

        // 计算交点
        for (double y = top; y > updateRect.minP().y; y -= gridSize)
        {
            points.emplace_back(DmVector(x, y));
        }
    }

    for (double y = top; y > updateRect.minP().y; y -= gridSize)
    {
        m_pBackgroundPainter->move_to(updateRect.minP().x, y);
        m_pBackgroundPainter->line_to(updateRect.maxP().x, y);
    }
    m_pBackgroundPainter->stroke();

    // 绘制网格辅线
    double minorColor_r = metaGridColor.red() / 255.;
    double minorColor_g = metaGridColor.green() / 255.;
    double minorColor_b = metaGridColor.blue() / 255.;
    double minorColor_a = metaGridColor.alpha() / 255.;
    m_pBackgroundPainter->source_rgba(minorColor_r, minorColor_g, minorColor_b, minorColor_a);

    int iNumMinorLines = static_cast<int>(NUM_MINOR_LINES);
    gridSize *= iNumMinorLines;
    left = updateRect.minP().x - fmod(updateRect.minP().x, gridSize);
    top = updateRect.maxP().y - fmod(updateRect.maxP().y, gridSize);

    for (double x = left; x < updateRect.maxP().x; x += gridSize)
    {
        m_pBackgroundPainter->move_to(x, updateRect.maxP().y);
        m_pBackgroundPainter->line_to(x, updateRect.minP().y);

        // 计算交点
        for (double y = top; y > updateRect.minP().y; y -= gridSize)
        {
            points.emplace_back(DmVector(x, y));
        }
    }

    for (double y = top; y > updateRect.minP().y; y -= gridSize)
    {
        m_pBackgroundPainter->move_to(updateRect.minP().x, y);
        m_pBackgroundPainter->line_to(updateRect.maxP().x, y);
    }

    m_pBackgroundPainter->stroke();

    grid->setPoints(points);
}

void GuiDocumentView::drawCursor()
{
    const double CURSOR_LENGTH_FACTOR = 60;  // 光标线长像素系数
    const double CURSOR_BOX_RATIO = 0.08;     // 光标中心方块尺寸比例

    if (m_isDrawCursor)
    {
        const double zr = toGraphDX(CURSOR_LENGTH_FACTOR);
        const double tr = zr * CURSOR_BOX_RATIO;

        auto vp = DmVector(m_currentMousePt.x, m_currentMousePt.y);

        m_pForegroundPainter->lineWidth(1.0);
        m_pForegroundPainter->setFill(false);

        // 鼠标绘图交互选点状态
        if (m_eCursorType == DM::CadCursor)
        {
            // 不带中心方块的十字线

            // 十字横线
            m_pForegroundPainter->move_to(vp.x - zr, vp.y);
            m_pForegroundPainter->line_to(vp.x + zr, vp.y);
            m_pForegroundPainter->source_rgba(1., 1., 1., 1.);
            // 十字纵线
            m_pForegroundPainter->move_to(vp.x, vp.y - zr);
            m_pForegroundPainter->line_to(vp.x, vp.y + zr);
            m_pForegroundPainter->source_rgba(1., 1., 1., 1.);
        }
        // 鼠标空载状态
        else if (m_eCursorType == DM::ArrowCursor)
        {
            // 带中心方块的十字光标

            // 十字中心矩形
            m_pForegroundPainter->source_rgba(1., 1., 1., 1.);
            m_pForegroundPainter->move_to(vp.x - tr, vp.y + tr);
            m_pForegroundPainter->line_to(vp.x + tr, vp.y + tr);
            m_pForegroundPainter->move_to(vp.x + tr, vp.y + tr);
            m_pForegroundPainter->line_to(vp.x + tr, vp.y - tr);
            m_pForegroundPainter->move_to(vp.x + tr, vp.y - tr);
            m_pForegroundPainter->line_to(vp.x - tr, vp.y - tr);
            m_pForegroundPainter->move_to(vp.x - tr, vp.y - tr);
            m_pForegroundPainter->line_to(vp.x - tr, vp.y + tr);

            // 十字横线
            m_pForegroundPainter->move_to(vp.x - zr, vp.y);
            m_pForegroundPainter->line_to(vp.x + zr, vp.y);
            m_pForegroundPainter->source_rgba(1., 1., 1., 1.);
            // 十字纵线
            m_pForegroundPainter->move_to(vp.x, vp.y - zr);
            m_pForegroundPainter->line_to(vp.x, vp.y + zr);
            m_pForegroundPainter->source_rgba(1., 1., 1., 1.);
        }

        m_pForegroundPainter->stroke();
    }
}

void GuiDocumentView::drawSnapIndicator()
{
    ActionInterface* action = getCurrentAction();
    if (!action)
        return;

    SnapResultType snapResult = action->getSnapResult();
    if (snapResult == SnapResultType::SnapNone)
        return;

    DmVector snapSpot = action->getSnapSpot();
    if (!snapSpot.valid)
        return;

    const double HALF_SIZE = toGraphDX(12);  // half size ~12px (x2 from original ~6px)

    m_pForegroundPainter->lineWidth(1.5);
    m_pForegroundPainter->setFill(false);

    m_pForegroundPainter->source_rgba(1.0, 0.85, 0.0, 1.0);

    double x = snapSpot.x;
    double y = snapSpot.y;
    double hs = HALF_SIZE;

    switch (snapResult)
    {
    case SnapResultType::SnapEndpoint:
    {
        m_pForegroundPainter->move_to(x - hs, y + hs);
        m_pForegroundPainter->line_to(x + hs, y + hs);
        m_pForegroundPainter->move_to(x + hs, y + hs);
        m_pForegroundPainter->line_to(x + hs, y - hs);
        m_pForegroundPainter->move_to(x + hs, y - hs);
        m_pForegroundPainter->line_to(x - hs, y - hs);
        m_pForegroundPainter->move_to(x - hs, y - hs);
        m_pForegroundPainter->line_to(x - hs, y + hs);
        break;
    }
    case SnapResultType::SnapCenter:
    {
        const int SEGS = 12;
        for (int i = 0; i < SEGS; i++)
        {
            double angle1 = 2.0 * M_PI * i / SEGS;
            double angle2 = 2.0 * M_PI * (i + 1) / SEGS;
            m_pForegroundPainter->move_to(x + hs * cos(angle1), y + hs * sin(angle1));
            m_pForegroundPainter->line_to(x + hs * cos(angle2), y + hs * sin(angle2));
        }
        break;
    }
    case SnapResultType::SnapMiddle:
    {
        double h = hs * 1.2;
        double w = hs * 1.2;
        m_pForegroundPainter->move_to(x, y + h);
        m_pForegroundPainter->line_to(x - w, y - h * 0.6);
        m_pForegroundPainter->move_to(x - w, y - h * 0.6);
        m_pForegroundPainter->line_to(x + w, y - h * 0.6);
        m_pForegroundPainter->move_to(x + w, y - h * 0.6);
        m_pForegroundPainter->line_to(x, y + h);
        break;
    }
    case SnapResultType::SnapIntersection:
    {
        double d = hs * 1.2;
        m_pForegroundPainter->move_to(x - d, y - d);
        m_pForegroundPainter->line_to(x + d, y + d);
        m_pForegroundPainter->move_to(x + d, y - d);
        m_pForegroundPainter->line_to(x - d, y + d);
        break;
    }
    case SnapResultType::SnapOnEntity:
    case SnapResultType::SnapSubsection:
    {
        m_pForegroundPainter->move_to(x, y + hs);
        m_pForegroundPainter->line_to(x + hs * 0.7, y);
        m_pForegroundPainter->move_to(x + hs * 0.7, y);
        m_pForegroundPainter->line_to(x, y - hs);
        m_pForegroundPainter->move_to(x, y - hs);
        m_pForegroundPainter->line_to(x - hs * 0.7, y);
        m_pForegroundPainter->move_to(x - hs * 0.7, y);
        m_pForegroundPainter->line_to(x, y + hs);
        break;
    }
    case SnapResultType::SnapGrid:
    {
        m_pForegroundPainter->move_to(x, y - hs);
        m_pForegroundPainter->line_to(x, y + hs);
        m_pForegroundPainter->move_to(x - hs, y);
        m_pForegroundPainter->line_to(x + hs, y);
        break;
    }
    default:
        break;
    }

    m_pForegroundPainter->stroke();
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

void GuiDocumentView::drawOverlayBox()
{
    const double DASH_PATTERN[] = { 5.0, -5.0 };
    const int DASH_PATTERN_SIZE = 2;

    if (m_isDrawOverlayBox)
    {
        m_pForegroundPainter->lineWidth(1.0);
        DmVector v1 = m_overlayCorner1;
        DmVector v2 = m_overlayCorner2;
        DmVector lt(std::min(v2.x, v1.x), std::max(v2.y, v1.y));
        double w = std::abs(v2.x - v1.x);
        double h = std::abs(v2.y - v1.y);
        DmVector lb(lt.x, lt.y - h);
        DmVector rb(lt.x + w, lt.y - h);
        DmVector rt(lt.x + w, lt.y);
        if (v1.x > v2.x)
        {
            m_pForegroundPainter->source_rgba(.1, .45, .2, .6);
            m_pForegroundPainter->rectangle(lt.x, lt.y, w, h);
            m_pForegroundPainter->setFill(true);
            m_pForegroundPainter->stroke();

            m_pForegroundPainter->move_to(lt.x, lt.y);
            m_pForegroundPainter->line_to(lb.x, lb.y);
            m_pForegroundPainter->move_to(lb.x, lb.y);
            m_pForegroundPainter->line_to(rb.x, rb.y);
            m_pForegroundPainter->move_to(rb.x, rb.y);
            m_pForegroundPainter->line_to(rt.x, rt.y);
            m_pForegroundPainter->move_to(rt.x, rt.y);
            m_pForegroundPainter->line_to(lt.x, lt.y);
            m_pForegroundPainter->source_rgba(1.0, 1.0, 1.0, 1.0);
            m_pForegroundPainter->setDash(DASH_PATTERN, DASH_PATTERN_SIZE);
            m_pForegroundPainter->setFill(false);
            m_pForegroundPainter->stroke();
            m_pForegroundPainter->resetDash();
        }
        else
        {
            m_pForegroundPainter->source_rgba(.1, 0.22, 0.55, .7);
            m_pForegroundPainter->rectangle(lt.x, lt.y, w, h);
            m_pForegroundPainter->setFill(true);
            m_pForegroundPainter->stroke();
            m_pForegroundPainter->move_to(lt.x, lt.y);
            m_pForegroundPainter->line_to(lb.x, lb.y);
            m_pForegroundPainter->move_to(lb.x, lb.y);
            m_pForegroundPainter->line_to(rb.x, rb.y);
            m_pForegroundPainter->move_to(rb.x, rb.y);
            m_pForegroundPainter->line_to(rt.x, rt.y);
            m_pForegroundPainter->move_to(rt.x, rt.y);
            m_pForegroundPainter->line_to(lt.x, lt.y);
            m_pForegroundPainter->source_rgba(1.0, 1.0, 1.0, 1.0);
            m_pForegroundPainter->setFill(false);
            m_pForegroundPainter->stroke();
        }
    }
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

/// @brief 设置默认捕捉模式（用于新创建的操作）
void GuiDocumentView::setDefaultSnapMode(SnapMode sm)
{
    defaultSnapMode = sm;
    if (eventHandler)
    {
        eventHandler->setSnapMode(sm);
    }
}

/// @brief 设置捕捉限制（如正交）
void GuiDocumentView::setSnapRestriction(DM::SnapRestriction sr)
{
    defaultSnapRes = sr;
    if (eventHandler)
    {
        eventHandler->setSnapRestriction(sr);
    }
}

/// @brief 将实际坐标转为屏幕坐标
DmVector GuiDocumentView::toGui(DmVector v) const
{
    double guiX, guiY;
    m_pBackgroundPainter->user_to_device(v.x, v.y, &guiX, &guiY);
    return DmVector(guiX, guiY);
}

/// @brief 将实际 X 坐标转为屏幕 X 坐标
double GuiDocumentView::toGuiX(double x) const
{
    double guiX, guiY;
    m_pBackgroundPainter->user_to_device(x, 0.0, &guiX, &guiY);
    return guiX;
}

/// @brief 将实际 Y 坐标转为屏幕 Y 坐标
double GuiDocumentView::toGuiY(double y) const
{
    double guiX, guiY;
    m_pBackgroundPainter->user_to_device(0.0, y, &guiX, &guiY);
    return guiY;
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
    double userX, userY;
    m_pBackgroundPainter->device_to_user(x, y, &userX, &userY);
    return DmVector(userX, userY);
}

/// @brief 将屏幕坐标 X 转换为实际坐标 X
double GuiDocumentView::toGraphX(int x) const
{
    double userX, userY;
    m_pBackgroundPainter->device_to_user(x, 0.0, &userX, &userY);
    return userX;
}

/// @brief 将屏幕坐标 Y 转换为实际坐标 Y
double GuiDocumentView::toGraphY(int y) const
{
    double userX, userY;
    m_pBackgroundPainter->device_to_user(0.0, y, &userX, &userY);
    return userY;
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

/// @brief 获取指定的前景覆盖容器
DmEntityContainer* GuiDocumentView::getOverlayContainer(DM::OverlayDocument position)
{
    if (m_overlayEntities[position])
    {
        return m_overlayEntities[position];
    }
    m_overlayEntities[position] = new DmEntityContainer(nullptr);

    return m_overlayEntities[position];
}

DmEntityContainer* GuiDocumentView::getPreviewContainer()
{
    return m_pPreviewEntityContainer;
}

void GuiDocumentView::specifyPreviewModified()
{
    m_pPreviewPainter->specifyModified();
}

void GuiDocumentView::specifyDocumentModified()
{
    if (m_pDocumentPainter)
    {
        m_pDocumentPainter->specifyModified();
    }
}

void GuiDocumentView::specifySelectChanged()
{
    if (m_pDocumentPainter)
    {
        //m_pDocumentPainter->specifySelectChanged();
    }
}

void GuiDocumentView::setPreviewModelOffset(const DmVector& offset)
{
    m_pPreviewPainter->setModelOffset(offset);
}

void GuiDocumentView::setDocumentPainterContainer(DmEntityContainer* container)
{
    m_pDocumentPainter->clearContainers();
    m_pDocumentPainter->addContainer(container);
    m_pDocumentPainter->specifyModified();
    redraw();
}

DmRect GuiDocumentView::getViewRect()
{
    return DmRect(toGraph(0, 0), toGraph(getWidth(), getHeight()));
}

GuiGrid* GuiDocumentView::getGrid() const
{
    return grid.get();
}

GuiEventHandler* GuiDocumentView::getEventHandler() const
{
    return eventHandler;
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
    if (m_pDocumentPainter)
        m_pDocumentPainter->setSelectedColor(c);
    if (m_pPreviewPainter)
        m_pPreviewPainter->setSelectedColor(c);
}

void GuiDocumentView::setHighlightColor(const QColor& c)
{
    highlightColor = c;
    if (m_pDocumentPainter)
        m_pDocumentPainter->setHighlightColor(c);
    if (m_pPreviewPainter)
        m_pPreviewPainter->setHighlightColor(c);
}

DmDocument* GuiDocumentView::getDocument() const
{
    return pDocument;
}

DmVector GuiDocumentView::getFactor() const
{
    double s = m_pBackgroundPainter->getScale();
    return DmVector(s, s);
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

void GuiDocumentView::setPrintPreview(bool pv)
{
    printPreview = pv;
}

bool GuiDocumentView::isPrintPreview() const
{
    return printPreview;
}

void GuiDocumentView::setPrinting(bool p)
{
    printing = p;
}

bool GuiDocumentView::isPrinting() const
{
    return printing;
}

bool GuiDocumentView::isDraftMode() const
{
    return draftMode;
}

void GuiDocumentView::setDraftMode(bool dm)
{
    m_pDocumentPainter->setIsDisplayLineWidth(dm);
    m_pPreviewPainter->setIsDisplayLineWidth(dm);
    draftMode = dm;
}

bool GuiDocumentView::isCleanUp(void) const
{
    return m_bIsCleanUp;
}

void GuiDocumentView::initializeGL()
{
    QOpenGLWidget::makeCurrent();
    QOpenGLContext* CC = QOpenGLContext::currentContext();

    int width = size().width();
    int height = size().height();

    if (CC != 0)
    {
        createPainters(width, height);
        m_pBackgroundPainter->create_resources();
        m_pPreviewPainter->create_resources();
        m_pDocumentPainter->create_resources();
        m_pForegroundPainter->create_resources();
    }
    else
    {
        createPainters(width, height);
    }
}

void GuiDocumentView::paintGL()
{
    auto start = std::chrono::system_clock::now();
    // 绘制背景层 背景网格等
    drawBackgroundLayer();

    // 绘制doc层
    drawDocumentLayer();

    // 绘制预览层
    drawPreviewLayer();

    // 绘制前景层
    drawForegroundLayer();

    auto end = std::chrono::system_clock::now();
    auto gap = end - start;
    std::cout << "drawing cost: " << std::chrono::duration_cast<std::chrono::milliseconds>(gap).count() << std::endl;
}

void GuiDocumentView::resizeGL(int w, int h)
{
    m_pBackgroundPainter->new_device_size(w, h);
    m_pPreviewPainter->new_device_size(w, h);
    m_pDocumentPainter->new_device_size(w, h);
    m_pForegroundPainter->new_device_size(w, h);
}

void GuiDocumentView::mousePressEvent(QMouseEvent* e)
{
    // pan zoom with middle mouse button
    if (e->button() == Qt::MiddleButton)
    {
        setCurrentAction(new ActionZoomPan(pDocument, this));
    }
    eventHandler->mousePressEvent(e);
}

void GuiDocumentView::mouseDoubleClickEvent(QMouseEvent* e)
{
    switch (e->button())
    {
    case Qt::MiddleButton:
        zoomAuto();
        break;
    case Qt::LeftButton:
        eventHandler->mouseDoubleClickEvent(e);
        break;
    default:
        break;
    }
    e->accept();
}

void GuiDocumentView::mouseReleaseEvent(QMouseEvent* e)
{
    e->accept();

    switch (e->button())
    {
    case Qt::RightButton:

        if (eventHandler->hasAction())
        {
            back();
        }
        break;

    case Qt::XButton1:
        enter();
        emit xbutton1_released();
        break;

    default:
        eventHandler->mouseReleaseEvent(e);
        break;
    }
}

void GuiDocumentView::mouseMoveEvent(QMouseEvent* e)
{
    m_currentMousePt = toGraph(DmVector(e->pos().x(), e->pos().y()));

    e->accept();
    eventHandler->mouseMoveEvent(e);

    // snap tooltip
    ActionInterface* action = getCurrentAction();
    if (action)
    {
        SnapResultType snapResult = action->getSnapResult();
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
                QPoint globalPos = mapToGlobal(e->pos() + QPoint(15, 15));
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
}

void GuiDocumentView::tabletEvent(QTabletEvent* e)
{
    if (testAttribute(Qt::WA_UnderMouse))
    {
        switch (e->device())
        {
        case QTabletEvent::Eraser:
            if (e->type() == QEvent::TabletRelease)
            {
                if (pDocument)
                {
                    ActionSelectSingle* a = new ActionSelectSingle(pDocument, this);
                    setCurrentAction(a);
                    QMouseEvent ev(QEvent::MouseButtonRelease, e->pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    mouseReleaseEvent(&ev);
                    a->finish();

                    if (pDocument->getEntityTable()->hasSelect())
                    {
                        setCurrentAction(new ActionModifyDelete(pDocument, this));
                    }
                }
            }
            break;

        case QTabletEvent::Stylus:
        case QTabletEvent::Puck:
            if (e->type() == QEvent::TabletPress)
            {
                QMouseEvent ev(QEvent::MouseButtonPress, e->pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                mousePressEvent(&ev);
            }
            else if (e->type() == QEvent::TabletRelease)
            {
                QMouseEvent ev(QEvent::MouseButtonRelease, e->pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                mouseReleaseEvent(&ev);
            }
            else if (e->type() == QEvent::TabletMove)
            {
                QMouseEvent ev(QEvent::MouseMove, e->pos(), Qt::NoButton, 0, Qt::NoModifier);
                mouseMoveEvent(&ev);
            }
            break;

        default:
            break;
        }
    }
}

void GuiDocumentView::leaveEvent(QEvent* e)
{
    eventHandler->mouseLeaveEvent();
    QWidget::leaveEvent(e);
}

void GuiDocumentView::enterEvent(QEvent* e)
{
    eventHandler->mouseEnterEvent();
    QWidget::enterEvent(e);
}

void GuiDocumentView::focusInEvent(QFocusEvent* e)
{
    eventHandler->mouseEnterEvent();
    QWidget::focusInEvent(e);
}

void GuiDocumentView::focusOutEvent(QFocusEvent* e)
{
    QWidget::focusOutEvent(e);
}

void GuiDocumentView::wheelEvent(QWheelEvent* e)
{
    const double ZOOM_FACTOR_MOUSE = 1.137;     // 鼠标滚轮缩放因子
    const double TRACKPAD_ZOOM_SCALE = 100.;     // 触控板缩放的每像素百分比
    const int TRACKPAD_ANGLE_DIVISOR = 4;        // 触控板角度增量除数

    DmVector mouse = toGraph(e->x(), e->y());

    if (m_strDevice == "Trackpad")
    {
        QPoint numPixels = e->pixelDelta();

        // 高分辨率滚轮触发平移而不是缩放
        isSmoothScrolling |= !numPixels.isNull();

        if (isSmoothScrolling)
        {
            if (e->phase() == Qt::ScrollEnd)
            {
                isSmoothScrolling = false;
            }
        }
        else // Trackpads that without high-resolution scrolling
        {
            numPixels = e->angleDelta() / TRACKPAD_ANGLE_DIVISOR;
        }

        if (!numPixels.isNull())
        {
            if (e->modifiers() == Qt::ControlModifier)
            {
                DMSETTINGS->beginGroup("/Defaults");
                bool invZoom = (DMSETTINGS->readNumEntry("/InvertZoomDirection", 0) == 1);
                DMSETTINGS->endGroup();

                // Hold ctrl to zoom. 1 % per pixel
                double v = (invZoom) ? (numPixels.y() / TRACKPAD_ZOOM_SCALE) : (-numPixels.y() / TRACKPAD_ZOOM_SCALE);
                DM::ZoomDirection direction;
                double factor;

                if (v < 0)
                {
                    direction = DM::In; factor = 1 - v;
                }
                else
                {
                    direction = DM::Out;  factor = 1 + v;
                }

                setCurrentAction(new ActionZoomIn(pDocument, this, direction, DM::Both, &mouse, factor));
            }
            redraw();
        }
        e->accept();
        return;
    }

    if (e->delta() == 0)
    {
        // A zero delta event occurs when smooth scrolling is ended. Ignore this
        e->accept();
        return;
    }

    // zoom in/out:
    if (e->modifiers() == 0)
    {
        DmVector mainViewCenter = toGraph(getWidth() / 2, getHeight() / 2);

        DMSETTINGS->beginGroup("/Defaults");
        bool invZoom = (DMSETTINGS->readNumEntry("/InvertZoomDirection", 0) == 1);
        DMSETTINGS->endGroup();

        if ((e->delta() > 0 && !invZoom) || (e->delta() < 0 && invZoom))
        {
            setCurrentAction(new ActionZoomIn(pDocument, this, DM::Out, DM::Both, &mouse, ZOOM_FACTOR_MOUSE));
        }
        else
        {
            setCurrentAction(new ActionZoomIn(pDocument, this, DM::In, DM::Both, &mouse, ZOOM_FACTOR_MOUSE));
        }
    }
    m_currentMousePt = toGraph(DmVector(e->pos().x(), e->pos().y()));

    QMouseEvent* event = new QMouseEvent(QEvent::MouseMove, QPoint(e->x(), e->y()), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    eventHandler->mouseMoveEvent(event);
    delete event;

    e->accept();
    emit viewChanged();
}

void GuiDocumentView::keyPressEvent(QKeyEvent* e)
{
    if (pDocument)
    {
        return;
    }

    eventHandler->keyPressEvent(e);
}

void GuiDocumentView::keyReleaseEvent(QKeyEvent* e)
{
    eventHandler->keyReleaseEvent(e);
}

void GuiDocumentView::createPainters(unsigned int width, unsigned int height)
{
    m_pBackgroundPainter = PainterCreator::createOpenGLPainter(width, height);
    m_pPreviewPainter = new DmCachePainter();
    m_pPreviewPainter->addContainer(m_pPreviewEntityContainer);
    m_pDocumentPainter = new DmCachePainter();
    m_pDocumentPainter->addContainer(pDocument->getEntityTable()->getEntityContainer());
    m_pDocumentPainter->setSelectedColor(selectedColor);
    m_pDocumentPainter->setHighlightColor(highlightColor);
    m_pPreviewPainter->setSelectedColor(selectedColor);
    m_pPreviewPainter->setHighlightColor(highlightColor);
    m_pForegroundPainter = new GLPainter();
}

void GuiDocumentView::deletePainters()
{
    if (m_pBackgroundPainter)
    {
        delete m_pBackgroundPainter;
        m_pBackgroundPainter = nullptr;
    }
    if (m_pDocumentPainter)
    {
        delete m_pDocumentPainter;
        m_pDocumentPainter = nullptr;
    }
    if (m_pPreviewPainter)
    {
        delete m_pPreviewPainter;
        m_pPreviewPainter = nullptr;
    }
    if (m_pForegroundPainter)
    {
        delete m_pForegroundPainter;
        m_pForegroundPainter = nullptr;
    }
}

void GuiDocumentView::setStrDevice(const QString& strDevice)
{
    m_strDevice = strDevice;
}

void GuiDocumentView::setIsDrawCursor(const bool& isDrawCursor)
{
    m_isDrawCursor = isDrawCursor;
}
