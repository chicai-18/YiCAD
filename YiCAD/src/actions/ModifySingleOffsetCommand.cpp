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
/// @brief 单个偏移命令 modify.single_offset，取代原 ActionModifySingleOffset：选一个实体，
///        再点一下偏移的一侧，按选项条上的距离偏移出一个新实体，然后结束

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Modification.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
constexpr double DEFAULT_OFFSET_DISTANCE = 30.0; ///< 默认偏移距离
constexpr unsigned DEFAULT_OFFSET_NUMBER = 1;    ///< 默认偏移数量

/// @brief 单个偏移命令：持有偏移参数（选项条经引用直接改写距离）并提交偏移
class ModifySingleOffsetCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifySingleOffsetCommand)

public:
    ModifySingleOffsetCommand()
    {
        m_data.distance = DEFAULT_OFFSET_DISTANCE;
        m_data.number = DEFAULT_OFFSET_NUMBER;
        m_data.useCurrentAttributes = true;
        m_data.useCurrentLayer = true;
        m_data.coord = DmVector();
    }

    /// @brief 偏移距离
    double distance() const { return m_data.distance; }

    /// @brief 预览 original 向 coord 一侧偏移的结果
    void previewOffset(DmEntity* original, const DmVector& coord)
    {
        preview().clear();
        DmEntity* clone = original->clone();
        if (clone->offset(coord, m_data.distance))
        {
            preview().entities().addEntity(clone);
        }
        else
        {
            delete clone;
        }
        preview().draw();
    }

    /// @brief 把 original 向 coord 一侧偏移出一个新实体（放在当前图层、用当前画笔），然后结束命令
    void commitOffset(DmEntity* original, const DmVector& coord)
    {
        preview().clear();

        Transaction t(tr("Offset").toStdString(), document());
        t.start();

        DmEntity* ec = original->clone();
        ec->setLayerToActive();
        ec->setPenToActive();
        ec->setHighlighted(false);

        if (!ec->offset(coord, m_data.distance))
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

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;

    void showOptions() override { GUIDIALOGFACTORY->requestModifySingleOffsetOptions(m_data.distance, true, false); }
    void hideOptions() override { GUIDIALOGFACTORY->requestModifySingleOffsetOptions(m_data.distance, false, false); }

private:
    OffsetData m_data; ///< 偏移参数（只用到距离）
};

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

std::unique_ptr<BasePlaceTool> ModifySingleOffsetCommand::createTool()
{
    return std::make_unique<ModifySingleOffsetTool>(*this, document(), view());
}

const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("modify.single_offset"),
    exclusiveCommandFactory<ModifySingleOffsetCommand>());
}  // namespace
