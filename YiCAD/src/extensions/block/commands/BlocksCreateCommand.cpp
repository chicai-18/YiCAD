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

/// @file BlocksCreateCommand.cpp
/// @brief 创建块命令与参考点工具的实现

#include "BlocksCreateCommand.h"

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "DmAttribute.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "SelectionSet.h"
#include "Transaction.h"
#include "UIBlockDialog.h"
#include "UIDialogRunner.h"
#include "UIDlgEditAttributes.h"

namespace
{
/// @brief 指定参考点的工具
class BlocksCreateTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetReferencePoint, ///< 设置参考点
        ShowDialog         ///< 显示名称对话框
    };

    BlocksCreateTool(BlocksCreateCommand& command, DmDocument* doc, IDocumentView* view)
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
            GUIDIALOGFACTORY->updateMouseWidget(BlocksCreateCommand::tr("Specify reference point"),
                                                BlocksCreateCommand::tr("Cancel"));
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
        if (status() == SetReferencePoint && m_command.createBlock(pos))
        {
            // 清除按键提示
            setStatus(ShowDialog);
        }
    }

private:
    BlocksCreateCommand& m_command;
};
}  // namespace

bool BlocksCreateCommand::onSelectionReady()
{
    activateTool(std::make_unique<BlocksCreateTool>(*this, document(), view()));
    return true;
}

bool BlocksCreateCommand::createBlock(const DmVector& referencePoint)
{
    DmBlockTable* blockTable = document()->getBlockTable();
    if (!blockTable)
    {
        return false;
    }

    // 对话框只给出块数据，块由这里创建并加进块表
    DmBlockData blockData("", DmVector(false), false);
    {
        UIBlockDialog dlg(UIDialogRunner::parentOf(view()));
        dlg.setBlockList(blockTable);
        if (UIDialogRunner::exec(dlg) == QDialog::Accepted)
        {
            blockData = dlg.getBlockData();
        }
    }
    if (blockData.name.isEmpty())
    {
        view()->redraw();
        finish();
        return true;
    }

    Transaction tg("Create Block", document());
    tg.start();

    // 创建块定义
    DmBlock* block = new DmBlock(document(), blockData);

    // 选中的实体取消选中后克隆到块容器，移除原始实体
    for (auto entity : selection()->entities())
    {
        selection()->remove(entity);
        DmEntity* clonedEntity = entity->clone();
        clonedEntity->move(-referencePoint);
        block->getEntityTable().add_direct(clonedEntity);
        document()->getEntityTable()->remove(entity);
    }

    // 通过块表添加块（走命令系统）
    blockTable->add(block);

    // 处理属性（如有）
    std::list<DmAttribute*> attrs;
    if (block->hasAttributeDefinitions())
    {
        std::list<DmAttributeDefinition*> attrDefs =
            block->getAttributeDefinitions();
        UIDlgEditAttributes::editAttributes(UIDialogRunner::parentOf(view()), block->getName(), attrDefs, attrs);
    }
    for (auto attr : attrs)
    {
        attr->move(referencePoint);
    }

    // 创建块参照并添加到文档
    constexpr double DEFAULT_SCALE = 1.0;
    constexpr double DEFAULT_ROTATION = 0.0;
    constexpr int DEFAULT_COLUMN_COUNT = 1;
    constexpr int DEFAULT_ROW_COUNT = 1;
    DmBlockReferenceData id(blockData.name, referencePoint,
        DmVector(DEFAULT_SCALE, DEFAULT_SCALE), DEFAULT_ROTATION,
        DEFAULT_COLUMN_COUNT, DEFAULT_ROW_COUNT,
        DmVector(0.0, 0.0), nullptr, DM::NoUpdate);
    DmBlockReference* ref = new DmBlockReference(nullptr, id);
    ref->setDocument(document());
    ref->addAttributes(attrs);
    ref->update();
    document()->getEntityTable()->add(ref);

    tg.commit();
    document()->regenerate();

    view()->redraw();
    finish();
    return true;
}
