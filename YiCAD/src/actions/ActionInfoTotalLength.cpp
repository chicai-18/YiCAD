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


/// @file ActionInfoTotalLength.cpp
/// @brief 总长度测量 Action 类的实现

#include <QAction>
#include "ActionInfoTotalLength.h"

#include "Debug.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionInfoTotalLength::ActionInfoTotalLength(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Info Total Length", doc, docView)
{
    actionType = DM::ActionInfoTotalLength;
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionInfoTotalLength::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发总长度计算和显示
void ActionInfoTotalLength::trigger()
{
    double totalLength = 0.0;
    auto table = pDocument->getEntityTable();
    for (auto it = table->begin(); it != table->end(); ++it)
    {
        if ((*it)->isSelected())
        {
            totalLength += (*it)->getLength();
        }
    }
    if (totalLength > 0.0)
    {
        QString len = DmUnits::formatLinear(totalLength, pDocument->getUnit(), pDocument->getLinearFormat(), pDocument->getLinearPrecision());
        GUIDIALOGFACTORY->commandMessage(tr("Total Length of selected entities: %1").arg(len));
    }
    else
    {
        GUIDIALOGFACTORY->commandMessage(tr("At least one of the selected entities cannot be measured."));
    }

    finish(false);
}
