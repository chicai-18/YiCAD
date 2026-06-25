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


/// @file ActionLayersActivate.cpp
/// @brief 图层激活 Action 类的实现

#include "ActionLayersActivate.h"

#include <QAbstractItemView>

#include "Debug.h"
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
ActionLayersActivate::ActionLayersActivate(QObject* sender, DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Activate Layer", doc, docView)
    , layer(nullptr)
    , cbxData(nullptr)
    , theButton(sender)
{
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionLayersActivate::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发图层激活操作
void ActionLayersActivate::trigger()
{
    ApplicationWindow* appWin = ApplicationWindow::getAppWindow();
    auto cbxItems = appWin->getLayerComboboxItems();

    // 按钮触发的action，获得按钮所在的图层，判断需要隐藏还是显示
    if (theButton && nullptr == layer)
    {
        // 图层下拉列表按钮触发
        for (auto& item : cbxItems)
        {
            auto data = item->getData();
            if (data->labelName == theButton)
            {
                setLayer(data->getLayerName());
                setComboBoxData(data);
            }
        }
    }

    if (layer)
    {
        Transaction t(tr("Activate Layer").toStdString(), pDocument);
        t.start();
        pDocument->getLayerTable()->activate(layer);

        // 更新选择的实体的图层
        auto table = pDocument->getEntityTable();
        for (auto ite = table->begin(); ite != table->end(); ite++)
        {
            if ((*ite)->isSelected())
            {
                DmLayer* entityLayer = (*ite)->getLayer();
                if (entityLayer != layer)
                {
                    // 仅需修改图层，不需要update，因为仅需设置最顶级的实体图层，子实体不用设置，
                    // 绘制时获得的pen就是改变图层后的（如果不是bylayer则不变）
                    (*ite)->setLayer(layer);
                }
            }
        }
        t.commit();
    }
    finish();
}

/// @brief 获取当前图层
/// @return 图层指针
DmLayer* ActionLayersActivate::getLayer() const
{
    return layer;
}

/// @brief 设置图层
/// @param layer 图层指针
void ActionLayersActivate::setLayer(DmLayer* layer)
{
    this->layer = layer;
}

/// @brief 根据图层名称设置图层
/// @param layerName 图层名称
void ActionLayersActivate::setLayer(const QString& layerName)
{
    layer = pDocument->getLayerTable()->find(layerName);
}

/// @brief 设置下拉框数据
/// @param data 下拉框数据指针
void ActionLayersActivate::setComboBoxData(ComboBoxData* data)
{
    cbxData = data;
}
