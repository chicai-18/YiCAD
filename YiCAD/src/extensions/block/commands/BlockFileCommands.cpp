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

/// @file BlockFileCommands.cpp
/// @brief BlockFileCommands 的实现；保存与导入的主体从原 Action 搬来

#include "BlockFileCommands.h"

#include <QApplication>
#include <QMessageBox>

#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmDimension.h"
#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "GuiDialogFactory.h"
#include "Transaction.h"
#include "UIBlockDelete.h"
#include "UIBlockSaveAs.h"
#include "UIFileDialog.h"

void BlockFileCommands::deleteBlocks(DmDocument* doc, QWidget* parent)
{
    UIBlockDelete dialog(parent);
    dialog.setBlockTable(doc->getBlockTable());
    dialog.setDocument(doc);
    dialog.exec();
}

void BlockFileCommands::showSaveAs(DmDocument* doc, QWidget* parent)
{
    // 原 Action 每次新建一个对话框、从不释放；关闭时释放。第三个参数原先由字符串字面量
    // 隐式转换成 true，对话框一直是模态的
    auto* dialog = new UIBlockSaveAs([doc, parent]() { saveActiveBlock(doc, parent); }, parent, true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setBlockList(doc->getBlockTable());
    dialog->show();
}

void BlockFileCommands::addBlock(DmBlockReference* ref, DmDocument* doc)
{
    for (auto entity : ref->getEntityList())
    {
        if (entity->getEntityType()
            == DM::EntityBlockReference)
        {
            auto* subRef =
                static_cast<DmBlockReference*>(entity);
            DmBlock* subBlock =
                subRef->getBlockForInsert();
            if (subBlock
                && !doc->getBlockTable()->find(
                    subBlock->getName()))
            {
                doc->getBlockTable()->add_direct(
                    subBlock);
            }
            addBlock(subRef, doc);
        }
    }
}

void BlockFileCommands::copyStyles(DmDocument* src, DmDocument* dst)
{
    // 线型
    for (auto lineType : *src->getLineTypeTable())
    {
        if (!dst->getLineTypeTable()->find(
                lineType->getLineTypeName()))
        {
            dst->getLineTypeTable()->add_direct(
                new DmLineType(lineType));
        }
    }
    // 图层
    for (auto layer : *src->getLayerTable())
    {
        if (!dst->getLayerTable()->find(
                layer->getName()))
        {
            dst->getLayerTable()->add_direct(
                layer->clone());
        }
    }
    // 文字样式
    for (auto style : *src->getTextStyleTable())
    {
        if (!dst->getTextStyleTable()->find(
                style->getName()))
        {
            dst->getTextStyleTable()->add_direct(
                style->clone());
        }
    }
    // 标注样式
    for (auto style : *src->getDimStyleTable())
    {
        if (!dst->getDimStyleTable()->find(
                style->getName()))
        {
            dst->getDimStyleTable()->add_direct(
                new DmDimensionStyle(*style));
        }
    }
}

void BlockFileCommands::saveActiveBlock(DmDocument* doc, QWidget* parent)
{
    DmBlockTable* blockTable =
        doc->getBlockTable();
    if (!blockTable)
    {
        return;
    }

    auto activeBlock = blockTable->getActive();
    if (!activeBlock)
    {
        GUIDIALOGFACTORY->commandMessage(
            tr("No block activated to save"));
        return;
    }

    // 创建临时文档用于保存
    DmDocument tmpDoc;
    tmpDoc.initDoc();

    // 复制文档级样式（块内实体可能引用这些样式）
    copyStyles(doc, &tmpDoc);

    // 将块作为整体添加到临时文档的块表
    tmpDoc.getBlockTable()->add_direct(activeBlock);

    // 递归添加块内嵌套引用的块定义
    for (auto entity : activeBlock->getEntityTable())
    {
        if (entity->getEntityType()
            == DM::EntityBlockReference)
        {
            addBlock(
                static_cast<DmBlockReference*>(entity),
                &tmpDoc);
        }
    }

    // 弹出文件保存对话框
    UIFileDialog dlg(
        parent,
        Qt::WindowFlags(),
        UIFileDialog::BlockFile);
    QString formatType;
    QString const& fn = dlg.getSaveFile(formatType);
    if (!fn.isEmpty())
    {
        QApplication::setOverrideCursor(
            QCursor(Qt::WaitCursor));
        tmpDoc.saveAs(fn, formatType);
        QApplication::restoreOverrideCursor();
    }
}

void BlockFileCommands::importBlocks(DmDocument* doc, QWidget* parent)
{
    // 1. 打开文件对话框
    UIFileDialog dlg(parent, Qt::WindowFlags(),
        UIFileDialog::BlockFile);
    QString const& fn = dlg.getOpenFile();
    if (fn.isEmpty())
    {
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // 2. 创建临时文档并加载文件（复用已有 filter 系统）
    DmDocument tempDoc;
    bool ok = tempDoc.open(fn);
    if (!ok)
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(parent, tr("Import Block"),
            tr("Failed to open file: %1").arg(fn));
        return;
    }

    DmBlockTable* srcTable = tempDoc.getBlockTable();
    DmBlockTable* dstTable = doc->getBlockTable();

    if (srcTable->count() == 0)
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::information(parent, tr("Import Block"),
            tr("No blocks found in file."));
        return;
    }

    // 3. Transaction 包裹
    Transaction t("Import Block", doc);
    t.start();

    // 3a. 导入线型（在图层之前，因为图层的画笔可能引用线型）
    DmLineTypeTable* srcLineTypeTable = tempDoc.getLineTypeTable();
    DmLineTypeTable* dstLineTypeTable = doc->getLineTypeTable();
    for (auto it = srcLineTypeTable->begin(); it != srcLineTypeTable->end(); ++it)
    {
        DmLineType* src = *it;
        if (!dstLineTypeTable->find(src->getLineTypeName()))
        {
            dstLineTypeTable->add(new DmLineType(src));
        }
    }

    // 3b. 导入图层：名字已存在则不覆盖
    DmLayerTable* srcLayerTable = tempDoc.getLayerTable();
    DmLayerTable* dstLayerTable = doc->getLayerTable();
    for (auto it = srcLayerTable->begin(); it != srcLayerTable->end(); ++it)
    {
        DmLayer* src = *it;
        if (!dstLayerTable->find(src->getName()))
        {
            DmLayer* clone = src->clone();
            dstLayerTable->add(clone);
        }
    }

    // 3c. 导入文字样式（在标注样式之前，因为标注样式引用文字样式）
    DmTextStyleTable* srcTextStyleTable = tempDoc.getTextStyleTable();
    DmTextStyleTable* dstTextStyleTable = doc->getTextStyleTable();
    for (auto it = srcTextStyleTable->begin(); it != srcTextStyleTable->end(); ++it)
    {
        DmTextStyle* src = *it;
        if (!dstTextStyleTable->find(src->getName()))
        {
            DmTextStyle* clone = src->clone();
            dstTextStyleTable->add(clone);
        }
    }

    // 3d. 导入标注样式，重定向内部的文字样式指针
    DmDimensionStyleTable* srcDimStyleTable = tempDoc.getDimStyleTable();
    DmDimensionStyleTable* dstDimStyleTable = doc->getDimStyleTable();
    for (auto it = srcDimStyleTable->begin(); it != srcDimStyleTable->end(); ++it)
    {
        DmDimensionStyle* src = *it;
        if (!dstDimStyleTable->find(src->getName()))
        {
            DmDimensionStyle* clone = new DmDimensionStyle(*src);
            DmTextStyle* srcTextStyle = clone->getDataRef().textStyle();
            if (srcTextStyle)
            {
                DmTextStyle* dstTextStyle = dstTextStyleTable->find(srcTextStyle->getName());
                if (dstTextStyle)
                {
                    clone->getDataRef().setTextStyle(dstTextStyle);
                }
            }
            dstDimStyleTable->add(clone);
        }
    }

    // 3e. 导入块定义，重定向实体的图层/样式引用
    for (auto it = srcTable->begin(); it != srcTable->end(); ++it)
    {
        DmBlock* src = *it;
        QString name = src->getName();

        // 重名处理：不导入
        if (dstTable->find(name))
        {
            continue;
        }

        // 创建新块定义
        DmBlockData newData;
        newData.name = name;
        newData.basePoint = src->getBasePoint();
        newData.frozen = src->isFrozen();
        newData.pathName = fn;
        DmBlock* newBlock = new DmBlock(doc, newData);

        // 克隆块内所有实体
        auto& srcContainer = src->getEntityTable();
        auto& dstContainer = newBlock->getEntityTable();
        for (auto entity : srcContainer)
        {
            DmEntity* clone = entity->clone();

            // 保存原始图层和画笔（setDocument 会覆盖为活动图层/画笔）
            QString savedLayerName;
            DmLayer* srcLayer = clone->getLayer(false);
            if (srcLayer)
            {
                savedLayerName = srcLayer->getName();
            }
            DmPen savedPen = clone->getPen(false);

            clone->setDocument(doc);

            // 恢复图层
            if (!savedLayerName.isEmpty())
            {
                DmLayer* dstLayer = dstLayerTable->find(savedLayerName);
                if (dstLayer)
                {
                    clone->setLayer(dstLayer);
                }
            }

            // 恢复画笔
            clone->setPen(savedPen);

            // 递归重定向文字样式、标注样式等指针
            repointEntityStyles(doc, clone);

            dstContainer.add_direct(clone);
        }

        // 通过命令系统添加（支持 undo/redo）
        dstTable->add(newBlock);
    }

    t.commit();

    QApplication::restoreOverrideCursor();

    doc->regenerate();
}

void BlockFileCommands::repointEntityStyles(DmDocument* doc, DmEntity* entity)
{
    if (!entity) return;

    // 重定向图层（子实体仍持有临时文档的图层指针）
    DmLayer* currentLayer = entity->getLayer(false);
    if (currentLayer)
    {
        DmLayer* newLayer = doc->getLayerTable()->find(currentLayer->getName());
        if (newLayer && newLayer != currentLayer)
        {
            entity->setLayer(newLayer);
        }
    }

    // 重定向画笔的线型指针
    DmPen currentPen = entity->getPen(false);
    DmLineType* currentLineType = currentPen.getLineType();
    if (currentLineType
        && currentLineType != DmLineTypeTable::ByLayer
        && currentLineType != DmLineTypeTable::ByBlock)
    {
        DmLineType* newLineType = doc->getLineTypeTable()->find(currentLineType->getLineTypeName());
        if (newLineType && newLineType != currentLineType)
        {
            currentPen.setLineType(newLineType);
            entity->setPen(currentPen);
        }
    }

    // 重定向 DmText 的文字样式
    if (auto* text = dynamic_cast<DmText*>(entity))
    {
        DmTextStyle* srcStyle = text->getStyle();
        if (srcStyle)
        {
            DmTextStyle* dstStyle = doc->getTextStyleTable()->find(srcStyle->getName());
            if (dstStyle && dstStyle != srcStyle)
            {
                text->setStyle(dstStyle);
            }
        }
    }

    // 重定向 DmMText 的文字样式
    if (auto* mtext = dynamic_cast<DmMText*>(entity))
    {
        DmTextStyle* srcStyle = mtext->getStyle();
        if (srcStyle)
        {
            DmTextStyle* dstStyle = doc->getTextStyleTable()->find(srcStyle->getName());
            if (dstStyle && dstStyle != srcStyle)
            {
                mtext->setStyle(dstStyle);
            }
        }
    }

    // 重定向 DmDimension 的标注样式
    if (auto* dim = dynamic_cast<DmDimension*>(entity))
    {
        DmDimensionStyle* srcDimStyle = dim->getStyle();
        if (srcDimStyle)
        {
            DmDimensionStyle* dstDimStyle = doc->getDimStyleTable()->find(srcDimStyle->getName());
            if (dstDimStyle && dstDimStyle != srcDimStyle)
            {
                dim->getDataRef().pDimStyle = dstDimStyle;
            }
        }
    }

    // 递归处理子实体
    for (auto* sub : entity->getSubEntities())
    {
        repointEntityStyles(doc, sub);
    }
}
