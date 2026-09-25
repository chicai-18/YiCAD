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

/// @file ModifySingleOffsetCommand.cpp
/// @brief 单个偏移命令 ext.modify.single_offset 的实现

#include "ModifySingleOffsetCommand.h"

#include <memory>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "ModifyCommands.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"

void ModifySingleOffsetCommand::previewOffset(DmEntity* original, const DmVector& coord)
{
    preview().clear();
    DmEntity* clone = original->clone();
    if (clone->offset(coord, m_distance))
    {
        preview().entities().addEntity(clone);
    }
    else
    {
        delete clone;
    }
    preview().draw();
}

void ModifySingleOffsetCommand::commitOffset(DmEntity* original, const DmVector& coord)
{
    preview().clear();

    Transaction t(tr("Offset").toStdString(), document());
    t.start();

    DmEntity* ec = original->clone();
    ec->setLayerToActive();
    ec->setPenToActive();
    ec->setHighlighted(false);

    if (!ec->offset(coord, m_distance))
    {
        delete ec;
    }
    else
    {
        document()->getEntityTable()->add(ec);
    }

    t.commit();

    view()->redraw();
    GUIDIALOGFACTORY->updateSelectionWidget(document()->getEntityTable()->countSelect());
    finish();
}

void ModifySingleOffsetCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true);
}

void ModifySingleOffsetCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

namespace
{
/// @brief 单个偏移工具：悬停高亮、单击选中原实体，再单击偏移一侧
class ModifySingleOffsetTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ChooseEntity ///< 选择原实体，然后指定偏移一侧
    };

    ModifySingleOffsetTool(ModifySingleOffsetCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    /// @brief 原 Action 只在 init 时显示一次提示；这里在回到画布时也重新显示
    void updateHints() override
    {
        GUIDIALOGFACTORY->updateMouseWidget(ModifySingleOffsetCommand::tr("Choose the original entity"));
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmEntity* se = snapper()->catchEntity(e);
        if (status() != ChooseEntity)
        {
            return;
        }

        // 未选中实体时：悬停高亮
        if (!m_pOriginalEntity && se != prevHighlighted)
        {
            unhighlightEntity();
            if (se)
            {
                se->setHighlighted(true);
                view()->specifyDocumentModified();
                view()->redraw();
                prevHighlighted = se;
            }
        }

        // 已选中实体后：偏移预览
        if (m_pOriginalEntity)
        {
            m_coord = view()->toGraph(e->pos().x(), e->pos().y());
            m_command.previewOffset(m_pOriginalEntity, m_coord);
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            if (status() != ChooseEntity)
            {
                return;
            }
            if (!m_pOriginalEntity)
            {
                m_pOriginalEntity = snapper()->catchEntity(e);
                if (m_pOriginalEntity)
                {
                    unhighlightEntity();
                    m_pOriginalEntity->setHighlighted(true);
                    view()->specifyDocumentModified();
                    view()->redraw();
                    prevHighlighted = m_pOriginalEntity;
                }
            }
            else if (m_coord.valid)
            {
                DmEntity* original = m_pOriginalEntity;
                unhighlightEntity();
                m_pOriginalEntity = nullptr;
                m_command.commitOffset(original, m_coord);
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            // 只有一步：右键结束命令
            snapper()->deleteSnapper();
            unhighlightEntity();
            m_command.preview().clear();
            m_pOriginalEntity = nullptr;
            m_coord = DmVector();
            stepBack();
        }
    }

    /// @brief 原 Action 在析构时取消高亮
    void onFinish() override { unhighlightEntity(); }

private:
    /// @brief 取消悬停或选中的高亮
    void unhighlightEntity()
    {
        if (prevHighlighted)
        {
            prevHighlighted->setHighlighted(false);
            view()->specifyDocumentModified();
            view()->redraw();
            prevHighlighted = nullptr;
        }
    }

    ModifySingleOffsetCommand& m_command;
    DmEntity* m_pOriginalEntity = nullptr; ///< 选中的原实体
    DmEntity* prevHighlighted = nullptr;   ///< 当前高亮的实体
    DmVector m_coord;                      ///< 偏移一侧的点（未选中实体时无效）
};

}  // namespace

std::unique_ptr<BasePlaceTool> ModifySingleOffsetCommand::createTool()
{
    return std::make_unique<ModifySingleOffsetTool>(*this, document(), view());
}

ExclusiveCommandFactory ModifyCommands::singleOffset()
{
    return exclusiveCommandFactory<ModifySingleOffsetCommand>();
}
