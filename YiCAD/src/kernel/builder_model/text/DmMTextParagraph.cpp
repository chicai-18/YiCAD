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


/// @file DmMTextParagraph.cpp
/// @brief 多行文字段落实体类实现，管理段落内的行

#include "TextConsts.h"
#include "DmMTextParagraph.h"
#include "DmMTextLine.h"
#include "DmChar.h"
#include "DmCharTemplate.h"
#include "DmMText.h"
#include "DmMTextLine.h"
#include "Tools.h"

DmMTextParagraph::DmMTextParagraph(DmEntity* parent)
    : DmEntity(parent), m_eAlign(DmMTextParagraph::Alignment::Default), m_leftTop(0.0, 0.0)
{
    DmPen pen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous);
    setPen(pen);
    setLayer(nullptr);
    DmMTextLine* line = new DmMTextLine(this);
    addLine(line);
}

DmMTextParagraph::~DmMTextParagraph()
{
    for (auto& line : lines)
    {
        delete line;
    }
    lines.clear();
}

DmMTextParagraph* DmMTextParagraph::clone() const
{
    DmMTextParagraph* clonedPara = new DmMTextParagraph(*this);
    return clonedPara;
}

DmVector DmMTextParagraph::getLeftTop() const
{
    return m_leftTop;
}

void DmMTextParagraph::setLeftTop(const DmVector& leftTop)
{
    m_leftTop = leftTop;

    double curPos = 0.0;
    for (auto line : lines)
    {
        line->setLeftTop(m_leftTop - DmVector(0.0, curPos));
        curPos += line->getHeightWithSpace();
    }
}

double DmMTextParagraph::getHeight() const
{
    double totalHeight = 0.0;
    for (auto line : lines)
    {
        totalHeight += line->getHeightWithSpace();
    }
    return totalHeight;
}

double DmMTextParagraph::getHeightWithoutLastDescenderSpace() const
{
    double totalHeight = 0.0;
    for (int i = 0; i < lines.size(); i++)
    {
        DmMTextLine* line = lines.at(i);
        // 最后一行取height+descender
        if (i == lines.size() - 1)
        {
            double height = line->getHeight();
            double descender = line->getDescender();
            totalHeight += (height + descender);
        }
        else
        {
            totalHeight += line->getHeightWithSpace();
        }
    }
    return totalHeight;
}

double DmMTextParagraph::getWidth() const
{
    DmMText* mtext = static_cast<DmMText*>(getParent());
    double defWidth = mtext->getWidth();
    if (lines.size() > 1 || m_eAlign == Alignment::Distribute)
    {
        return defWidth;
    }

    // 仅有1行文字
    DmMTextLine* line = lines.at(0);
    return line->getWidth();
}

DmMTextParagraph::Alignment DmMTextParagraph::getAlignment() const
{
    return m_eAlign;
}

void DmMTextParagraph::setAlignent(const DmMTextParagraph::Alignment& align)
{
    m_eAlign = align;
}

DmVector DmMTextParagraph::getStartPosition() const
{
    // 对齐优先级高于对正
    DmMText* text = static_cast<DmMText*>(getParent());
    double defWidth = text->getDataConstPtr()->getDefineWidth();
    EMTextMode m_justification = text->getDataConstPtr()->getJustification();
    if (m_eAlign == Alignment::Justify || m_eAlign == Alignment::Left)
    {
        return m_leftTop;
    }
    else if (m_eAlign == Alignment::Distribute || m_eAlign == Alignment::Mid)
    {
        return DmVector(m_leftTop.x + defWidth / 2.0, m_leftTop.y);
    }
    else if (m_eAlign == Alignment::Right)
    {
        return DmVector(m_leftTop.x + defWidth, m_leftTop.y);
    }

    // 当对齐为default时，根据对正规则
    if (m_eAlign == Alignment::Default)
    {
        if (m_justification == EMTextMode::kTextTopCenter || m_justification == EMTextMode::kTextMiddleCenter || m_justification == EMTextMode::kTextBottomCenter)
        {
            return DmVector(m_leftTop.x + defWidth / 2.0, m_leftTop.y);
        }
        else if (m_justification == EMTextMode::kTextTopRight || m_justification == EMTextMode::kTextMiddleRight || m_justification == EMTextMode::kTextBottomRight)
        {
            return DmVector(m_leftTop.x + defWidth, m_leftTop.y);
        }
        else
        {
            return m_leftTop;
        }
    }
}

void DmMTextParagraph::locateChar(const DmVector& pos, const bool force, DmChar*& preChar, DmChar*& postChar, const bool containNewLine)
{
    preChar = nullptr;
    postChar = nullptr;
    DmMTextLine* line = nullptr;

    // 检索每一行
    for (auto e : lines)
    {
        line = e;
        line->locateChar(pos, false, preChar, postChar, containNewLine);
        if (preChar != nullptr || postChar != nullptr)
        {
            break;
        }
    }

    // 如果没有检索到位置，按需要强制取最后一行
    if (preChar == nullptr && postChar == nullptr)
    {
        if (force)
        {
            line = static_cast<DmMTextLine*>(lineAt(size() - 1));
            line->locateChar(pos, true, preChar, postChar, containNewLine);
        }
    }
}

void DmMTextParagraph::locateCharIndex(const DmChar* preChar, const DmChar* postChar, int& index)
{
    if (preChar == nullptr && postChar == nullptr)
    {
        index = -1;
        return;
    }
    else
    {
        if (preChar != nullptr)
        {
            index = indexOf(preChar) + 1;
        }
        // postChar != nullptr
        else
        {
            index = indexOf(postChar);
        }
    }
}

int DmMTextParagraph::indexOf(const DmChar* c) const
{
    int idx = 0;
    for (auto line : lines)
    {
        for (auto ch : *line)
        {
            if (c == ch)
            {
                return idx;
            }
            else
            {
                idx++;
            }
        }
    }
}

void DmMTextParagraph::insertChars(int index, const std::vector<DmChar*>& charsToInsert)
{
    if (index == -1)
    {
        index = 0;
    }
    for (auto l : lines)
    {
        int size = (int)l->size();
        if (index <= size)
        {
            l->insertChars(index, charsToInsert);
            break;
        }
        index -= size;
    }
}

void DmMTextParagraph::insertChars(const DmChar* preChar, const DmChar* postChar, const std::vector<DmChar*>& charsToInsert)
{
    if (preChar == nullptr && postChar == nullptr)
    {
        DmMTextLine* firstLine = lineAt(0);
        firstLine->insertChars(0, charsToInsert);
    }
    else
    {
        if (preChar != nullptr)
        {
            DmMTextLine* line = static_cast<DmMTextLine*>(preChar->getParent());
            int idx = line->findChar(preChar);
            if (idx >= 0)
            {
                line->insertChars(idx + 1, charsToInsert);
            }
        }
        else if (postChar != nullptr)
        {
            DmMTextLine* line = static_cast<DmMTextLine*>(postChar->getParent());
            int idx = line->findChar(postChar);
            if (idx >= 0)
            {
                line->insertChars(idx, charsToInsert);
            }
        }
    }
}

void DmMTextParagraph::insertChar(const DmChar* preChar, const DmChar* postChar, DmChar* charToInsert)
{
    if (preChar == nullptr && postChar == nullptr)
    {
        DmMTextLine* firstLine = lineAt(0);
        firstLine->insertChar(0, charToInsert);
        charToInsert->setParent(firstLine);
    }
    else
    {
        if (preChar != nullptr)
        {
            DmMTextLine* line = static_cast<DmMTextLine*>(preChar->getParent());
            int idx = line->findChar(preChar);
            if (idx >= 0)
            {
                line->insertChar(idx + 1, charToInsert);
                charToInsert->setParent(line);
            }
        }
        else if (postChar != nullptr)
        {
            DmMTextLine* line = static_cast<DmMTextLine*>(postChar->getParent());
            int idx = line->findChar(postChar);
            if (idx >= 0)
            {
                line->insertChar(idx, charToInsert);
                charToInsert->setParent(line);
            }
        }
    }
}

void DmMTextParagraph::removeChars(int index, int count, std::vector<DmChar*>& originChars)
{
    if (index == -1)
    {
        index = 0;
    }
    bool isFirstLine = true;
    int leftCount = count;
    int startLineIdx = 0; // 开始删除字符的行
    int endLineIdx = -1; // 结束删除字符的行
    int newIndex = 0;
    int counter = 0;
    bool found = false;

    // 计算开始删除的行
    while (startLineIdx < lines.size())
    {
        auto l = lines.at(startLineIdx);
        if (index < counter + (int)l->size())
        {
            found = true;
            newIndex = index - counter;
            break;
        }
        else
        {
            startLineIdx++;
            counter += (int)l->size();
        }
    }
    if (!found)
    {
        return;
    }
    // 删除字符
    counter = 0;
    int i = startLineIdx;
    for (; i < (int)lines.size(); i++)
    {
        auto l = lines.at(i);
        if (isFirstLine)
        {
            isFirstLine = false;
            int c = l->size() - newIndex;
            if (leftCount > c)
            {
                l->removeChars(newIndex, c, originChars);
                leftCount -= c;
            }
            else
            {
                l->removeChars(newIndex, leftCount, originChars);
                endLineIdx = i;
                break;
            }
        }
        else
        {
            int c = l->size();
            if (leftCount > c)
            {
                l->removeChars(0, c, originChars);
                leftCount -= c;
            }
            else
            {
                l->removeChars(0, leftCount, originChars);
                endLineIdx = i;
                break;
            }
        }
    }
    if (endLineIdx == -1)
    {
        endLineIdx = i;
    }

    // 删除空行。如果仅剩空行，保留
    std::vector<int> indices; // 空行索引
    for (int k = startLineIdx; k <= endLineIdx; k++)
    {
        if (lines.at(k)->size() == 0)
        {
            indices.emplace_back(k);
        }
    }
    if (indices.size() == lines.size())
    {
        // 留下最后的空行
        for (int k = indices.size() - 2; k >= 0; k--)
        {
            int idx = indices.at(k);
            delete lines.at(idx);
        }
        if ((int)lines.size() > 0)
        {
            lines.erase(lines.begin(), lines.end() - 1);
        }
    }
    else
    {
        for (int k = indices.size() - 1; k >= 0; k--)
        {
            int idx = indices.at(k);
            delete lines.at(idx);
        }
        if ((int)indices.size() > 0)
        {
            lines.erase(lines.begin() + indices.front(), lines.begin() + indices.back() + 1);
        }
    }
}

void DmMTextParagraph::updateTextPosition(const double& defineWidth)
{
    // 重排文字位置
    // 先按左对齐排列x坐标（行重新构造）
    DmMText* mtext = static_cast<DmMText*>(getParent());
    std::vector<DmMTextLine*> newLines;
    DmMTextLine* newLine = nullptr;
    for (int i = 0; i < lines.size(); i++)
    {
        DmMTextLine* line = lines.at(i);
        if (line->size() == 0) // 复制空行（最后一个正在编辑的行，什么字符都没有）
        {
            newLines.emplace_back(new DmMTextLine(*line));
            continue;
        }
        for (auto ch : *line)
        {
            DmChar* c = static_cast<DmChar*>(ch);
            double cWidth = c->getWidth();
            double cNominalHeight = c->getNominalHeight();
            double gap = cNominalHeight * CHARGAPFACTOR;
            double lineWidth = 0.0;
            if (newLine != nullptr)
            {
                lineWidth = newLine->getWidth();
            }

            bool widthNotEnough = lineWidth + gap * 2.0 + cWidth > defineWidth;
            bool needNewLine = false; // 是否需新启一行
            // 如果不是空段落，即使加上换行符空间不够，换行符不单独成行
            if (newLine != nullptr && c->isNewLine())
            {
                needNewLine = false;
            }
            // （位置不够，需新启一行）或者（起始行）
            else if (widthNotEnough || newLine == nullptr)
            {
                needNewLine = true;
            }
            if (needNewLine)
            {
                newLine = new DmMTextLine(this);
                c->setParent(newLine);
                c->moveTo(m_leftTop + DmVector(gap, 0.0)); // y后面调整
                newLine->addChar(c);
                newLines.emplace_back(newLine);
            }
            // 在当前行继续添加文字
            else
            {
                c->setParent(newLine);
                c->moveTo(m_leftTop + DmVector(lineWidth + gap, 0.0)); // y后面调整
                newLine->addChar(c);
            }
        }
    }

    // 如果实际对齐方式对齐方式不是左对齐，重新排列x位置
    updateLineXPosition(newLines, defineWidth);

    // 设置排列后的文字y坐标
    double total = 0.0;
    for (auto line : newLines)
    {
        double h = line->getHeight();
        double lineHeightWithSpace = line->getHeightWithSpace();
        double y = m_leftTop.y - total - h;
        double cCount = line->size();
        for (int i = 0; i < cCount; i++)
        {
            DmChar* c = line->charAt(i);
            auto pos = c->getPosition();
            c->moveTo(DmVector(pos.x, y));
        }
        DmVector leftTop = m_leftTop + DmVector(0.0, -total);
        line->setLeftTop(leftTop);
        total += lineHeightWithSpace;
    }

    // 清空原始，再添加排列后的行
    clearWithoutDelChars();
    for (auto line : newLines)
    {
        addLine(line);
    }
}

void DmMTextParagraph::removeRange(DmChar* beginC, DmChar* endC, std::vector<DmChar*>& removedCollect)
{
    // 收集要删除的字符
    bool isBegin = false;
    bool isStop = false;
    if (beginC == nullptr)
    {
        isBegin = true;
    }
    std::vector<DmChar*> removed;
    DmMTextLine* line = nullptr;
    DmMTextParagraph* para = nullptr;
    DmChar* ch = nullptr;
    for (auto l : lines)
    {
        line = static_cast<DmMTextLine*>(l);
        for (auto c : *line)
        {
            ch = static_cast<DmChar*>(c);
            if (isBegin && c == endC)
            {
                isStop = true;
                break;
            }
            if (isBegin)
            {
                removed.emplace_back(ch);
            }
            else if (c == beginC)
            {
                isBegin = true;
                removed.emplace_back(ch);
            }
        }
        if (isStop)
        {
            break;
        }
    }

    // 从行中移除字符的引用，如果行没有字符了，释放他
    for (auto c : removed)
    {
        line = static_cast<DmMTextLine*>(c->getParent());
        line->removeChar(c, false);
        c->setParent(nullptr);
        if (line->size() == 0)
        {
            para = static_cast<DmMTextParagraph*>(line->getParent());
            para->removeLine(line);
        }
    }

    // 提取结果
    removedCollect.insert(removedCollect.end(), removed.begin(), removed.end());
}

DmChar* DmMTextParagraph::getChar(int index) const
{
    std::vector<DmChar*> temp;
    getCharsByRange(index, 1, temp);
    if (temp.size() == 1)
    {
        return temp.at(0);
    }
    return nullptr;
}

void DmMTextParagraph::getCharsByRange(int index, int count, std::vector<DmChar*>& chars) const
{
    if (index == -1)
    {
        index = 0;
    }
    // 如果超出范围，不获得任何字符
    int charCount = getCharsCount(true);
    if (index + count > charCount)
    {
        return;
    }

    bool isFirstLine = true;
    int leftCount = count;
    int startLineIdx = 0; // 开始获得字符的行
    int newIndex = 0;
    int counter = 0;
    bool found = false;
    // 计算开始的行
    while (startLineIdx < lines.size())
    {
        auto l = lines.at(startLineIdx);
        if (index < counter + (int)l->size())
        {
            found = true;
            newIndex = index - counter;
            break;
        }
        else
        {
            startLineIdx++;
            counter += (int)l->size();
        }
    }
    if (!found)
    {
        return;
    }
    // 获得字符
    for (int i = startLineIdx; i < (int)lines.size(); i++)
    {
        auto l = lines.at(i);
        if (isFirstLine)
        {
            isFirstLine = false;
            int c = l->size() - newIndex;
            if (leftCount > c)
            {
                l->getCharsByRange(newIndex, c, chars);
                leftCount -= c;
            }
            else
            {
                l->getCharsByRange(newIndex, leftCount, chars);
                break;
            }
        }
        else
        {
            int c = l->size();
            if (leftCount > c)
            {
                l->getCharsByRange(0, c, chars);
                leftCount -= c;
            }
            else
            {
                l->getCharsByRange(0, leftCount, chars);
                break;
            }
        }
    }
}

int DmMTextParagraph::getCharsCount(bool countLineFeed) const
{
    int c = 0;
    for (auto l : lines)
    {
        c += (int)l->size();
    }
    if (!countLineFeed && hasLineFeed())
    {
        c--;
    }
    return c;
}

DmMTextLine* DmMTextParagraph::getPreLine(DmMTextLine* line)
{
    return getPrePostLine(line, true);
}

DmMTextLine* DmMTextParagraph::getPreLine(int lineIdx)
{
    return getPrePostLine(lineIdx, true);
}

DmMTextLine* DmMTextParagraph::getPostLine(DmMTextLine* line)
{
    return getPrePostLine(line, false);
}

DmMTextLine* DmMTextParagraph::getPostLine(int lineIdx)
{
    return getPrePostLine(lineIdx, false);
}

void DmMTextParagraph::clearWithoutDelChars()
{
    for (auto& ent : lines)
    {
        DmMTextLine* line = static_cast<DmMTextLine*>(ent);
        line->clearWithoutDelChars();
        delete line;
    }
    lines.clear();
}

bool DmMTextParagraph::appendLineFeed(DmChar* newLineChar)
{
    // 已经有换行符，不能再添加
    if (hasLineFeed())
    {
        return false;
    }

    DmMTextLine* lastLine = lineAt(size() - 1);
    newLineChar->setParent(lastLine);
    newLineChar->setPosition(lastLine->getLeftTop() + DmVector(lastLine->getWidth(), -lastLine->getHeight()));
    lastLine->addChar(newLineChar);
    return true;
}

bool DmMTextParagraph::removeLineFeed(DmChar*& removedNewLineChar)
{
    removedNewLineChar = nullptr;
    if (!hasLineFeed())
    {
        return false;
    }
    DmMTextLine* lastLine = lineAt(size() - 1);
    removedNewLineChar = lastLine->last();
    removedNewLineChar->setParent(nullptr);
    std::vector<DmChar*> nouse;
    lastLine->removeChars(lastLine->size() - 1, 1, nouse);
}

bool DmMTextParagraph::hasLineFeed() const
{
    DmMTextLine* lastLine = lineAt(size() - 1);

    DmChar* lastChar = lastLine->getLastChar(true);
    if (lastChar != nullptr && lastChar->isNewLine())
    {
        return true;
    }
    return false;
}

bool DmMTextParagraph::isEmptyParagraph() const
{
    bool isEmptyPara = false;
    if (lines.size() == 1)
    {
        DmMTextLine* line = lines.back();
        if (line->size() == 1)
        {
            DmChar* lastChar = static_cast<DmChar*>(line->last());
            isEmptyPara = lastChar->isNewLine();
        }
        else if (line->size() == 0)
        {
            isEmptyPara = true;
        }
    }
    return isEmptyPara;
}

void DmMTextParagraph::addLine(DmMTextLine* line)
{
    if (line)
    {
        lines.emplace_back(line);
    }
}

DmMTextLine* DmMTextParagraph::lineAt(int index) const
{
    if (lines.size() > index && index >= 0)
    {
        return lines.at(index);
    }
    else
    {
        return nullptr;
    }
}

int DmMTextParagraph::size() const
{
    return lines.size();
}

void DmMTextParagraph::removeLine(DmMTextLine* line)
{
    auto it = std::find(lines.begin(), lines.end(), line);
    if (it != lines.end())
    {
        delete *it;
        lines.erase(it);
    }
}

int DmMTextParagraph::findLine(DmMTextLine* line) const
{
    auto it = std::find(lines.begin(), lines.end(), line);
    if (it == lines.end())
    {
        return -1;
    }
    else
    {
        return it - lines.begin();
    }
}

DmMTextLine* DmMTextParagraph::first() const
{
    if (lines.size() == 0)
    {
        return nullptr;
    }
    return lines.front();
}

DmMTextLine* DmMTextParagraph::last() const
{
    if (lines.size() == 0)
    {
        return nullptr;
    }
    return lines.back();
}

std::vector<DmMTextLine*>::const_iterator DmMTextParagraph::begin() const
{
    return lines.begin();
}

std::vector<DmMTextLine*>::const_iterator DmMTextParagraph::end() const
{
    return lines.end();
}

std::vector<DmMTextLine*>::iterator DmMTextParagraph::begin()
{
    return lines.begin();
}

std::vector<DmMTextLine*>::iterator DmMTextParagraph::end()
{
    return lines.end();
}

std::list<DmEntity*> DmMTextParagraph::getSubEntities() const
{
    std::list<DmEntity*> subEnts = std::list<DmEntity*>();
    for (auto line : lines)
    {
        for (auto c : *line)
        {
            auto wCharSub = c->getSubEntities();
            subEnts.splice(subEnts.end(), wCharSub);
        }
    }
    return subEnts;
}

bool DmMTextParagraph::isContainer() const
{
    return false;
}

void DmMTextParagraph::calculateBorders()
{
    resetBorders();
    for (auto line : lines)
    {
        line->calculateBorders();
        minV = DmVector::minimum(line->getMin(), minV);
        maxV = DmVector::maximum(line->getMax(), maxV);
    }
}

double DmMTextParagraph::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
    if (coord.x < maxV.x && coord.y < maxV.y && coord.x > minV.x && coord.y > minV.y)
    {
        double minDist = DM_MAXDOUBLE;
        double curDist = 0.0;
        for (auto e : lines)
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

DmVector DmMTextParagraph::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmMTextParagraph::getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity, double* dist, DmEntity** entity) const
{
    return DmVector(false);
}

DmVector DmMTextParagraph::getNearestCenter(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmMTextParagraph::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    return DmVector(false);
}

void DmMTextParagraph::setVisible(bool v)
{
    DmEntity::setVisible(v);
    for (auto e : lines)
    {
        e->setVisible(v);
    }
}

bool DmMTextParagraph::setSelected(bool select)
{
    if (DmEntity::setSelected(select))
    {
        for (auto e : lines)
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

void DmMTextParagraph::setHighlighted(bool highlight)
{
    DmEntity::setHighlighted(highlight);
    for (auto e : lines)
    {
        if (e->isVisible())
        {
            e->setHighlighted(highlight);
        }
    }
}

void DmMTextParagraph::move(const DmVector& offset)
{
    for (auto e : lines)
    {
        e->move(offset);
        e->moveBorders(offset);
    }
    moveBorders(offset);
}

void DmMTextParagraph::rotate(const DmVector& center, const DmVector& angleVector)
{
    for (auto e : lines)
    {
        e->rotate(center, angleVector);
    }
    calculateBorders();
}

void DmMTextParagraph::scale(const DmVector& center, const DmVector& factor)
{
    if (std::fabs(factor.x) > DM_TOLERANCE && std::fabs(factor.y) > DM_TOLERANCE)
    {
        for (auto e : lines)
        {
            e->scale(center, factor);
        }
    }
    calculateBorders();
}

void DmMTextParagraph::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    if (axisPoint1.distanceTo(axisPoint2) > DM_TOLERANCE)
    {
        for (auto e : lines)
        {
            e->mirror(axisPoint1, axisPoint2);
        }
    }
    calculateBorders();
}

DmMTextLine* DmMTextParagraph::getPrePostLine(DmMTextLine* line, bool getPre)
{
    if (nullptr == line)
    {
        return nullptr;
    }
    int foundIdx = findLine(line);
    return getPrePostLine(foundIdx, getPre);
}

DmMTextLine* DmMTextParagraph::getPrePostLine(int lineIdx, bool getPre)
{
    if (lineIdx < 0)
    {
        return nullptr;
    }

    int count = lines.size();
    if (getPre)
    {
        int preIdx = lineIdx - 1;
        if (preIdx >= 0 && preIdx < count)
        {
            return static_cast<DmMTextLine*>(lines.at(preIdx));
        }
    }
    else
    {
        int postIdx = lineIdx + 1;
        if (postIdx >= 0 && postIdx < count)
        {
            return static_cast<DmMTextLine*>(lines.at(postIdx));
        }
    }
    return nullptr;
}

void DmMTextParagraph::updateLineXPosition(std::vector<DmMTextLine*>& lines, const double& defineWidth)
{
    // 调用此函数前已保证左对齐，所以左对齐不处理
    DmMText* mtext = static_cast<DmMText*>(getParent());
    // 居中
    if (m_eAlign == Alignment::Mid || (m_eAlign == Alignment::Default && mtext->isJustifyAlignHCenter()))
    {
        updateLineXPosition_Mid(lines, defineWidth);
    }
    // 右对齐
    else if (m_eAlign == Alignment::Right || (m_eAlign == Alignment::Default && mtext->isJustifyAlignHRight()))
    {
        updateLineXPosition_Right(lines, defineWidth);
    }
    // 分散对齐
    else if (m_eAlign == Alignment::Distribute)
    {
        updateLineXPosition_Distribute(lines, defineWidth);
    }
}

void DmMTextParagraph::updateLineXPosition_Mid(std::vector<DmMTextLine*>& lines, const double& defineWidth)
{
    double newMidX = m_leftTop.x + defineWidth / 2.0;
    for (auto line : lines)
    {
        double width = line->getWidth();
        double originMidX = m_leftTop.x + width / 2.0;
        DmVector offsetVec(newMidX - originMidX, 0.0);
        for (auto ch : *line)
        {
            DmChar* c = static_cast<DmChar*>(ch);
            c->move(offsetVec);
        }
    }
}

void DmMTextParagraph::updateLineXPosition_Right(std::vector<DmMTextLine*>& lines, const double& defineWidth)
{
    for (auto line : lines)
    {
        double width = line->getWidth();
        double newMidX = m_leftTop.x + defineWidth - width / 2.0;
        double originMidX = m_leftTop.x + width / 2.0;
        DmVector offsetVec(newMidX - originMidX, 0.0);
        for (auto ch : *line)
        {
            DmChar* c = static_cast<DmChar*>(ch);
            c->move(offsetVec);
        }
    }
}

void DmMTextParagraph::updateLineXPosition_Distribute(std::vector<DmMTextLine*>& lines, const double& defineWidth)
{
    double paraCenterX = m_leftTop.x + defineWidth / 2.0;
    for (auto line : lines)
    {
        double width = line->getWidth();
        double originMidX = m_leftTop.x + width / 2.0;
        double charNum = line->size();
        if (line->hasLineFeed())
        {
            charNum--;
        }
        // 仅含换行或者没有任何字符，仅含1个可显示字符
        if (charNum == 0 || charNum == 1)
        {
            if (line->size() == 0)
            {
                continue;
            }
            DmVector offsetVec(paraCenterX - originMidX, 0.0);
            for (auto ch : *line)
            {
                DmChar* c = static_cast<DmChar*>(ch);
                c->move(offsetVec);
            }
        }
        // 含2个及以上可显示字符
        else
        {
            double charGap = (defineWidth - width) / (charNum - 1);
            DmChar* lastChar = nullptr;
            bool isFirst = true;
            for (auto ch : *line)
            {
                DmChar* c = static_cast<DmChar*>(ch);
                if (isFirst)
                {
                    isFirst = false;
                    lastChar = c;
                    continue;
                }
                double cNominalHeight = c->getNominalHeight();
                DmVector lastPos = lastChar->getPosition();
                double lastWidth = lastChar->getWidth();
                double lastNominalHeight = lastChar->getNominalHeight();
                double curX = lastPos.x + lastWidth + lastNominalHeight * CHARGAPFACTOR + charGap + cNominalHeight * CHARGAPFACTOR;
                c->moveTo(DmVector(curX, c->getPosition().y));
                lastChar = c;
            }
        }
    }
}
