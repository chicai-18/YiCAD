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

/// @file ModifyScaleCommand.cpp
/// @brief 缩放命令与缩放工具的实现

#include "ModifyScaleCommand.h"

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Transaction.h"

namespace
{
/// @brief 缩放计算中使用的常量因子
constexpr double SCALE_DISTANCE_FACTOR = 2.0;

/// @brief 缩放工具：指定基点，再指定或输入比例
class ModifyScaleTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetReferencePoint, ///< 设置参考点
        SetScale           ///< 设置缩放比例
    };

    ModifyScaleTool(ModifyScaleCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetReferencePoint:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyScaleCommand::tr("Specify reference point"),
                                                ModifyScaleCommand::tr("Cancel"));
            break;
        case SetScale:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyScaleCommand::tr("Input scale"), ModifyScaleCommand::tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        switch (status())
        {
        case SetReferencePoint:
            m_referencePoint = mouse;
            break;

        case SetScale:
            m_factor = m_command.factorAt(m_referencePoint, mouse);
            m_command.previewScale(m_referencePoint, m_factor);
            break;

        default:
            break;
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.clearPreview();
            stepBack();
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        switch (status())
        {
        case SetReferencePoint:
            m_referencePoint = pos;
            setStatus(SetScale);
            break;

        case SetScale:
            m_factor = m_command.factorAt(m_referencePoint, pos);
            m_command.commitScale(m_referencePoint, m_factor);
            break;

        default:
            break;
        }
    }

    /// @brief 设置比例时输入比例并结束命令：输入无效时提示，仍按上一次鼠标位置的比例
    ///        缩放（原有行为）；设置基点时文本被接受但不起作用
    void onCommand(GuiCommandEvent* e) override
    {
        e->accept();
        if (status() != SetScale)
        {
            return;
        }
        bool ok = false;
        double r = Math2d::eval(e->getCommand().toLower(), &ok);
        if (ok && (r > 0))
        {
            m_factor = r;
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(ModifyScaleCommand::tr("Input invalid"),
                                                ModifyScaleCommand::tr("Back"));
        }
        m_command.commitScale(m_referencePoint, m_factor);
    }

private:
    ModifyScaleCommand& m_command;
    DmVector m_referencePoint;
    /// @brief 当前比例；原先的 ScaleData 值初始化为 0，还没移动鼠标就输入无效文本时按 0 缩放
    double m_factor = 0.0;
};
}  // namespace

ModifyScaleCommand::ModifyScaleCommand() = default;

ModifyScaleCommand::~ModifyScaleCommand() = default;

bool ModifyScaleCommand::onSelectionReady()
{
    // 选择集包围框宽高的较大值，作为鼠标距离换算比例的基准
    DmEntityContainer ec(nullptr, false);
    auto table = document()->getEntityTable();
    for (auto e : *table)
    {
        if (e->isSelected())
        {
            ec.addEntity(e);
        }
    }
    DmVector deltaXY = ec.getMax() - ec.getMin();
    m_boxRange = (deltaXY.x > deltaXY.y) ? deltaXY.x : deltaXY.y;

    m_preview = std::make_unique<CommandPreview>(document(), view());
    auto tool = std::make_unique<ModifyScaleTool>(*this, document(), view());
    tool->setPreview(m_preview.get());
    activateTool(std::move(tool));
    return true;
}

double ModifyScaleCommand::factorAt(const DmVector& reference, const DmVector& mouse) const
{
    return SCALE_DISTANCE_FACTOR * mouse.distanceTo(reference) / m_boxRange;
}

void ModifyScaleCommand::previewScale(const DmVector& reference, double factor)
{
    m_preview->clear();
    m_preview->entities().addSelectionFromDocument();
    m_preview->entities().getEntityContainer()->scale(reference, DmVector(factor, factor));
    m_preview->draw();
}

void ModifyScaleCommand::clearPreview()
{
    m_preview->clear();
}

void ModifyScaleCommand::commitScale(const DmVector& reference, double factor)
{
    Transaction t(tr("Scale").toStdString(), document());
    t.start();

    auto table = document()->getEntityTable();
    DmVector scaleVec(factor, factor);
    for (auto e : *table)
    {
        if (e->isSelected())
        {
            table->startModify(e);
            e->scale(reference, scaleVec);
        }
    }

    t.commit();

    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    finish();
}

namespace
{
const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("modify.scale"), exclusiveCommandFactory<ModifyScaleCommand>());
}  // namespace
