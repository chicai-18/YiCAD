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

/// @file ModifyTrimCommand.cpp
/// @brief 修剪命令 modify.trim，取代原 ActionModifyTrim：先逐个选边界实体，小键盘回车后
///        逐个点选要剪掉的部分，再按小键盘回车结束
///
/// 命令没有选项条，只在本文件里定义；工具的事件处理从原 Action 机械改写而来。

#include <algorithm>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QKeyEvent>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "DmEntityContainer.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Modification.h"
#include "PlaceCommand.h"

namespace
{
/// @brief 修剪命令；交互由 ModifyTrimTool 驱动
class ModifyTrimCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyTrimCommand)

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 修剪工具：选边界实体，再逐个修剪
class ModifyTrimTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ChooseLimitEntity, ///< 选择限制边界实体
        ChooseTrimEntity   ///< 选择要修剪的实体
    };

    ModifyTrimTool(ModifyTrimCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    void updateHints() override;
    void onMouseMove(QMouseEvent* e) override;
    void onMouseRelease(QMouseEvent* e) override;
    void onKeyPress(QKeyEvent* e) override;

    void onFinish() override
    {
        restoreEntityUnderCursor();
        unhighlightLimitingEntity();
    }

private:
    /// @brief 回到某一状态（原 init(status)）；status < 0 时结束命令
    void init(int s)
    {
        if (s < 0)
        {
            command().finish();
            return;
        }
        restoreEntityUnderCursor();
        restart(s);
    }

    /// @brief 预览修剪时隐藏了光标下的实体；离开这一步前让它重新可见
    ///        （原 Action 在右键退回或结束时没有恢复，实体会一直不可见）
    void restoreEntityUnderCursor()
    {
        if (status() == ChooseTrimEntity && m_entUnderCursor)
        {
            m_entUnderCursor->setVisible(true);
        }
    }

    void trigger();
    void unhighlightLimitingEntity();

    ModifyTrimCommand& m_command;
    std::vector<DmEntity*> m_seleltedEnts; ///< 选择的实体，作为求交的边界
    DmEntity* m_entToTrim = nullptr;       ///< 待修剪的实体
    DmEntity* m_entUnderCursor = nullptr;  ///< 当前鼠标下的实体
    DmVector m_trimPt;                     ///< 确认修剪时，鼠标点下的位置
};

/// @brief 执行修剪操作
///
/// 使用选中的边界实体对目标实体执行修剪，
/// 修剪点由 m_trimPt 指定。
void ModifyTrimTool::trigger()
{
    if ((m_seleltedEnts.size() > 0) && (m_entToTrim != nullptr))
    {
        Modification m(view());
        bool res = m.trim(m_seleltedEnts, m_entToTrim, m_trimPt);

        if (res)
        {
            m_entToTrim = nullptr;
            m_trimPt = {};
            m_entUnderCursor = nullptr;
        }

        updateHints();
    }
}

void ModifyTrimTool::onMouseMove(QMouseEvent* e)
{
    DmVector mouse = view()->toGraph(e->x(), e->y());
    DmEntity* se = snapper()->catchEntity(e);

    switch (status())
    {
        case ChooseLimitEntity:
        {
            // 如果与上次选择的实体一样，不做操作
            if ((se != nullptr) && (se == m_entUnderCursor))
            {
                break;
            }

            // 上次鼠标移动时，下面没有点击选择的实体，这个实体需要还原为不高亮
            if ((nullptr != m_entUnderCursor) && (std::find(m_seleltedEnts.begin(), m_seleltedEnts.end(), m_entUnderCursor) == m_seleltedEnts.end()))
            {
                m_entUnderCursor->setHighlighted(false);
                view()->specifyDocumentModified();
                view()->redraw();
            }

            // 设置当前光标下的实体
            m_entUnderCursor = se;

            if (nullptr != m_entUnderCursor)
            {
                m_entUnderCursor->setHighlighted(true);
                view()->specifyDocumentModified();
                view()->redraw();
            }
        }
            break;

        case ChooseTrimEntity:
        {
            // 上次光标下的实体与现在不同，还原该实体为可见
            if (m_entUnderCursor && (m_entUnderCursor != se))
            {
                m_entUnderCursor->setVisible(true);
            }

            m_entUnderCursor = se;
            m_command.preview().clear();

            if (nullptr != m_entUnderCursor)
            {
                std::vector<DmEntity*> remainEnts;
                DmEntity* deleteEnt = nullptr;
                std::vector<DmEntity*> selectedEntsCopy = m_seleltedEnts;
                auto it = std::find(selectedEntsCopy.begin(), selectedEntsCopy.end(), m_entUnderCursor);

                if (it != selectedEntsCopy.end())
                {
                    // 鼠标下的实体是剪切实体
                    selectedEntsCopy.erase(it);
                }

                m_entUnderCursor->setVisible(true); // 临时设为可见以可裁剪
                DmVector pointOnEnt = m_entUnderCursor->getNearestPointOnEntity(mouse);
                bool isDel = Modification::tryTrim(selectedEntsCopy, m_entUnderCursor, pointOnEnt, remainEnts, deleteEnt);

                if (isDel)
                {
                    m_entUnderCursor->setVisible(false);

                    if (deleteEnt)
                    {
                        m_command.preview().entities().getEntityContainer()->addEntity(deleteEnt);
                    }

                    for (auto e : remainEnts)
                    {
                        m_command.preview().entities().getEntityContainer()->addEntity(e);
                    }
                }
            }

            m_command.preview().draw();
        }
            break;

        default:
            break;
    }
}

void ModifyTrimTool::onMouseRelease(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector mouse = view()->toGraph(e->x(), e->y());
        DmEntity* se = snapper()->catchEntity(e);

        switch (status())
        {
            case ChooseLimitEntity:
            {
                if ((se != nullptr) && (m_seleltedEnts.end() == std::find(m_seleltedEnts.begin(), m_seleltedEnts.end(), se)))
                {
                    se->setHighlighted(true);
                    view()->specifyDocumentModified();
                    view()->redraw();
                    m_seleltedEnts.emplace_back(se);
                }
            }
                break;

            case ChooseTrimEntity:
            {
                if (nullptr != se)
                {
                    DmVector pointOnEnt = se->getNearestPointOnEntity(mouse);
                    m_entToTrim = se;
                    m_entToTrim->setVisible(true);
                    m_trimPt = pointOnEnt;
                    trigger();
                    m_command.preview().clear();
                    m_command.preview().draw();
                }
            }
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        m_command.preview().clear();
        init(status() - 1);
    }
    else
    {
        // 其他按钮不做处理
    }
}

void ModifyTrimTool::updateHints()
{
    switch (status())
    {
        case ChooseLimitEntity:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyTrimCommand::tr("Select entitys"), ModifyTrimCommand::tr("Back"));
            break;

        case ChooseTrimEntity:
            GUIDIALOGFACTORY->updateMouseWidget(ModifyTrimCommand::tr("Select entity to be cut"), ModifyTrimCommand::tr("Back"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ModifyTrimTool::onKeyPress(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Enter)
    {
        if (status() == ChooseLimitEntity)
        {
            setStatus(ChooseTrimEntity);
        }
        else if (status() == ChooseTrimEntity)
        {
            // 结束命令
            init(ChooseLimitEntity - 1);
        }
        else
        {
            // 其他状态下不处理 Enter 键
        }
    }

    // 与原 ActionInterface::keyPressEvent 一样不接受该键
    e->ignore();
}

/// @brief 取消所有限制边界实体的高亮状态
void ModifyTrimTool::unhighlightLimitingEntity()
{
    for (auto& ent : m_seleltedEnts)
    {
        ent->setHighlighted(false);
    }

    view()->specifyDocumentModified();
    view()->redraw();
}

std::unique_ptr<BasePlaceTool> ModifyTrimCommand::createTool()
{
    return std::make_unique<ModifyTrimTool>(*this, document(), view());
}

const bool g_registered = CommandRegistry::instance().registerExclusiveCommand(
    DM::ActionModifyTrim, QStringLiteral("modify.trim"), exclusiveCommandFactory<ModifyTrimCommand>());
}  // namespace
