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


/// @file DmText.cpp
/// @brief 单行文字实体类实现，管理单行文字的字符

#include <iostream>
#include <cmath>
#include "DmFont.h"
#include "DmText.h"

#include "DmFontList.h"
#include "DmBlockReference.h"
#include "Math2d.h"
#include "Debug.h"
#include "GuiDocumentView.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "DmDocument.h"
#include "DmCharTemplate.h"
#include "DmChar.h"

/// @brief 水平空格因子
constexpr double HORIZONTALSPACEFACTOR = 0.8;

/// @brief 水平字符间隔因子
constexpr double HORIZONTALLETTERSPACEFACTOR = 0.1;

/// @brief 垂直空格因子
constexpr double VERTICALSPACEFACTOR = 1.0;

/// @brief 垂直字符间隔因子
constexpr double VERTICALLETTERSPACEFACTOR = 0.2;

/// @brief 文字宽高比的假设值
constexpr double ASSUMED_TEXT_ASPECT_RATIO = 0.5;

/// @brief Y方向三分之一的常量
constexpr double ONE_THIRD_Y = 1.0 / 3.0;

/// @brief 直径符号Unicode编号
constexpr auto DIAMETER_UNICODE = "%%248";

/// @brief 正负号Unicode编号
constexpr auto PLUSMINUS_UNICODE = "%%177";

/// @brief 度符号Unicode编号
constexpr auto DEGREE_UNICODE = "%%176";

TYPESYSTEM_SOURCE(DmText, DmEntity, 0);

DmText::DmText(DmEntity* parent, const TextData& d)
    : DmEntity(parent)
{
    usedTextHeight = 0.0;
    usedTextWidth = 0.0;
    m_data = std::shared_ptr<TextData>(new TextData(d));
    setText(m_data->getTextString());
}

DmText::DmText(const DmText& text)
    : DmEntity(text)
{
    clear();
    usedTextHeight = text.usedTextHeight;
    usedTextWidth = text.usedTextWidth;
    m_data = std::shared_ptr<TextData>(new TextData(*(text.m_data)));
}

DmText::~DmText()
{
    clear();
}

DmEntity* DmText::clone() const
{
    DmText* clonedText = new DmText(*this);
    clonedText->update();
    return clonedText;
}

DM::EntityType DmText::getEntityType() const
{
    return DM::EntityText;
}

void DmText::setText(const QString& t)
{
    m_data->setTextString(t);
}

QString DmText::getText() const
{
    return m_data->getTextString();
}

void DmText::setStyle(DmTextStyle* style)
{
    m_data->setTextStyle(style);
}

DmTextStyle* DmText::getStyle() const
{
    return m_data->getTextStyle();
}

void DmText::setAngle(double a)
{
    m_data->setAngle(a);
}

double DmText::getAngle() const
{
    return m_data->getAngle();
}

bool DmText::getUpsideDown() const
{
    return m_data->getUpsideDown();
}

void DmText::setUpsideDown(const bool& upsideDown)
{
    m_data->setUpsideDown(upsideDown);
}

bool DmText::getReverseDirection() const
{
    return m_data->getReverseDirection();
}

void DmText::setReverseDirection(const bool& reverseDirection)
{
    m_data->setReverseDirection(reverseDirection);
}

double DmText::getWidthFactor() const
{
    return m_data->getWidthFactor();
}

void DmText::setWidthFactor(const double& widthFactor)
{
    m_data->setWidthFactor(widthFactor);
}

double DmText::getSlashAngle() const
{
    return m_data->getSlashAngle();
}

void DmText::setSlashAngle(const double& slashAngle)
{
    m_data->setSlashAngle(slashAngle);
}

double DmText::getUsedTextWidth() const
{
    return usedTextWidth;
}

double DmText::getUsedTextHeight() const
{
    return usedTextHeight;
}

ETextVertMode DmText::getVAlign() const
{
    return m_data->getTextVertMode();
}

void DmText::setVAlign(ETextVertMode va)
{
    m_data->setTextVertMode(va);
}

ETextHorzMode DmText::getHAlign() const
{
    return m_data->getTextHorzMode();
}

void DmText::setHAlign(ETextHorzMode ha)
{
    m_data->setTextHorzMode(ha);
}

DmVector DmText::getPosition() const
{
    return m_data->getPosition();
}

DmVector DmText::getAlignment() const
{
    return m_data->getAlignment();
}

void DmText::setAlignment(const DmVector& pt)
{
    m_data->setAlignment(pt);
}

double DmText::getHeight() const
{
    return m_data->getTextHeight();
}

void DmText::setHeight(double h)
{
    m_data->setTextHeight(h);
}

TextData DmText::getData() const
{
    return *m_data;
}

const TextData* DmText::getDataConstPtr() const
{
    return m_data.get();
}

void DmText::update()
{
    clear();
    QString text = m_data->getTextString();
    if (text.isEmpty())
    {
        return;
    }

    addEntitiesOfText(text);
}

bool DmText::isContainer() const
{
    return false;
}

void DmText::calculateBorders()
{
    resetBorders();
    for (auto c : chars)
    {
        c->calculateBorders();
        minV = DmVector::minimum(c->getMin(), minV);
        maxV = DmVector::maximum(c->getMax(), maxV);
    }
}

double DmText::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
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

DmVector DmText::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    if (dist)
    {
        *dist = m_data->getPosition().distanceTo(coord);
    }
    return m_data->getPosition();
}

DmVector DmText::getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity, double* dist, DmEntity** entity) const
{
    return DmVector(false);
}

DmVector DmText::getNearestCenter(const DmVector& coord, double* dist) const
{
    return DmVector(false);
}

DmVector DmText::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    return DmVector(false);
}

void DmText::setVisible(bool v)
{
    DmEntity::setVisible(v);
    for (auto e : chars)
    {
        e->setVisible(v);
    }
}

bool DmText::setSelected(bool select)
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

void DmText::setHighlighted(bool highlight)
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

void DmText::clear()
{
    if (chars.size() != 0)
    {
        for (auto c : chars)
        {
            delete c;
        }
    }
    chars.clear();
}

void DmText::addChar(DmChar* c)
{
    chars.emplace_back(c);
}

DmVector DmText::getNearestRef(const DmVector& coord, double* dist) const
{
    return DmEntity::getNearestRef(coord, dist);
}

DmVector DmText::getNearestSelectedRef(const DmVector& coord, double* dist) const
{
    return DmEntity::getNearestSelectedRef(coord, dist);
}

DmVectorSolutions DmText::getRefPoints() const
{
    if (m_data->getTextMode() == ETextMode::kTextLeft)
    {
        // 左对齐的对齐点始终为0，不予显示
        return DmVectorSolutions({ m_data->getPosition() });
    }
    else
    {
        return DmVectorSolutions({ m_data->getAlignment(), m_data->getPosition() });
    }
}

void DmText::move(const DmVector& offset)
{
    for (auto c : chars)
    {
        c->move(offset);
    }
    if (m_data->getTextMode() != ETextMode::kTextLeft)
    {
        m_data->setAlignment(m_data->getAlignment().move(offset));
    }
    m_data->setPosition(m_data->getPosition().move(offset));
    moveBorders(offset);
}

void DmText::rotate(const DmVector& center, const DmVector& angleVector)
{
    for (auto c : chars)
    {
        c->rotate(center, angleVector);
    }
    if (m_data->getTextMode() != ETextMode::kTextLeft)
    {
        m_data->setAlignment(getAlignment().rotate(center, angleVector));
    }
    m_data->setPosition(getPosition().rotate(center, angleVector));
    m_data->setAngle(Math2d::correctAngle(m_data->getAngle() + angleVector.angle()));
    calculateBorders();
}

void DmText::scale(const DmVector& center, const DmVector& factor)
{
    if (m_data->getTextMode() != ETextMode::kTextLeft)
    {
        m_data->setAlignment(getAlignment().scale(center, factor));
    }
    m_data->setPosition(getPosition().scale(center, factor));
    m_data->setTextHeight(m_data->getTextHeight() * factor.x);
    update();
}

void DmText::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    bool readable = Math2d::isAngleReadable(m_data->getAngle());

    DmVector vec = DmVector::polar(1.0, m_data->getAngle());
    vec.mirror(DmVector(0.0, 0.0), axisPoint2 - axisPoint1);
    m_data->setAngle(vec.angle());

    bool corr = false;
    m_data->setAngle(Math2d::makeAngleReadable(m_data->getAngle(), readable, &corr));

    if (corr)
    {
        m_data->setAlignment(getAlignment().mirror(axisPoint1, axisPoint2));
        m_data->setPosition(getPosition().mirror(axisPoint1, axisPoint2));
        if (m_data->getTextHorzMode() == ETextHorzMode::kTextLeft)
        {
            m_data->setTextHorzMode(ETextHorzMode::kTextRight);
        }
        else if (m_data->getTextHorzMode() == ETextHorzMode::kTextRight)
        {
            m_data->setTextHorzMode(ETextHorzMode::kTextLeft);
            m_data->setAlignment(DmVector(0.0, 0.0));
        }
    }
    else
    {
        DmVector minP = DmVector(getMin().x, getMax().y);
        minP = minP.mirror(axisPoint1, axisPoint2);
        double mirrAngle = axisPoint1.angleTo(axisPoint2) * 2.0;
        if (m_data->getTextMode() != ETextMode::kTextLeft)
        {
            m_data->setAlignment(getAlignment().move(minP - getMin()));
            m_data->setAlignment(getAlignment().rotate(minP, mirrAngle));
        }
        m_data->setPosition(getPosition().move(minP - getMin()));
        m_data->setPosition(getPosition().rotate(minP, mirrAngle));
    }
    update();
}

void DmText::moveRef(const DmVector& ref, const DmVector& offset)
{
    ETextMode alignMode = m_data->getTextMode();
    // 对齐及布满的拖拽关键点效果
    if (alignMode == ETextMode::kTextAligned || alignMode == ETextMode::kTextFit)
    {
        DmVector alignPt = m_data->getAlignment();
        DmVector pos = m_data->getPosition();
        if (pos.distanceTo(ref) < 1e-4)
        {
            m_data->setPosition(pos.move(offset));
            update();
        }
        else if (alignPt.distanceTo(ref) < 1e-4)
        {
            m_data->setAlignment(alignPt.move(offset));
            update();
        }
    }
    else
    {
        // 不是对齐及布满的情况，拖动关键点均是移动。
        // 说明：DmEntityContainer::moveRef是递归对所有子实体调用moveRef，最基本的实体moveRef是拖动一个端点，会产生变形效果，
        // 比如，对于DmSolid拖拽产生不同的填充三角形，对于DmLine拖拽使一个端点变化。
        // 所以正确的做法是调用move()，而不是DmEntityContainer::moveRef()
        move(offset);
    }
}

bool DmText::hasEndpointsWithinWindow(const DmVector& /*v1*/, const DmVector& /*v2*/)
{
    return false;
}

std::list<DmEntity*> DmText::getSubEntities() const
{
    std::list<DmEntity*> subEnts = std::list<DmEntity*>();
    for (auto c : chars)
    {
        // 收集所有单个文字轮廓线
        auto singleTextEnts = c->getSubEntities();
        if (singleTextEnts.size() > 0)
        {
            subEnts.splice(subEnts.end(), singleTextEnts);
        }
    }
    return subEnts;
}

void DmText::addEntitiesOfText(const QString& text)
{
    usedTextWidth = 0.0;
    usedTextHeight = 0.0;

    DmTextStyle* pStyle = getStyle();
    DmTextStyleData styleData = pStyle->getData();

    if (styleData.isVertical)
    {
        createTextVertical(text, pStyle);
    }
    else
    {
        createTextHorizontal(text, pStyle);
    }
}

void DmText::moveChars(const DmVector& offset)
{
    for (auto c : chars)
    {
        c->move(offset);
    }
}

void DmText::rotateChars(const DmVector& center, const double& angle)
{
    DmVector angleVector(angle);
    for (auto c : chars)
    {
        c->rotate(center, angleVector);
    }
}

void DmText::mirrorChars(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    for (auto c : chars)
    {
        c->mirror(axisPoint1, axisPoint2);
    }
}

void DmText::scaleChars(const DmVector& center, const DmVector& factor)
{
    for (auto c : chars)
    {
        c->scale(center, factor);
    }
}

void DmText::createTextVertical(const QString& text, DmTextStyle* pStyle)
{
    double widthFactor = m_data->getWidthFactor();
    ETextMode alignMode = m_data->getTextMode();
    constexpr double vSize = 1.0; // 所有文字块"A"基线以上不留白高度是1.0
    // 计算原始文字高度
    double height = getTextHeightVertical(text, pStyle);
    double trueHeight = height * m_data->getTextHeight();
    // 布满，对齐的特殊处理
    if (alignMode == ETextMode::kTextFit || alignMode == ETextMode::kTextAligned)
    {
        DmVector vec = m_data->getAlignment() - m_data->getPosition();
        double dist = vec.magnitude();
        double scale = dist / trueHeight;
        // 布满
        if (alignMode == ETextMode::kTextFit)
        {
            double newWidthFactor = widthFactor / scale;
            createTextVertical(text, pStyle, newWidthFactor);
            m_data->setWidthFactor(newWidthFactor);
            double newHeight = m_data->getTextHeight() * scale;
            m_data->setTextHeight(newHeight);
        }
        // 对齐
        else if (alignMode == ETextMode::kTextAligned)
        {
            double newHeight = m_data->getTextHeight() * scale;
            createTextVertical(text, pStyle, widthFactor);
            m_data->setTextHeight(newHeight);
        }
        m_data->setAngle(Math2d::correctAngle(vec.angle() + M_PI_2)); // 初始文字是竖直的
        DmVector textSize = getWidthHeight();
        if (m_data->getUpsideDown() ^ m_data->getReverseDirection()) // 颠倒或反向有一个为true，则镜像
        {
            DmVector basePt(0.0, 0.0);
            mirrorChars(basePt, basePt + DmVector(0.0, 1.0));
        }
        // Scale:
        scaleChars(DmVector(0.0, 0.0), DmVector(m_data->getTextHeight() / vSize, m_data->getTextHeight() / vSize));
        // Update actual text size (before rotating, after scaling!):
        usedTextWidth = m_data->getTextHeight() * ASSUMED_TEXT_ASPECT_RATIO; // 假设宽高比为0.5
        usedTextHeight = trueHeight;
        // Rotate:
        rotateChars(DmVector(0.0, 0.0), m_data->getAngle());
        moveChars(m_data->getPosition());
    }
    // 其他对齐方式
    else
    {
        createTextVertical(text, pStyle, widthFactor);
        DmVector textSize = getWidthHeight();
        DmVector offset(0.0, 0.0);
        switch (m_data->getTextVertMode())
        {
        case ETextVertMode::kTextTop:
            offset.move(DmVector(0.0, 0.0));
            break;
        case ETextVertMode::kTextVertMid:
            offset.move(DmVector(0.0, textSize.y / 2.0));
            break;

        case ETextVertMode::kTextBottom:
        case ETextVertMode::kTextBase:
            offset.move(DmVector(0.0, textSize.y));
            break;

        default:
            break;
        }

        // Horizontal Align:
        switch (m_data->getTextHorzMode())
        {
        case ETextHorzMode::kTextMid: // 中间（前面ETextVertMode::kTextBase移动多了）
            offset.move(DmVector(0.0, -textSize.y / 2.0));
            offset.move(DmVector(-textSize.x / 2.0, 0.0));
            break;
        case ETextHorzMode::kTextCenter:
        {
            offset.move(DmVector(-textSize.x / 2.0, 0.0));
            break;
        }
        case ETextHorzMode::kTextRight:
            offset.move(DmVector(-textSize.x, 0.0));
            break;

        default:
            break;
        }
        moveChars(offset);
        DmVector pos = offset; // 待设置的位置点
        if (m_data->getUpsideDown())
        {
            mirrorChars(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
            pos.mirror(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
        }
        if (m_data->getReverseDirection())
        {
            mirrorChars(DmVector(0.0, 0.0), DmVector(0.0, 1.0));
            pos.mirror(DmVector(0.0, 0.0), DmVector(0.0, 1.0));
        }
        // Scale:
        DmVector scaleVec(m_data->getTextHeight() / vSize, m_data->getTextHeight() / vSize);
        scaleChars(DmVector(0.0, 0.0), scaleVec);
        pos.scale(DmVector(0.0, 0.0), scaleVec);
        // Update actual text size (before rotating, after scaling!):
        usedTextWidth = m_data->getTextHeight() * ASSUMED_TEXT_ASPECT_RATIO; // 假设宽高比为0.5
        usedTextHeight = trueHeight;
        // Rotate:
        rotateChars(DmVector(0.0, 0.0), m_data->getAngle());
        pos.rotate(DmVector(0.0, 0.0), m_data->getAngle());
        // 左对齐的对齐点坐标全为0，文字位置由Position确定
        if (alignMode == ETextMode::kTextLeft)
        {
            moveChars(m_data->getPosition());
            m_data->setAlignment(DmVector(0.0, 0.0));
        }
        // 其他情况Position按对齐点确定
        else
        {
            moveChars(m_data->getAlignment());
            pos.move(m_data->getAlignment());
            m_data->setPosition(pos);
        }
    }
    calculateBorders();
}

void DmText::createTextVertical(const QString& text, DmTextStyle* pStyle, const double& widthFactor)
{
    DmVector letterPos = DmVector(0.0, 0.0);
    double space = VERTICALSPACEFACTOR;
    DmVector spaceVec(0.0, space);
    double letterSpace = VERTICALLETTERSPACEFACTOR;

    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    int i = 0;
    while (i < (int)text.length())
    {
        QChar ch = text.at(i);
        QString letterText(ch);
        handlePercentChar(ch, text, spaceVec, letterText, letterPos, i);
        // 生成字符
        if (letterText.trimmed() != "")
        {
            std::unique_ptr<DmChar> c(pStyle->findOrCreateLetter(letterText, widthFactor, true));
            if (c.get() == nullptr)
            {
                continue;
            }
            c->move(letterPos);
            c->setParent(this);
            c->setPen(pen);
            c->update();
            double letterHeight = c->getHeight();
            c->move(DmVector(0.0, -letterHeight));
            addChar(c.release());
            letterPos.y -= letterHeight;

            letterPos.y -= VERTICALLETTERSPACEFACTOR;
        }
    }
    calculateBorders();
}

double DmText::getTextHeightVertical(const QString& text, DmTextStyle* pStyle)
{
    DmVector spaceVec(0.0, VERTICALSPACEFACTOR);
    DmVector letterPos(0.0, 0.0);
    double widthFactor = m_data->getWidthFactor();

    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    int i = 0;
    while (i < (int)text.length())
    {
        QChar ch = text.at(i);
        QString letterText(ch);
        handlePercentChar(ch, text, spaceVec, letterText, letterPos, i);
        // 查找字符
        if (letterText.trimmed() != "")
        {
            std::unique_ptr<DmChar> c(pStyle->findOrCreateLetter(letterText, widthFactor, true));
            if (c.get() == nullptr)
            {
                continue;
            }
            letterPos.y -= c->getHeight();
            letterPos.y -= VERTICALLETTERSPACEFACTOR;
        }
    }
    if (letterPos.y < 0)
    {
        // 去掉最后多加的字符间隔
        double dist = letterPos.y + VERTICALLETTERSPACEFACTOR;
        return abs(dist);
    }
    else
    {
        return 0.0;
    }
}

void DmText::createTextHorizontal(const QString& text, DmTextStyle* pStyle)
{
    double widthFactor = m_data->getWidthFactor();
    ETextMode alignMode = m_data->getTextMode();
    constexpr double vSize = 1.0; // 所有文字块"A"基线以上不留白高度是1.0
    // 计算原始文字宽度
    double width = getTextWidthHorizontal(text, pStyle, widthFactor);
    double trueWidth = width * m_data->getTextHeight(); // 前面的文字没有经过放缩，高度为1.0左右，乘文字高度才为真实文字宽度;
    // 布满，对齐的特殊处理
    if (alignMode == ETextMode::kTextFit || alignMode == ETextMode::kTextAligned)
    {
        // 按照宽度设置宽度系数
        DmVector vec = m_data->getAlignment() - m_data->getPosition();
        double dist = vec.magnitude();
        double scale = dist / trueWidth;
        // 布满
        if (alignMode == ETextMode::kTextFit)
        {
            double newWidthFactor = widthFactor * scale;
            createTextHorizontal(text, pStyle, newWidthFactor);
            m_data->setWidthFactor(newWidthFactor);
        }
        // 对齐
        else if (alignMode == ETextMode::kTextAligned)
        {
            double newHeight = m_data->getTextHeight() * scale;
            createTextHorizontal(text, pStyle, widthFactor);
            m_data->setTextHeight(newHeight);
        }
        m_data->setAngle(vec.angle());
        DmVector textSize = getWidthHeight();
        if (m_data->getUpsideDown() ^ m_data->getReverseDirection()) // 颠倒或反向有一个为true，则镜像
        {
            DmVector basePt(0.0, 0.0);
            mirrorChars(basePt, basePt + DmVector(1.0, 0.0));
        }
        // Scale:
        scaleChars(DmVector(0.0, 0.0), DmVector(m_data->getTextHeight() / vSize, m_data->getTextHeight() / vSize));
        // Update actual text size (before rotating, after scaling!):
        usedTextWidth = trueWidth;
        usedTextHeight = m_data->getTextHeight();
        // Rotate:
        rotateChars(DmVector(0.0, 0.0), m_data->getAngle());
        moveChars(m_data->getPosition());
    }
    // 其他对齐方式
    else
    {
        createTextHorizontal(text, pStyle, widthFactor);
        DmVector textSize = getWidthHeight();
        DmVector offset(0.0, 0.0);
        switch (m_data->getTextVertMode())
        {
        case ETextVertMode::kTextTop:
            offset.move(DmVector(0.0, -vSize));
            break;
        case ETextVertMode::kTextVertMid:
            offset.move(DmVector(0.0, -textSize.y / 2.0));
            break;

        case ETextVertMode::kTextBottom:
            offset.move(DmVector(0.0, ONE_THIRD_Y));
            break;

        case ETextVertMode::kTextBase:
            offset.move(DmVector(0.0, 0.0));
            break;

        default:
            break;
        }

        // Horizontal Align:
        switch (m_data->getTextHorzMode())
        {
        case ETextHorzMode::kTextMid: // 中间
            offset.move(DmVector(-width / 2.0, 0.0));
            offset.move(DmVector(0.0, -textSize.y / 2.0)); // 中间的Y在此处理
            break;
        case ETextHorzMode::kTextCenter:
            offset.move(DmVector(-width / 2.0, 0.0));
            break;
        case ETextHorzMode::kTextRight:
            offset.move(DmVector(-width, 0.0));
            break;
        case ETextHorzMode::kTextLeft:
        default:
            break;
        }
        moveChars(offset);
        DmVector pos = offset; // 待设置的位置点
        if (m_data->getUpsideDown())
        {
            mirrorChars(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
            pos.mirror(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
        }
        if (m_data->getReverseDirection())
        {
            mirrorChars(DmVector(0.0, 0.0), DmVector(0.0, 1.0));
            pos.mirror(DmVector(0.0, 0.0), DmVector(0.0, 1.0));
        }
        // Scale:
        DmVector scaleVec(m_data->getTextHeight() / vSize, m_data->getTextHeight() / vSize);
        scaleChars(DmVector(0.0, 0.0), scaleVec);
        pos.scale(DmVector(0.0, 0.0), scaleVec);
        // Update actual text size (before rotating, after scaling!):
        usedTextWidth = trueWidth;
        usedTextHeight = m_data->getTextHeight();
        // Rotate:
        rotateChars(DmVector(0.0, 0.0), m_data->getAngle());
        pos.rotate(DmVector(0.0, 0.0), m_data->getAngle());
        // 左对齐的对齐点坐标全为0，文字位置由Position确定
        if (alignMode == ETextMode::kTextLeft)
        {
            moveChars(m_data->getPosition());
            m_data->setAlignment(DmVector(0.0, 0.0));
        }
        // 其他情况Position按对齐点确定
        else
        {
            moveChars(m_data->getAlignment());
            pos.move(m_data->getAlignment());
            m_data->setPosition(pos);
        }
    }
    calculateBorders();
}

void DmText::createTextHorizontal(const QString& text, DmTextStyle* pStyle, const double& widthFactor)
{
    DmVector letterPos = DmVector(0.0, 0.0);
    double space = HORIZONTALSPACEFACTOR * widthFactor;
    DmVector spaceVec(space, 0.0);
    double letterSpace = HORIZONTALLETTERSPACEFACTOR * widthFactor;

    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    int i = 0;
    while (i < (int)text.length())
    {
        QChar ch = text.at(i);
        QString letterText(ch);
        handlePercentChar(ch, text, spaceVec, letterText, letterPos, i);
        // 生成字符
        if (letterText.trimmed() != "")
        {
            std::unique_ptr<DmChar> c(pStyle->findOrCreateLetter(letterText, widthFactor, true));
            if (c.get() == nullptr)
            {
                continue;
            }
            c->move(letterPos);
            c->setParent(this);
            c->setPen(pen);
            c->update();
            double letterWidth = c->getWidth();
            addChar(c.release());
            letterPos.x += letterWidth;
            letterPos += DmVector(letterSpace, 0.0);
        }
    }
    calculateBorders();
}

double DmText::getTextWidthHorizontal(const QString& text, DmTextStyle* pStyle, const double& widthFactor)
{
    DmVector spaceVec(HORIZONTALSPACEFACTOR, 0.0);
    double letterSpace = HORIZONTALLETTERSPACEFACTOR * widthFactor;
    DmVector letterPos(0.0, 0.0);

    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    int i = 0;
    while (i < (int)text.length())
    {
        QChar ch = text.at(i);
        QString letterText(ch);
        handlePercentChar(ch, text, spaceVec, letterText, letterPos, i);
        // 查找字符
        if (letterText.trimmed() != "")
        {
            std::unique_ptr<DmChar> c(pStyle->findOrCreateLetter(letterText, widthFactor, true));
            if (c.get() == nullptr)
            {
                continue;
            }
            double letterWidth = c->getWidth();
            letterPos.x += letterWidth;
            letterPos += DmVector(letterSpace, 0.0);
        }
    }
    if (letterPos.x > 0)
    {
        // 去掉最后多加的字符间隔
        double dist = letterPos.x - letterSpace;
        return dist;
    }
    else
    {
        return 0.0;
    }
}

void DmText::handlePercentChar(const QChar& ch, const QString& text, const DmVector& spaceVec, QString& letterText, DmVector& letterPos, int& currentIdx)
{
    if (ch == '%')
    {
        bool find = false;
        int k = currentIdx + 2;
        // 匹配特殊字符
        if (k < text.length())
        {
            QString tempStr = text.mid(currentIdx, 3);
            if (tempStr == "%%c" || tempStr == "%%C")
            {
                letterText = DIAMETER_UNICODE; // 直径，数值为unicode编号
                find = true;
            }
            else if (tempStr == "%%p" || tempStr == "%%P")
            {
                letterText = PLUSMINUS_UNICODE; // 正负号，数值为unicode编号
                find = true;
            }
            else if (tempStr == "%%d" || tempStr == "%%D")
            {
                letterText = DEGREE_UNICODE; // 度，数值为unicode编号
                find = true;
            }
            if (find)
            {
                currentIdx += 3;
            }
            else
            {
                // 匹配编码
                if (k < text.length() && text.at(currentIdx + 1) == '%' && text.at(currentIdx + 2).isDigit())
                {
                    // 可能要连续取字符
                    int numStartIdx = k;
                    for (; k < text.length(); k++)
                    {
                        QChar tempCh = text.at(k);
                        if (!tempCh.isDigit())
                        {
                            if (tempCh.unicode() == 0x20) // 丢弃一个空格
                            {
                                k++;
                            }
                            break;
                        }
                    }
                    if (k < text.length()) // 提前结束（后面有空格或非数字字符）
                    {
                        letterText = text.mid(currentIdx, k - currentIdx);
                        currentIdx = k;
                    }
                    else
                    {
                        // 直到结尾都是数字
                        letterText = text.mid(currentIdx);
                        currentIdx = text.length();
                    }
                }
                else
                {
                    // 无匹配，直接打印%字符
                    currentIdx++;
                }
            }
        }
        else
        {
            // 长度不够，直接打印%字符
            currentIdx++;
        }
    }
    else
    {
        currentIdx++;
        if (ch.unicode() == 0x20)
        {
            letterPos += spaceVec;
        }
    }
}

void DmText::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);

    DmVector position = m_data->getPosition();
    double height = m_data->getTextHeight();
    double angle = m_data->getAngle();
    std::string style = m_data->getTextStyle()->getName().toStdString();
    int textHorzMode = (int)m_data->getTextHorzMode();
    int textVertMode = (int)m_data->getTextVertMode();
    bool isMirrorInX = m_data->getUpsideDown();
    bool isMirrorInY = m_data->getReverseDirection();
    double widthFactor = m_data->getWidthFactor();
    double oblique = m_data->getSlashAngle();
    DmVector alignment = m_data->getAlignment();
    std::string textString = m_data->getTextString().toStdString();

    wrt << (double)position.x << (double)position.y << (double)height << (double)angle << (std::string)style << (int)textHorzMode << (int)textVertMode << (bool)isMirrorInX << (bool)isMirrorInY << (double)widthFactor << (double)oblique << (double)alignment.x << (double)alignment.y << (std::string)textString;
}

void DmText::restoreStream(InputStream& rdr, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmText", revs);
    if (revId > fileRev)
    {
        DmEntity::restoreStream(rdr, revs);
        // 老文件格式
        restoreStreamWithRev(rdr, fileRev);
    }
    else
    {
        restoreStream(rdr);
    }
}

void DmText::restoreStreamWithRev(InputStream& rdr, int rev)
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

void DmText::restoreStream(InputStream& rdr)
{
    DmEntity::restoreStream(rdr);

    DmVector position(true);
    double height = 0.0;
    double angle = 0.0;
    std::string style;
    int textHorzMode = 0;
    int textVertMode = 0;
    bool isMirrorInX = false;
    bool isMirrorInY = false;
    double widthFactor = 1.0;
    double oblique = 0.0;
    DmVector alignment(true);
    std::string textString;

    rdr >> (double&)position.x >> (double&)position.y >> (double&)height >> (double&)angle >> (std::string&)style >> (int&)textHorzMode >> (int&)textVertMode >> (bool&)isMirrorInX >> (bool&)isMirrorInY >> (double&)widthFactor >> (double&)oblique >> (double&)alignment.x >> (double&)alignment.y >> (std::string&)textString;

    DmTextStyleTable* textStyleTable = getDocument()->getTextStyleTable();
    DmTextStyle* pStyle = textStyleTable->find(QString::fromStdString(style));
    m_data.reset(new TextData());
    m_data->setPosition(position);
    m_data->setTextHeight(height);
    m_data->setAngle(angle);
    m_data->setTextStyle(pStyle);
    m_data->setTextHorzMode((ETextHorzMode)textHorzMode);
    m_data->setTextVertMode((ETextVertMode)textVertMode);
    m_data->setUpsideDown(isMirrorInX);
    m_data->setReverseDirection(isMirrorInY);
    m_data->setWidthFactor(widthFactor);
    m_data->setSlashAngle(oblique);
    m_data->setAlignment(alignment);
    m_data->setTextString(QString::fromStdString(textString));
    update();
}
