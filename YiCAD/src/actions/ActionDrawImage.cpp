/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file ActionDrawImage.cpp
/// @brief 图片插入交互动作实现

#include <cmath>
#include "ActionDrawImage.h"

#include <QAction>
#include <QImage>
#include <QMouseEvent>

#include "DmImage.h"
#include "DmLine.h"
#include "DmUnits.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "DmDocument.h"
#include "Transaction.h"
#include "EntityTable.h"

namespace
{
    constexpr int IMAGE_DEFAULT_WIDTH = 50.0;   ///< 默认图片宽度（像素）
    constexpr int IMAGE_DEFAULT_HEIGHT = 50.0;  ///< 默认图片高度（像素）
    constexpr double IMAGE_ZERO_COORD = 0.0;       ///< 默认坐标零值
    constexpr double IMAGE_UNIT_SCALE = 1.0;       ///< 默认单位缩放
    constexpr int IMAGE_DEFAULT_DPI = 0;           ///< 默认 DPI（0 表示未设置）
}

struct ActionDrawImage::AImageData
{
    ImageData data;
    QImage img;
};

ActionDrawImage::ActionDrawImage(DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Image", doc, docView)
    , pImg(new AImageData())
    , m_lastStatus(ShowDialog)
{
    actionType = DM::ActionDrawImage;
}

ActionDrawImage::~ActionDrawImage() = default;

void ActionDrawImage::init(int status)
{
    PreviewActionInterface::init(status);

    reset();

    pImg->data.setPath(GUIDIALOGFACTORY->requestImageOpenDialog().toStdString());

    if (!QString::fromStdString(pImg->data.getPath()).isEmpty())
    {
        pImg->img = QImage(QString::fromStdString(pImg->data.getPath()));

        setStatus(SetTargetPoint);
    }
    else
    {
        setFinished();
    }
}

void ActionDrawImage::reset()
{
    pImg->data = { IMAGE_DEFAULT_DPI, {IMAGE_ZERO_COORD, IMAGE_ZERO_COORD},
                   {IMAGE_UNIT_SCALE, IMAGE_ZERO_COORD},
                   {IMAGE_ZERO_COORD, IMAGE_UNIT_SCALE},
                   {IMAGE_UNIT_SCALE, IMAGE_UNIT_SCALE},
                   "", IMAGE_DEFAULT_WIDTH, IMAGE_DEFAULT_HEIGHT, IMAGE_DEFAULT_DPI };
}

void ActionDrawImage::trigger()
{
    deletePreview();

    if (!QString::fromStdString(pImg->data.getPath()).isEmpty())
    {
        Transaction t(tr("Draw Image").toStdString(), pDocument);
        t.start();
        DmImage* img = new DmImage(nullptr, pImg->data);
        img->setDocument(pDocument);
        img->update();
        pDocument->getEntityTable()->add(img);
        t.commit();
    }

    docView->redraw();
    finish(false);
}

void ActionDrawImage::mouseMoveEvent(QMouseEvent* e)
{
    if (getStatus() == SetTargetPoint)
    {
        pImg->data.setInsertionPoint(snapPoint(e));

        deletePreview();
        double const w = pImg->img.width();
        double const h = pImg->img.height();

        DmLine* line = new DmLine(nullptr, { IMAGE_ZERO_COORD, IMAGE_ZERO_COORD }, { w, IMAGE_ZERO_COORD });
        preview->addEntity(line);
        line = new DmLine(nullptr, { w, IMAGE_ZERO_COORD }, { w, h });
        preview->addEntity(line);
        line = new DmLine(nullptr, { w, h }, { IMAGE_ZERO_COORD, h });
        preview->addEntity(line);
        line = new DmLine(nullptr, { IMAGE_ZERO_COORD, h }, { IMAGE_ZERO_COORD, IMAGE_ZERO_COORD });
        preview->addEntity(line);

        preview->getEntityContainer()->scale({ IMAGE_ZERO_COORD, IMAGE_ZERO_COORD },
                                              { pImg->data.getScale().x, pImg->data.getScale().y });
        preview->getEntityContainer()->rotate({ IMAGE_ZERO_COORD, IMAGE_ZERO_COORD }, pImg->data.getUVector());
        preview->move(pImg->data.getInsertionPoint());
        drawPreview();
    }
}

void ActionDrawImage::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        finish(false);
    }
}

void ActionDrawImage::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }

    pImg->data.setInsertionPoint(e->getCoordinate());
    trigger();
}

void ActionDrawImage::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
    case SetTargetPoint:
        if (checkCommand("angle", c))
        {
            deletePreview();
            m_lastStatus = static_cast<Status>(getStatus());
            setStatus(SetAngle);
        }
        else if (checkCommand("factor", c))
        {
            deletePreview();
            m_lastStatus = static_cast<Status>(getStatus());
            setStatus(SetFactor);
        }
        else if (checkCommand("dpi", c))
        {
            deletePreview();
            m_lastStatus = static_cast<Status>(getStatus());
            setStatus(SetDPI);
        }
        break;

    case SetAngle:
        {
            bool ok = false;
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                setAngle(Math2d::deg2rad(a));
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(m_lastStatus);
        }
        break;

    case SetFactor:
        {
            bool ok = false;
            double f = Math2d::eval(c, &ok);
            if (ok)
            {
                setFactor(f);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(m_lastStatus);
        }
        break;

    case SetDPI:
        {
            bool ok = false;
            double dpi = Math2d::eval(c, &ok);

            if (ok)
            {
                setFactor(DmUnits::dpiToScale(dpi, pDocument->getUnit()));
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(m_lastStatus);
        }
        break;

    default:
        break;
    }
}

double ActionDrawImage::getAngle() const
{
    return pImg->data.getUVector().angle();
}

void ActionDrawImage::setAngle(double a) const
{
    double l = pImg->data.getUVector().magnitude();
    pImg->data.getUVector().setPolar(l, a);
    pImg->data.getVVector().setPolar(l, a + M_PI_2);
}

double ActionDrawImage::getFactor() const
{
    return pImg->data.getScale().x;
}

void ActionDrawImage::setFactor(double f) const
{
    pImg->data.setScale(DmVector(f, f));
}

double ActionDrawImage::dpiToScale(double dpi) const
{
    return DmUnits::dpiToScale(dpi, pDocument->getUnit());
}

double ActionDrawImage::scaleToDpi(double scale) const
{
    return DmUnits::scaleToDpi(scale, pDocument->getUnit());
}

void ActionDrawImage::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

QStringList ActionDrawImage::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
    case SetTargetPoint:
        cmd += command("angle");
        cmd += command("factor");
        cmd += command("dpi");
        break;
    default:
        break;
    }

    return cmd;
}

void ActionDrawImage::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

void ActionDrawImage::hideOptions()
{
    ActionInterface::hideOptions();
    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDrawImage::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case SetTargetPoint:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Specify reference point"), tr("Cancel"));
        break;
    case SetAngle:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Enter angle:"), "");
        break;
    case SetFactor:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Enter factor:"), "");
        break;
    case SetDPI:
        GUIDIALOGFACTORY->updateMouseWidget(tr("Enter dpi:"), "");
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}
