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


/// @file DmMTextLine.cpp
/// @brief 多行文字行实体类实现，管理一行内的字符

#include "TextConsts.h"
#include "DmMTextLine.h"
#include "DmChar.h"
#include "DmMText.h"
#include <cmath>

// 行高间距因子（5/3），来源于AutoCAD默认行距
constexpr double LINE_HEIGHT_SPACING_FACTOR = 5.0 / 3.0;

DmMTextLine::DmMTextLine(DmEntity* parent)
    : DmEntity(parent), m_dHeight(0.0), m_dDescender(0.0), m_dWidth(0.0)
{
    DmPen pen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous);
    setPen(pen);
    setLayer(nullptr);
}

DmMTextLine::~DmMTextLine()
{
    for (auto c : chars)
    {
        delete c;
    }
    chars.clear();
}

DmMTextLine* DmMTextLine::clone() const
{
    DmMTextLine* clonedLine = new DmMTextLine(*this);
    clonedLine->chars.clear();
    for (auto c : chars)
    {
        DmChar* newC = static_cast<DmChar*>(c->clone());
        clonedLine->chars.emplace_back(newC);
    }
    return clonedLine;
}

void DmMTextLine::updateHeight()
{
    // 更新基线以上高度
    DmChar* pc = nullptr;
    double maxHeight = 0.0;
    double temp = 0.0;
    for (auto c : chars)
    {
        pc = c;
        temp = pc->getNominalHeight();

        // 汉字会比名义高度高，取较大者
        if (temp < pc->getAscender()) // 取基线以上的非留白高度
        {
            temp = pc->getAscender();
        }
        if (maxHeight < temp)
        {
            maxHeight = temp;
        }
    }
    m_dHeight = maxHeight;

    // 更新基线以下高度
    double maxDescender = 0.0;
    temp = 0.0;
    for (auto c : chars)
    {
        pc = c;
        temp = pc->getDescender();
        if (temp > maxDescender)
        {
            maxDescender = temp;
        }
    }
    m_dDescender = maxDescender;
}

double DmMTextLine::getHeight() const
{
    return m_dHeight;
}

void DmMTextLine::setHeight(double height)
{
    m_dHeight = height;
}

double DmMTextLine::getHeightWithSpace() const
{
    DmMText* mtext = static_cast<DmMText*>(getParent()->getParent());
    double heightFactor = mtext->getDataConstPtr()->getLineSpacingFactor();
    return LINE_HEIGHT_SPACING_FACTOR * m_dHeight * heightFactor;
}

double DmMTextLine::getDescender() const
{
    return m_dDescender;
}

void DmMTextLine::updateWidth()
{
    DmChar* pc = nullptr;
    double totalWidth = 0.0;
    double temp = 0.0;
    double nominalHeight = 0.0;
    for (auto c : chars)
    {
        pc = c;
        if (pc->isNewLine()) // 换行符不参与计算宽度
        {
            continue;
        }
        temp = pc->getWidth();
        nominalHeight = pc->getNominalHeight();
        totalWidth += (temp + nominalHeight * CHARGAPFACTOR * 2.0);
    }
    m_dWidth = totalWidth;
}

double DmMTextLine::getWidth() const
{
    return m_dWidth;
}

DmVector DmMTextLine::getLeftTop() const
{
    return m_leftTop;
}

void DmMTextLine::setLeftTop(const DmVector& leftTop)
{
    m_leftTop = leftTop;
}

void DmMTextLine::locateChar(const DmVector& pos, const bool force, DmChar*& preChar, DmChar*& postChar, const bool containNewLine)
{
    preChar = nullptr;
    postChar = nullptr;
    double topY = m_leftTop.y;
    double bottomY = m_leftTop.y - getHeightWithSpace();
    if (chars.size() == 0)
    {
        return;
    }

    // 命中
    if (pos.y > bottomY && pos.y <= topY)
    {
        // 如果需要定位换行符，特殊处理
        if (containNewLine)
        {
            auto trueLastChar = getLastChar(true);
            double width = trueLastChar->getWidth();
            if (trueLastChar->isNewLine())
            {
                if (pos.x >= trueLastChar->getPosition().x + width)
                {
                    preChar = trueLastChar;
                    postChar = nullptr;
                    return;
                }
            }
        }

        // 不在换行符位置，或者不需要定位换行符
        DmChar* firstChar = getFirstChar(false);
        if (firstChar == nullptr) // 没有非换行字符
        {
            preChar = nullptr;
            postChar = getLastChar(true);
            return;
        }
        DmVector firstCharPos = firstChar->getPosition();
        double firstCharWidth = firstChar->getWidth();
        DmChar* lastChar = getLastChar(false); // 不含换行
        DmVector lastCharPos = lastChar->getPosition();
        double lastCharWidth = lastChar->getWidth();
        bool hasNewLine = hasLineFeed();
        // 开始处
        if (pos.x < firstCharPos.x + firstCharWidth / 2.0)
        {
            preChar = nullptr;
            postChar = firstChar;
        }
        // 结尾处
        else if (pos.x > lastCharPos.x + lastCharWidth / 2.0)
        {
            preChar = lastChar;
            postChar = hasNewLine ? getLastChar(true) : nullptr;
        }
        // 中间处
        else
        {
            DmChar* c = nullptr;
            DmChar* preC = nullptr;
            double curWidth = 0.0;
            double preWidth = 0.0;
            int s = (int)chars.size();
            double leftPos = 0.0;
            double rightPos = 0.0;
            for (auto i = 1; i < s; i++)
            {
                c = charAt(i);
                preC = charAt(i - 1);
                curWidth = c->getWidth();
                preWidth = preC->getWidth();
                leftPos = preC->getPosition().x + preWidth / 2.0; // 左边界
                rightPos = c->getPosition().x + curWidth / 2.0; // 右边界
                if (pos.x >= leftPos && pos.x < rightPos)
                {
                    preChar = preC;
                    postChar = c;
                    break;
                }
            }
        }
    }
    // 未命中
    else
    {
        if (force)
        {
            preChar = getLastChar(false);
            postChar = getLastChar(true);
        }
    }
}

void DmMTextLine::clearWithoutDelChars()
{
    chars.clear();
}

DmChar* DmMTextLine::getLastChar(bool includeLineFeed) const
{
    if (size() == 0)
    {
        return nullptr;
    }
    DmChar* lastChar = charAt(size() - 1);
    if (includeLineFeed)
    {
        return lastChar;
    }
    else
    {
        if (lastChar->isNewLine())
        {
            if (size() > 1)
            {
                return charAt(size() - 2);
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            return lastChar;
        }
    }
}

DmChar* DmMTextLine::getFirstChar(bool includeLineFeed)
{
    if (size() == 0)
    {
        return nullptr;
    }
    DmChar* firstChar = charAt(0);
    if (includeLineFeed)
    {
        return firstChar;
    }
    else
    {
        if (firstChar->isNewLine())
        {
            return nullptr;
        }
        else
        {
            return firstChar;
        }
    }
}

bool DmMTextLine::hasLineFeed()
{
    if (size() == 0)
    {
        return false;
    }
    DmChar* lastChar = charAt(size() - 1);
    return lastChar->isNewLine();
}

void DmMTextLine::insertChars(int index, const std::vector<DmChar*>& chs)
{
    chars.insert(chars.begin() + index, chs.begin(), chs.end());
    for (auto c : chs)
    {
        c->setParent(this);
    }
    updateHeight();
    updateWidth();
}

void DmMTextLine::insertChar(int index, DmChar* c)
{
    if (!c)
    {
        return;
    }
    chars.insert(chars.begin() + index, c);
    c->setParent(this);
    updateHeight();
    updateWidth();
}

void DmMTextLine::addChar(DmChar* c)
{
    if (!c)
    {
        return;
    }
    chars.emplace_back(c);
    updateHeight();
    updateWidth();
}

void DmMTextLine::removeChar(DmChar* c, bool del)
{
    auto it = std::find(chars.begin(), chars.end(), c);
    if (it != chars.end())
    {
        if (del)
        {
            delete *it;
        }
        chars.erase(it);
    }
}

void DmMTextLine::removeChars(int index, int count, std::vector<DmChar*>& originChars)
{
    int endIndex = index + count;
    for (auto i = index; i < endIndex; i++)
    {
        chars.at(i)->setParent(nullptr);
    }
    originChars.insert(originChars.end(), chars.begin() + index, chars.begin() + index + count);
    chars.erase(chars.begin() + index, chars.begin() + index + count);
}

void DmMTextLine::getCharsByRange(int index, int count, std::vector<DmChar*>& gotChars) const
{
    gotChars.insert(gotChars.end(), chars.begin() + index, chars.begin() + index + count);
}

DmChar* DmMTextLine::charAt(int index) const
{
    if (chars.size() > index && index >= 0)
    {
        return chars.at(index);
    }
    else
    {
        return nullptr;
    }
}

int DmMTextLine::size() const
{
    return chars.size();
}

int DmMTextLine::findChar(const DmChar* c) const
{
    auto it = std::find(chars.begin(), chars.end(), c);
    if (it == chars.end())
    {
        return -1;
    }
    else
    {
        return it - chars.begin();
    }
}

DmChar* DmMTextLine::first()
{
    if (chars.size() == 0)
    {
        return nullptr;
    }
    return chars.front();
}

DmChar* DmMTextLine::last()
{
    if (chars.size() == 0)
    {
        return nullptr;
    }
    return chars.back();
}

bool DmMTextLine::isEmpty() const
{
    return chars.size() == 0;
}

std::vector<DmChar*>::const_iterator DmMTextLine::begin() const
{
    return chars.begin();
}

std::vector<DmChar*>::const_iterator DmMTextLine::end() const
{
    return chars.end();
}

std::vector<DmChar*>::iterator DmMTextLine::begin()
{
    return chars.begin();
}

std::vector<DmChar*>::iterator DmMTextLine::end()
{
    return chars.end();
}

std::list<DmEntity*> DmMTextLine::getSubEntities() const
{
    return std::list<DmEntity*>();
}

bool DmMTextLine::isContainer() const
{
    return false;
}

void DmMTextLine::calculateBorders()
{
    resetBorders();
    for (auto c : chars)
    {
        c->calculateBorders();
        minV = DmVector::minimum(c->getMin(), minV);
        maxV = DmVector::maximum(c->getMax(), maxV);
    }
}

double DmMTextLine::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
    if (coord.x < maxV.x && coord.y < maxV.y && coord.x > minV.x && coord.y > minV.y)
    {
        double minDist = DM_MAXDOUBLE;
        double curDist = 0.0;
        for (auto e : chars)
        {
            curDist = e->getDistanceToPoint(coord);
            if (curDist < minDist)
            {
                minDist = curDist;
            }
        }
        return minDist;
    }
    return DM_MAXDOUBLE;
}

DmVector DmMTextLine::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmMTextLine::getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity, double* dist, DmEntity** entity) const
{
    return DmVector(false);
}

DmVector DmMTextLine::getNearestCenter(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmMTextLine::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    return DmVector(false);
}

void DmMTextLine::setVisible(bool v)
{
    DmEntity::setVisible(v);
    for (auto e : chars)
    {
        e->setVisible(v);
    }
}

bool DmMTextLine::setSelected(bool select)
{
    if (DmEntity::setSelected(select))
    {
        for (auto e : chars)
        {
            if (e->isVisible())
            {
                e->setSelected(select);
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

void DmMTextLine::setHighlighted(bool highlight)
{
    DmEntity::setHighlighted(highlight);
    for (auto e : chars)
    {
        if (e->isVisible())
        {
            e->setHighlighted(highlight);
        }
    }
}

void DmMTextLine::move(const DmVector& offset)
{
    for (auto e : chars)
    {
        e->move(offset);
    }
    moveBorders(offset);
}

void DmMTextLine::rotate(const DmVector& center, const DmVector& angleVector)
{
    for (auto e : chars)
    {
        e->rotate(center, angleVector);
    }
    calculateBorders();
}

void DmMTextLine::scale(const DmVector& center, const DmVector& factor)
{
    if (std::fabs(factor.x) > DM_TOLERANCE && std::fabs(factor.y) > DM_TOLERANCE)
    {
        for (auto e : chars)
        {
            e->scale(center, factor);
        }
    }
    calculateBorders();
}

void DmMTextLine::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	if (axisPoint1.distanceTo(axisPoint2) > DM_TOLERANCE)
	{
		for (auto e : chars)
		{
			e->mirror(axisPoint1, axisPoint2);
		}
	}
	calculateBorders();
}
