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

/// @file EditCopyCommand.cpp
/// @brief 复制到剪贴板、剪切命令与参考点工具的实现

#include "EditCopyCommand.h"

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "EditCommands.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Modification.h"

namespace
{
/// @brief 指定参考点的工具
class EditCopyTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetReferencePoint ///< 设置参考点
    };

    EditCopyTool(EditCopyCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
        // 原 ActionEditCopy 没有设置 Action 类型，结束时不复位正交零点
        setResetsOrthogonalOnFinish(false);
    }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetReferencePoint:
            GUIDIALOGFACTORY->updateMouseWidget(EditCopyCommand::tr("Specify reference point"),
                                                EditCopyCommand::tr("Cancel"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        (void)snapper()->snapPoint(e);
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            stepBack();
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        m_command.commitCopy(pos);
    }

private:
    EditCopyCommand& m_command;
};
}  // namespace

EditCopyCommand::EditCopyCommand(bool copy)
    : m_copy(copy)
{
}

bool EditCopyCommand::onSelectionReady()
{
    activateTool(std::make_unique<EditCopyTool>(*this, document(), view()));
    return true;
}

void EditCopyCommand::commitCopy(const DmVector& referencePoint)
{
    Modification m(view());
    m.copy(referencePoint, !m_copy);

    finish();
    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
}

// 剪切与复制到剪贴板共用一个类，copy 参数区分
ExclusiveCommandFactory EditCommands::cut()
{
    return [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand> { return std::make_unique<EditCopyCommand>(false); };
}

ExclusiveCommandFactory EditCommands::copy()
{
    return [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand> { return std::make_unique<EditCopyCommand>(true); };
}
