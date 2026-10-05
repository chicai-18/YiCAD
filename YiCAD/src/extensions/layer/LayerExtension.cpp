/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
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

/// @file LayerExtension.cpp
/// @brief 图层命令，取代原 ActionLayersActivate/Add/Color/Delete/Freeze/FreezeAll/Lock/
///        LockAll/Print/Rename
///
/// 逐行按钮触发的命令（激活、颜色、删除、显示/隐藏、锁定、打印）由按钮上记的图层名
/// 找到图层；没有按钮（如从命令行启动）时什么也不做，与原 Action 一致。

#include "LayerExtension.h"

#include <memory>

#include <QColorDialog>
#include <QCoreApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>

#include "CommandRegistry.h"
#include "CustomComboboxItem.h"
#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmColor.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmPen.h"
#include "DmSystem.h"
#include "EntityTable.h"
#include "IExtensionContext.h"
#include "ProxyPermissions.h"
#include "SelectionSet.h"
#include "Transaction.h"
#include "UIDialogRunner.h"
#include "UILayerDialog.h"

namespace
{
/// @brief 图层命令的实现；翻译上下文与扩展同名
class LayerCommands
{
    Q_DECLARE_TR_FUNCTIONS(LayerExtension)

public:
    /// @brief 激活按钮所在行的图层，并把选中的实体移到这个图层
    static void activate(const CommandContext& ctx)
    {
        DmLayer* layer = layerOf(ctx);
        if (!layer)
        {
            return;
        }
        DmDocument* doc = ctx.document;
        Transaction t(tr("Activate Layer").toStdString(), doc);
        t.start();
        doc->getLayerTable()->activate(layer);
        for (DmEntity* entity : allowedForProxies(ctx.selection->entities(), DmProxyFlags::LayerChange))
        {
            // 只改顶层实体的图层：子实体绘制时取到的画笔随之改变（非随层的不变）
            if (entity->getLayer() != layer)
            {
                entity->setLayer(layer);
            }
        }
        t.commit();
    }

    /// @brief 经对话框新建图层；新图层名由当前图层名末尾的数字递增得出
    static void add(const CommandContext& ctx, QWidget* parent)
    {
        if (!ctx.document)
        {
            return;
        }
        DmLayerTable* layerTable = ctx.document->getLayerTable();
        DmLayer* layer = new DmLayer(nextLayerName(layerTable));
        layer->setDocument(layerTable->getDocument());
        UILayerDialog dlg(parent, QStringLiteral("Layer Dialog"));
        dlg.setLayer(layer);
        dlg.setLayerTable(layerTable);
        dlg.getQLineEdit()->selectAll();
        if (UIDialogRunner::exec(dlg) != QDialog::Accepted)
        {
            delete layer;
            return;
        }
        dlg.updateLayer();

        Transaction t("Add Layer", ctx.document);
        t.start();
        layerTable->add(layer);
        t.commit();
    }

    /// @brief 经对话框修改当前图层（名称等）：对话框改的是当前图层的副本，确认后写回
    static void rename(const CommandContext& ctx, QWidget* parent)
    {
        if (!ctx.document)
        {
            return;
        }
        DmLayerTable* layerTable = ctx.document->getLayerTable();
        DmLayer* activeLayer = layerTable->getActive();
        if (!activeLayer)
        {
            return;
        }
        std::unique_ptr<DmLayer> layer(activeLayer->clone());
        UILayerDialog dlg(parent, QStringLiteral("Layer Dialog"));
        dlg.setLayer(layer.get());
        dlg.setLayerTable(layerTable);
        dlg.setEditLayer(true);
        if (UIDialogRunner::exec(dlg) != QDialog::Accepted)
        {
            return;
        }
        dlg.updateLayer();

        Transaction t(tr("Rename Layer").toStdString(), ctx.document);
        t.start();
        layerTable->startModify(activeLayer);
        activeLayer->setData(layer->getData());
        t.commit();
    }

    /// @brief 经颜色对话框修改按钮所在行图层的颜色
    static void color(const CommandContext& ctx)
    {
        DmLayer* layer = layerOf(ctx);
        if (!layer)
        {
            return;
        }
        const DmColor current = layer->getPen().getColor();
        const QColor initColor(current.red(), current.green(), current.blue(), current.alpha());
        const QColor color =
            QColorDialog::getColor(initColor, nullptr, tr("Select Color"), QColorDialog::DontUseNativeDialog);
        if (!color.isValid())
        {
            return;
        }
        DmPen pen = layer->getPen();
        pen.setColor(DmColor(color.red(), color.green(), color.blue()));
        Transaction t(tr("Layer Color").toStdString(), ctx.document);
        t.start();
        ctx.document->getLayerTable()->startModify(layer);
        layer->setPen(pen);
        t.commit();
    }

    /// @brief 删除按钮所在行的图层；0 层、当前图层与有实体的图层不能删除
    static void remove(const CommandContext& ctx)
    {
        DmLayer* layer = layerOf(ctx);
        if (!layer)
        {
            return;
        }
        if (!canRemove(ctx.document, layer))
        {
            const QString text = tr("The layer: %1 can not be remove. The following layers can not be removed: \n1. \"0\" layer; \n2. current layer; \n3. layer contains entities");
            QMessageBox::information(nullptr, QObject::tr("Tips"), text.arg(layer->getName()));
            return;
        }
        Transaction t(tr("Delete Layer").toStdString(), ctx.document);
        t.start();
        ctx.document->getLayerTable()->remove(layer);
        t.commit();
    }

    /// @brief 切换按钮所在行图层的显示/隐藏（冻结）
    static void freeze(const CommandContext& ctx)
    {
        DmLayer* layer = layerOf(ctx);
        if (!layer)
        {
            return;
        }
        Transaction t(tr("Freeze Layer").toStdString(), ctx.document);
        t.start();
        ctx.document->getLayerTable()->startModify(layer);
        layer->freeze(!layer->isFrozen());
        t.commit();
    }

    /// @brief 切换按钮所在行图层的锁定；锁定后取消选择该图层上的实体
    static void lock(const CommandContext& ctx)
    {
        DmLayer* layer = layerOf(ctx);
        if (!layer)
        {
            return;
        }
        Transaction t(tr("Lock Layer").toStdString(), ctx.document);
        t.start();
        ctx.document->getLayerTable()->startModify(layer);
        layer->lock(!layer->isLocked());
        t.commit();

        for (DmEntity* entity : ctx.selection->entities())
        {
            if (entity->getLayer() == layer)
            {
                ctx.selection->remove(entity);
            }
        }
    }

    /// @brief 切换按钮所在行图层是否打印
    static void print(const CommandContext& ctx)
    {
        DmLayer* layer = layerOf(ctx);
        if (!layer)
        {
            return;
        }
        Transaction t(tr("Print Layer").toStdString(), ctx.document);
        t.start();
        ctx.document->getLayerTable()->startModify(layer);
        layer->setPrint(!layer->isPrint());
        t.commit();
    }

    /// @brief 冻结或解冻全部图层
    static void freezeAll(const CommandContext& ctx, bool frozen)
    {
        if (!ctx.document)
        {
            return;
        }
        Transaction t(tr("Freeze Layer").toStdString(), ctx.document);
        t.start();
        DmLayerTable* table = ctx.document->getLayerTable();
        for (auto it = table->begin(); it != table->end(); ++it)
        {
            table->startModify(*it);
            (*it)->freeze(frozen);
        }
        t.commit();
    }

    /// @brief 锁定或解锁全部图层；锁定前先取消选择全部实体
    static void lockAll(const CommandContext& ctx, bool locked)
    {
        if (!ctx.document)
        {
            return;
        }
        if (locked)
        {
            ctx.selection->clear();
        }
        Transaction t(tr("Lock All Layers").toStdString(), ctx.document);
        t.start();
        DmLayerTable* table = ctx.document->getLayerTable();
        for (auto it = table->begin(); it != table->end(); ++it)
        {
            table->startModify(*it);
            (*it)->lock(locked);
        }
        t.commit();
    }

private:
    /// @brief 新图层的默认名：从当前图层名匹配"基本名+数字"，数字往上累加直到不重名；
    ///        当前图层是 0 层或没有名字时从"Level"起
    static QString nextLayerName(DmLayerTable* layerTable)
    {
        QString layerName = layerTable->getActive()->getName();
        if (layerName.isEmpty() || !layerName.compare("0"))
        {
            layerName = tr("Level");
        }

        QString baseName(layerName);
        int width = 1;
        int number = 0;
        const QRegularExpressionMatch match = QRegularExpression("^(.*\\D+|)(\\d+)$").match(layerName);
        if (match.hasMatch())
        {
            baseName = match.captured(1);
            if (1 < match.lastCapturedIndex())
            {
                const QString digits = match.captured(2);
                width = digits.length();
                number = digits.toInt();
            }
        }

        QString newName;
        do
        {
            newName = QString("%1%2").arg(baseName).arg(++number, width, 10, QChar('0'));
        } while (layerTable->find(newName));
        return newName;
    }

    /// @brief 触发命令的按钮所在行的图层；没有文档、不是图层行的按钮或图层已不存在时返回空
    static DmLayer* layerOf(const CommandContext& ctx)
    {
        if (!ctx.document)
        {
            return nullptr;
        }
        const QString name = ComboBoxData::layerNameOf(ctx.sender);
        return name.isEmpty() ? nullptr : ctx.document->getLayerTable()->find(name);
    }

    /// @brief 图层能否删除：不是 0 层、不是当前图层，且文档与块里都没有实体在这个图层上
    static bool canRemove(DmDocument* doc, DmLayer* layer)
    {
        if (layer->isZeroLayer() || doc->getLayerTable()->getActive() == layer)
        {
            return false;
        }
        for (DmEntity* entity : *doc->getEntityTable())
        {
            if (entity->getLayer() == layer)
            {
                return false;
            }
        }
        DmBlockTable* blocks = doc->getBlockTable();
        for (auto it = blocks->begin(); it != blocks->end(); ++it)
        {
            for (DmEntity* entity : (*it)->getEntityTable())
            {
                if (auto* container = dynamic_cast<DmEntityContainer*>(entity))
                {
                    for (DmEntity* e = container->firstEntity(DM::ResolveAll); e;
                         e = container->nextEntity(DM::ResolveAll))
                    {
                        if (e->getLayer() == layer)
                        {
                            return false;
                        }
                    }
                }
                else if (entity->getLayer() == layer)
                {
                    return false;
                }
            }
        }
        return true;
    }
};
}  // namespace

void LayerExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/layer/ts/）。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("layer"));

    // 上下文有效到 OnShutdown 返回，命令在那之后才注销（IExtension.h 的契约）
    IExtensionContext* context = &ctx;
    ctx.registerInstantCommand(QStringLiteral("ext.layer.activate"), &LayerCommands::activate, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.add"),
                               [context](const CommandContext& c) { LayerCommands::add(c, context->mainWindow()); }, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.rename"),
                               [context](const CommandContext& c) { LayerCommands::rename(c, context->mainWindow()); },
                               {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.color"), &LayerCommands::color, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.delete"), &LayerCommands::remove, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.freeze"), &LayerCommands::freeze, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.lock"), &LayerCommands::lock, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.print"), &LayerCommands::print, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.freeze_all"),
                               [](const CommandContext& c) { LayerCommands::freezeAll(c, true); }, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.defreeze_all"),
                               [](const CommandContext& c) { LayerCommands::freezeAll(c, false); }, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.lock_all"),
                               [](const CommandContext& c) { LayerCommands::lockAll(c, true); }, {});
    ctx.registerInstantCommand(QStringLiteral("ext.layer.unlock_all"),
                               [](const CommandContext& c) { LayerCommands::lockAll(c, false); }, {});
}
