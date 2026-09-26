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

/// @file DmTextStyleTable.cpp
/// @brief 文字样式表实现

#include "DmTextStyleTable.h"
#include "TextConsts.h"
#include <algorithm>
#include "DmFontList.h"
#include "DmSettings.h"
#include "DmFont.h"
#include "TextStyleTableCmd.h"
#include "DmDocument.h"


std::unique_ptr<DmTextStyle> DmTextStyleTable::g_defaultStyle;
bool DmTextStyleTable::g_defaultStyleInit = false;

DmTextStyleTable::~DmTextStyleTable()
{
    for (size_t i = 0; i < m_styles.size(); i++)
    {
        delete m_styles.at(i);
    }
    m_styles.clear();
}

/// @brief 添加文字样式
void DmTextStyleTable::add(DmTextStyle* e)
{
    if (!m_pDoc)
        return;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    TextStyleTableAddCmd* cmd = new TextStyleTableAddCmd(this, e);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 查找文字样式
DmTextStyle* DmTextStyleTable::find(const QString& name)
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

/// @brief 获取当前激活的文字样式
DmTextStyle* DmTextStyleTable::getActive()
{
    return m_pActiveStyle;
}

/// @brief 设置关联文档并初始化
void DmTextStyleTable::setDocument(DmDocument* pDoc)
{
    ITable::setDocument(pDoc);
    // 做一些初始化操作，添加 "Standard" 文字样式
    DmTextStyle* pStandardStyle = new DmTextStyle(DEFAULT_TEXTSTYLE_NAME);
    pStandardStyle->setDocument(pDoc);
    add_direct(pStandardStyle);
    m_pActiveStyle = pStandardStyle;
}

/// @brief 获得默认文字样式
DmTextStyle* DmTextStyleTable::getDefaultStyle()
{
    initDefaultStyle();
    return g_defaultStyle.get();
}

/// @brief 初始化默认文字样式
void DmTextStyleTable::initDefaultStyle()
{
    if (isDefaultStyleInit())
        return;
    g_defaultStyleInit = true;
    DmTextStyleData styleData;
    styleData.pSysFont = DMFONTLIST->requestSysFont(DEFAULT_FONT_FAMILY_NAME, DEFAULT_FONT_FAMILY_BOLD, DEFAULT_FONT_FAMILY_ITALIC);
    if (styleData.pSysFont == nullptr)
        return;
    styleData.defaultHeight = DEFAULT_TEXT_HEIGHT;
    styleData.isReverseDirection = false;
    styleData.isSystemFont = false;
    styleData.isUpsideDown = false;
    styleData.isUseBigfont = false;
    styleData.isVertical = false;
    styleData.name = "defaultTextStyle";
    styleData.pBigFont = nullptr;
    styleData.pSysFont = nullptr;
    styleData.slashAngle = DEFAULT_SLASH_ANGLE;
    styleData.sysFontFamily = DEFAULT_FONT_FAMILY_NAME;
    styleData.isSysFontBold = DEFAULT_FONT_FAMILY_BOLD;
    styleData.isSysFontItalic = DEFAULT_FONT_FAMILY_ITALIC;
    styleData.widhFactor = DEFAULT_WIDTH_FACTOR;

    g_defaultStyle = std::make_unique<DmTextStyle>(styleData);
}

/// @brief 检查默认样式是否已初始化
bool DmTextStyleTable::isDefaultStyleInit()
{
    return g_defaultStyleInit;
}

/// @brief 开始修改文字样式
void DmTextStyleTable::startModify(DmObject *e)
{
    DmTextStyle* ent = static_cast<DmTextStyle*>(e);
    TextStyleTableModifyCmd* cmd = new TextStyleTableModifyCmd(this, ent);
    m_pDoc->getCmdManager()->addToCurrentCmd(cmd);
}

/// @brief 按名称激活文字样式
void DmTextStyleTable::activate(const QString &name)
{
    activate(find(name));
}

/// @brief 激活文字样式
void DmTextStyleTable::activate(DmTextStyle *textStyle)
{
    TextStyleTableActivateCmd* cmd = new TextStyleTableActivateCmd(this, textStyle);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 直接激活文字样式
void DmTextStyleTable::activate_direct(DmTextStyle *textStyle)
{
    m_pActiveStyle = textStyle;
}

/// @brief 通过 id 移除文字样式
void DmTextStyleTable::remove(DmId id)
{
    if (!m_pDoc)
        return;
    auto it = m_textStyleMap.find(id);
    if (it == m_textStyleMap.end())
        return;
    TextStyleTableRemoveCmd* cmd = new TextStyleTableRemoveCmd(this, it->second);
    m_pDoc->getCmdManager()->addAndExecuteCmd(cmd);
}

/// @brief 移除文字样式
void DmTextStyleTable::remove(DmTextStyle *e)
{
    remove(e->getId());
}

/// @brief 直接删除文字样式
bool DmTextStyleTable::remove_direct(DmTextStyle *e)
{
    auto it2 = std::find(m_styles.begin(), m_styles.end(), e);
    m_styles.erase(it2);
    m_textStyleMap.erase(e->getId());
    m_pDoc->getIdManager()->removeID(e->getId());
    delete e;
    return true;
}

/// @brief 通过 id 查找文字样式
DmTextStyle *DmTextStyleTable::find(const DmId &id)
{
    auto it = m_textStyleMap.find(id);
    if (it == m_textStyleMap.end())
        return nullptr;
    return it->second;
}

/// @brief 直接添加文字样式
bool DmTextStyleTable::add_direct(DmTextStyle *e)
{
    if (!m_pDoc)
        return false;
    DmId id = e->getId();
    if (!id.isValid())
    {
        id = m_pDoc->getIdManager()->assignID(e);
    }
    auto it = m_textStyleMap.find(id);
    if (it != m_textStyleMap.end())
        return false;
    m_textStyleMap[id] = e;
    m_styles.emplace_back(e);
    return true;
}

DmTextStyleTable::iterator DmTextStyleTable::begin()
{
    return DmTextStyleTable::iterator(m_styles.begin(), m_styles.end());
}

DmTextStyleTable::iterator DmTextStyleTable::end()
{
    return DmTextStyleTable::iterator(m_styles.end());
}
