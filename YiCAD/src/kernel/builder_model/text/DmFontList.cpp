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


/// @file DmFontList.cpp
/// @brief 全局字体列表实现

#include "DmFontList.h"
#include "TextConsts.h"

#include <iostream>
#include <QHash>
#include <QSettings>

#include "Debug.h"
#include "DmFont.h"
#include "DmSystem.h"
#include "DmSettings.h"
#include <qdebug.h>

// 系统字体样式数量（常规/斜体/粗体/粗斜体）
constexpr int SYS_FONT_STYLE_COUNT = 4;
// 字体样式索引
constexpr int STYLE_IDX_REGULAR = 0;
constexpr int STYLE_IDX_ITALIC = 1;
constexpr int STYLE_IDX_BOLD = 2;
constexpr int STYLE_IDX_BOLD_ITALIC = 3;

DmFontList* DmFontList::m_pUniqueInstance = nullptr;

DmFontList* DmFontList::instance()
{
    if (!m_pUniqueInstance)
    {
        m_pUniqueInstance = new DmFontList();
    }
    return m_pUniqueInstance;
}

DmFontList::~DmFontList()
{
    for (auto f : m_fonts)
    {
        delete f;
    }
    m_fonts.clear();

    DmFont::freeLibrary();
}

void DmFontList::deleteFontList()
{
    if (m_pUniqueInstance)
    {
        delete m_pUniqueInstance;
    }
}

/// @brief 初始化字体列表，创建空DmFont对象用于每个可找到的字体
void DmFontList::readAllFontFiles()
{
    if (m_bHasReadAll)
    {
        return;
    }

    // 获得所有字体文件，解析shx文件头
    QStringList list = DMSYSTEM->getFontFiles();
    QHash<QString, int> added; // used to remember added fonts (avoid duplication)
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fi(list.at(i));
        QString fileNameLower = fi.fileName().toLower();
        if (!added.contains(fileNameLower))
        {
            auto it = std::find_if(m_fonts.begin(), m_fonts.end(), [&fileNameLower](DmFont* font) {
                if (font->getFileName().toLower() == fileNameLower)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            });
            if (it != m_fonts.end())
            {
                continue;
            }

            QString fontName = fi.fileName();
            std::unique_ptr<DmFont> font(new DmFont(fontName));
            bool load = font->loadPrefix();
            if (load)
            {
                m_fonts.emplace_back(font.release());
                added.insert(fi.fileName().toLower(), 1);
            }
        }
    }

    // 按名字排序
    std::sort(m_fonts.begin(), m_fonts.end(), [](DmFont* f1, DmFont* f2)
        {
            return f1->getFileName() < f2->getFileName();
        });

    // 加载系统字体
    for (auto& f : m_fonts)
    {
        if (f->isSystemFont())
        {
            if (!f->isLoaded())
            {
                f->loadFont();
            }
            if (!f->isValid())
            {
                continue;
            }
            QString family = f->getFamily();
            auto it = m_sysFontsMap.find(family);
            std::vector<DmFont*> fonts(SYS_FONT_STYLE_COUNT, nullptr);
            if (it != m_sysFontsMap.end())
            {
                fonts = it->second;
            }

            QString style = f->getStyle().toLower();
            bool isBold = false;
            bool isItalic = false;
            if (style == "regular")
            {
                isBold = false;
                isItalic = false;
            }
            else if (style == "italic")
            {
                isBold = false;
                isItalic = true;
            }
            else if (style == "bold")
            {
                isBold = true;
                isItalic = false;
            }
            else
            {
                isBold = true;
                isItalic = true;
            }
            int idx = getSysFontIdx(isBold, isItalic);
            fonts[idx] = f;
            m_sysFontsMap[family] = fonts;
        }
    }

    // 移除无效的字体
    auto it = m_fonts.begin();
    while (it != m_fonts.end())
    {
        if ((*it)->isLoaded() && !(*it)->isValid())
        {
            it = m_fonts.erase(it);
        }
        else
        {
            ++it;
        }
    }

    m_bHasReadAll = true;
}

QString DmFontList::getFontFamilyName(const DmFont* font, bool& isBold, bool& isItalic)
{
    QString fileName = font->getFileName();
    // shx字体
    if (DmFont::isShxFont(fileName))
    {
        isBold = false;
        isItalic = false;
        return fileName;
    }
    // 系统字体
    else
    {
        for (auto& kv : m_sysFontsMap)
        {
            for (int idx = 0; idx < SYS_FONT_STYLE_COUNT; idx++)
            {
                if (kv.second.at(idx) == font)
                {
                    getSysFontStyleByIdx(idx, isBold, isItalic);
                    return kv.first;
                }
            }
        }
    }
    return "";
}

const std::map<QString, std::vector<DmFont*>>& DmFontList::getSysFontsMapConstRef() const
{
    return m_sysFontsMap;
}

const QString DmFontList::getSysFontStyleName(const bool isBold, const bool isItalic) const
{
    if (isBold && isItalic)
    {
        return QObject::tr("Bold Italic");
    }
    else if (isBold)
    {
        return QObject::tr("Bold");
    }
    else if (isItalic)
    {
        return QObject::tr("Italic");
    }
    else
    {
        return QObject::tr("Regular");
    }
}

size_t DmFontList::countFonts() const
{
    return m_fonts.size();
}

std::vector<DmFont*>::const_iterator DmFontList::begin() const
{
    return m_fonts.begin();
}

std::vector<DmFont*>::const_iterator DmFontList::end() const
{
    return m_fonts.end();
}

DmFontList::DmFontList()
{
}

int DmFontList::getSysFontIdx(bool isBlod, bool isItalic) const
{
    int idx = STYLE_IDX_REGULAR;
    if (isBlod && isItalic)
    {
        idx = STYLE_IDX_BOLD_ITALIC;
    }
    else if (isBlod)
    {
        idx = STYLE_IDX_BOLD;
    }
    else if (isItalic)
    {
        idx = STYLE_IDX_ITALIC;
    }
    return idx;
}

void DmFontList::getSysFontStyleByIdx(int idx, bool& isBold, bool& isItalic) const
{
    switch (idx)
    {
    default:
    case STYLE_IDX_REGULAR:
        isBold = false;
        isItalic = false;
        break;
    case STYLE_IDX_ITALIC:
        isBold = false;
        isItalic = true;
        break;
    case STYLE_IDX_BOLD:
        isBold = true;
        isItalic = false;
        break;
    case STYLE_IDX_BOLD_ITALIC:
        isBold = true;
        isItalic = true;
        break;
    }
}

DmFont* DmFontList::requestSysFontCloset(const QString& family, bool& isBold, bool& isItalic)
{
    DmFont* font = requestSysFont(family, isBold, isItalic);
    if (font)
    {
        return font;
    }

    // 此时，该字体的其他样式（加粗，倾斜等）字体均已加载（如果存在）
    auto it = m_sysFontsMap.find(family);
    if (it == m_sysFontsMap.end())
    {
        return nullptr;
    }
    // 取最匹配的字体
    std::vector<int> priorityIdx;
    if (isBold)
    {
        if (isItalic)
        {
            priorityIdx = {STYLE_IDX_BOLD_ITALIC, STYLE_IDX_BOLD, STYLE_IDX_ITALIC, STYLE_IDX_REGULAR};
        }
        else
        {
            priorityIdx = {STYLE_IDX_BOLD, STYLE_IDX_REGULAR, STYLE_IDX_ITALIC, STYLE_IDX_BOLD_ITALIC};
        }
    }
    else
    {
        if (isItalic)
        {
            priorityIdx = {STYLE_IDX_ITALIC, STYLE_IDX_BOLD_ITALIC, STYLE_IDX_REGULAR, STYLE_IDX_BOLD};
        }
        else
        {
            priorityIdx = {STYLE_IDX_REGULAR, STYLE_IDX_ITALIC, STYLE_IDX_BOLD, STYLE_IDX_BOLD_ITALIC};
        }
    }

    DmFont* closeFont = nullptr;
    for (int i : priorityIdx)
    {
        closeFont = it->second.at(i);
        if (nullptr != closeFont)
        {
            getSysFontStyleByIdx(i, isBold, isItalic);
            return closeFont;
        }
    }
    return closeFont;
}

DmFont* DmFontList::requestFontCloset(const QString& family, bool& isBold, bool& isItalic)
{
    DmFont* f = nullptr;
    if (DmFont::isShxFont(family))
    {
        f = DmFontList::instance()->requestFont(family, false);
        isBold = false;
        isItalic = false;
    }
    else
    {
        f = DmFontList::instance()->requestSysFontCloset(family, isBold, isItalic);
    }
    return f;
}

const std::map<QString, QString>& DmFontList::FamilyToTranslateMap()
{
    static bool initialized = false;
    if (!initialized)
    {
        s_familyToTranslateMap = {
            {"SimSun", QObject::tr("SimSun")},                  // 宋体
            {"NSimSun", QObject::tr("NSimSun")},                // 新宋体
            {"Microsoft YaHei", QObject::tr("Microsoft YaHei")}, // 微软雅黑
            {"Microsoft YaHei UI", QObject::tr("Microsoft YaHei UI")}, // 微软雅黑
            {"Microsoft YaHei Light", QObject::tr("Microsoft YaHei Light")}, // 微软雅黑 Light
            {"KaiTi", QObject::tr("KaiTi")},                    // 楷体
            {"FangSong", QObject::tr("FangSong")},              // 仿宋
            {"DengXian", QObject::tr("DengXian")},              // 等线
            {"SimHei", QObject::tr("SimHei")},                  // 黑体
        };

        initialized = true;
    }
    return s_familyToTranslateMap;
}

const std::map<QString, QString>& DmFontList::TranslateToFamilyMap()
{
    static bool initialized = false;
    if (!initialized)
    {
        auto& map = FamilyToTranslateMap();
        for (auto kv : map)
        {
            s_translateToFamilyMap[kv.second] = kv.first;
        }
        initialized = true;
    }
    return s_translateToFamilyMap;
}

void DmFontList::clearFonts()
{
    m_fonts.clear();
}

DmFont* DmFontList::requestFont(const QString& fileName, const bool getDefaultIfNotFound /* = true*/)
{
    // 在现有字体列表中查找
    QString fileNameLower = fileName.toLower();
    DmFont* foundFont = nullptr;
    // Search our list of available fonts:
    for (auto const& f : m_fonts)
    {
        if (f->getFileName().toLower() == fileNameLower)
        {
            // Make sure this font is loaded into memory:
            if (!f->isLoaded())
            {
                f->loadFont();
            }
            foundFont = f;
            break;
        }
    }

    // 如果不在现有字体列表中，从字体文件中加载字体
    if (!m_bHasReadAll && nullptr == foundFont)
    {
        QStringList list = DMSYSTEM->getFontFiles();
        for (int i = 0; i < list.size(); ++i)
        {
            QFileInfo fi(list.at(i));
            if (fi.fileName().toLower() == fileNameLower)
            {
                std::unique_ptr<DmFont> font(new DmFont(fileName));
                bool load = font->loadPrefix();
                if (load)
                {
                    foundFont = font.release();
                    m_fonts.emplace_back(foundFont);
                }
                break;
            }
        }
    }

    if (getDefaultIfNotFound && !foundFont)
    {
        foundFont = requestSysFont(DEFAULT_FONT_FAMILY_NAME, DEFAULT_FONT_FAMILY_BOLD, DEFAULT_FONT_FAMILY_ITALIC);
    }
    return foundFont;
}

DmFont* DmFontList::requestSysFont(const QString& family, bool isBlod, bool isItalic)
{
    readAllFontFiles();
    DmFont* foundFont = nullptr;
    int idx = getSysFontIdx(isBlod, isItalic);
    // 从缓存中查找
    auto it = m_sysFontsMap.find(family);
    if (it != m_sysFontsMap.end())
    {
        foundFont = it->second.at(idx);
        return foundFont;
    }
    return nullptr;
}
