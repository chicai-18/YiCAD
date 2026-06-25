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


/// @file DmTextStyle.cpp
/// @brief 文字样式类实现，定义文字的字体、高度、宽度因子等属性

#include "DmTextStyle.h"
#include "DmFont.h"
#include "DmCharTemplate.h"
#include "DmChar.h"
#include "DmPolyline.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmFontList.h"
#include "ApplicationWindow.h"
#include "MDIWindow.h"
#include "DmDocument.h"
#include "DmSettings.h"
#include "DmText.h"
#include <ostream>

TYPESYSTEM_SOURCE(DmTextStyle, DmObject, 0)

DmTextStyle::DmTextStyle()
{
    m_data = std::shared_ptr<DmTextStyleData>(new DmTextStyleData());
}

DmTextStyle::DmTextStyle(const DmTextStyleData& data)
{
    m_data = std::shared_ptr<DmTextStyleData>(new DmTextStyleData(data));
}

DmTextStyle::DmTextStyle(const DmTextStyle& temp, const QString name)
{
    auto data = new DmTextStyleData(*temp.m_data);
    m_data = std::shared_ptr<DmTextStyleData>(data);
    m_ulID = DmId();
    m_data->name = name;
}

DmTextStyle::DmTextStyle(const QString& name)
{
    m_data = std::shared_ptr<DmTextStyleData>(new DmTextStyleData());
    m_data->defaultHeight = DEFAULT_TEXT_HEIGHT;
    m_data->isReverseDirection = false;
    m_data->isSystemFont = true;
    m_data->isUpsideDown = false;
    m_data->isUseBigfont = false;
    m_data->isVertical = false;
    m_data->name = name;
    m_data->pAsciiFont = nullptr;
    m_data->pBigFont = nullptr;
    m_data->pSysFont = DMFONTLIST->requestSysFont(DEFAULT_FONT_FAMILY_NAME, DEFAULT_FONT_FAMILY_BOLD, DEFAULT_FONT_FAMILY_ITALIC);
    m_data->slashAngle = DEFAULT_SLASH_ANGLE;
    m_data->sysFontFamily = DEFAULT_FONT_FAMILY_NAME;
    m_data->isSysFontBold = false;
    m_data->isSysFontItalic = false;
    m_data->widhFactor = DEFAULT_WIDTH_FACTOR;
    if (!m_data->pSysFont)
    {
        m_data->invalidSysFontFamily = DEFAULT_FONT_FAMILY_NAME;
    }
}

DmTextStyle* DmTextStyle::clone() const
{
    auto cloned = new DmTextStyle(*this);
    cloned->m_data = std::shared_ptr<DmTextStyleData>(new DmTextStyleData(*m_data));
    return cloned;
}

DmChar* DmTextStyle::findOrCreateLetter(QString& letter, const double& widthFactor, const bool& getDefaultIfNotFound)
{
    DmCharTemplate* charTemplate = nullptr;
    // 根据文字样式的字体查找文字模板
    bool valid = false; // 文字样式有效（字体文件都存在），此处暂时不用
    if (m_data->isSystemFont)
    {
        if (m_data->pSysFont != nullptr)
        {
            charTemplate = m_data->pSysFont->findLetter(letter);
            valid = true;
        }
    }
    else
    {
        if (m_data->pAsciiFont != nullptr)
        {
            charTemplate = m_data->pAsciiFont->findLetter(letter);
            if (m_data->isUseBigfont && charTemplate == nullptr)
            {
                if (m_data->pBigFont != nullptr)
                {
                    charTemplate = m_data->pBigFont->findLetter(letter);
                    valid = true;
                }
            }
            else
            {
                valid = true;
            }
        }
    }

    // 由文字模板生成文字实例
    DmChar* c = nullptr;
    if (charTemplate != nullptr)
    {
        c = charTemplate->generateChar(widthFactor, getDataConstPtr()->slashAngle);
    }
    if (c != nullptr)
    {
        return c;
    }

    // 上面的方法没有获得文字，查找配置的字体文件，生成文字
    if (getDefaultIfNotFound)
    {
        QString fontName = DmFont::getPredefineFontNameOfLetter(letter);
        if (fontName.isEmpty()) // 没有配置该语言的文字，一般不会发生，生成问号
        {
            auto defStyle = DmTextStyleTable::getDefaultStyle();
            if (defStyle != nullptr)
            {
                letter = QChar(0x003F); // "?"字符
                return defStyle->findOrCreateLetter(letter, widthFactor, false);
            }
        }
        else
        {
            DmFont* font = DMFONTLIST->requestFont(fontName);
            if (font != nullptr)
            {
                DmCharTemplate* fontChar = font->findLetter(letter);
                if (fontChar != nullptr)
                {
                    return fontChar->generateChar(widthFactor, getDataConstPtr()->slashAngle);
                }
            }
        }
    }

    return c;
}

double DmTextStyle::getValidDefaultHeight() const
{
    if (m_data->defaultHeight == 0.0)
    {
        return DEFAULT_TEXT_HEIGHT;
    }
    return m_data->defaultHeight;
}

QString DmTextStyle::getName() const
{
    return m_data->name;
}

DmTextStyleData DmTextStyle::getData() const
{
    return *m_data;
}

void DmTextStyle::setData(const DmTextStyleData& data)
{
    m_data.reset(new DmTextStyleData(data));
}

const DmTextStyleData* DmTextStyle::getDataConstPtr() const
{
    return m_data.get();
}

bool DmTextStyle::isStandard()
{
    return DEFAULT_TEXTSTYLE_NAME == getName();
}

bool DmTextStyle::isValid() const
{
    if (m_data->isSystemFont)
    {
        return m_data->invalidSysFontFamily.isEmpty();
    }
    else if (m_data->isUseBigfont)
    {
        return m_data->invalidAsciiFont.isEmpty() && m_data->invalidBigFont.isEmpty();
    }
    else
    {
        return m_data->invalidAsciiFont.isEmpty();
    }
}

void DmTextStyle::getPreview(DmEntityContainer* previewContainer) const
{
    previewContainer->clear();
    TextData data(DmVector(0.0, 0.0), 1.0, ETextVertMode::kTextVertMid, ETextHorzMode::kTextCenter, QString::fromLocal8Bit("abc测试"), const_cast<DmTextStyle*>(this), 0.0);
    data.setUpsideDown(m_data->isUpsideDown);
    data.setReverseDirection(m_data->isReverseDirection);
    data.setWidthFactor(m_data->widhFactor);
    data.setSlashAngle(m_data->slashAngle);
    DmText* text = new DmText(previewContainer, data);
    text->update();
    previewContainer->addEntity(text);
}

void DmTextStyle::saveStream(OutputStream& wrt) const
{
    DmObject::saveStream(wrt);

    DmTextStyleData data = this->getData();
    std::string strName = data.name.toStdString();
    std::string strBigFontName = "";
    std::string strSysFontFileName = "";
    std::string strSysFontName = "";
    bool sysFontBold = false;
    bool sysFontItalic = false;
    if (data.pBigFont != nullptr)
    {
        strBigFontName = data.pBigFont->getFileName().toStdString();
    }

    if (data.pAsciiFont != nullptr)
    {
        // todo : 默认的txt.shx无法写出
        strSysFontFileName = data.pAsciiFont->getFileName().toStdString();
    }
    if (data.pSysFont != nullptr)
    {
        strSysFontFileName = data.pSysFont->getFileName().toStdString();
        strSysFontName = data.sysFontFamily.toStdString();
        sysFontBold = data.isSysFontBold;
        sysFontItalic = data.isSysFontItalic;
    }

    // 如果有不存在的字体，写入
    if (!data.invalidAsciiFont.isEmpty())
    {
        strSysFontFileName = data.invalidAsciiFont.toStdString();
    }
    if (!data.invalidBigFont.isEmpty())
    {
        strBigFontName = data.invalidBigFont.toStdString();
    }
    if (!data.invalidSysFontFamily.isEmpty())
    {
        strSysFontFileName = data.invalidSysFontFamily.toStdString();
        strSysFontName = data.sysFontFamily.toStdString();
    }
    bool isBackward = data.isReverseDirection;
    bool isUpsidedown = data.isUpsideDown;
    bool isVertical = data.isVertical;
    double obliquingAngle = data.slashAngle;
    double textSize = data.defaultHeight;
    double widthFactor = data.widhFactor;

    wrt << (std::string)strName << (std::string)strBigFontName << (std::string)strSysFontFileName << (std::string)strSysFontName << (bool)sysFontBold << (bool)sysFontItalic << (bool)isBackward << (bool)isUpsidedown << (bool)isVertical << (double)obliquingAngle << (double)textSize << (double)widthFactor;
}

void DmTextStyle::restoreStream(InputStream& rdr, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmTextStyle", revs);
    if (revId > fileRev)
    {
        DmObject::restoreStream(rdr, revs);
        // 老文件格式
        restoreStreamWithRev(rdr, fileRev);
    }
    else
    {
        restoreStream(rdr);
    }
}

void DmTextStyle::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
        // TODO:  - rev==0时无操作，需要确认是否应该实现
    }
    else // big change, e.g. change supper class of DmTextStyle
    {
        // step1.
        // read all legacy data one by one
        // TODO:  - 需要实现旧版本数据读取逻辑
    }
}

void DmTextStyle::restoreStream(InputStream& rdr)
{
    DmObject::restoreStream(rdr);

    std::string strName;
    std::string strBigFontName;
    std::string strSysFontFileName;
    std::string strSysFontName;
    bool sysFontBold = false;
    bool sysFontItalic = false;
    bool isBackward = false;
    bool isUpsidedown = false;
    bool isVertical = false;
    double obliquingAngle = 0.0;
    double textSize = 0.0;
    double widthFactor = 1.0;

    rdr >> (std::string&)strName >> (std::string&)strBigFontName >> (std::string&)strSysFontFileName >> (std::string&)strSysFontName >> (bool&)sysFontBold >> (bool&)sysFontItalic >> (bool&)isBackward >> (bool&)isUpsidedown >> (bool&)isVertical >> (double&)obliquingAngle >> (double&)textSize >> (double&)widthFactor;

    DmTextStyleData styleData = getData();
    styleData.name = QString::fromStdString(strName);
    styleData.pAsciiFont = nullptr; // 默认有一个字体，置空
    QString bigFontName = QString::fromStdString(strBigFontName);
    if (!bigFontName.isEmpty())
    {
        if (!bigFontName.toLower().endsWith(SHX_POST)) // 大字体GBCBIG没有后缀
        {
            bigFontName.append(SHX_POST);
        }
        DmFont* pBigFont = DMFONTLIST->requestFont(bigFontName, false);
        if (pBigFont)
        {
            styleData.pBigFont = pBigFont;
        }
        else
        {
            styleData.invalidBigFont = bigFontName;
        }
        styleData.isUseBigfont = true;
        styleData.isSystemFont = false;
    }
    else
    {
        styleData.isUseBigfont = false;
    }

    QString fontName = QString::fromStdString(strSysFontName);
    if (!fontName.isEmpty())
    {
        QString fontLower = fontName.toLower();
        // 对于宋体，getSysFontName()返回SimSun。对于仿宋getSysFontName()返回"仿宋"。AutoCAD是啥奇葩操作
        static std::map<QString, QString> covertMap{ {"simsun", QString::fromLocal8Bit("宋体")},
                                                     {"simhei", QString::fromLocal8Bit("黑体")},
                                                     {"nsimsun", QString::fromLocal8Bit("新宋体")} };
        auto it = covertMap.find(fontLower);
        if (covertMap.end() != it)
        {
            fontName = it->second;
        }
        bool isSysBold = sysFontBold;
        bool isSysItalic = sysFontItalic;
        DmFont* pFont = DMFONTLIST->requestSysFont(fontName, isSysBold, isSysItalic);
        if (pFont)
        {
            styleData.sysFontFamily = fontName;
            styleData.pSysFont = pFont;
        }
        else
        {
            styleData.invalidSysFontFamily = fontName;
        }
        styleData.isSystemFont = true;
    }
    else
    {
        QString fontFileName = QString::fromStdString(strSysFontFileName);
        if (!fontFileName.isEmpty())
        {
            if (!fontFileName.endsWith(SHX_POST))
            {
                fontFileName += SHX_POST;
            }
            DmFont* asciiFont = DMFONTLIST->requestFont(fontFileName);
            if (asciiFont)
            {
                styleData.pAsciiFont = asciiFont;
            }
            else
            {
                styleData.invalidAsciiFont = fontFileName;
            }
        }
        else
        {
            styleData.invalidAsciiFont = fontFileName;
        }
        styleData.isSystemFont = false;
    }

    styleData.isReverseDirection = isBackward;
    styleData.isUpsideDown = isUpsidedown;
    styleData.isVertical = isVertical;
    styleData.slashAngle = obliquingAngle;
    styleData.defaultHeight = textSize;
    styleData.widhFactor = widthFactor;
    this->setData(styleData);
}

void DmTextStyle::setName(const QString& name)
{
    m_data->name = name;
}
