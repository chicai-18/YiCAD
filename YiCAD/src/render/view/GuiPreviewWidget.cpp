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

/// @file GuiPreviewWidget.cpp
/// @brief 预览控件的实现

#include "GuiPreviewWidget.h"

#include <algorithm>

#include <QColor>

#include "DmBlock.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "GLRhiDevice.h"
#include "GLRhiSurface.h"
#include "GsModel.h"
#include "GsView.h"
#include "Math2d.h"

namespace
{
/// @brief 与画布相同的多重采样数（图形系统按表面的采样数画场景）
constexpr int kPreviewSamples = 4;
}

GuiPreviewWidget::GuiPreviewWidget(QWidget* parent, Qt::WindowFlags f)
    : QOpenGLWidget(parent, f)
    , m_view(std::make_unique<GsView>())
    , m_surface(std::make_unique<GLRhiWidgetSurface>(*this))
{
    // 与程序的其他 GL 上下文同一格式（4.3 core，RENDER_PLAN.md 第 4.7.3 节）
    QSurfaceFormat format = GLRhiDevice::surfaceFormat();
    format.setSamples(kPreviewSamples);
    setFormat(format);
}

GuiPreviewWidget::~GuiPreviewWidget()
{
    // 图形系统的资源放进设备的延迟释放队列，不需要当前上下文
    m_view->release();
    // 容器由使用者负责释放
}

void GuiPreviewWidget::setContainer(DmEntityContainer* container)
{
    this->container = container;
    m_block = nullptr;
    syncContainerModel();
    if (m_model)
    {
        m_model->invalidate();
    }
    update();
}

void GuiPreviewWidget::setBlock(std::shared_ptr<GsModel> model, const DmBlock* block)
{
    container = nullptr;
    m_modelContainer = nullptr;
    m_block = block;
    m_hasBlockBounds = false;
    if (block)
    {
        // 块的包围框：定义坐标，适屏用
        for (DmEntity* e : block->getEntityTable())
        {
            if (!e || e->isErased() || !e->isVisible())
            {
                continue;
            }
            const DmVector lo = e->getMin();
            const DmVector hi = e->getMax();
            if (!m_hasBlockBounds)
            {
                m_blockMin = lo;
                m_blockMax = hi;
                m_hasBlockBounds = true;
            }
            else
            {
                m_blockMin = DmVector(std::min(m_blockMin.x, lo.x), std::min(m_blockMin.y, lo.y));
                m_blockMax = DmVector(std::max(m_blockMax.x, hi.x), std::max(m_blockMax.y, hi.y));
            }
        }
    }
    if (!model && block)
    {
        // 没有文档的图形模型：为这个块建一个只有它的模型（块的共享几何在其中编译）
        model = std::make_shared<GsModel>(static_cast<const DmEntityContainer*>(nullptr));
    }
    m_model = std::move(model);
    m_view->setBlock(m_model, block);
    update();
}

void GuiPreviewWidget::syncContainerModel()
{
    if (m_block)
    {
        return;
    }
    if (container != m_modelContainer || !m_model)
    {
        m_modelContainer = container;
        m_model = std::make_shared<GsModel>(container);
        m_view->setModel(m_model, false);
    }
}

void GuiPreviewWidget::zoomAuto()
{
    m_fit = true;
    update();
}

void GuiPreviewWidget::redraw()
{
    update();
}

void GuiPreviewWidget::specifyModified()
{
    if (m_model && !m_block)
    {
        m_model->invalidate();
    }
    update();
}

void GuiPreviewWidget::setCamera(const DmVector& center, double worldPerPixel)
{
    m_fit = false;
    m_center = center;
    m_worldPerPixel = worldPerPixel;
    update();
}

bool GuiPreviewWidget::contentBounds(DmVector& minCorner, DmVector& maxCorner) const
{
    if (m_block)
    {
        minCorner = m_blockMin;
        maxCorner = m_blockMax;
        return m_hasBlockBounds;
    }
    if (!container)
    {
        return false;
    }
    minCorner = container->getMin();
    maxCorner = container->getMax();
    return minCorner.valid && maxCorner.valid && maxCorner.x >= minCorner.x && maxCorner.y >= minCorner.y;
}

void GuiPreviewWidget::fitCamera()
{
    if (width() <= 0 || height() <= 0)
    {
        return;
    }
    DmVector lo;
    DmVector hi;
    if (!contentBounds(lo, hi))
    {
        return;
    }
    const double sx = std::max(hi.x - lo.x, 0.0);
    const double sy = std::max(hi.y - lo.y, 0.0);
    if (sx <= DM_TOLERANCE && sy <= DM_TOLERANCE)
    {
        return;
    }
    // 与原先相同：内容恰好铺满控件，不留边
    m_center = (lo + hi) / 2.0;
    m_worldPerPixel = std::max(sx / width(), sy / height());
}

void GuiPreviewWidget::initializeGL()
{
    // 图形系统在第一次画时建设备与资源
}

void GuiPreviewWidget::paintGL()
{
    syncContainerModel();
    if (m_fit)
    {
        fitCamera();
    }
    GsViewStyle style;
    style.background = QColor(background.red(), background.green(), background.blue(), background.alpha());
    m_view->setStyle(style);
    m_view->setCamera(m_center, m_worldPerPixel);
    m_view->overlay().clear();
    m_view->render(*m_surface, devicePixelRatioF());
}

void GuiPreviewWidget::resizeGL(int, int)
{
    update();
}
