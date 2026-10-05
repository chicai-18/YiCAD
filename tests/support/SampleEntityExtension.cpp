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

/// @file SampleEntityExtension.cpp
/// @brief 示例扩展 ext.sample 的实现

#include "support/SampleEntityExtension.h"

#include <memory>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "IExtensionContext.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"
#include "support/SamplePipeEntity.h"

int SampleEntityExtension::propertyRuns = 0;
DmEntity* SampleEntityExtension::lastEditedEntity = nullptr;

namespace
{
/// @brief 画管道命令：起点、终点
class SamplePipeCommand : public PlaceCommand
{
public:
    /// @brief 预览从 start 到 end 的管道
    void previewPipe(const DmVector& start, const DmVector& end)
    {
        preview().clear();
        auto* pipe = new SamplePipeEntity({start, end}, SampleEntityExtension::kDiameter);
        pipe->setDocument(document());
        pipe->update();
        preview().entities().addEntity(pipe);
        preview().draw();
    }

    /// @brief 提交从 start 到 end 的管道
    void commitPipe(const DmVector& start, const DmVector& end)
    {
        preview().clear();
        auto* pipe = new SamplePipeEntity({start, end}, SampleEntityExtension::kDiameter);
        pipe->setDocument(document());
        pipe->update();
        Transaction t("Create Pipe", document());
        t.start();
        document()->getEntityTable()->add(pipe);
        t.commit();
    }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 画管道工具：指定起点，再指定终点；画完一段回到第一步
class SamplePipeTool : public BasePlaceTool
{
public:
    SamplePipeTool(SamplePipeCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        GUIDIALOGFACTORY->updateMouseWidget(status() == 0 ? QStringLiteral("Specify start point")
                                                          : QStringLiteral("Specify end point"),
                                            QStringLiteral("Cancel"));
    }

    void onMouseMove(QMouseEvent* e) override
    {
        const DmVector mouse = snapper()->snapPoint(e);
        if (status() == 1 && m_start.valid)
        {
            m_command.previewPipe(m_start, mouse);
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
            m_command.preview().clear();
            stepBack();
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        if (status() == 0)
        {
            m_start = pos;
            setStatus(1);
            return;
        }
        m_command.commitPipe(m_start, pos);
        setStatus(0);
    }

private:
    SamplePipeCommand& m_command;
    DmVector m_start{false};
};

std::unique_ptr<BasePlaceTool> SamplePipeCommand::createTool()
{
    return std::make_unique<SamplePipeTool>(*this, document(), view());
}
}  // namespace

void SampleEntityExtension::OnRegister(IExtensionContext& ctx)
{
    ctx.registerEntityClass<SamplePipeEntity>(kProxyFlags);
    ctx.registerExclusiveCommand(QStringLiteral("ext.sample.pipe"), exclusiveCommandFactory<SamplePipeCommand>(),
                                 CommandInfo{QStringLiteral("Pipe"), {QStringLiteral("samplepipe")}});
    ctx.registerInstantCommand(QStringLiteral("ext.sample.properties"),
                               [](const CommandContext& c)
                               {
                                   ++propertyRuns;
                                   lastEditedEntity = c.entity;
                               },
                               CommandInfo{QStringLiteral("Pipe Properties")});
    ctx.registerPropertyEditor(QStringLiteral("ext.sample.Pipe"), QStringLiteral("ext.sample.properties"));
}
