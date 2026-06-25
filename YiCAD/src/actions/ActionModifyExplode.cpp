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


/// @file ActionModifyExplode.cpp
/// @brief 分解实体（块、多段线、多行文字等）的交互动作类实现

#include "ActionModifyExplode.h"

#include <QAction>

#include "DmArc.h"
#include "DmBlockReference.h"
#include "DmChar.h"
#include "DmCharTemplate.h"
#include "DmEntityContainer.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmLine.h"
#include "DmMText.h"
#include "DmMTextLine.h"
#include "DmMTextParagraph.h"
#include "DmPolyline.h"
#include "DmText.h"
#include "EntityTable.h"
#include "GeometryMethods.h"
#include "GuiDialogFactory.h"
#include "Transaction.h"

namespace
{
    /// @brief 多行文字分解时创建新文字样式的名称前缀
    const QString MTEXT_EXPLODE_STYLE_PREFIX = "MTX_";

    /// @brief 新文字样式起始编号
    constexpr int MTEXT_EXPLODE_STYLE_START_NUM = 2;
}

/// @brief 构造函数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionModifyExplode::ActionModifyExplode(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Entity Explode", doc, docView)
{
    actionType = DM::ActionModifyExplode;
}

/// @brief 初始化动作
/// @param [in] status 初始状态
void ActionModifyExplode::init(int status)
{
    PreviewActionInterface::init(status);

    trigger();
    finish(false);
}

/// @brief 触发分解操作
void ActionModifyExplode::trigger()
{
    explode(true);
}

/// @brief 执行分解操作
/// @param [in] remove 是否在分解后删除原实体
/// @return 分解成功返回true，否则返回false
bool ActionModifyExplode::explode(const bool remove)
{
    auto entTable = pDocument->getEntityTable();

    std::vector<DmEntity*> toExplode;
    for (auto e : *entTable)
    {
        if (e->isLocked() || !e->isSelected())
            continue;
        toExplode.emplace_back(e);
    }

    if (toExplode.empty())
    {
        GUIDIALOGFACTORY->updateMouseWidget(tr("No entity explode."), tr("Back"));
        return false;
    }

    Transaction t(tr("Explode").toStdString(), pDocument);
    t.start();

    std::vector<DmEntity*> entsToRemove;
    std::vector<DmEntity*> subEntities;

    for (auto e : toExplode)
    {
        DmLayer* layer = e->getLayer();
        DmPen pen = e->getPen(false);
        auto eType = e->getEntityType();

        // 分解块参照为子实体
        if (eType == DM::EntityBlockReference)
        {
            DmBlockReference* blockRef = static_cast<DmBlockReference*>(e);
            std::list<DmEntity*> subs = blockRef->getSubEntities();
            for (auto sub : subs)
            {
                DmEntity* clone = sub->clone();
                clone->setParent(nullptr);
                clone->setLayer(layer);
                clone->setPen(pen);
                subEntities.emplace_back(clone);
            }
            entsToRemove.emplace_back(e);
        }
        // 分解通用容器为子实体
        else if (e->isContainer())
        {
            DmEntityContainer* ec = static_cast<DmEntityContainer*>(e);
            for (auto subEnt : *ec)
            {
                DmEntity* clone = subEnt->clone();
                clone->setParent(nullptr);
                clone->setLayer(layer);
                clone->setPen(pen);
                subEntities.emplace_back(clone);
            }
            entsToRemove.emplace_back(e);
        }
        // 分解多段线为直线/圆弧段
        else if (eType == DM::EntityPolyline)
        {
            DmPolyline* poly = static_cast<DmPolyline*>(e);
            double bulge = 0.0;
            DmVector pt1(false), pt2(false);
            int segCount = poly->getSegmentCount();
            for (int i = 0; i < segCount; i++)
            {
                poly->getSegmentInfoAt(i, bulge, pt1, pt2);
                DmEntity* subEnt = nullptr;
                if (bulge == 0)
                {
                    subEnt = new DmLine(pt1, pt2);
                }
                else
                {
                    DmVector center(false), normal(false);
                    double radius = 0.0, startAng = 0.0, endAng = 0.0;
                    GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAng, endAng, normal);
                    subEnt = new DmArc(nullptr, ArcData(center, normal, radius, startAng, endAng));
                }
                subEnt->setLayer(layer);
                subEnt->setPen(pen);
                subEntities.emplace_back(subEnt);
            }
            entsToRemove.emplace_back(e);
        }
        // 分解多行文字为单行文字
        else if (eType == DM::EntityMText)
        {
            DmMText* text = static_cast<DmMText*>(e);
            explodeMTextIntoLetters(text, subEntities);
            entsToRemove.emplace_back(e);
        }
        // 标注和其他类型暂不支持分解
    }

    if (entsToRemove.empty())
    {
        t.rollback();
        GUIDIALOGFACTORY->updateMouseWidget(tr("No entity explode."), tr("Back"));
        return false;
    }

    if (remove)
    {
        for (auto e : entsToRemove)
        {
            entTable->remove(e);
        }
    }
    for (auto e : subEntities)
    {
        entTable->add(e);
    }

    t.commit();

    GUIDIALOGFACTORY->updateMouseWidget(
        tr("Explode success, %1 entities exploded.").arg(entsToRemove.size()), tr("Back"));
    return true;
}

/// @brief 将多行文字分解为单行文字
/// @param [in] text 多行文字实体指针
/// @param [out] addList 分解后产生的单行文字实体列表
/// @return 分解成功返回true，否则返回false
bool ActionModifyExplode::explodeMTextIntoLetters(DmMText* text, std::vector<DmEntity*>& addList)
{
    if (!text)
    {
        return false;
    }

    if (text->isLocked() || !text->isVisible())
    {
        return false;
    }

    // 收集单个文字
    std::vector<DmChar*> chs;
    for (auto paraEnt : *text)
    {
        if (!paraEnt)
        {
            break;
        }
        DmMTextParagraph* para = static_cast<DmMTextParagraph*>(paraEnt);
        for (auto lineEnt : *para)
        {
            DmMTextLine* line = static_cast<DmMTextLine*>(lineEnt);
            for (auto charEnt : *line)
            {
                DmChar* c = static_cast<DmChar*>(charEnt);
                if (c->isWriteSpace())
                {
                    continue;
                }
                chs.emplace_back(c);
            }
        }
    }

    // 生成单行文字
    std::vector<DmFont*> asciiFonts;
    for (auto& font : *DMFONTLIST)
    {
        if (font->getFontType() == FontType::ShxASCII || font->getFontType() == FontType::ShxUnifont)
        {
            asciiFonts.emplace_back(font);
        }
    }
    DmTextStyleTable* textStyleTable = pDocument->getTextStyleTable();
    for (DmChar* c : chs)
    {
        // 查找已有文字样式是否包含字体
        DmFont* f = c->getCharTemplate()->getOwner()->getFont();
        DmTextStyle* theStyle = nullptr;
        for (DmTextStyle* style : *textStyleTable)
        {
            auto styleData = style->getDataConstPtr();
            if (f == styleData->pAsciiFont || f == styleData->pBigFont || f == styleData->pSysFont)
            {
                theStyle = style;
                break;
            }
        }

        // 没有文字样式则创建
        if (nullptr == theStyle)
        {
            int num = MTEXT_EXPLODE_STYLE_START_NUM;
            QString newStyleName;
            do
            {
                num++;
                newStyleName = QString("%1%2").arg(MTEXT_EXPLODE_STYLE_PREFIX).arg(num);
            }
            while (textStyleTable->find(newStyleName) != nullptr);
            DmTextStyle* newStyle = new DmTextStyle(newStyleName);
            auto newData = newStyle->getData();
            if (f->isSystemFont())
            {
                newData.isSystemFont = true;
                newData.isUseBigfont = false;
                newData.sysFontFamily = DMFONTLIST->getFontFamilyName(f, newData.isSysFontBold, newData.isSysFontItalic);
                newData.pAsciiFont = nullptr;
                newData.pBigFont = nullptr;
                newData.pSysFont = f;
            }
            else if (f->getFontType() == FontType::ShxBigFont)
            {
                newData.isSystemFont = false;
                newData.isUseBigfont = true;
                newData.sysFontFamily = "";
                newData.pAsciiFont = asciiFonts.front();
                newData.pBigFont = f;
                newData.pSysFont = nullptr;
            }
            else if (f->getFontType() == FontType::ShxASCII || f->getFontType() == FontType::ShxUnifont)
            {
                newData.isSystemFont = false;
                newData.isUseBigfont = false;
                newData.sysFontFamily = "";
                newData.pAsciiFont = f;
                newData.pBigFont = nullptr;
                newData.pSysFont = nullptr;
            }
            else
            {
                // 未知字体类型，默认使用系统字体配置
                newData.isSystemFont = false;
                newData.isUseBigfont = false;
                newData.sysFontFamily = "";
                newData.pAsciiFont = nullptr;
                newData.pBigFont = nullptr;
                newData.pSysFont = nullptr;
            }
            newStyle->setData(newData);
            textStyleTable->add(newStyle);
            theStyle = newStyle;
        }

        // 由文字样式创建单行文字
        DmText* newText = new DmText(
            text->getParent(),
            TextData(c->getPosition(), c->getNominalHeight(), ETextVertMode::kTextBase,
                ETextHorzMode::kTextLeft, c->getName(), theStyle, text->getDataConstPtr()->getAngle(), EUpdateMode::Update));
        newText->setLayer(text->getLayer(false));
        newText->setPen(c->getPen(false));
        newText->update();
        addList.push_back(newText);
    }

    return true;
}
