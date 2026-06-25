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

/// @file UICloudLineOptions.cpp
/// @brief 云线选项控件实现

#include "UICloudLineOptions.h"

#include "ActionDrawCloudLineRectangle.h"
#include "ActionDrawCloudLinePolygon.h"
#include "ActionDrawCloudLineFree.h"
#include "ui_UICloudLineOptions.h"
#include "Debug.h"
#include "DmSettings.h"

UICloudLineOptions::UICloudLineOptions(QWidget* parent /*= 0*/, Qt::WindowFlags fl /*= 0*/)
    : QWidget(parent, fl)
    , ui(new Ui::UICloudLineOptions{})
{
    ui->setupUi(this);
}

UICloudLineOptions::~UICloudLineOptions()
{
    destroy();
}

void UICloudLineOptions::setAction(ActionInterface* a, bool update)
{
    if (a && (a->getEntityType() == DM::ActionCloudLineRectangle || a->getEntityType() == DM::ActionCloudLinePolygon ||
        a->getEntityType() == DM::ActionCloudLineFree))
    {
        action = a;
        if (a->getEntityType() == DM::ActionCloudLineRectangle)
        {
            ActionDrawCloudLineRectangle* actionRect = dynamic_cast<ActionDrawCloudLineRectangle*>(a);
            QString strMinLen, strMaxLen;
            if (update)
            {
                strMinLen = QString("%1").arg(actionRect->getMinLength());
                strMaxLen = QString("%1").arg(actionRect->getMaxLength());
            }
            else
            {
                DMSETTINGS->beginGroup("/Draw");
                strMinLen = DMSETTINGS->readEntry("/CloudLineMinLen", "50.0");
                strMaxLen = DMSETTINGS->readEntry("/CloudLineMaxLen", "100.0");
                DMSETTINGS->endGroup();
                if (strMinLen.isEmpty())
                {
                    strMinLen = "50";
                }
                if (strMaxLen.isEmpty())
                {
                    strMaxLen = "100";
                }
                actionRect->setMinLength(strMinLen.toDouble());
                actionRect->setMaxLength(strMaxLen.toDouble());
            }
            ui->leMinLen->setText(strMinLen);
            ui->leMaxLen->setText(strMaxLen);
            ui->chkReverse->hide();
            ui->btnUndo->hide();
        }
        else if (a->getEntityType() == DM::ActionCloudLinePolygon)
        {
            ActionDrawCloudLinePolygon* actionPoly = dynamic_cast<ActionDrawCloudLinePolygon*>(a);
            QString strMinLen, strMaxLen;
            if (update)
            {
                strMinLen = QString("%1").arg(actionPoly->getMinLength());
                strMaxLen = QString("%1").arg(actionPoly->getMaxLength());
            }
            else
            {
                DMSETTINGS->beginGroup("/Draw");
                strMinLen = DMSETTINGS->readEntry("/CloudLineMinLen", "50.0");
                strMaxLen = DMSETTINGS->readEntry("/CloudLineMaxLen", "100.0");
                DMSETTINGS->endGroup();
                if (strMinLen.isEmpty())
                {
                    strMinLen = "50";
                }
                if (strMaxLen.isEmpty())
                {
                    strMaxLen = "100";
                }
                actionPoly->setMinLength(strMinLen.toDouble());
                actionPoly->setMaxLength(strMaxLen.toDouble());
            }
            ui->leMinLen->setText(strMinLen);
            ui->leMaxLen->setText(strMaxLen);
            ui->chkReverse->hide();
            ui->btnUndo->show();
        }
        else if (a->getEntityType() == DM::ActionCloudLineFree)
        {
            ActionDrawCloudLineFree* actionFree = dynamic_cast<ActionDrawCloudLineFree*>(a);
            bool isReversed;
            if (update)
            {
                isReversed = actionFree->getReversed();
            }
            else
            {
                DMSETTINGS->beginGroup("/Draw");
                QString reversed = DMSETTINGS->readEntry("/CloudLineReversed", "0");
                DMSETTINGS->endGroup();
                isReversed = (reversed == "0") ? false : true;
                actionFree->setReversed(isReversed);
            }
            ui->lblMaxlen->hide();
            ui->lblMinLen->hide();
            ui->leMaxLen->hide();
            ui->leMinLen->hide();
            ui->chkReverse->setChecked(isReversed);
            ui->chkReverse->show();
            ui->btnUndo->hide();
        }
    }
    else
    {
        action = nullptr;
    }
}

void UICloudLineOptions::undo()
{
    if (action)
    {
        if (action->getEntityType() == DM::ActionCloudLinePolygon)
        {
            ActionDrawCloudLinePolygon* actionPoly = dynamic_cast<ActionDrawCloudLinePolygon*>(action);
            actionPoly->undo();
        }
    }
}

void UICloudLineOptions::updateMinLength(const QString& s)
{
    if (action)
    {
        if (action->getEntityType() == DM::ActionCloudLineRectangle)
        {
            ActionDrawCloudLineRectangle* actionRect = dynamic_cast<ActionDrawCloudLineRectangle*>(action);
            actionRect->setMinLength(s.toDouble());
        }
        else if (action->getEntityType() == DM::ActionCloudLinePolygon)
        {
            ActionDrawCloudLinePolygon* actionPoly = dynamic_cast<ActionDrawCloudLinePolygon*>(action);
            actionPoly->setMinLength(s.toDouble());
        }
    }
}

void UICloudLineOptions::updateMaxLength(const QString& s)
{
    if (action)
    {
        if (action->getEntityType() == DM::ActionCloudLineRectangle)
        {
            ActionDrawCloudLineRectangle* actionRect = dynamic_cast<ActionDrawCloudLineRectangle*>(action);
            actionRect->setMaxLength(s.toDouble());
        }
        else if (action->getEntityType() == DM::ActionCloudLinePolygon)
        {
            ActionDrawCloudLinePolygon* actionPoly = dynamic_cast<ActionDrawCloudLinePolygon*>(action);
            actionPoly->setMaxLength(s.toDouble());
        }
    }
}

void UICloudLineOptions::updateReverse(int reverse)
{
    constexpr int REVERSED_CHECKED_VALUE = 2;

    if (action)
    {
        ActionDrawCloudLineFree* actionFree = dynamic_cast<ActionDrawCloudLineFree*>(action);
        actionFree->setReversed(reverse == REVERSED_CHECKED_VALUE);
    }
}

/// @brief 语言切换时刷新界面文本
void UICloudLineOptions::languageChange()
{
    ui->retranslateUi(this);
}

void UICloudLineOptions::destroy()
{
    DMSETTINGS->beginGroup("/Draw");
    if (action)
    {
        if (action->getEntityType() == DM::ActionCloudLineRectangle || action->getEntityType() == DM::ActionCloudLinePolygon)
        {
            DMSETTINGS->writeEntry("/CloudLineMinLen", ui->leMinLen->text());
            DMSETTINGS->writeEntry("/CloudLineMaxLen", ui->leMaxLen->text());
        }
        else
        {
            DMSETTINGS->writeEntry("/CloudLineReversed", ui->chkReverse->isChecked() ? 1 : 0);
        }
    }
    DMSETTINGS->endGroup();
}
