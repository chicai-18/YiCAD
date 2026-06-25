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


/// @file ActionBlocksInsert.cpp
/// @brief 块插入动作类实现文件

#include "ActionBlocksInsert.h"

#include <QMouseEvent>
#include <QAction>

#include "DmBlock.h"
#include "DmDocument.h"
#include "DmBlockReference.h"
#include "DmAttribute.h"
#include "DmAttributeDefinition.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 构造函数
/// @param[in] doc 文档指针
/// @param[in] docView 文档视图指针
ActionBlocksInsert::ActionBlocksInsert(
    DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Blocks Insert", doc, docView)
    , block(nullptr)
    , lastStatus(eSetUndefined)
{
    actionType = DM::ActionBlocksInsert;
    reset();  // 初始化数据成员
}

/// @brief 析构函数
ActionBlocksInsert::~ActionBlocksInsert() = default;

/// @brief 初始化动作
/// @param[in] status 初始状态
void ActionBlocksInsert::init(int status)
{
    PreviewActionInterface::init(status);

    reset();

    if (pDocument)
    {
        block = pDocument->getBlockTable()->getActive();
        if (block)
        {
            QString blockName = block->getName();
            data->name = blockName;
            //TODO :块待优化
            //if (pDocument->getEntityType() == DM::EntityBlock)
            {
                QStringList bnChain = block->findNestedInsert(blockName);
                if (!bnChain.empty())
                {
                    GUIDIALOGFACTORY->commandMessage(
                        blockName
                        + tr(" has nested insert of "
                             "current block in:\n")
                        + bnChain.join("->")
                        + tr("\nThis block cannot be "
                             "inserted."));
                    finish(false);
                }
            }
        }
        else
        {
            finish(false);
        }
    }
}

/// @brief 重置块参照数据为默认值
void ActionBlocksInsert::reset()
{
    constexpr double DEFAULT_SCALE = 1.0;
    constexpr double DEFAULT_ANGLE = 0.0;
    constexpr int DEFAULT_COUNT = 1;

    data.reset(new DmBlockReferenceData(
        "", DmVector(0.0, 0.0),
        DmVector(DEFAULT_SCALE, DEFAULT_SCALE),
        DEFAULT_ANGLE, DEFAULT_COUNT, DEFAULT_COUNT,
        DmVector(DEFAULT_SCALE, DEFAULT_SCALE),
        nullptr, DM::Update));
}

/// @brief 触发动作执行，创建块参照并添加到文档
void ActionBlocksInsert::trigger()
{
    deletePreview();

    if (block)
    {
        std::list<DmAttribute*> attrs;
        if (block->hasAttributeDefinitions())
        {
            std::list<DmAttributeDefinition*> attrDefs = block->getAttributeDefinitions();
            GUIDIALOGFACTORY->requestBlockEditAttributeDialog(block->getName(), attrDefs, attrs);
        }
        for (auto attr : attrs)
        {
            attr->move(data->insertionPoint);
            attr->scale(data->insertionPoint, data->scaleFactor);
            attr->rotateAngle(data->insertionPoint, data->angle);
        }

        Transaction t("Insert Block", pDocument);
        t.start();

        data->updateMode = DM::Update;
        DmBlockReference* ref = new DmBlockReference(nullptr, *data);
        ref->addAttributes(attrs);
        ref->setDocument(pDocument);
        ref->update();
        pDocument->getEntityTable()->add(ref);

        t.commit();
    }

    docView->redraw();
}

/// @brief 鼠标移动事件处理，更新预览
/// @param[in] e 鼠标事件指针
void ActionBlocksInsert::mouseMoveEvent(QMouseEvent* e)
{
    switch (getStatus())
    {
        case eSetTargetPoint:
            data->insertionPoint = snapPoint(e);
            if (block)
            {
                deletePreview();
                data->updateMode = DM::PreviewUpdate;
                DmBlockReference* ref = new DmBlockReference(nullptr, *data);
                ref->setDocument(pDocument);
                std::list<DmAttribute*> previewAttrs;
                if (block->hasAttributeDefinitions())
                {
                    std::list<DmAttributeDefinition*> attrDefs = block->getAttributeDefinitions();
                    for (auto def : attrDefs)
                    {
                        DmAttribute* attr = new DmAttribute(nullptr, def->getData(), AttributeData(def->getTag()));
                        attr->setPen(DmPen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous));
                        attr->setLayer(nullptr);
                        attr->setText(def->getText());
                        attr->update();
                        attr->move(data->insertionPoint);
                        attr->scale(data->insertionPoint, data->scaleFactor);
                        attr->rotateAngle(data->insertionPoint, data->angle);
                        previewAttrs.emplace_back(attr);
                    }
                    ref->addAttributes(previewAttrs);
                }
                ref->update();
                preview->addEntity(ref);
                data->updateMode = DM::Update;
                drawPreview();
            }
            break;

        default:
            break;
    }
}

/// @brief 鼠标释放事件处理
/// @param[in] e 鼠标事件指针
void ActionBlocksInsert::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        init(getStatus() - 1);
    }
}

/// @brief 坐标事件处理，设置插入点并触发
/// @param[in] e 坐标事件指针
void ActionBlocksInsert::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }

    data->insertionPoint = e->getCoordinate();
    trigger();
}

/// @brief 命令事件处理，处理命令行输入
/// @param[in] e 命令事件指针
void ActionBlocksInsert::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus())
    {
        case eSetTargetPoint:
            if (checkCommand("angle", c))
            {
                deletePreview();
                lastStatus = (EStatus)getStatus();
                setStatus(eSetAngle);
            }
            else if (checkCommand("factor", c))
            {
                deletePreview();
                lastStatus = (EStatus)getStatus();
                setStatus(eSetFactor);
            }
            else if (checkCommand("columns", c))
            {
                deletePreview();
                lastStatus = (EStatus)getStatus();
                setStatus(eSetColumns);
            }
            else if (checkCommand("rows", c))
            {
                deletePreview();
                lastStatus = (EStatus)getStatus();
                setStatus(eSetRows);
            }
            else if (checkCommand("columnspacing", c))
            {
                deletePreview();
                lastStatus = (EStatus)getStatus();
                setStatus(eSetColumnSpacing);
            }
            else if (checkCommand("rowspacing", c))
            {
                deletePreview();
                lastStatus = (EStatus)getStatus();
                setStatus(eSetRowSpacing);
            }
            break;

        case eSetAngle:
        {
            bool ok;
            double a = Math2d::eval(c, &ok);
            if (ok)
            {
                data->angle = Math2d::deg2rad(a);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
            break;
        }

        case eSetFactor:
        {
            bool ok;
            double f = Math2d::eval(c, &ok);
            if (ok)
            {
                setFactor(f);
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
            break;
        }

        case eSetColumns:
        {
            bool ok;
            int cols = static_cast<int>(
                Math2d::eval(c, &ok));
            if (ok)
            {
                data->cols = cols;
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
            break;
        }

        case eSetRows:
        {
            bool ok;
            int rows = static_cast<int>(
                Math2d::eval(c, &ok));
            if (ok)
            {
                data->rows = rows;
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
            break;
        }

        case eSetColumnSpacing:
        {
            bool ok;
            double cs = Math2d::eval(c, &ok);
            if (ok)
            {
                data->spacing.x = cs;
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
            break;
        }

        case eSetRowSpacing:
        {
            bool ok;
            double rs = Math2d::eval(c, &ok);
            if (ok)
            {
                data->spacing.y = rs;
            }
            else
            {
                GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            GUIDIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
            break;
        }

        default:
            break;
    }
}

/// @brief 获取角度值
/// @return 角度值（弧度）
double ActionBlocksInsert::getAngle() const
{
    return data->angle;
}

/// @brief 设置角度值
/// @param[in] a 角度值（弧度）
void ActionBlocksInsert::setAngle(double a)
{
    data->angle = a;
}

/// @brief 获取比例因子
/// @return 比例因子
double ActionBlocksInsert::getFactor() const
{
    return data->scaleFactor.x;
}

/// @brief 设置比例因子
/// @param[in] f 比例因子
void ActionBlocksInsert::setFactor(double f)
{
    data->scaleFactor = DmVector(f, f);
}

/// @brief 获取列数
/// @return 列数
int ActionBlocksInsert::getColumns() const
{
    return data->cols;
}

/// @brief 设置列数
/// @param[in] c 列数
void ActionBlocksInsert::setColumns(int c)
{
    data->cols = c;
}

/// @brief 获取行数
/// @return 行数
int ActionBlocksInsert::getRows() const
{
    return data->rows;
}

/// @brief 设置行数
/// @param[in] r 行数
void ActionBlocksInsert::setRows(int r)
{
    data->rows = r;
}

/// @brief 获取列间距
/// @return 列间距
double ActionBlocksInsert::getColumnSpacing() const
{
    return data->spacing.x;
}

/// @brief 设置列间距
/// @param[in] cs 列间距
void ActionBlocksInsert::setColumnSpacing(double cs)
{
    data->spacing.x = cs;
}

/// @brief 获取行间距
/// @return 行间距
double ActionBlocksInsert::getRowSpacing() const
{
    return data->spacing.y;
}

/// @brief 设置行间距
/// @param[in] rs 行间距
void ActionBlocksInsert::setRowSpacing(double rs)
{
    data->spacing.y = rs;
}

/// @brief 获取可用命令列表
/// @return 可用命令字符串列表
QStringList ActionBlocksInsert::getAvailableCommands()
{
    QStringList cmd;

    switch (getStatus())
    {
        case eSetTargetPoint:
            cmd += command("angle");
            cmd += command("factor");
            cmd += command("columns");
            cmd += command("rows");
            cmd += command("columnspacing");
            cmd += command("rowspacing");
            break;
        default:
            break;
    }

    return cmd;
}

/// @brief 判断是否为子动作
/// @return true 表示是子动作
bool ActionBlocksInsert::isSubAction()
{
    return true;
}

/// @brief 显示选项对话框
void ActionBlocksInsert::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true);
}

/// @brief 隐藏选项对话框
void ActionBlocksInsert::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 更新鼠标按钮提示
void ActionBlocksInsert::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case eSetTargetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify reference point"), tr("Cancel"));
            break;
        case eSetAngle:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter angle:"), "");
            break;
        case eSetFactor:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter factor:"), "");
            break;
        case eSetColumns:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter columns:"), "");
            break;
        case eSetRows:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter rows:"), "");
            break;
        case eSetColumnSpacing:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter column spacing:"), "");
            break;
        case eSetRowSpacing:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Enter row spacing:"), "");
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标
void ActionBlocksInsert::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}

// 文件结束
