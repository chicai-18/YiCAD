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


/// @file ActionLayersColor.cpp
/// @brief 图层颜色 Action 类的实现

#include "ActionLayersColor.h"
#include "DmLayer.h"
#include "DmDocument.h"
#include "ApplicationWindow.h"
#include "CustomComboboxItem.h"
#include "GuiDocumentView.h"
#include "SARibbonComboBox.h"
#include "MDIWindow.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param sender 发送者对象指针
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionLayersColor::ActionLayersColor(QObject* sender, DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Layer Color", doc, docView)
    , layer(nullptr)
    , theButton(sender)
{
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionLayersColor::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发图层颜色修改操作
void ActionLayersColor::trigger()
{
    ApplicationWindow* appWin = ApplicationWindow::getAppWindow();
    auto cbxItems = appWin->getLayerComboboxItems();

    // 按钮触发的action，获得按钮所在的图层，判断需要隐藏还是显示
    if (theButton && nullptr == layer)
    {
        // 当前图层按钮触发
        ComboBoxData* currentData = appWin->getCurrentLayerItem();
        if (currentData->btnColor == theButton)
        {
            setLayer(currentData->getLayerName());
        }
        // 图层下拉列表按钮触发
        else
        {
            for (auto& item : cbxItems)
            {
                auto data = item->getData();
                if (data->btnColor == theButton)
                {
                    setLayer(data->getLayerName());
                }
            }
        }
    }

    if (layer)
    {
        QColorDialog dlg;
        const DmColor dmcolor = layer->getPen().getColor();
        const QColor initColor(dmcolor.red(), dmcolor.green(), dmcolor.blue(), dmcolor.alpha());
        const QColor color = dlg.getColor(initColor, nullptr, tr("Select Color"), QColorDialog::DontUseNativeDialog);

        // 确定
        if (color.isValid())
        {
            DmColor dcolor(color.red(), color.green(), color.blue());
            DmPen dpen = layer->getPen();
            dpen.setColor(dcolor);
            Transaction t(tr("Layer Color").toStdString(), pDocument);
            t.start();
            pDocument->getLayerTable()->startModify(layer);
            layer->setPen(dpen);
            t.commit();
        }
    }
    finish();
}

/// @brief 根据图层名称设置图层
/// @param layerName 图层名称
void ActionLayersColor::setLayer(const QString& layerName)
{
    layer = pDocument->getLayerTable()->find(layerName);
}
