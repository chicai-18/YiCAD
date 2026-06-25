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


/// @file ActionLayersFreeze.cpp
/// @brief 图层冻结/解冻动作类实现文件

#include "ActionLayersFreeze.h"

#include "ApplicationWindow.h"
#include "CustomComboboxItem.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "GuiDocumentView.h"
#include "MDIWindow.h"
#include "SARibbonComboBox.h"
#include "Transaction.h"


/// @brief 构造函数
/// @param sender 触发此动作的按钮对象
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionLayersFreeze::ActionLayersFreeze(QObject* sender, DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Freeze Layer", doc, docView)
    , layer(nullptr)
    , toBeOn(false)
    , theButton(sender)
{
}

/// @brief 初始化动作
/// @param status 状态参数，默认为0
void ActionLayersFreeze::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 执行冻结/解冻操作
void ActionLayersFreeze::trigger()
{
    ApplicationWindow* appWin = ApplicationWindow::getAppWindow();
    auto cbxItems = appWin->getLayerComboboxItems();

    // 按钮触发的action，获得按钮所在的图层，判断需要冻结还是解冻
    if (theButton && nullptr == layer)
    {
        // 当前图层按钮触发
        ComboBoxData* currentData = appWin->getCurrentLayerItem();
        if (currentData->btnOn == theButton)
        {
            setLayer(currentData->getLayerName());
            setToBeOn(!currentData->isOn);
        }
        // 图层下拉列表按钮触发
        else
        {
            for (auto& item : cbxItems)
            {
                auto data = item->getData();
                if (data->btnOn == theButton)
                {
                    setLayer(data->getLayerName());
                    setToBeOn(!data->isOn);
                }
            }
        }
    }

    // 冻结/解冻图层
    if (layer)
    {
        Transaction t(tr("Freeze Layer").toStdString(), pDocument);
        t.start();
        pDocument->getLayerTable()->startModify(layer);
        layer->freeze(!toBeOn);
        t.commit();
    }
    finish();
}

/// @brief 设置要操作的图层
/// @param layerName 图层名称
void ActionLayersFreeze::setLayer(const QString& layerName)
{
    layer = pDocument->getLayerTable()->find(layerName);
}

/// @brief 设置目标状态（开启或关闭）
/// @param on true表示设为开启，false表示关闭
void ActionLayersFreeze::setToBeOn(bool on)
{
    toBeOn = on;
}
