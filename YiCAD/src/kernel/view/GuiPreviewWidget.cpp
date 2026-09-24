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
/// @brief OpenGL 预览控件实现

#include "GuiPreviewWidget.h"

#include "DmLine.h"
#include "DmCircle.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmSolid.h"
#include "DmPolyline.h"
#include "DmSpline.h"
#include "DmText.h"
#include "DmMText.h"
#include "DmDocument.h"
#include "GuiGrid.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmBlockReference.h"
#include "DmSettings.h"
#include "Math2d.h"
#include "Debug.h"
#include "GeometryMethods.h"


GuiPreviewWidget::GuiPreviewWidget(QWidget* parent, Qt::WindowFlags f)
    : QOpenGLWidget(parent, f)
    , container(nullptr)
    , background(30, 30, 30, 255)
    , m_isInitialized(false)
    , m_pPainter(nullptr)
{
}

GuiPreviewWidget::~GuiPreviewWidget()
{
    if (m_pPainter)
    {
        delete m_pPainter;
        m_pPainter = nullptr;
    }
    // 由使用者负责释放
}

/// @brief 指定预览的实体集
void GuiPreviewWidget::setContainer(DmEntityContainer* container)
{
    this->container = container;
}

/// @brief 适屏显示
void GuiPreviewWidget::zoomAuto()
{
    if (container)
    {
        if (width() == 0 || height() == 0)
        {
            return;
        }
        double sx = 0.0;
        double sy = 0.0;
        DmVector max = container->getMax();
        DmVector min = container->getMin();
        DmVector center = (max + min) / 2.0;
        auto const dV = max - min;
        sx = std::max(dV.x, 0.);
        sy = std::max(dV.y, 0.);

        double fx = 1., fy = 1.;
        if (sx > DM_TOLERANCE && sy > DM_TOLERANCE)
        {
            fx = sx / width();
            fy = sy / height();
        }
        else
        {
            return;
        }
        fx = fy = std::max(fx, fy);

        if (m_pPainter)
        {
            m_pPainter->setViewPosition(center.x, center.y);
            m_pPainter->setScale(fx);
        }
        redraw();
    }
}

/// @brief 检查是否已初始化
bool GuiPreviewWidget::initialized() const
{
    return m_isInitialized;
}

/// @brief 重绘控件
void GuiPreviewWidget::redraw()
{
    update();
}

/// @brief 指示内容已修改
void GuiPreviewWidget::specifyModified()
{
    if (m_pPainter)
    {
        m_pPainter->specifyModified();
    }
}

void GuiPreviewWidget::initializeGL()
{
    QOpenGLWidget::makeCurrent();
    QOpenGLContext* CC = QOpenGLContext::currentContext();

    int width = size().width();
    int height = size().height();

    if (CC != 0)
    {
        GLenum err = glewInit();
        // TODO : 在部分linux虚拟机中获得GLEW_ERROR_NO_GLX_DISPLAY
        if (err == GLEW_ERROR_NO_GLX_DISPLAY)
        {
            err = GLEW_OK;
        }
        if (err != GLEW_OK)
        {
            exit(1);
        }

        if (!GLEW_VERSION_2_1)
        {
            exit(1);
        }

        createPainters(width, height);
        if (m_pPainter)
        {
            m_pPainter->create_resources();
            m_isInitialized = true;
        }
    }
    else
    {
        createPainters(width, height);
    }
}

void GuiPreviewWidget::paintGL()
{
    if (!m_isInitialized)
    {
        return;
    }
    m_pPainter->draw();
}

void GuiPreviewWidget::resizeGL(int w, int h)
{
    if (!m_isInitialized)
    {
        return;
    }
    m_pPainter->new_device_size(w, h);
    zoomAuto();
    redraw();
}

void GuiPreviewWidget::createPainters(unsigned int width, unsigned int height)
{
    if (container == nullptr)
    {
        return;
    }
    m_pPainter = new DmCachePainter();
    m_pPainter->addContainer(container);
}
