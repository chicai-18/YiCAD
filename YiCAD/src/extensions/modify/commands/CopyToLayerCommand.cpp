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

/// @file CopyToLayerCommand.cpp
/// @brief 复制到图层命令与工具的实现

#include "CopyToLayerCommand.h"

#include <vector>

#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "ModifyCommands.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Transaction.h"

namespace
{
/// @brief 复制到图层工具：拾取目标图层上的实体，指定基点，再指定终点
/// @details 提示写在命令行里（原 ActionCopyToLayer 如此），不改按键提示；也没有
///          自己的光标。
class CopyToLayerTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetLayer,     ///< 选择目标图层
        SetBasePoint, ///< 设置基点
        SetEndPoint   ///< 设置终点
    };

    CopyToLayerTool(CopyToLayerCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    /// @brief 原 ActionCopyToLayer 不设置光标，沿用之前的
    std::optional<DM::CursorType> getCursor() const override { return std::nullopt; }

protected:
    void updateHints() override
    {
        switch (status())
        {
        case SetLayer:
            GUIDIALOGFACTORY->commandMessage(CopyToLayerCommand::tr("Select the object on the target layer"));
            break;
        case SetBasePoint:
            GUIDIALOGFACTORY->commandMessage(CopyToLayerCommand::tr("Set base point"));
            break;
        case SetEndPoint:
            GUIDIALOGFACTORY->commandMessage(CopyToLayerCommand::tr("Set end point"));
            break;
        default:
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        const DmVector pos = snapper()->snapPoint(e);
        if (status() == SetEndPoint)
        {
            m_command.previewAt(m_basePoint, pos);
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            switch (status())
            {
            case SetLayer:
                if (DmEntity* ent = snapper()->catchEntity(e))
                {
                    m_command.setTargetLayer(ent->getLayer());
                    m_command.clearPreview();
                    setStatus(SetBasePoint);
                }
                break;
            case SetBasePoint:
            case SetEndPoint:
                onCoordinate(snapper()->snapPoint(e));
                break;
            default:
                break;
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            // 原先是 setStatus(getStatus() - 1)：不重新初始化捕捉器，也不清除预览
            if (status() <= SetLayer)
            {
                command().finish();
            }
            else
            {
                setStatus(status() - 1);
            }
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        switch (status())
        {
        case SetBasePoint:
            m_basePoint = pos;
            setStatus(SetEndPoint);
            break;
        case SetEndPoint:
            m_command.clearPreview();
            snapper()->deleteSnapper();
            m_command.commitCopy(m_basePoint, pos);
            GUIDIALOGFACTORY->commandMessage(CopyToLayerCommand::tr("Finish"));
            break;
        default:
            break;
        }
    }

private:
    CopyToLayerCommand& m_command;
    DmVector m_basePoint;
};
}  // namespace

CopyToLayerCommand::CopyToLayerCommand() = default;

CopyToLayerCommand::~CopyToLayerCommand() = default;

bool CopyToLayerCommand::onSelectionReady()
{
    m_preview = std::make_unique<CommandPreview>(document(), view());
    auto tool = std::make_unique<CopyToLayerTool>(*this, document(), view());
    tool->setPreview(m_preview.get());
    activateTool(std::move(tool));
    return true;
}

void CopyToLayerCommand::previewAt(const DmVector& basePoint, const DmVector& mouse)
{
    Preview& preview = m_preview->entities();
    if (preview.isEmpty())
    {
        // 第一次：把选择集的克隆按原位放进预览，记下它对应的基点
        m_preview->clear();
        m_previewPos = basePoint;
        auto table = document()->getEntityTable();
        for (auto it = table->begin(); it != table->end(); ++it)
        {
            if ((*it)->isSelected())
            {
                DmEntity* clone = (*it)->clone();
                clone->setLayer(m_targetLayer->getName());
                clone->setPen(m_targetLayer->getPen());
                clone->setSelected(false);
                clone->setParent(nullptr);
                preview.addEntity(clone);
            }
        }
        preview.setVisible(true);
        m_preview->draw();
    }
    else
    {
        DmVector offset = mouse - m_previewPos;
        preview.move(offset);
        m_previewPos.move(offset);
    }
    m_preview->draw();
}

void CopyToLayerCommand::clearPreview()
{
    m_preview->clear();
}

void CopyToLayerCommand::commitCopy(const DmVector& basePoint, const DmVector& endPoint)
{
    // 复制选择的实体
    std::vector<DmEntity*> vec;
    auto table = document()->getEntityTable();
    for (auto it = table->begin(); it != table->end(); ++it)
    {
        if ((*it)->isSelected())
        {
            (*it)->setSelected(false);
            DmEntity* ent = (*it)->clone();
            ent->move(endPoint - basePoint);
            ent->setLayer(m_targetLayer->getName());
            ent->setPen(m_targetLayer->getPen());
            vec.emplace_back(ent);
        }
    }

    // 添加到文档
    if (!vec.empty())
    {
        Transaction t(tr("Copy Entities To Layer").toStdString(), document());
        t.start();
        for (DmEntity* ent : vec)
        {
            document()->getEntityTable()->add(ent);
            view()->redraw();
        }
        t.commit();
    }

    finish();
}

ExclusiveCommandFactory ModifyCommands::copyToLayer()
{
    return exclusiveCommandFactory<CopyToLayerCommand>();
}
