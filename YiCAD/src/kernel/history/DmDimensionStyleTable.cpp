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

/// @file DmDimensionStyleTable.cpp
/// @brief 标注样式表实现

#include "DmDimensionStyleTable.h"
#include "DimensionStyleTableCmd.h"
#include <algorithm>
#include "DmSolid.h"
#include "DmBlock.h"
#include "DmLine.h"
#include "DmCircle.h"
#include "DmArc.h"
#include "Math2d.h"
#include "DmDocument.h"

DmDimensionStyleTable::~DmDimensionStyleTable()
{
    for (size_t i = 0; i < m_styles.size(); i++)
    {
        delete m_styles.at(i);
    }
    m_styles.clear();
}

/// @brief 设置关联文档并初始化
void DmDimensionStyleTable::setDocument(DmDocument* pDocument)
{
    ITable::setDocument(pDocument);
    // 做一些初始化操作，初始化箭头块，添加一个标注样式
    initArrowBlocks();
    auto pStandardStyle = pDocument->getTextStyleTable()->find(DEFAULT_TEXTSTYLE_NAME);
    auto dimStyle = new DmDimensionStyle(DEFAULT_DIMSTYLE_NAME, pStandardStyle);
    dimStyle->setDocument(pDocument);
    add_direct(dimStyle);
    m_pActiveStyle = dimStyle;
}

/// @brief 开始修改标注样式
void DmDimensionStyleTable::startModify(DmObject *e)
{
    DmDimensionStyle* ent = static_cast<DmDimensionStyle*>(e);
    DimensionStyleTableModifyCmd* cmd = new DimensionStyleTableModifyCmd(this, ent);
    m_pDoc->getCmdManager()->addToCurrentCmd(cmd);
}

/// @brief 添加标注样式
void DmDimensionStyleTable::add(DmDimensionStyle *e)
{
    if (!m_pDoc)
        return;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    DimensionStyleTableAddCmd* cmd = new DimensionStyleTableAddCmd(this, e);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 通过 id 移除标注样式
void DmDimensionStyleTable::remove(DmId id)
{
    if (!m_pDoc)
        return;
    auto it = m_dimStyleMap.find(id);
    if (it == m_dimStyleMap.end())
        return;
    DimensionStyleTableRemoveCmd* cmd = new DimensionStyleTableRemoveCmd(this, it->second);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 移除标注样式
void DmDimensionStyleTable::remove(DmDimensionStyle *e)
{
    remove(e->getId());
}

/// @brief 查找标注样式（不能获得已删除的）
DmDimensionStyle *DmDimensionStyleTable::find(const QString &name)
{
    for (auto& style : m_styles)
    {
        if (!style->isErased() && style->getName() == name)
        {
            return style;
        }
    }
    return nullptr;
}

/// @brief 通过 id 查找标注样式
DmDimensionStyle *DmDimensionStyleTable::find(const DmId &id)
{
    auto it = m_dimStyleMap.find(id);
    if (it == m_dimStyleMap.end())
        return nullptr;
    return it->second;
}

/// @brief 直接添加（不产生命令）
bool DmDimensionStyleTable::add_direct(DmDimensionStyle *e)
{
    if (!m_pDoc)
        return false;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    auto it = m_dimStyleMap.find(id);
    if (it != m_dimStyleMap.end())
        return false;
    m_dimStyleMap[id] = e;
    m_styles.emplace_back(e);
    return true;
}

/// @brief 直接删除（不产生命令）
bool DmDimensionStyleTable::remove_direct(DmDimensionStyle *e)
{
    auto it2 = std::find(m_styles.begin(), m_styles.end(), e);
    m_styles.erase(it2);
    m_dimStyleMap.erase(e->getId());
    m_pDoc->getIdManager()->removeID(e->getId());
    delete e;
    return true;
}

/// @brief 获取当前激活的标注样式
DmDimensionStyle *DmDimensionStyleTable::getActive()
{
    return m_pActiveStyle;
}

/// @brief 按名称激活标注样式
void DmDimensionStyleTable::activate(const QString &name)
{
    activate(find(name));
}

/// @brief 激活标注样式
void DmDimensionStyleTable::activate(DmDimensionStyle *dimStyle)
{
    DimensionStyleTableActivateCmd* cmd = new DimensionStyleTableActivateCmd(this, dimStyle);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 直接激活标注样式
void DmDimensionStyleTable::activate_direct(DmDimensionStyle *textStyle)
{
    m_pActiveStyle = textStyle;
}

DmDimensionStyleTable::iterator DmDimensionStyleTable::begin()
{
    return DmDimensionStyleTable::iterator(m_styles.begin(), m_styles.end());
}

DmDimensionStyleTable::iterator DmDimensionStyleTable::end()
{
    return DmDimensionStyleTable::iterator(m_styles.end());
}

unsigned int DmDimensionStyleTable::count() const
{
    unsigned int size = 0;
    for (auto style : m_styles)
    {
        if (!style->isErased())
        {
            size++;
        }
    }
    return size;
}

/// @brief 获取箭头块表
DmBlockTable* DmDimensionStyleTable::getArrowBlocks()
{
    return m_pDoc->getBlockTable();
}

/// @brief 初始化各类箭头块
void DmDimensionStyleTable::initArrowBlocks()
{
    // 以右侧箭头（第二箭头）为标准，箭头顶部坐标为(0,0)，一般箭头整体宽度为1.0（小点等除外）
    DmPen pen(DmColor(DM::FlagByBlock), DM::WidthByBlock, DmLineTypeTable::ByBlock);
    {
        // 实心闭合
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::ClosedFilled), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        std::vector<DmVector> corners{ DmVector(0.0,0.0), DmVector{-1.0,0.3}, DmVector{-1.0, -0.3} };
        SolidData solidData(corners);
        DmSolid* solid = new DmSolid(nullptr, solidData);
        solid->setPen(pen);
        blk->getEntityTable().add_direct(solid);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 空心闭合
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::ClosedBlank), DmVector(0.0, 0.0), false);
        DmBlock*  blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmLine* line1 = new DmLine(DmVector(0.0, 0.0), DmVector{ -1.0,0.3 });
        DmLine* line2 = new DmLine(DmVector(-1.0, 0.3), DmVector{ -1.0,-0.3 });
        DmLine* line3 = new DmLine(DmVector(-1.0, -0.3), DmVector{ 0.0, 0.0 });
        line1->setPen(pen);
        line2->setPen(pen);
        line3->setPen(pen);
        blk->getEntityTable().add_direct(line1);
        blk->getEntityTable().add_direct(line2);
        blk->getEntityTable().add_direct(line3);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 闭合
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::Closed), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmLine* line1 = new DmLine(DmVector(0.0, 0.0), DmVector{ -1.0,0.3 });
        DmLine* line2 = new DmLine(DmVector(-1.0, 0.3), DmVector{ -1.0,-0.3 });
        DmLine* line3 = new DmLine(DmVector(-1.0, -0.3), DmVector{ 0.0, 0.0 });
        line1->setPen(pen);
        line2->setPen(pen);
        line3->setPen(pen);
        blk->getEntityTable().add_direct(line1);
        blk->getEntityTable().add_direct(line2);
        blk->getEntityTable().add_direct(line3);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 点
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::Dot), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        double r = 0.5;
        createFillCircle(blk, pen, DmVector(0.0, 0.0), r);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 建筑标记
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::ArchitecturalTick), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmVector slashDir = DmVector(1.0, 1.0).normalize();
        DmVector slashVerticalDir = DmVector(slashDir).rotate(M_PI / 2);
        DmVector slashStartPt = DmVector(0.0, 0.0).move(-slashDir * 0.5 * sqrt(2.0));
        DmVector slashEndPt = DmVector(0.0, 0.0).move(slashDir * 0.5 * sqrt(2.0));
        double slashThickness = 0.15;
        DmVector slashStartPtOffset1 = DmVector(slashStartPt).move(slashVerticalDir * slashThickness / 2.0);
        DmVector slashStartPtOffset2 = DmVector(slashStartPt).move(-slashVerticalDir * slashThickness / 2.0);
        DmVector slashEndPtOffset1 = DmVector(slashEndPt).move(slashVerticalDir * slashThickness / 2.0);
        DmVector slashEndPtOffset2 = DmVector(slashEndPt).move(-slashVerticalDir * slashThickness / 2.0);
        std::vector<DmVector> corners{ slashStartPtOffset1 ,slashStartPtOffset2,slashEndPtOffset2,slashEndPtOffset1 };
        SolidData solidData(corners);
        DmSolid* solid = new DmSolid(nullptr, solidData);
        solid->setPen(pen);
        blk->getEntityTable().add_direct(solid);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 倾斜
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::Oblique), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmVector slashDir = DmVector(1.0, 1.0).normalize();
        DmVector slashStartPt = DmVector(0.0, 0.0).move(-slashDir * 0.5 * sqrt(2.0));
        DmVector slashEndPt = DmVector(0.0, 0.0).move(slashDir * 0.5 * sqrt(2.0));
        DmLine* line = new DmLine(slashStartPt, slashEndPt);
        line->setPen(pen);
        blk->getEntityTable().add_direct(line);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 打开
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::Open), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmLine* line1 = new DmLine(DmVector(0.0, 0.0), DmVector{ -1.0,0.3 });
        DmLine* line3 = new DmLine(DmVector(-1.0, -0.3), DmVector{ 0.0, 0.0 });
        blk->getEntityTable().add_direct(line1);
        blk->getEntityTable().add_direct(line3);
        line1->setPen(pen);
        line3->setPen(pen);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 指示原点
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::OriginIndicator), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        CircleData cdata(DmVector(0.0, 0.0), 0.5);
        DmCircle* circle = new DmCircle(nullptr, cdata);
        circle->setPen(pen);
        blk->getEntityTable().add_direct(circle);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 指示原点2
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::OriginIndicator2), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        CircleData cdata1(DmVector(0.0, 0.0), 0.5);
        DmCircle* circle1 = new DmCircle(nullptr, cdata1);
        circle1->setPen(pen);
        blk->getEntityTable().add_direct(circle1);
        CircleData cdata2(DmVector(0.0, 0.0), 0.25);
        DmCircle* circle2 = new DmCircle(nullptr, cdata2);
        circle2->setPen(pen);
        blk->getEntityTable().add_direct(circle2);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 直角
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::RightAngle), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmLine* line1 = new DmLine(DmVector(0.0, 0.0), DmVector{ -0.5,0.5 });
        DmLine* line3 = new DmLine(DmVector(-0.5, -0.5), DmVector{ 0.0, 0.0 });
        line1->setPen(pen);
        line3->setPen(pen);
        blk->getEntityTable().add_direct(line1);
        blk->getEntityTable().add_direct(line3);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 30度角
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::Open30), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmLine* line1 = new DmLine(DmVector(0.0, 0.0), DmVector{ -1.0, tan(Math2d::deg2rad(15.0)) });
        DmLine* line3 = new DmLine(DmVector(-1.0, -tan(Math2d::deg2rad(15.0))), DmVector{ 0.0, 0.0 });
        line1->setPen(pen);
        line3->setPen(pen);
        blk->getEntityTable().add_direct(line1);
        blk->getEntityTable().add_direct(line3);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 小点
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::DotSmall), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        double r = 0.3;
        createFillCircle(blk, pen, DmVector(0.0, 0.0), r);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 空心点
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::DotBlank), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        CircleData cdata(DmVector(0.0, 0.0), 0.5);
        DmCircle* circle = new DmCircle(nullptr, cdata);
        circle->setPen(pen);
        blk->getEntityTable().add_direct(circle);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 空心小点
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::DotSmallBlank), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        CircleData cdata(DmVector(0.0, 0.0), 0.25);
        DmCircle* circle = new DmCircle(nullptr, cdata);
        circle->setPen(pen);
        blk->getEntityTable().add_direct(circle);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 方框
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::Box), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        DmLine* line1 = new DmLine(DmVector(-0.5, -0.5), DmVector{ 0.5,-0.5 });
        DmLine* line2 = new DmLine(DmVector(0.5, -0.5), DmVector{ 0.5,0.5 });
        DmLine* line3 = new DmLine(DmVector(0.5, 0.5), DmVector{ -0.5,0.5 });
        DmLine* line4 = new DmLine(DmVector(-0.5, 0.5), DmVector{ -0.5,-0.5 });
        line1->setPen(pen);
        line2->setPen(pen);
        line3->setPen(pen);
        line4->setPen(pen);
        blk->getEntityTable().add_direct(line1);
        blk->getEntityTable().add_direct(line2);
        blk->getEntityTable().add_direct(line3);
        blk->getEntityTable().add_direct(line4);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 实心方框
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::BoxFilled), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        std::vector<DmVector> corners{ DmVector(-0.5,-0.5), DmVector{0.5,-0.5}, DmVector{0.5,0.5},DmVector{-0.5,0.5} };
        SolidData solidData(corners);
        DmSolid* solid = new DmSolid(nullptr, solidData);
        solid->setPen(pen);
        blk->getEntityTable().add_direct(solid);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 基准三角形
        DmBlockData data = DmBlockData(DmDimensionStyle::getArrowBlockName(DM::ArrowType::DatumTriangle), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        double tan30 = tan(M_PI / 6.0);
        DmLine* line1 = new DmLine(DmVector(0.0, tan30), DmVector{ -1.0,0.0 });
        DmLine* line2 = new DmLine(DmVector(-1.0, 0.0), DmVector{ 0.0,-tan30 });
        DmLine* line3 = new DmLine(DmVector(0.0, -tan30), DmVector{ 0.0,tan30 });
        line1->setPen(pen);
        line2->setPen(pen);
        line3->setPen(pen);
        blk->getEntityTable().add_direct(line1);
        blk->getEntityTable().add_direct(line2);
        blk->getEntityTable().add_direct(line3);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 实心基准三角形
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::DatumTriangleFilled), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        double tan30 =  tan(M_PI / 6.0);
        std::vector<DmVector> corners{ DmVector(0.0, tan30), DmVector{-1.0,0.0}, DmVector{0.0,-tan30} };
        SolidData solidData(corners);
        DmSolid* solid = new DmSolid(nullptr, solidData);
        solid->setPen(pen);
        blk->getEntityTable().add_direct(solid);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }

    {
        // 积分
        DmBlockData data(DmDimensionStyle::getArrowBlockName(DM::ArrowType::Integral), DmVector(0.0, 0.0), false);
        DmBlock* blk = new DmBlock(nullptr, data);
        blk->setDocument(m_pDoc);
        double radius = 0.35;
        ArcData data1(DmVector(radius, 0.0), DmVector(0.0,0.0,1.0), radius, M_PI / 2.0, M_PI);
        DmArc* arc1 = new DmArc(nullptr, data1);
        arc1->setPen(pen);
        blk->getEntityTable().add_direct(arc1);
        ArcData data2(DmVector(-radius, 0.0), DmVector(0.0, 0.0, 1.0), radius, M_PI * 3.0 / 2.0, M_PI * 2.0);
        DmArc* arc2 = new DmArc(nullptr, data2);
        arc2->setPen(pen);
        blk->getEntityTable().add_direct(arc2);
        if (!m_pDoc->getBlockTable()->find(blk->getName())) {
            m_pDoc->getIdManager()->assignID(blk);
            m_pDoc->getBlockTable()->add_direct(blk);
        } else {
            delete blk;
        }
    }
}

/// @brief 创建填充圆（用三角形拼合）
void DmDimensionStyleTable::createFillCircle(DmBlock* blk, const DmPen& pen, const DmVector& center, double r)
{
    constexpr int CIRCLE_SEGMENT = 10;
    constexpr double CIRCLE_DELTA = M_PI * 2.0 / CIRCLE_SEGMENT;
    DmVector lastPt(false);
    for (int i = 0; i <= CIRCLE_SEGMENT; i++)
    {
        double x = std::cos(i * CIRCLE_DELTA) * r;
        double y = std::sin(i * CIRCLE_DELTA) * r;
        if (!lastPt.valid)
        {
            lastPt.x = x;
            lastPt.y = y;
            lastPt.valid = true;
        }
        else
        {
            DmVector curPt(x,y);
            DmSolid* s = new DmSolid(nullptr, SolidData({ center, lastPt, curPt }));
            s->setPen(pen);
            blk->getEntityTable().add_direct(s);
            lastPt = curPt;
        }
    }
}
