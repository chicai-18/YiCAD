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


/// @file ActionOptionsGeneral.cpp
/// @brief 通用选项交互命令实现

#include "ActionOptionsGeneral.h"

#include "GuiDialogFactory.h"
#include "DmSettings.h"
#include "ApplicationWindow.h"
#include "MDIWindow.h"
#include "GuiDocumentView.h"

#include <QMdiSubWindow>
#include <QMdiArea>

// TODO: 建议在构造函数中设置 actionType = DM::ActionOptionsGeneral（与其他文件保持一致）

ActionOptionsGeneral::ActionOptionsGeneral(DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("General Options", doc, docView)
{
}

void ActionOptionsGeneral::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发通用选项对话框显示
///
/// 打开通用选项对话框，并从存储的设置中读取当前颜色配置，
/// 将其应用到所有打开的文档视图中。
void ActionOptionsGeneral::trigger()
{
    GUIDIALOGFACTORY->requestOptionsGeneralDialog();

    DMSETTINGS->beginGroup("Colors");
    QColor background(DMSETTINGS->readEntry("/background", Colors::BACKGROUND));
    QColor gridColor(DMSETTINGS->readEntry("/grid", Colors::GRID));
    QColor metaGridColor(DMSETTINGS->readEntry("/meta_grid", Colors::META_GRID));
    QColor selectedColor(DMSETTINGS->readEntry("/select", Colors::SELECT));
    QColor highlightColor(DMSETTINGS->readEntry("/highlight", Colors::HIGHLIGHT));
    DMSETTINGS->endGroup();

    QList<QMdiSubWindow*> windows = ApplicationWindow::getAppWindow()->getMdiArea()->subWindowList();

    for (int i = 0; i < windows.size(); ++i)
    {
        MDIWindow* m = qobject_cast<MDIWindow*>(windows.at(i));

        if (m)
        {
            GuiDocumentView* gv = m->getDocumentView();

            if (gv)
            {
                gv->setBackground(background);
                gv->setGridColor(gridColor);
                gv->setMetaGridColor(metaGridColor);
                gv->setSelectedColor(selectedColor);
                gv->setHighlightColor(highlightColor);
                gv->redraw();
            }
        }
    }

    finish();
}
