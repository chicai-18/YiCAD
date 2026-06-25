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


/// @file ActionModifyTrim.cpp
/// @brief 修剪实体交互命令实现

#include "ActionModifyTrim.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "Preview.h"

ActionModifyTrim::ActionModifyTrim(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Trim Entity", doc, docView)
    , m_entToTrim(nullptr)
    , m_entUnderCursor(nullptr)
    , m_trimPt{}
{
    setActionType(DM::ActionModifyTrim);
}

ActionModifyTrim::~ActionModifyTrim()
{
    unhighlightLimitingEntity();
}

void ActionModifyTrim::init(int status)
{
    snapMode.clear();
    snapMode.restriction = DM::RestrictNothing;
    PreviewActionInterface::init(status);
}

void ActionModifyTrim::finish(bool updateTB)
{
    PreviewActionInterface::finish(updateTB);
    unhighlightLimitingEntity();
}

/// @brief 执行修剪操作
///
/// 使用选中的边界实体对目标实体执行修剪，
/// 修剪点由 m_trimPt 指定。
void ActionModifyTrim::trigger()
{
    if ((m_seleltedEnts.size() > 0) && (m_entToTrim != nullptr))
    {
        Modification m(docView);
        bool res = m.trim(m_seleltedEnts, m_entToTrim, m_trimPt);

        if (res)
        {
            m_entToTrim = nullptr;
            m_trimPt = {};
            m_entUnderCursor = nullptr;
        }

        updateMouseButtonHints();
    }
}

void ActionModifyTrim::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    DmEntity* se = catchEntity(e);

    switch (getStatus())
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
                docView->specifyDocumentModified();
                docView->redraw();
            }

            // 设置当前光标下的实体
            m_entUnderCursor = se;

            if (nullptr != m_entUnderCursor)
            {
                m_entUnderCursor->setHighlighted(true);
                docView->specifyDocumentModified();
                docView->redraw();
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
            deletePreview();

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
                        preview->getEntityContainer()->addEntity(deleteEnt);
                    }

                    for (auto e : remainEnts)
                    {
                        preview->getEntityContainer()->addEntity(e);
                    }
                }
            }

            drawPreview();
        }
            break;

        default:
            break;
    }
}

void ActionModifyTrim::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector mouse = docView->toGraph(e->x(), e->y());
        DmEntity* se = catchEntity(e);

        switch (getStatus())
        {
            case ChooseLimitEntity:
            {
                if ((se != nullptr) && (m_seleltedEnts.end() == std::find(m_seleltedEnts.begin(), m_seleltedEnts.end(), se)))
                {
                    se->setHighlighted(true);
                    docView->specifyDocumentModified();
                    docView->redraw();
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
                    deletePreview();
                    drawPreview();
                }
            }
                break;

            default:
                break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
    }
    else
    {
        // 其他按钮不做处理
    }
}

void ActionModifyTrim::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case ChooseLimitEntity:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select entitys"), tr("Back"));
            break;

        case ChooseTrimEntity:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Select entity to be cut"), tr("Back"));
            break;

        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void ActionModifyTrim::updateMouseCursor()
{
    if ((getStatus() == ChooseLimitEntity) || (getStatus() == ChooseTrimEntity))
    {
        docView->setMouseCursor(DM::SelectCursor);
    }
    else
    {
        docView->setMouseCursor(DM::ArrowCursor);
    }
}

void ActionModifyTrim::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Enter)
    {
        if (getStatus() == ChooseLimitEntity)
        {
            setStatus(ChooseTrimEntity);
        }
        else if (getStatus() == ChooseTrimEntity)
        {
            // 结束命令
            init(ChooseLimitEntity - 1);
            updateMouseCursor();
        }
        else
        {
            // 其他状态下不处理 Enter 键
        }
    }

    ActionInterface::keyPressEvent(e);
}

/// @brief 取消所有限制边界实体的高亮状态
void ActionModifyTrim::unhighlightLimitingEntity()
{
    for (auto& ent : m_seleltedEnts)
    {
        ent->setHighlighted(false);
    }

    docView->specifyDocumentModified();
    docView->redraw();
}
