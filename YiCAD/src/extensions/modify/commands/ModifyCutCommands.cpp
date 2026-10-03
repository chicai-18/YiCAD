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

/// @file ModifyCutCommands.cpp
/// @brief 打断命令 ext.modify.cut（原 ActionModifyCut，在一点打断）与两点打断命令
///        ext.modify.cut_2p（原 ActionModifyCut2P，删去两点间的一段）
///
/// 两个命令没有选项条，只在本文件里定义；工具的事件处理从原 Action 机械改写而来。

#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "ModifyCommands.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "HiddenSet.h"
#include "HighlightSet.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Modification.h"
#include "PlaceCommand.h"
#include "SelectionSet.h"

namespace
{
/// @brief 打断命令；交互由 ModifyCutTool 驱动
class ModifyCutCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyCutCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 两点打断命令；交互由 ModifyCut2PTool 驱动
class ModifyCut2PCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyCut2PCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 打断工具：选实体，再指定打断点；可连续打断
class ModifyCutTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ChooseCutEntity, ///< 选择要打断的实体
        SetCutCoord      ///< 指定打断点
    };

    ModifyCutTool(ModifyCutCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override
    {
        return status() == SetCutCoord ? DM::CadCursor : DM::SelectCursor;
    }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;

private:
    /// @brief 回到某一状态（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
    }

    /// @brief 打断并回到第一步（原 trigger()）
    void trigger()
    {
        if (cutEntity && cutCoord.valid && cutEntity->isPointOnEntity(cutCoord))
        {
            command().highlight()->clear();

            Modification m(document());
            m.cut(cutCoord, cutEntity);

            cutEntity = nullptr;
            cutCoord = DmVector(false);
            setStatus(ChooseCutEntity);
        }
    }

    bool entityTrimmable(DmEntity* e) const;

    ModifyCutCommand& m_command;
    DmEntity* cutEntity = nullptr; ///< 当前选中的实体
    DmVector cutCoord;             ///< 打断点
};

/// @brief 两点打断工具：选实体（选中处为第一点），再指定第二点；打断后命令结束
class ModifyCut2PTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ChooseCutEntity, ///< 选择待打断实体
        SetCutCoord      ///< 设置第二点
    };

    ModifyCut2PTool(ModifyCut2PCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override
    {
        return status() == SetCutCoord ? DM::CadCursor : DM::SelectCursor;
    }

protected:
    /// @brief 原 Action 没有按键提示
    void updateHints() override {}
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;

private:
    /// @brief 回到某一状态（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restart(s);
    }

    /// @brief 打断后结束命令（原 trigger()）
    void trigger()
    {
        if (cutEntity && firstCoord.valid && secondCoord.valid)
        {
            command().hidden()->remove(cutEntity);
            view()->redraw();

            Modification m(document());
            m.cut2P(firstCoord, secondCoord, cutEntity);

            cutEntity = nullptr;
            firstCoord = DmVector(false);
            secondCoord = DmVector(false);
            command().finish();

            GUIDIALOGFACTORY->updateSelectionWidget(command().selection()->count());
        }
    }

    ModifyCut2PCommand& m_command;
    DmEntity* cutEntity = nullptr; ///< 待打断的实体
    DmVector firstCoord;           ///< 第一点
    DmVector secondCoord;          ///< 第二点
};

/// @brief 判断实体是否可裁剪（支持直线、圆弧、椭圆、多段线、样条曲线）。
/// @param e 待检查的实体指针。
/// @return 若实体类型可裁剪则返回 true。
bool ModifyCutTool::entityTrimmable(DmEntity* e) const
{
    switch (e->getEntityType())
    {
    case DM::EntityArc:
    case DM::EntityEllipse:
    case DM::EntityLine:
    case DM::EntityPolyline:
    case DM::EntitySpline:
        return true;
    default:
        return false;
    }
}

/// @brief 处理鼠标移动事件。
/// @param e 鼠标事件对象。
void ModifyCutTool::onMouseMove(QMouseEvent* e)
{
    switch (status())
    {
    case ChooseCutEntity:
        snapper()->deleteSnapper();
        break;

    case SetCutCoord:
        snapper()->snapPoint(e);
        break;

    default:
        break;
    }
}

/// @brief 处理鼠标释放事件：左键选择实体/设置裁剪点，右键回退状态。
/// @param e 鼠标事件对象。
void ModifyCutTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
        case ChooseCutEntity:
            cutEntity = snapper()->catchEntity(e);
            if (cutEntity == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCutCommand::tr("No Entity found."));
            }
            else if (entityTrimmable(cutEntity))
            {
                command().highlight()->add(cutEntity);
                setStatus(SetCutCoord);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCutCommand::tr("Entity must be a line, arc, ellipse or polyline."));
            }
            break;

        case SetCutCoord:
            cutCoord = snapper()->snapPoint(e);
            if (cutEntity == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCutCommand::tr("No Entity found."));
            }
            else if (!cutCoord.valid)
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCutCommand::tr("Cutting point is invalid."));
            }
            else if (!cutEntity->isPointOnEntity(cutCoord))
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCutCommand::tr("Cutting point is not on entity."));
            }
            else
            {
                trigger();
                snapper()->deleteSnapper();
            }
            break;

        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        command().highlight()->clear();
        init(status() - 1);
    }
}

/// @brief 更新鼠标按钮提示文本。
void ModifyCutTool::updateHints()
{
    switch (status())
    {
    case ChooseCutEntity:
        GUIDIALOGFACTORY->updateMouseWidget(ModifyCutCommand::tr("Specify entity to cut"), ModifyCutCommand::tr("Cancel"));
        break;
    case SetCutCoord:
        GUIDIALOGFACTORY->updateMouseWidget(ModifyCutCommand::tr("Specify cutting point"), ModifyCutCommand::tr("Back"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件指针
void ModifyCut2PTool::onMouseMove(QMouseEvent* e)
{
    DmVector pt;
    // TODO: startAngle 和 endAngle 变量已移除，原声明未使用，如需角度计算逻辑请在此处添加
    switch (status())
    {
    case ChooseCutEntity:
        snapper()->deleteSnapper();
        break;

    case SetCutCoord:
    {
        pt = snapper()->snapPoint(e);
        command().hidden()->remove(cutEntity);
        m_command.preview().clear();
        secondCoord = cutEntity->getNearestPointOnEntity(pt);
        std::vector<DmEntity*> remainEnts;
        DmEntity* deleteEnt = nullptr;
        const bool res = Modification::tryCut2P(firstCoord, secondCoord, cutEntity, remainEnts, deleteEnt);
        if (res)
        {
            m_command.preview().entities().getEntityContainer()->addEntity(deleteEnt);
            for (const auto& e : remainEnts)
            {
                m_command.preview().entities().getEntityContainer()->addEntity(e);
            }
            // 原实体换成打断后的预览（视图的临时隐藏集，RENDER_PLAN.md 第 4.3.9 节）
            command().hidden()->add(cutEntity);
        }
        m_command.preview().draw();
    }
        break;
    default:
        break;
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件指针
void ModifyCut2PTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (status())
        {
        case ChooseCutEntity:
            cutEntity = snapper()->catchEntity(e);
            if (cutEntity == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCut2PCommand::tr("No Entity found."));
            }
            else if (Modification::isCutableEntity(cutEntity))
            {
                view()->redraw();
                setStatus(SetCutCoord);
                DmVector pt = snapper()->snapPoint(e);
                firstCoord = cutEntity->getNearestPointOnEntity(pt);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCut2PCommand::tr("Entity must be a line, arc, circle, ellipse or polyline."));
            }
            break;
        case SetCutCoord:
        {
            DmVector pt = snapper()->snapPoint(e);

            secondCoord = cutEntity->getNearestPointOnEntity(pt);
            if (cutEntity == nullptr)
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCut2PCommand::tr("No Entity found."));
            }
            else if (!firstCoord.valid)
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCut2PCommand::tr("Cutting point is invalid."));
            }
            else if (!cutEntity->isPointOnEntity(secondCoord))
            {
                GUIDIALOGFACTORY->commandMessage(ModifyCut2PCommand::tr("Cutting point is not on entity."));
            }
            else
            {
                trigger();
                snapper()->deleteSnapper();
            }
            break;
        }
        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        if (cutEntity)
        {
            command().hidden()->remove(cutEntity);    // 恢复显示
            m_command.preview().clear();
            view()->redraw();
        }
        init(status() - 1);
    }
}

std::unique_ptr<BasePlaceTool> ModifyCutCommand::createTool()
{
    return std::make_unique<ModifyCutTool>(*this, document(), view());
}

std::unique_ptr<BasePlaceTool> ModifyCut2PCommand::createTool()
{
    return std::make_unique<ModifyCut2PTool>(*this, document(), view());
}

}  // namespace

ExclusiveCommandFactory ModifyCommands::cut()
{
    return exclusiveCommandFactory<ModifyCutCommand>();
}

ExclusiveCommandFactory ModifyCommands::cut2p()
{
    return exclusiveCommandFactory<ModifyCut2PCommand>();
}
