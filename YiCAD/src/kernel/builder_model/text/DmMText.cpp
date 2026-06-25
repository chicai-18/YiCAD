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


/// @file DmMText.cpp
/// @brief 多行文字实体类实现，管理段落/行/字符的层级结构

#include "TextConsts.h"
#include "DmMText.h"
#include "DmMTextParagraph.h"
#include "DmMTextLine.h"
#include "DmChar.h"
#include "DmSolid.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmCharTemplate.h"
#include "DmTextStyle.h"
#include "DmPen.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "DmMTextContentCmd.h"
#include <cmath>

TYPESYSTEM_SOURCE(DmMText, DmEntity, 0);

DmMText::DmMText(DmEntity* parent)
    : DmEntity(parent)
{
}

DmMText::DmMText(DmEntity* parent, const MTextData& d)
    : DmEntity(parent)
{
    m_data = std::shared_ptr<MTextData>(new MTextData(d));
}

DmMText::~DmMText()
{
    for (auto para : paragraphs)
    {
        delete para;
    }
    paragraphs.clear();
}

DM::EntityType DmMText::getEntityType() const
{
    return DM::EntityMText;
}

MTextData DmMText::getData() const
{
    return *m_data;
}

DmEntity* DmMText::clone() const
{
    DmMText* clonedText = new DmMText(*this);
    clonedText->m_data = std::shared_ptr<MTextData>(new MTextData(*m_data));
    clonedText->paragraphs.clear(); // 复制的段落属于原来的文字，不能释放
    clonedText->update();
    return clonedText;
}

void DmMText::move(const DmVector& offset)
{
    moveEntities(offset);
    DmVector pos = m_data->getPosition();
    pos.move(offset);
    m_data->setPosition(pos);
    moveBorders(offset);
}

void DmMText::rotate(const DmVector& center, const DmVector& angleVector)
{
    rotateEntities(center, angleVector.angle());
    m_data->setAngle(Math2d::correctAngle(m_data->getAngle() + angleVector.angle()));
    m_data->setPosition(m_data->getPosition().rotate(center, m_data->getAngle()));
    calculateBorders();
}

void DmMText::scale(const DmVector& center, const DmVector& factor)
{
    for (auto para : paragraphs)
    {
        para->scale(center, factor);
    }
    m_data->setAngle(m_data->getAngle() * factor.x);
    m_data->setLineSpace(m_data->getLineSpace() * factor.x);
    m_data->setCharHeight(m_data->getCharHeight() * factor.x);
    m_data->setDefineWidth(m_data->getDefineWidth() * factor.x);
    m_data->setDefineHeight(m_data->getDefineHeight() * factor.x);
    calculateBorders();
}

void DmMText::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    // 镜像会真的成为镜像，对比AutoCAD行为不一致
    // 镜像只修改对齐方式，暂不考虑对正信息
    for (auto para : paragraphs)
    {
        para->mirror(axisPoint1, axisPoint2);
    }
    m_data->setPosition(m_data->getPosition().mirror(axisPoint1, axisPoint2));
    calculateBorders();
}

void DmMText::moveEntities(const DmVector& offset)
{
    for (auto para : paragraphs)
    {
        para->move(offset);
    }
}

void DmMText::rotateEntities(const DmVector& center, const double& angle)
{
    DmVector angleVector(angle);
    for (auto para : paragraphs)
    {
        para->rotate(center, angleVector);
    }
}

int DmMText::getSelectCharCount(const DmChar* preCharBegin, const DmChar* postCharBegin,
    const DmChar* preCharEnd, const DmChar* postCharEnd)
{
    int index1, index2;
    locateCharIndex(preCharBegin, postCharBegin, index1);
    locateCharIndex(preCharEnd, postCharEnd, index2);
    return index2 - index1;
}

int DmMText::indexOf(const DmChar* ch) const
{
    int idx = 0;
    for (auto para : paragraphs)
    {
        for (auto line: *para)
        {
            for (auto c: *line)
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
    return -1;
}

DmChar* DmMText::getCharByIndex(int idx) const
{
    int currentIdx = 0;
    for (auto para : paragraphs)
    {
        for (auto line: *para)
        {
            for (auto ch: *line)
            {
                if (currentIdx == idx)
                {
                    return ch;
                }
                else
                {
                    currentIdx++;
                }
            }
        }
    }
    return nullptr;
}

void DmMText::replaceByChars(const std::vector<DmChar*>& newChars, std::vector<DmChar*>& originChars)
{
    getChars(originChars);
    clearWithoutDelChars();

    // 重新构建段落
    DmMTextParagraph* para = nullptr;
    DmMTextLine* line = nullptr;
    for (auto ch : newChars)
    {
        if (!para)
        {
            para = new DmMTextParagraph(this);
            paragraphs.emplace_back(para);
            line = para->first();
        }
        line->addChar(ch);
        ch->setParent(line);
        if (ch->isNewLine())
        {
            para = nullptr;
            line = nullptr;
        }
    }

    // 保证至少一个段落
    if (paragraphs.size() == 0)
    {
        para = new DmMTextParagraph(this);
        paragraphs.emplace_back(para);
    }
    updateTextPosition();
}

void DmMText::clearWithoutDelChars()
{
    for (auto para : paragraphs)
    {
        para->clearWithoutDelChars();
        delete para;
    }
    paragraphs.clear();
}

void DmMText::getChars(std::vector<DmChar*>& chs)
{
    for (auto para : paragraphs)
    {
        for (auto line: *para)
        {
            for (auto ch: *line)
            {
                ch->setParent(nullptr);
                chs.emplace_back(ch);
            }
        }
    }
}

void DmMText::insertChars(const DmChar* preChar, const DmChar* postChar,
    const std::vector<DmChar*>& charsToInsert)
{
    DmMTextParagraph* para = getParagraph(preChar, postChar);
    if (nullptr == para)
    {
        return;
    }
    para->insertChars(preChar, postChar, charsToInsert);
}

void DmMText::insertChars(int index, const std::vector<DmChar*>& chs)
{
    // TODO:  - 此方法实现为空白，需要确认是否需要完善
}

void DmMText::removeChars(int index, int count, std::vector<DmChar*>& removedChars)
{
    // TODO:  - 此方法实现为空白，需要确认是否需要完善
}

void DmMText::locateByIndex(int index, DmChar*& preChar, DmChar*& postChar) const
{
    if (index == -1)
    {
        preChar = nullptr;
        postChar = nullptr;
        return;
    }
    postChar = getCharByIndex(index);
    preChar = getCharByIndex(index - 1);
    if (preChar && preChar->isNewLine())
    {
        preChar = nullptr;
    }
}

void DmMText::locateCharIndex(const DmChar* preChar, const DmChar* postChar, int& index)
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

void DmMText::locateChar(const DmVector& pos, const bool force, DmChar*& preChar,
    DmChar*& postChar, const bool containNewLine)
{
    preChar = nullptr;
    postChar = nullptr;
    DmMTextParagraph* para = nullptr;

    // 检索每一段落
    for (auto paraIter : paragraphs)
    {
        paraIter->locateChar(pos, false, preChar, postChar, containNewLine);
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
            // 只有存在段落才检索位置
            if (size() != 0)
            {
                para = paraAt(size() - 1);
                para->locateChar(pos, true, preChar, postChar, containNewLine);
            }
        }
    }
}

DmMTextParagraph* DmMText::getParagraph(const DmChar* preChar, const DmChar* postChar)
{
    DmMTextParagraph* para = nullptr;
    if (preChar == nullptr && postChar == nullptr)
    {
        para = paraAt(size() - 1);
    }
    else
    {
        para = nullptr;
        if (preChar != nullptr)
        {
            para = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
        }
        else if (postChar != nullptr)
        {
            para = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        }
    }
    return para;
}

void DmMText::insertChar(const DmChar* preChar, const DmChar* postChar, DmChar* charToInsert)
{
    DmMTextParagraph* para = getParagraph(preChar, postChar);
    if (nullptr == para)
    {
        return;
    }
    para->insertChar(preChar, postChar, charToInsert);
}

void DmMText::updateParas()
{
    // 提取文字，清空原始段落
    std::vector<DmChar*> chs;
    getChars(chs);
    clearWithoutDelChars();

    std::vector<DmChar*> nouse; // 将会使空
    replaceByChars(chs, nouse);
}

void DmMText::updateTextPosition()
{
    // 找到起始段落索引
    using size_type = QList<DmEntity*>::size_type;
    size_type startParaIdx = 0;

    // 从起始段落往下更新文字位置（按左上对正），此后的每个段落，每行的高度，文字个数均已确定
    for (size_type i = startParaIdx; i < paragraphs.size(); i++)
    {
        DmMTextParagraph* para = paraAt(i);
        if (i != 0)
        {
            DmMTextParagraph* lastPara = paragraphs.at(i - 1);
            double lastParaHeight = lastPara->getHeight();
            DmVector leftTop = lastPara->getLeftTop() + DmVector(0.0, -lastParaHeight);
            para->setLeftTop(leftTop);
        }
        else
        {
            para->setLeftTop(getPosition());
        }
        para->updateTextPosition(m_data->getDefineWidth());
    }

    // 如果不是左上对正，全部段落里的文字水平位置需要重排
    if (getJustification() != EMTextMode::kTextTopLeft)
    {
        updateTextPositionByJustification();
    }
}

void DmMText::updateTextPositionByJustification()
{
    // 求出从顶部往下的偏移
    double height = calculateHeight();
    double defHeight = m_data->getDefineHeight();
    double vOffset = 0.0;
    if (isJustifyAlignVMid())
    {
        vOffset = -(defHeight - height) / 2.0;
    }
    else if (isJustifyAlignVBottom())
    {
        vOffset = -(defHeight - height);
    }
    if (vOffset == 0.0)
    {
        return;
    }

    // 更新每个段落
    double defWidth = m_data->getDefineWidth();
    for (auto para : paragraphs)
    {
        DmVector leftTop = para->getLeftTop();
        para->setLeftTop(DmVector(leftTop.x, leftTop.y + vOffset));
        para->updateTextPosition(defWidth);
    }
}

DmMTextParagraph* DmMText::newParagraph(const DmChar* preChar, const DmChar* postChar,
    DmChar* newLineChar)
{
    DmMTextParagraph* newPara = nullptr;
    DmMTextParagraph* lastPara = nullptr;
    bool newParaAfter = true;
    // 在最后段落的最后一行添加新行
    if (postChar == nullptr)
    {
        lastPara = paraAt(size() - 1);
        lastPara->appendLineFeed(newLineChar);
        newPara = new DmMTextParagraph(this);
        newPara->setAlignent(lastPara->getAlignment());
        newPara->lineAt(0)->setHeight(newLineChar->getNominalHeight());
        addPara(newPara);
    }
    // 在非最后段落结尾处添加新行
    else if (postChar->isNewLine())
    {
        lastPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        int lastParaIdx = findPara(lastPara);
        newPara = new DmMTextParagraph(this);
        newPara->setAlignent(lastPara->getAlignment());
        newPara->appendLineFeed(newLineChar);
        insertPara(lastParaIdx + 1, newPara);
    }
    // 在段落中间处添加新行
    else
    {
        // 在段落开头处换行
        if (preChar == nullptr)
        {
            lastPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
            int lastParaIdx = findPara(lastPara);
            newPara = new DmMTextParagraph(this);
            newPara->setAlignent(lastPara->getAlignment());
            newPara->appendLineFeed(newLineChar);
            insertPara(lastParaIdx, newPara);
            newParaAfter = false;
        }
        // 在段落中间处换行
        else
        {
            // 分割段落
            lastPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
            int lastParaIdx = findPara(lastPara);

            // 移除前段文字到新段落
            newPara = new DmMTextParagraph(this);
            newPara->setAlignent(lastPara->getAlignment());
            DmMTextLine* newLine = newPara->lineAt(0);
            std::vector<DmChar*> removedChs;
            lastPara->removeRange(nullptr, const_cast<DmChar*>(postChar), removedChs);
            for (auto c : removedChs)
            {
                c->setParent(newLine);
                newLine->addChar(c);
            }
            newPara->appendLineFeed(newLineChar);
            insertPara(lastParaIdx, newPara);
            newParaAfter = false;
        }
    }
    return newParaAfter ? newPara : lastPara;
}

void DmMText::backspace(const DmChar* preChar, const DmChar* postChar,
    DmChar*& newPreChar, DmChar*& newPostChar)
{
    newPreChar = nullptr;
    newPostChar = nullptr;
    // 最后一个空行
    if (nullptr == preChar && nullptr == postChar)
    {
        if (size() == 1) // 整个文字就只有空行
        {
            return;
        }
        DmMTextParagraph* lastPara = static_cast<DmMTextParagraph*>(last());
        DmMTextParagraph* preLastPara = getPreParagraph(lastPara);
        DmMTextLine* preLastLine = static_cast<DmMTextLine*>(preLastPara->last());
        newPostChar = nullptr;
        if (preLastLine->size() == 1) // 前一个段落只有换行（换行符不会独立成一行）
        {
            newPostChar = static_cast<DmChar*>(preLastLine->last());
            newPreChar = nullptr;
            return;
        }
        newPreChar = preLastLine->charAt(preLastLine->size() - 2);
        removePara(last());
        preLastLine->removeChar(preLastLine->last(), true); // 移除前一段换行
        return;
    }
    // 一行行首
    if (nullptr == preChar)
    {
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        DmMTextLine* curLine = static_cast<DmMTextLine*>(postChar->getParent());
        bool isFirstLine = curPara->findLine(curLine) == 0;
        bool isFirstPara = findPara(curPara) == 0;
        // 段落的首行
        if (isFirstLine)
        {
            // 第一个段落
            if (isFirstPara)
            {
                newPreChar = const_cast<DmChar*>(preChar);
                newPostChar = const_cast<DmChar*>(postChar);
                return;
            }
            else
            {
                // 删除本段落，内容合并到前一段落
                DmMTextParagraph* prePara = getPreParagraph(curPara);
                DmMTextLine* preLastLine = prePara->last();
                preLastLine->removeChar(preLastLine->last(), true); // 移除前一段换行
                if (preLastLine->size() == 0) // 前一个段落只有换行
                {
                    newPreChar = nullptr;
                }
                else
                {
                    newPreChar = preLastLine->last();
                }
                std::vector<DmChar*> temp;
                curPara->removeRange(nullptr, nullptr, temp);
                for (auto ent : temp)
                {
                    DmChar* c = static_cast<DmChar*>(ent);
                    c->setParent(preLastLine);
                    preLastLine->addChar(c);
                }
                removePara(curPara);
                newPostChar = const_cast<DmChar*>(postChar);
            }
        }
        else
        {
            DmMTextLine* preLine = curPara->getPreLine(curLine);
            preLine->removeChar(preLine->last(), true);
            newPreChar = preLine->last();
            newPostChar = nullptr;
        }
        return;
    }

    // 一般位置（preChar不为空（也不可能是换行））
    DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
    DmMTextLine* curLine = static_cast<DmMTextLine*>(preChar->getParent());
    int curLineIdx = curPara->findLine(curLine);
    int preCharIdx = curLine->findChar(preChar);
    if (preCharIdx == 0) // 当前行的第一个字符
    {
        if (curLineIdx == 0) // 当前是第一个行
        {
            newPreChar = nullptr;
        }
        else
        {
            DmMTextLine* lastLine = curPara->lineAt(curLineIdx - 1);
            newPreChar = static_cast<DmChar*>(lastLine->last());
        }
    }
    else
    {
        newPreChar = curLine->charAt(preCharIdx - 1);
    }
    curLine->removeChar(const_cast<DmChar*>(preChar), true);
    if (curLine->isEmpty() && curPara->size() != 1) // 当存在多行的时候，本行没有字符了，删除本行
    {
        curPara->removeLine(curLine);
    }
    newPostChar = const_cast<DmChar*>(postChar);
}

void DmMText::del(const DmChar* preChar, const DmChar* postChar,
    DmChar*& newPreChar, DmChar*& newPostChar)
{
    newPreChar = nullptr;
    newPostChar = nullptr;
    // 最后的空行
    if (nullptr == preChar && nullptr == postChar)
    {
        return;
    }
    // 一行的末尾
    if (nullptr == postChar)
    {
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
        DmMTextLine* curLine = static_cast<DmMTextLine*>(preChar->getParent());
        bool isLastLine = curPara->findLine(curLine) == curPara->size() - 1;
        bool isLastPara = findPara(curPara) == size() - 1;
        // 一个段落的最后一行
        if (isLastLine)
        {
            // 最后一个段落
            if (isLastPara)
            {
                newPreChar = const_cast<DmChar*>(preChar);
                newPostChar = const_cast<DmChar*>(postChar);
                return;
            }
            else
            {
                curLine->removeChar(curLine->last(), true); // 移除换行符
                DmMTextParagraph* postPara = getPostParagraph(curPara);
                // 将下一段落的文字移动到当前段落
                std::vector<DmChar*> temp;
                postPara->removeRange(nullptr, nullptr, temp);
                for (auto ent : temp)
                {
                    DmChar* c = static_cast<DmChar*>(ent);
                    c->setParent(curLine);
                    curLine->addChar(c);
                }
                removePara(postPara); // 移除下一段落
                newPreChar = const_cast<DmChar*>(preChar);
                newPostChar = temp.size() != 0 ? temp.front() : nullptr;
            }
        }
        else
        {
            DmMTextLine* postLine = curPara->getPostLine(curLine);
            postLine->removeChar(postLine->first(), true);
            newPreChar = const_cast<DmChar*>(preChar);
            newPostChar = nullptr; // TODO ：得重排文字后才之后再设置才能准确
        }
        return;
    }

    // 非最后段落的末尾
    if (postChar->isNewLine())
    {
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        DmMTextLine* curLine = static_cast<DmMTextLine*>(postChar->getParent());
        curLine->removeChar(curLine->last(), true); // 移除换行符
        DmMTextParagraph* postPara = getPostParagraph(curPara);
        // 将下一段落的文字移动到当前段落
        std::vector<DmChar*> temp;
        postPara->removeRange(nullptr, nullptr, temp);
        for (auto ent : temp)
        {
            DmChar* c = static_cast<DmChar*>(ent);
            c->setParent(curLine);
            curLine->addChar(c);
        }
        removePara(postPara); // 移除下一段落
        newPreChar = const_cast<DmChar*>(preChar);
        newPostChar = temp.size() != 0 ? temp.front() : nullptr;
    }
    // 一般情况（postChar不会是换行或空）
    else
    {
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        DmMTextLine* curLine = static_cast<DmMTextLine*>(postChar->getParent());
        int postCharIdx = curLine->findChar(postChar);
        curLine->removeChar(const_cast<DmChar*>(postChar), true); // 移除后一个字符
        newPreChar = const_cast<DmChar*>(preChar);
        newPostChar = curLine->charAt(postCharIdx);
    }
}

void DmMText::getPreviousChar(const DmChar* preChar, const DmChar* postChar,
    DmChar*& newPreChar, DmChar*& newPostChar)
{
    newPreChar = nullptr;
    newPostChar = nullptr;

    // 最后一个空行
    if (nullptr == preChar && nullptr == postChar)
    {
        if (size() == 1) // 整个文字就只有空行
        {
            return;
        }
        DmMTextParagraph* lastPara = static_cast<DmMTextParagraph*>(last());
        DmMTextParagraph* preLastPara = getPreParagraph(lastPara);
        DmMTextLine* preLastLine = static_cast<DmMTextLine*>(preLastPara->last());
        if (preLastLine->size() == 1) // 前一个段落只有换行（换行符不会独立成一行）
        {
            newPostChar = preLastLine->last();
            newPreChar = nullptr;
            return;
        }
        newPreChar = preLastLine->charAt(preLastLine->size() - 2);
        return;
    }
    // 一行行首
    if (nullptr == preChar)
    {
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        DmMTextLine* curLine = static_cast<DmMTextLine*>(postChar->getParent());
        bool isFirstLine = curPara->findLine(curLine) == 0;
        bool isFirstPara = findPara(curPara) == 0;
        // 段落的首行
        if (isFirstLine)
        {
            // 第一个段落
            if (isFirstPara)
            {
                newPreChar = const_cast<DmChar*>(preChar);
                newPostChar = const_cast<DmChar*>(postChar);
                return;
            }
            else
            {
                DmMTextParagraph* prePara = getPreParagraph(curPara);
                DmMTextLine* preParaLastLine = static_cast<DmMTextLine*>(prePara->last());
                newPreChar = preParaLastLine->charAt(preParaLastLine->size() - 2);
                newPostChar = preParaLastLine->last(); // 换行符
            }
        }
        else
        {
            DmMTextLine* preLine = curPara->getPreLine(curLine);
            newPreChar = preLine->last();
            newPostChar = nullptr;
        }
        return;
    }

    // 一般位置（preChar不为空（也不可能是换行））
    DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
    DmMTextLine* curLine = static_cast<DmMTextLine*>(preChar->getParent());
    int charIdx = curLine->findChar(preChar);
    newPreChar = curLine->charAt(charIdx - 1);
    newPostChar = const_cast<DmChar*>(preChar);
}

void DmMText::getPostChar(const DmChar* preChar, const DmChar* postChar,
    DmChar*& newPreChar, DmChar*& newPostChar)
{
    newPreChar = nullptr;
    newPostChar = nullptr;
    // 最后的空行
    if (nullptr == preChar && nullptr == postChar)
    {
        return;
    }
    // 一行的末尾
    if (nullptr == postChar)
    {
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
        DmMTextLine* curLine = static_cast<DmMTextLine*>(preChar->getParent());
        bool isLastLine = curPara->findLine(curLine) == curPara->size() - 1;
        bool isLastPara = findPara(curPara) == size() - 1;
        // 一个段落的最后一行
        if (isLastLine)
        {
            // 最后一个段落
            if (isLastPara)
            {
                newPreChar = const_cast<DmChar*>(preChar);
                newPostChar = const_cast<DmChar*>(postChar);
                return;
            }
            else
            {
                DmMTextParagraph* postPara = getPostParagraph(curPara);
                DmMTextLine* postParaFirstLine = postPara->lineAt(0);
                newPreChar = nullptr;
                newPostChar = postParaFirstLine->charAt(0);
            }
        }
        else
        {
            DmMTextLine* postLine = curPara->getPostLine(curLine);
            newPreChar = nullptr;
            newPostChar = postLine->charAt(0);
        }
        return;
    }

    // 非最后段落的末尾
    if (postChar->isNewLine())
    {
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        DmMTextParagraph* postPara = getPostParagraph(curPara);
        DmMTextLine* postFirstLine = static_cast<DmMTextLine*>(postPara->first());
        newPreChar = nullptr;
        newPostChar = postFirstLine->isEmpty() ? nullptr : static_cast<DmChar*>(postFirstLine->first());
        return;
    }

    // 一般情况（postChar不会是换行或空）
    DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
    DmMTextLine* curLine = static_cast<DmMTextLine*>(postChar->getParent());
    int postCharIdx = curLine->findChar(postChar);
    newPreChar = const_cast<DmChar*>(postChar);
    newPostChar = curLine->charAt(postCharIdx + 1);
}

void DmMText::getUpChar(const DmChar* preChar, const DmChar* postChar,
    DmChar*& newPreChar, DmChar*& newPostChar)
{
    newPreChar = nullptr;
    newPostChar = nullptr;

    // 最后一个空行
    if (nullptr == preChar && nullptr == postChar)
    {
        if (size() == 1) // 仅有1个段落
        {
            newPreChar = nullptr;
            newPostChar = nullptr;
            return;
        }
        // 有1个以上段落
        DmMTextParagraph* prePara = paraAt(size() - 2);
        DmMTextLine* preLastLine = static_cast<DmMTextLine*>(prePara->last());
        newPreChar = nullptr;
        newPostChar = static_cast<DmChar*>(preLastLine->first());
        return;
    }

    // 其他情况，用当前光标位置去定位上一行文字
    double x = 0.0;
    DmMTextParagraph* curPara = nullptr;
    DmMTextLine* curLine = nullptr;
    if (nullptr == postChar)
    {
        curPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
        curLine = static_cast<DmMTextLine*>(preChar->getParent());
        x = preChar->getPosition().x + preChar->getWidth() + preChar->getNominalHeight() * CHARGAPFACTOR;
    }
    else
    {
        curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        curLine = static_cast<DmMTextLine*>(postChar->getParent());
        x = postChar->getPosition().x - postChar->getNominalHeight() * CHARGAPFACTOR;
    }
    DmMTextLine* preLine = curPara->getPreLine(curLine);
    if (nullptr == preLine)
    {
        DmMTextParagraph* prePara = getPreParagraph(curPara);
        if (nullptr != prePara)
        {
            preLine = static_cast<DmMTextLine*>(prePara->last());
        }
    }
    if (nullptr == preLine)
    {
        newPreChar = const_cast<DmChar*>(preChar);
        newPostChar = const_cast<DmChar*>(postChar);
        return;
    }
    DmVector leftTop = preLine->getLeftTop();
    double height = preLine->getHeightWithSpace();
    DmVector pos(x, leftTop.y - height / 2.0);
    preLine->locateChar(pos, false, newPreChar, newPostChar);
}

void DmMText::getDownChar(const DmChar* preChar, const DmChar* postChar,
    DmChar*& newPreChar, DmChar*& newPostChar)
{
    newPreChar = nullptr;
    newPostChar = nullptr;

    // 最后一个空行
    if (nullptr == preChar && nullptr == postChar)
    {
        newPreChar = const_cast<DmChar*>(preChar);
        newPostChar = const_cast<DmChar*>(postChar);
        return;
    }

    // 其他情况
    double x = 0.0;
    DmMTextParagraph* curPara = nullptr;
    DmMTextLine* curLine = nullptr;
    if (nullptr == postChar)
    {
        curPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
        curLine = static_cast<DmMTextLine*>(preChar->getParent());
        x = preChar->getPosition().x + preChar->getWidth() + preChar->getNominalHeight() * CHARGAPFACTOR;
    }
    else
    {
        curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        curLine = static_cast<DmMTextLine*>(postChar->getParent());
        x = postChar->getPosition().x - postChar->getNominalHeight() * CHARGAPFACTOR;
    }
    // 没有后一行，不处理
    DmMTextLine* postLine = curPara->getPostLine(curLine);
    if (nullptr == postLine)
    {
        DmMTextParagraph* postPara = getPostParagraph(curPara);
        if (nullptr != postPara)
        {
            postLine = static_cast<DmMTextLine*>(postPara->first());
        }
    }
    if (nullptr == postLine)
    {
        newPreChar = const_cast<DmChar*>(preChar);
        newPostChar = const_cast<DmChar*>(postChar);
        return;
    }
    // 用当前光标位置去定位下一行文字
    DmVector leftTop = postLine->getLeftTop();
    double height = postLine->getHeightWithSpace();
    DmVector pos(x, leftTop.y - height / 2.0);
    postLine->locateChar(pos, false, newPreChar, newPostChar);
}

DmEntityContainer* DmMText::getSelectedCover(const DmChar* preCharBegin,
    const DmChar* postCharBegin, const DmChar* preCharEnd, const DmChar* postCharEnd)
{
    // 同一个位置
    if (preCharBegin == preCharEnd && postCharBegin == postCharEnd)
    {
        return nullptr;
    }

    // 查找起始终止文字
    DmChar* startChar = nullptr;
    DmChar* endChar = nullptr;
    getStartEndCharByPos(startChar, endChar, preCharBegin, postCharBegin, preCharEnd, postCharEnd);
    if (startChar == nullptr || endChar == nullptr) // 啥也没选中
    {
        return nullptr;
    }

    // 从起始字符生成选择高亮
    std::unique_ptr<DmEntityContainer> cover(std::make_unique<DmEntityContainer>(nullptr));
    DmMTextLine* startLine = static_cast<DmMTextLine*>(startChar->getParent());
    DmMTextParagraph* startPara = static_cast<DmMTextParagraph*>(startLine->getParent());
    int startLineIdx = startPara->findLine(startLine);
    int startParaIdx = findPara(startPara);
    // 未选择到空行
    DmMTextLine* endLine = static_cast<DmMTextLine*>(endChar->getParent());
    DmMTextParagraph* endPara = static_cast<DmMTextParagraph*>(endLine->getParent());
    int endLineIdx = endPara->findLine(endLine);
    int endParaIdx = findPara(endPara);

    // 仅选择一行的部分文字
    DmSolid* solid = nullptr;
    if (startPara == endPara && startLine == endLine)
    {
        solid = getSelectedCoverOfLine(startChar, endChar);
        if (solid)
        {
            solid->setParent(cover.get());
            cover->addEntity(solid);
        }
    }
    else
    {
        DmMTextLine* curLine = startLine;
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(curLine->getParent());
        int curLineIdx = curPara->findLine(curLine);
        int curParaIdx = findPara(curPara);
        bool isStart = false;
        while (curLine != endLine)
        {
            // 第一行遮罩从起始字符到最后
            if (!isStart)
            {
                int startIdx = curLine->findChar(startChar);
                if (startIdx >= 0 && startIdx <= (int)curLine->size() - 1)
                {
                    // 添加遮罩
                    solid = getSelectedCoverOfLine(startChar, static_cast<DmChar*>(curLine->last()));
                }
                isStart = true;
            }
            // 中间行遮罩为整行
            else
            {
                solid = getSelectedCoverOfLine(static_cast<DmChar*>(curLine->first()),
                    static_cast<DmChar*>(curLine->last()));
            }
            if (solid)
            {
                solid->setParent(cover.get());
                cover->addEntity(solid);
            }

            // 迭代下一行
            curLine = curPara->getPostLine(curLineIdx);
            if (nullptr == curLine)
            {
                curPara = getPostParagraph(curParaIdx);
                curParaIdx++;
                curLine = static_cast<DmMTextLine*>(curPara->first());
                curLineIdx = 0;
            }
            else
            {
                curLineIdx++;
            }
        }
        // 最后一行遮罩为行首到终止字符
        solid = getSelectedCoverOfLine(static_cast<DmChar*>(curLine->first()), endChar);
        if (solid)
        {
            solid->setParent(cover.get());
            cover->addEntity(solid);
        }
    }
    if (cover->isEmpty())
    {
        return nullptr;
    }
    else
    {
        return cover.release();
    }
}

DmSolid* DmMText::getSelectedCoverOfLine(DmChar* startChar, DmChar* endChar)
{
    if (nullptr == startChar || nullptr == endChar)
    {
        return nullptr;
    }
    DmMTextLine* line = static_cast<DmMTextLine*>(startChar->getParent());
    DmVector lineTopLeft = line->getLeftTop();
    double heightWidthSpace = line->getHeightWithSpace();
    double x1 = startChar->getPosition().x - startChar->getNominalHeight() * CHARGAPFACTOR;
    double x2 = endChar->getPosition().x + endChar->getWidth() + endChar->getNominalHeight() * CHARGAPFACTOR;
    double topY = lineTopLeft.y;
    DmVector p1(x1, topY);
    DmVector p2(x2, topY);
    DmVector p3(x2, topY - heightWidthSpace);
    DmVector p4(x1, topY - heightWidthSpace);
    std::vector<DmVector> pts{ p1, p2, p3, p4 };
    DmSolid* solid = new DmSolid(nullptr, SolidData(pts));
    DmPen pen(DmColor(DM::FlagByBlock), DM::WidthByBlock, DmLineTypeTable::ByBlock);
    solid->setPen(pen);
    return solid;
}

void DmMText::getOrderedPos(const DmChar* preCharBegin, const DmChar* postCharBegin,
    const DmChar* preCharEnd, const DmChar* postCharEnd,
    const DmChar*& preCharBeginNew, const DmChar*& postCharBeginNew,
    const DmChar*& preCharEndNew, const DmChar*& postCharEndNew)
{
    // 确定文字前后顺序
    preCharBeginNew = nullptr;
    postCharBeginNew = nullptr;
    preCharEndNew = nullptr;
    postCharEndNew = nullptr;
    if (nullptr == preCharBegin && nullptr == postCharBegin)
    {
        preCharBeginNew = preCharEnd;
        postCharBeginNew = postCharEnd;
    }
    else if (nullptr == preCharEnd && nullptr == postCharEnd)
    {
        preCharBeginNew = preCharBegin;
        postCharBeginNew = postCharBegin;
    }
    else
    {
        DmChar* beginChar = nullptr;
        if (nullptr == preCharBegin)
        {
            beginChar = const_cast<DmChar*>(postCharBegin);
        }
        else
        {
            beginChar = const_cast<DmChar*>(preCharBegin);
        }
        DmMTextLine* beginLine = static_cast<DmMTextLine*>(beginChar->getParent());
        DmMTextParagraph* beginPara = static_cast<DmMTextParagraph*>(beginLine->getParent());
        int beginParaIdx = findPara(beginPara);
        int beginLineIdx = beginPara->findLine(beginLine);
        int beginCharIdx = beginLine->findChar(beginChar);

        DmChar* endChar = nullptr;
        if (nullptr == preCharEnd)
        {
            endChar = const_cast<DmChar*>(postCharEnd);
        }
        else
        {
            endChar = const_cast<DmChar*>(preCharEnd);
        }
        DmMTextLine* endLine = static_cast<DmMTextLine*>(endChar->getParent());
        DmMTextParagraph* endPara = static_cast<DmMTextParagraph*>(endLine->getParent());
        int endParaIdx = findPara(endPara);
        int endLineIdx = endPara->findLine(endLine);
        int endCharIdx = endLine->findChar(endChar);

        bool reverse = false;
        if (beginParaIdx == endParaIdx)
        {
            if (beginLineIdx == endLineIdx)
            {
                if (beginCharIdx > endCharIdx)
                {
                    reverse = true;
                }
            }
            else if (beginLineIdx > endLineIdx)
            {
                reverse = true;
            }
        }
        else if (beginParaIdx > endParaIdx)
        {
            reverse = true;
        }
        if (reverse)
        {
            preCharBeginNew = preCharEnd;
            postCharBeginNew = postCharEnd;
            preCharEndNew = preCharBegin;
            postCharEndNew = postCharBegin;
        }
        else
        {
            preCharBeginNew = preCharBegin;
            postCharBeginNew = postCharBegin;
            preCharEndNew = preCharEnd;
            postCharEndNew = postCharEnd;
        }
    }
}

void DmMText::getStartEndCharByOrderedPos(DmChar*& startChar, DmChar*& endChar,
    const DmChar* preCharBeginNew, const DmChar* postCharBeginNew,
    const DmChar* preCharEndNew, const DmChar* postCharEndNew)
{
    // 查找起始文字
    startChar = nullptr;
    // 啥也没选中
    if (nullptr == preCharBeginNew && nullptr == postCharBeginNew)
    {
        return;
    }
    // 一行的末尾
    else if (nullptr == postCharBeginNew)
    {
        DmMTextLine* curLine = static_cast<DmMTextLine*>(preCharBeginNew->getParent());
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(curLine->getParent());
        auto postLine = curPara->getPostLine(curLine);
        if (nullptr == postLine)
        {
            auto postPara = getPostParagraph(curPara);
            if (nullptr == postPara)
            {
                return;
            }
            auto postParaFirstLine = static_cast<DmMTextLine*>(postPara->first());
            if (postParaFirstLine->isEmpty())
            {
                return;
            }
            startChar = static_cast<DmChar*>(postParaFirstLine->first());
        }
        else
        {
            startChar = static_cast<DmChar*>(postLine->first());
        }
    }
    else
    {
        startChar = const_cast<DmChar*>(postCharBeginNew);
    }

    // 查找终止文字
    endChar = nullptr;
    // 最后的空行
    if (nullptr == preCharEndNew && nullptr == postCharEndNew)
    {
        DmMTextParagraph* lastPrePara = paraAt(size() - 2);
        DmMTextLine* lastPreParaLastLine = static_cast<DmMTextLine*>(lastPrePara->last());
        endChar = static_cast<DmChar*>(lastPreParaLastLine->last());
    }
    // 行的起始
    else if (nullptr == preCharEndNew)
    {
        DmMTextLine* curLine = static_cast<DmMTextLine*>(postCharEndNew->getParent());
        DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(curLine->getParent());
        DmMTextLine* preLine = curPara->getPreLine(curLine);
        if (nullptr == preLine)
        {
            auto prePara = getPreParagraph(curPara);
            if (nullptr == prePara)
            {
                return;
            }
            auto preParaLastLine = static_cast<DmMTextLine*>(prePara->last());
            if (preParaLastLine->isEmpty())
            {
                return;
            }
            endChar = static_cast<DmChar*>(preParaLastLine->last());
        }
        else
        {
            endChar = static_cast<DmChar*>(preLine->last());
        }
    }
    else
    {
        endChar = const_cast<DmChar*>(preCharEndNew);
    }
}

void DmMText::getStartEndCharByPos(DmChar*& startChar, DmChar*& endChar,
    const DmChar* preCharBegin, const DmChar* postCharBegin,
    const DmChar* preCharEnd, const DmChar* postCharEnd)
{
    // 确定位置前后顺序
    const DmChar* preCharBeginNew = nullptr;
    const DmChar* postCharBeginNew = nullptr;
    const DmChar* preCharEndNew = nullptr;
    const DmChar* postCharEndNew = nullptr;
    getOrderedPos(preCharBegin, postCharBegin, preCharEnd, postCharEnd,
        preCharBeginNew, postCharBeginNew, preCharEndNew, postCharEndNew);

    // 查找起始终止文字
    startChar = nullptr;
    endChar = nullptr;
    getStartEndCharByOrderedPos(startChar, endChar,
        preCharBeginNew, postCharBeginNew, preCharEndNew, postCharEndNew);
}

void DmMText::delSelectedChars(const DmChar* preCharBegin, const DmChar* postCharBegin,
    const DmChar* preCharEnd, const DmChar* postCharEnd)
{
    // 同一个位置
    if (preCharBegin == preCharEnd && postCharBegin == postCharEnd)
    {
        return;
    }

    // 查找起始终止文字
    DmChar* startChar = nullptr;
    DmChar* endChar = nullptr;
    getStartEndCharByPos(startChar, endChar, preCharBegin, postCharBegin, preCharEnd, postCharEnd);
    if (startChar == nullptr || endChar == nullptr) // 啥也没选中
    {
        return;
    }

    // 移除文字
    bool paraRemoved = false;
    bool lineRemoved = false;
    bool charRemoved = false;
    // 仅选择了一个文字
    if (startChar == endChar)
    {
        delChar(startChar, paraRemoved, lineRemoved);
    }
    // 选择了多个文字
    else
    {
        bool end = false;
        bool start = false;
        int paraIdx = 0;
        while (paraIdx < paragraphs.size())
        {
            DmEntity* paraEnt = paragraphs.at(paraIdx);
            DmMTextParagraph* para = static_cast<DmMTextParagraph*>(paraEnt);
            paraRemoved = false;
            int lineIdx = 0;
            while (lineIdx < para->size())
            {
                DmEntity* lineEnt = para->lineAt(lineIdx);
                DmMTextLine* line = static_cast<DmMTextLine*>(lineEnt);
                lineRemoved = false;
                int cIdx = 0;
                while (cIdx < line->size())
                {
                    DmEntity* charEnt = line->charAt(cIdx);
                    DmChar* c = static_cast<DmChar*>(charEnt);
                    charRemoved = false;
                    if (!start)
                    {
                        if (c == startChar)
                        {
                            start = true;
                        }
                    }
                    if (start)
                    {
                        if (!end && c == endChar)
                        {
                            end = true;
                        }
                    }
                    if (start)
                    {
                        delChar(c, paraRemoved, lineRemoved);
                        charRemoved = true;
                    }
                    if (lineRemoved || end)
                    {
                        break;
                    }
                    if (!charRemoved)
                    {
                        cIdx++;
                    }
                }
                if (paraRemoved || end)
                {
                    break;
                }
                if (!lineRemoved)
                {
                    lineIdx++;
                }
            }
            if (end)
            {
                break;
            }
            if (!paraRemoved)
            {
                paraIdx++;
            }
        }
    }
}

void DmMText::delChar(DmChar* theChar, bool& paraRemoved, bool& lineRemoved)
{
    DmMTextLine* line = static_cast<DmMTextLine*>(theChar->getParent());
    DmMTextParagraph* para = static_cast<DmMTextParagraph*>(line->getParent());
    // 移除换行符，将下一个段落的文字塞入当前段落
    if (theChar->isNewLine())
    {
        int charCount = line->size();
        int lineCount = para->size();
        // 仅剩换行，删除该段落
        if (lineCount == 1 && charCount == 1)
        {
            removePara(para);
            paraRemoved = true;
            lineRemoved = true;
        }
        // 本段落还有文字
        else
        {
            line->removeChar(theChar, true); // 移除换行符
            // 将下一段落文字塞入本行
            DmMTextParagraph* postPara = getPostParagraph(para);
            for (auto& postParaLineEnt : *postPara)
            {
                DmMTextLine* postLine = static_cast<DmMTextLine*>(postParaLineEnt);
                for (auto& ch : *postLine)
                {
                    ch->setParent(line);
                    line->addChar(ch);
                }
            }
            postPara->clearWithoutDelChars();
            removePara(postPara);
            paraRemoved = false;
            lineRemoved = false;
        }
    }
    // 移除一般字符
    else
    {
        line->removeChar(theChar, true);
        paraRemoved = false;
        lineRemoved = false;
        if (line->size() == 0)
        {
            if (size() > 1 || para->size() > 1) // 当仅剩一个空行时不能删除
            {
                para->removeLine(line);
                lineRemoved = true;
            }

            if (size() > 1 && para->size() == 0) // 最后一个段落不能删除
            {
                removePara(para);
                paraRemoved = true;
                lineRemoved = true;
            }
        }
    }
}

DmMTextParagraph* DmMText::getPreParagraph(DmMTextParagraph* para)
{
    return getPrePostParagraph(para, true);
}

DmMTextParagraph* DmMText::getPreParagraph(int paraIdx)
{
    return getPrePostParagraph(paraIdx, true);
}

DmMTextParagraph* DmMText::getPostParagraph(DmMTextParagraph* para)
{
    return getPrePostParagraph(para, false);
}

DmMTextParagraph* DmMText::getPostParagraph(int paraIdx)
{
    return getPrePostParagraph(paraIdx, false);
}

DmChar* DmMText::getPostChar(const DmChar* c, bool containNewLine, bool crossPara)
{
    // 先查找当前行的后一个字符是否存在
    DmMTextLine* line = static_cast<DmMTextLine*>(c->getParent());
    DmMTextParagraph* para = static_cast<DmMTextParagraph*>(line->getParent());
    DmMText* mtext = static_cast<DmMText*>(para->getParent());
    int cIdx = line->findChar(c);
    if (cIdx + 1 < line->size())
    {
        DmChar* nextC = line->charAt(cIdx + 1);
        if (!nextC->isNewLine())
        {
            return nextC;
        }
        if (containNewLine)
        {
            return nextC;
        }
    }

    // 当前行不满足
    DmChar* nextC = nullptr;
    bool found = false;
    while (!found)
    {
        line = para->getPostLine(line);
        if (nullptr == line)
        {
            if (crossPara)
            {
                para = mtext->getPostParagraph(para);
                if (nullptr == para)
                {
                    break;
                }
                else
                {
                    line = static_cast<DmMTextLine*>(para->first());
                }
            }
            else
            {
                break;
            }
        }

        // 验证当前行
        if (!line->isEmpty())
        {
            nextC = static_cast<DmChar*>(line->first());
            if (!nextC->isNewLine())
            {
                found = true;
                break;
            }
            if (containNewLine)
            {
                found = true;
                break;
            }
        }
    }

    // 返回结果
    if (found)
    {
        return nextC;
    }
    else
    {
        return nullptr;
    }
}

const MTextData* DmMText::getDataConstPtr() const
{
    return m_data.get();
}

double DmMText::calculateHeight() const
{
    DmMTextParagraph* lastPara = last();
    double totalHeight = 0.0;
    for (auto para : paragraphs)
    {
        if (para == lastPara)
        {
            totalHeight += para->getHeightWithoutLastDescenderSpace();
        }
        else
        {
            totalHeight += para->getHeight();
        }
    }
    return totalHeight;
}

DmChar* DmMText::createChar(const QString& charStr, const DmTextStyle* style,
    const DmColor& color, const double& height, const double& widthFactor,
    const double& slashAngle, const QString& fontName, const bool& bold,
    const bool& italic, const bool& hasUnderline, const bool& hasStrikethrough,
    const bool& hasOverline)
{
    DmPen pen(color, DM::Width00, DmLineTypeTable::Continuous);
    DmChar* c = nullptr;
    bool isBold = bold;
    bool isItalic = italic;
    DmFont* font = nullptr;
    DmCharTemplate* templ = nullptr;
    font = DMFONTLIST->requestFontCloset(fontName, isBold, isItalic);
    if (!font)
    {
        return nullptr;
    }
    bool isShx = font->isShxFont();
    templ = font->findLetter(charStr);
    // 字体不匹配文字，用其他字体替代
    if (!templ)
    {
        // 如果UI选择的是文字样式的shx字体，文字样式有大字体，用大字体再找一遍
        DmFont* asciiFont = style->getDataConstPtr()->pAsciiFont;
        DmFont* bigFont = style->getDataConstPtr()->pBigFont;
        if (isShx && bigFont != nullptr && asciiFont != nullptr && asciiFont->getFileName() == fontName)
        {
            font = bigFont;
            templ = font->findLetter(charStr);
        }

        // 找不到再用预定义的字体
        if (!templ)
        {
            QString predefineFontName = DmFont::getPredefineFontNameOfLetter(charStr);
            if (predefineFontName.isEmpty())
            {
                return nullptr;
            }
            font = DMFONTLIST->requestFont(predefineFontName, false);
            if (!font)
            {
                return nullptr;
            }
            templ = font->findLetter(charStr);
        }
    }
    if (!templ)
    {
        return nullptr;
    }
    c = templ->generateChar(widthFactor, slashAngle);
    c->setBold(bold);
    c->setItalic(italic);
    if (hasUnderline)
    {
        c->addUnderline();
    }
    if (hasStrikethrough)
    {
        c->addStrikethrough();
    }
    if (hasOverline)
    {
        c->addOverline();
    }
    c->setPen(pen);
    c->scale(DmVector(0.0, 0.0), DmVector(height, height)); // 高度
    return c;
}

void DmMText::replaceChar(DmChar* originChar, DmChar* replaceChar, const bool& delOrigin)
{
    if (nullptr == originChar)
    {
        return;
    }
    if (nullptr == replaceChar)
    {
        return;
    }

    // 删除原始，插入新增
    DmMTextLine* line = static_cast<DmMTextLine*>(originChar->getParent());
    int originIdx = line->findChar(originChar);
    line->removeChar(originChar, delOrigin);
    line->insertChar(originIdx, replaceChar);
    replaceChar->setParent(line);
}

void DmMText::initInfoFromStyle(const DmTextStyle* style, DmColor& color, double& height,
    double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic,
    bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline)
{
    auto styleData = style->getDataConstPtr();
    color = DmColor(DM::FlagByLayer); // 默认随层，在autocad中，起始字符随层没有"\\C256"命令
    height = styleData->defaultHeight;
    widthFactor = styleData->widhFactor;
    slashAngle = styleData->slashAngle;
    isBold = false;
    isItalic = false;
    hasUnderline = false;
    hasStrikethrough = false;
    hasOverline = false;
    if (styleData->isSystemFont)
    {
        fontName = styleData->sysFontFamily;
        isBold = styleData->isSysFontBold;
        isItalic = styleData->isSysFontItalic;
    }
    else
    {
        if (styleData->pAsciiFont)
        {
            fontName = styleData->pAsciiFont->getFileName();
        }
    }
}

void DmMText::initInfoFromChar(const DmChar* c, DmColor& color, double& height,
    double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic,
    bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline)
{
    color = c->getPen().getColor();
    height = c->getNominalHeight();
    widthFactor = c->getWidthFactor();
    slashAngle = c->getSlashAngle();
    isBold = c->isBold();
    isItalic = c->isItalic();
    hasUnderline = c->hasUnderline();
    hasStrikethrough = c->hasStrikethrough();
    hasOverline = c->hasOverline();

    bool bold = false;
    bool italic = false;
    DmCharTemplate* charTempl = c->getCharTemplate();
    DmCharTemplateList* templList = charTempl->getOwner();
    DmFont* font = templList->getFont();
    fontName = DMFONTLIST->getFontFamilyName(font, bold, italic);
}

std::list<DmEntity*> DmMText::getSubEntities() const
{
    std::list<DmEntity*> subEnts = std::list<DmEntity*>();
    for (auto e : paragraphs)
    {
        auto paraEnts = ((DmMTextParagraph*)e)->getSubEntities();
        subEnts.splice(subEnts.end(), paraEnts);
    }
    return subEnts;
}

void DmMText::init(const DmVector& pos, const double width, const double height,
    const double charHeight)
{
    setPosition(pos);
    setWidth(width);
    setHeight(height);

    init(charHeight);
}

void DmMText::init(const double charHeight)
{
    // 初始为空的时候添加一个段落
    DmMTextParagraph* para = new DmMTextParagraph(this);
    para->setLeftTop(getPosition());
    DmMTextLine* firstLine = para->lineAt(0);
    firstLine->setHeight(charHeight);
    addPara(para);
}

bool DmMText::isEmptyText() const
{
    DmMTextParagraph* para = nullptr;
    DmMTextLine* line = nullptr;
    DmChar* ch = nullptr;
    for (auto& p : paragraphs)
    {
        para = p;
        for (auto& l : *para)
        {
            line = static_cast<DmMTextLine*>(l);
            for (auto& c : *line)
            {
                ch = static_cast<DmChar*>(c);
                if (!ch->isSpace() && !ch->isNewLine() && !ch->isTab())
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool DmMText::isJustifyAlignHLeft() const
{
    EMTextMode justify = getJustification();
    bool justifyAlignLeft = justify == EMTextMode::kTextTopLeft
        || justify == EMTextMode::kTextMiddleLeft
        || justify == EMTextMode::kTextBottomLeft;
    return justifyAlignLeft;
}

bool DmMText::isJustifyAlignHCenter() const
{
    EMTextMode justify = getJustification();
    bool justifyAlignCenter = justify == EMTextMode::kTextTopCenter
        || justify == EMTextMode::kTextMiddleCenter
        || justify == EMTextMode::kTextBottomCenter;
    return justifyAlignCenter;
}

bool DmMText::isJustifyAlignHRight() const
{
    EMTextMode justify = getJustification();
    bool justifyAlignRight = justify == EMTextMode::kTextTopRight
        || justify == EMTextMode::kTextMiddleRight
        || justify == EMTextMode::kTextBottomRight;
    return justifyAlignRight;
}

bool DmMText::isJustifyAlignVTop() const
{
    EMTextMode justify = getJustification();
    bool justifyAlignTop = justify == EMTextMode::kTextTopLeft
        || justify == EMTextMode::kTextTopCenter
        || justify == EMTextMode::kTextTopRight;
    return justifyAlignTop;
}

bool DmMText::isJustifyAlignVMid() const
{
    EMTextMode justify = getJustification();
    bool justifyAlignMid = justify == EMTextMode::kTextMiddleLeft
        || justify == EMTextMode::kTextMiddleCenter
        || justify == EMTextMode::kTextMiddleRight;
    return justifyAlignMid;
}

bool DmMText::isJustifyAlignVBottom() const
{
    EMTextMode justify = getJustification();
    bool justifyAlignBottom = justify == EMTextMode::kTextBottomLeft
        || justify == EMTextMode::kTextBottomCenter
        || justify == EMTextMode::kTextBottomRight;
    return justifyAlignBottom;
}

DmVector DmMText::getPosition() const
{
    return m_data->getPosition();
}

void DmMText::setPosition(const DmVector& pos)
{
    m_data->setPosition(pos);
}

double DmMText::getWidth() const
{
    return m_data->getDefineWidth();
}

void DmMText::setWidth(const double width)
{
    m_data->setDefineWidth(width);
}

double DmMText::calculateWidth() const
{
    double maxWidth = 0.0;
    double temp = 0.0;
    for (auto para : paragraphs)
    {
        temp = para->getWidth();
        if (temp > maxWidth)
        {
            maxWidth = temp;
        }
    }

    // 不能大于定义宽度
    if (maxWidth > m_data->getDefineWidth())
    {
        return m_data->getDefineWidth();
    }
    else
    {
        return maxWidth;
    }
}

double DmMText::getHeight() const
{
    return m_data->getDefineHeight();
}

void DmMText::setHeight(const double height)
{
    m_data->setDefineHeight(height);
}

double DmMText::getCharHeight() const
{
    return m_data->getCharHeight();
}

void DmMText::setCharHeight(const double height)
{
    m_data->setCharHeight(height);
}

EMTextMode DmMText::getJustification() const
{
    return m_data->getJustification();
}

void DmMText::setJustification(EMTextMode justification)
{
    m_data->setJustification(justification);
}

DmTextStyle* DmMText::getTextStyle() const
{
    return m_data->getTextStyle();
}

void DmMText::setTextStyle(DmTextStyle* style)
{
    m_data->setTextStyle(style);
}

double DmMText::getLineSpacingFactor() const
{
    return m_data->getLineSpacingFactor();
}

void DmMText::setLineSpacingFactor(const double factor)
{
    m_data->setLineSpacingFactor(factor);
}

double DmMText::getAngle() const
{
    return m_data->getAngle();
}

void DmMText::setAngle(const double angle)
{
    m_data->setAngle(angle);
}

double DmMText::getLineSpace() const
{
    return m_data->getLineSpace();
}

void DmMText::setLineSpace(const double lineSpace)
{
    m_data->setLineSpace(lineSpace);
}

void DmMText::update()
{
    clear();
    MTextContentCmdMgr mgr(this);
    mgr.generateEntities();

    // Rotate:
    for (auto para : paragraphs)
    {
        para->move(-m_data->getPosition());
    }
    for (auto para : paragraphs)
    {
        DmVector angleVector(m_data->getAngle());
        para->rotate(DmVector(0.0, 0.0), angleVector);
    }
    for (auto para : paragraphs)
    {
        para->move(m_data->getPosition());
    }

    calculateBorders();
}

void DmMText::updateContent()
{
    MTextContentCmdMgr mgr(this);
    QString content = mgr.generateContent();
    m_data->setTextString(content);
}

QString DmMText::getContent() const
{
    return m_data->getTextString();
}

QString DmMText::getContentByRange(const DmChar* startChar, const DmChar* endChar) const
{
    DmMText* mtext = const_cast<DmMText*>(this);
    MTextContentCmdMgr mgr(mtext);
    QString content = mgr.generateContentOfRange(startChar, endChar);
    return content;
}

DmChar* DmMText::insertContentAtPos(const DmChar* preChar, const DmChar* postChar,
    const QString& content)
{
    MTextContentCmdMgr mgr(this);
    return mgr.insertContentAtPos(preChar, postChar, content);
}

void DmMText::setContent(const QString& content)
{
    m_data->setTextString(content);
    update();
}

void DmMText::setStyle(DmTextStyle* pStyle)
{
    m_data->setTextStyle(pStyle);
}

DmTextStyle* DmMText::getStyle() const
{
    return m_data->getTextStyle();
}

int DmMText::size() const
{
    return paragraphs.size();
}

DmMTextParagraph* DmMText::first() const
{
    if (paragraphs.size() == 0)
    {
        return nullptr;
    }
    return paragraphs.front();
}

DmMTextParagraph* DmMText::last() const
{
    if (paragraphs.size() == 0)
    {
        return nullptr;
    }
    return paragraphs.back();
}

DmMTextParagraph* DmMText::paraAt(int index)
{
    if (paragraphs.size() > index && index >= 0)
    {
        return paragraphs.at(index);
    }
    else
    {
        return nullptr;
    }
}

void DmMText::addPara(DmMTextParagraph* para)
{
    paragraphs.emplace_back(para);
}

int DmMText::findPara(DmMTextParagraph* para) const
{
    auto it = std::find(paragraphs.begin(), paragraphs.end(), para);
    if (it == paragraphs.end())
    {
        return -1;
    }
    else
    {
        return it - paragraphs.begin();
    }
}

void DmMText::insertPara(int index, DmMTextParagraph* para)
{
    if (!para)
    {
        return;
    }
    paragraphs.insert(paragraphs.begin() + index, para);
}

void DmMText::insertParas(int index, const std::vector<DmMTextParagraph*>& paras)
{
    if (paras.size() == 0)
    {
        return;
    }
    paragraphs.insert(paragraphs.begin() + index, paras.begin(), paras.end());
}

void DmMText::removePara(int index, DmMTextParagraph*& removedPara)
{
    removedPara = paragraphs.at(index);
    paragraphs.erase(paragraphs.begin() + index);
}

void DmMText::removeParas(int index, int count, std::vector<DmMTextParagraph*>& paras)
{
    paras.insert(paras.begin(), paragraphs.begin() + index, paragraphs.begin() + index + count);
    paragraphs.erase(paragraphs.begin() + index, paragraphs.begin() + index + count);
}

void DmMText::getParas(int index, int count, std::vector<DmMTextParagraph*>& paras)
{
    paras.insert(paras.begin(), paragraphs.begin() + index, paragraphs.begin() + index + count);
}

void DmMText::removePara(DmMTextParagraph* para)
{
    auto it = std::find(paragraphs.begin(), paragraphs.end(), para);
    if (it != paragraphs.end())
    {
        delete *it;
        paragraphs.erase(it);
    }
}

void DmMText::clear()
{
    for (auto para : paragraphs)
    {
        delete para;
    }
    paragraphs.clear();
}

std::vector<DmMTextParagraph*>::const_iterator DmMText::begin() const
{
    return paragraphs.begin();
}

std::vector<DmMTextParagraph*>::const_iterator DmMText::end() const
{
    return paragraphs.end();
}

std::vector<DmMTextParagraph*>::iterator DmMText::begin()
{
    return paragraphs.begin();
}

std::vector<DmMTextParagraph*>::iterator DmMText::end()
{
    return paragraphs.end();
}

bool DmMText::isContainer() const
{
    return false;
}

void DmMText::calculateBorders()
{
    resetBorders();
    for (auto para : paragraphs)
    {
        para->calculateBorders();
        minV = DmVector::minimum(para->getMin(), minV);
        maxV = DmVector::maximum(para->getMax(), maxV);
    }
}

double DmMText::getDistanceToPoint(const DmVector& coord, DmEntity** entity,
    DM::ResolveLevel level) const
{
    if (coord.x < maxV.x && coord.y < maxV.y && coord.x > minV.x && coord.y > minV.y)
    {
        double minDist = DM_MAXDOUBLE;
        double curDist = 0.0;
        for (auto e : paragraphs)
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

DmVector DmMText::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmMText::getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity,
    double* dist, DmEntity** entity) const
{
    return DmVector(false);
}

DmVector DmMText::getNearestCenter(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmMText::getNearestMiddle(const DmVector& coord, double* dist,
    int middlePoints) const
{
    return DmVector(false);
}

void DmMText::setVisible(bool v)
{
    DmEntity::setVisible(v);
    for (auto e : paragraphs)
    {
        e->setVisible(v);
    }
}

bool DmMText::setSelected(bool select)
{
    if (DmEntity::setSelected(select))
    {
        for (auto e : paragraphs)
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

void DmMText::setHighlighted(bool highlight)
{
    DmEntity::setHighlighted(highlight);
    for (auto e : paragraphs)
    {
        if (e->isVisible())
        {
            e->setHighlighted(highlight);
        }
    }
}

void DmMText::saveStream(OutputStream& str) const
{
    DmEntity::saveStream(str);

    std::string content = m_data->getTextString().toStdString();
    std::string style = m_data->getTextStyle()->getName().toStdString();
    double angle = m_data->getAngle();
    DmVector pos = m_data->getPosition();
    int hMode = (int)m_data->getTextHorzMode();
    int vMode = (int)m_data->getTextVertMode();
    double lineSpacingFactor = m_data->getLineSpacingFactor();
    double lineSpace = m_data->getLineSpace();
    double charHeight = m_data->getCharHeight();
    int justification = (int)m_data->getJustification();
    double defWidth = m_data->getDefineWidth();
    double defHeight = m_data->getDefineHeight();
    int updateMode = (int)m_data->getUpdateMode();
    str << (std::string)content << (std::string)style << (double)angle << (double)pos.x
        << (double)pos.y << (int)hMode << (int)vMode << (double)lineSpacingFactor
        << (double)lineSpace << (double)charHeight << (int)justification << (double)defWidth
        << (double)defHeight << (int)updateMode;
}

void DmMText::restoreStream(InputStream& str, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmMText", revs);
    if (revId > fileRev)
    {
        DmEntity::restoreStream(str, revs);
        // 老文件格式
        restoreStreamWithRev(str, fileRev);
    }
    else
    {
        restoreStream(str);
    }
}

void DmMText::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
        // TODO:  - rev==0时无操作，需要确认是否应该实现
    }
    else // big change, e.g. change supper class of DmText
    {
        // step1.
        // read all legacy data one by one
        // TODO:  - 需要实现旧版本数据读取逻辑
    }
}

void DmMText::restoreStream(InputStream& rdr)
{
    DmEntity::restoreStream(rdr);

    std::string content;
    std::string style;
    double angle = 0.0;
    DmVector pos(true);
    int hMode = 0;
    int vMode = 0;
    double lineSpacingFactor = 1.0;
    double lineSpace = 0.0;
    double charHeight = 0.0;
    int justification = 0;
    double defWidth = 0.0;
    double defHeight = 0.0;
    int updateMode = 0;
    rdr >> (std::string&)content >> (std::string&)style >> (double&)angle >> (double&)pos.x
        >> (double&)pos.y >> (int&)hMode >> (int&)vMode >> (double&)lineSpacingFactor
        >> (double&)lineSpace >> (double&)charHeight >> (int&)justification >> (double&)defWidth
        >> (double&)defHeight >> (int&)updateMode;

    DmTextStyleTable* textStyleTable = getDocument()->getTextStyleTable();
    m_data.reset(new MTextData());
    m_data->setTextString(QString::fromStdString(content));
    DmTextStyle* pStyle = textStyleTable->find(QString::fromStdString(style));
    m_data->setTextStyle(pStyle);
    m_data->setAngle(angle);
    m_data->setPosition(pos);
    m_data->setTextHorzMode((EMTextHorzMode)hMode);
    m_data->setTextVertMode((EMTextVertMode)vMode);
    m_data->setLineSpacingFactor(lineSpacingFactor);
    m_data->setLineSpace(lineSpace);
    m_data->setCharHeight(charHeight);
    m_data->setJustification(EMTextMode(justification));
    m_data->setDefineWidth(defWidth);
    m_data->setDefineHeight(defHeight);
    m_data->setUpdateMode((EUpdateMode)updateMode);
    update();
}

DmMTextParagraph* DmMText::getPrePostParagraph(DmMTextParagraph* para, bool getPre)
{
    if (nullptr == para)
    {
        return nullptr;
    }
    int foundIdx = findPara(para);
    return getPrePostParagraph(foundIdx, getPre);
}

DmMTextParagraph* DmMText::getPrePostParagraph(int paraIdx, bool getPre)
{
    if (paraIdx < 0)
    {
        return nullptr;
    }

    int count = paragraphs.size();
    if (getPre)
    {
        int preIdx = paraIdx - 1;
        if (preIdx >= 0 && preIdx < count)
        {
            return paragraphs.at(preIdx);
        }
    }
    else
    {
        int postIdx = paraIdx + 1;
        if (postIdx >= 0 && postIdx < count)
        {
            return paragraphs.at(postIdx);
        }
    }
    return nullptr;
}
