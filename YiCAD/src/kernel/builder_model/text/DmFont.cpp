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


/// @file DmFont.cpp
/// @brief 字体类实现，包括shx字体解析、系统字体(freetype)解析、字体命令管理

#include "DmFont.h"
#include "TextConsts.h"
#include <iostream>
#include <fstream>
#include <QTextStream>
#include <QTextCodec>

#include <freetype/ftoutln.h>
#include <freetype/ftsnames.h>
#include <freetype/ttnameid.h>

#include "DmCircle.h"
#include "DmArc.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "DmCharTemplate.h"
#include "DmSystem.h"
#include "DmTriangle.h"
#include "DmPoint.h"
#include "Math2d.h"
#include "ConstrainedDelaunayTriangulation.h"
#include "Debug.h"
#include <qdebug.h>

// ---- shx字体格式常量 ----

// ASCII字体: 字形码数偏移
constexpr int SHX_ASCII_CODECOUNT_OFFSET = 0x1C;
// ASCII字体: 字形信息起始偏移
constexpr int SHX_ASCII_CODEINFO_OFFSET = 0x1E;
// ASCII字体: 每条字形记录大小
constexpr int SHX_ASCII_CODE_RECORD_SIZE = 4;
// ASCII字体: 字形数据地址偏移 (在codeCount后面)
constexpr int SHX_ASCII_DATA_OFFSET_START = 0x1E;

// Unifont字体: 字形码数偏移
constexpr int SHX_UNI_CODECOUNT_OFFSET = 0x19;
// Unifont字体: 字形信息起始偏移
constexpr int SHX_UNI_CODEINFO_OFFSET = 0x1B;

// 大字体: 字形码数偏移
constexpr int SHX_BIG_CODECOUNT_OFFSET = 27;
// 大字体: 区间码数偏移
constexpr int SHX_BIG_SECTIONNUM_OFFSET = 29;
// 大字体: 字形数据起始偏移 = 31 + sectionNum * 4
constexpr int SHX_BIG_CODE_START_BASE = 31;
// 大字体: 字形记录大小
constexpr int SHX_BIG_CODE_RECORD_SIZE = 8;
// 大字体: 最小字形码偏移
constexpr int SHX_BIG_SECTION_RECORD_SIZE = 4;

// shx头解析常量
constexpr int SHX_HEADER_OFFSET = 11;
constexpr int SHX_HEADER_BIGFONT_LEN = 7;
constexpr int SHX_HEADER_SHAPES_LEN = 6;
constexpr int SHX_HEADER_UNIFONT_LEN = 7;
constexpr int SHX_MINIMUM_CODE_OFFSET = 0x18;

// 字体缩放相关
constexpr double SHX_SCALE_TO_ONE = 0.75;
constexpr double SHX_DESCENDER_ASCENDER_RATIO = 1.0 / 3.0;
constexpr int FONT26P6_DIVISOR = 64;

// 默认值
constexpr int DEFAULT_LETTER_SPACING = 3;
constexpr double DEFAULT_WORD_SPACING = 6.75;
constexpr double DEFAULT_LINE_SPACING_FACTOR = 1.0;
constexpr int MAX_ASCII_CHAR_CODE = 256;
constexpr int SHX_FONT_SPECIAL_CODE = 0;

// 二阶贝塞尔采样点
constexpr double BEZIER2_SAMPLE_T1 = 0.2;
constexpr double BEZIER2_SAMPLE_T2 = 0.5;
constexpr double BEZIER2_SAMPLE_T3 = 0.7;

// 三阶贝塞尔采样点
constexpr double BEZIER3_SAMPLE_POINTS[] = {0.1, 0.2, 0.4, 0.5, 0.6, 0.8, 0.9};
constexpr int BEZIER3_SAMPLE_COUNT = 7;

// 系统字体样式
constexpr int SYS_FONT_STYLE_COUNT = 4;

// 文字高度 = 1.0 / m_dScaleToOne
constexpr double DEFAULT_CHAR_HEIGHT_INIT = 0.0;

FT_Library DmFont::library = nullptr;

DmFont::DmFont(const QString& m_strFileName)
    : m_strFileName(m_strFileName)
    , m_strFileLicense("unknown")
    , m_fontType(FontType::Invalid)
{
    m_bLoaded = false;
    m_dLetterSpacing = static_cast<double>(DEFAULT_LETTER_SPACING);
    m_dWordSpacing = DEFAULT_WORD_SPACING;
    m_dLineSpacingFactor = DEFAULT_LINE_SPACING_FACTOR;
    m_dScaleToOne = 1.0;
    m_dDAScale = SHX_DESCENDER_ASCENDER_RATIO;
    m_letterList.setFont(this);
}

DmFont::~DmFont()
{
    if (m_fontType == FontType::System && m_sysFontFace != nullptr)
    {
        FT_Done_Face(m_sysFontFace);
        m_sysFontFace = nullptr;
    }
}

QString DmFont::getFileName() const
{
    return m_strFileName;
}

QString DmFont::getFileLicense() const
{
    return m_strFileLicense;
}

QString DmFont::getFileCreate() const
{
    return m_strFileCreate;
}

QString DmFont::getEncoding() const
{
    return m_strEncoding;
}

const QStringList& DmFont::getNames() const
{
    return m_names;
}

const QStringList& DmFont::getAuthors() const
{
    return m_authors;
}

double DmFont::getLetterSpacing()
{
    return m_dLetterSpacing;
}

double DmFont::getWordSpacing()
{
    return m_dWordSpacing;
}

double DmFont::getLineSpacingFactor()
{
    return m_dLineSpacingFactor;
}

DmCharTemplateList* DmFont::getLetterList()
{
    return &m_letterList;
}

double DmFont::getHeight() const
{
    return m_dCharHeight;
}

QString DmFont::getStyle() const
{
    return m_strSysStyle;
}

QString DmFont::getFamily() const
{
    return m_strSysFamily;
}

FontType DmFont::getFontType() const
{
    return m_fontType;
}

bool DmFont::isShxFont() const
{
    return (m_fontType == FontType::ShxASCII
        || m_fontType == FontType::ShxBigFont
        || m_fontType == FontType::ShxUnifont);
}

bool DmFont::isSystemFont() const
{
    return m_fontType == FontType::System;
}

bool DmFont::isValid() const
{
    return m_fontType != FontType::Invalid;
}

void DmFont::freeLibrary()
{
    if (library)
    {
        FT_Done_FreeType(library);
        library = nullptr;
    }
}

/// @brief 读取并解析字体文件的内容。调用前保证已调用loadPrefix()并返回true
/// @return true 字体已加载或现在加载成功, false 加载失败
bool DmFont::loadFont()
{
    if (m_fontType == FontType::Invalid)
    {
        return false;
    }
    if (m_bLoaded)
    {
        return true;
    }

    QString path = m_strFullFileName;
    m_bLoaded = true;
    if (m_fontType == FontType::ShxASCII
        || m_fontType == FontType::ShxUnifont
        || m_fontType == FontType::ShxBigFont)
    {
        readShx(path);
        m_dScaleToOne = SHX_SCALE_TO_ONE;
        m_dDAScale = SHX_DESCENDER_ASCENDER_RATIO;
    }
    else
    {
        readSystem(path);
    }

    return true;
}

bool DmFont::isLoaded() const
{
    return m_bLoaded;
}

bool DmFont::loadPrefix()
{
    // 查找存在的字体文件
    QString path;
    QString fileLower = m_strFileName.toLower();
    if (fileLower.contains(SHX_POST) || fileLower.contains(TTF_POST) || fileLower.contains(TTC_POST))
    {
        QStringList fonts = DMSYSTEM->getFontFiles();
        QFileInfo file;
        for (QStringList::Iterator it = fonts.begin(); it != fonts.end(); it++)
        {
            if (QFileInfo(*it).fileName().toLower() == fileLower)
            {
                path = *it;
                break;
            }
        }
    }
    else
    {
        path = m_strFileName;
        m_fontType = FontType::Invalid;
        return false;
    }

    // 判断文件是否可被打开
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        return false;
    }
    else
    {
        m_strFullFileName = path;
    }
    f.close();

    // 解析shx文件头
    if (path.endsWith(SHX_POST))
    {
        readShxPrefix(path);
    }
    else
    {
        m_fontType = FontType::System;
    }
    return true;
}

void DmFont::readShx(QString path)
{
    std::string str = path.toStdString();
    std::fstream fs;
    fs.open(str.c_str(), std::ios::binary | std::ios::in);
    if (!fs.is_open())
    {
        m_fontType = FontType::Invalid;
        return;
    }

    // 解析shx字体数据
    if (m_fontType == FontType::ShxBigFont)
    {
        #pragma region 大字体
        unsigned short codeCount;      // 字形码数，序号4
        fs.seekg(SHX_BIG_CODECOUNT_OFFSET, std::ios::beg);
        fs.read((char*)&codeCount, sizeof(unsigned short));
        unsigned short sectionNum;     // 区间码数，序号5。对于gbcbig.shx为1，hztxt.shx为3
        fs.seekg(SHX_BIG_SECTIONNUM_OFFSET, std::ios::beg);
        fs.read((char*)&sectionNum, sizeof(unsigned short));
        int codeStartAddr = SHX_BIG_CODE_START_BASE
            + static_cast<int>(sectionNum) * SHX_BIG_SECTION_RECORD_SIZE;
        unsigned short size = 0;
        unsigned int addr = 0;
        unsigned short codeNum = 0;   // 字形码
        for (unsigned short i = 0; i < codeCount; i++)
        {
            size = 0;
            addr = 0;
            codeNum = 0;
            fs.seekg(codeStartAddr + i * SHX_BIG_CODE_RECORD_SIZE, std::ios::beg);
            fs.read((char*)&codeNum, sizeof(unsigned short));
            fs.seekg(codeStartAddr + 2 + i * SHX_BIG_CODE_RECORD_SIZE, std::ios::beg);
            fs.read((char*)&size, sizeof(unsigned short));
            fs.seekg(codeStartAddr + 4 + i * SHX_BIG_CODE_RECORD_SIZE, std::ios::beg);
            fs.read((char*)&addr, sizeof(unsigned int));
            std::vector<unsigned char> bytes;
            bytes.resize(size);
            unsigned char tmpByte;
            for (unsigned short k = 0; k < size; k++)
            {
                fs.seekg(addr + k, std::ios::beg);
                fs.read((char*)&tmpByte, 1);
                bytes.at(k) = tmpByte;
            }
            m_shxRawData.insert(std::make_pair(codeNum, bytes));
        }
        #pragma endregion
    }
    else if (m_fontType == FontType::ShxASCII)
    {
        #pragma region ASCII字体
        unsigned short codeCount;
        fs.seekg(SHX_ASCII_CODECOUNT_OFFSET, std::ios::beg);
        fs.read((char*)&codeCount, sizeof(unsigned short));
        unsigned short size;
        unsigned short codeNum;        // 字形码
        size_t byteDataAddr = SHX_ASCII_DATA_OFFSET_START
            + SHX_ASCII_CODE_RECORD_SIZE * static_cast<size_t>(codeCount);
        for (unsigned short i = 0; i < codeCount; i++)
        {
            fs.seekg(SHX_ASCII_CODEINFO_OFFSET
                + i * SHX_ASCII_CODE_RECORD_SIZE, std::ios::beg);
            fs.read((char*)&codeNum, sizeof(unsigned short));
            fs.seekg(SHX_ASCII_CODEINFO_OFFSET + 2
                + i * SHX_ASCII_CODE_RECORD_SIZE, std::ios::beg);
            fs.read((char*)&size, sizeof(unsigned short));
            std::vector<unsigned char> bytes;
            bytes.resize(size);
            unsigned char tmpByte;
            for (unsigned short k = 0; k < size; k++)
            {
                fs.seekg(byteDataAddr + k, std::ios::beg);
                fs.read((char*)&tmpByte, 1);
                bytes.at(k) = tmpByte;
            }
            m_shxRawData.insert(std::make_pair(codeNum, bytes));
            byteDataAddr += size;
        }
        #pragma endregion
    }
    else if (m_fontType == FontType::ShxUnifont)
    {
        #pragma region unifont字体（与ASCII字体相比，少了最小字形码，字形数据直接在字形长度后）
        unsigned short codeCount;
        fs.seekg(SHX_UNI_CODECOUNT_OFFSET, std::ios::beg);
        fs.read((char*)&codeCount, sizeof(unsigned short));
        unsigned short size;
        unsigned short codeNum;        // 字形码
        size_t curAddr = SHX_UNI_CODEINFO_OFFSET;
        for (unsigned short i = 0; i < codeCount; i++)
        {
            fs.seekg(curAddr, std::ios::beg);
            fs.read((char*)&codeNum, sizeof(unsigned short));
            curAddr += 2;
            fs.seekg(curAddr, std::ios::beg);
            fs.read((char*)&size, sizeof(unsigned short));
            curAddr += 2;
            std::vector<unsigned char> bytes;
            bytes.resize(size);
            unsigned char tmpByte;
            for (unsigned short k = 0; k < size; k++)
            {
                fs.seekg(curAddr + k, std::ios::beg);
                fs.read((char*)&tmpByte, 1);
                bytes.at(k) = tmpByte;
            }
            curAddr += size;
            m_shxRawData.insert(std::make_pair(codeNum, bytes));
        }
        #pragma endregion
    }
    else
    {
        return;
    }
    fs.close();

    // 从字形码0取出文字的高度
    auto code0 = m_shxRawData.find(SHX_FONT_SPECIAL_CODE);
    if (code0 == m_shxRawData.end())
    {
        m_fontType = FontType::Invalid;
        return;
    }
    auto& code0Data = code0->second;
    auto noCommentIt = code0Data.begin();
    while (noCommentIt != code0Data.end())
    {
        if (*noCommentIt == 0)
        {
            break;
        }
        ++noCommentIt;
    }
    ++noCommentIt;
    size_t count = code0Data.end() - noCommentIt;
    constexpr size_t MIN_CODE0_DATA_SIZE = 4;
    if (count < MIN_CODE0_DATA_SIZE)
    {
        m_fontType = FontType::Invalid;
        return;
    }
    m_shxAbove = *noCommentIt;
    m_shxBelow = *(noCommentIt + 1);
    m_shxHeight = m_shxAbove + m_shxBelow;
}

void DmFont::readShxPrefix(QString path)
{
    // 与readShx()类似
    std::string str = path.toStdString();
    std::fstream fs;
    fs.open(str.c_str(), std::ios::binary | std::ios::in);
    if (!fs.is_open())
    {
        return;
    }
    char header[8];
    memset(header, 0, 8);
    fs.seekg(SHX_HEADER_OFFSET, std::ios::beg);
    fs.read(header, SHX_HEADER_BIGFONT_LEN);
    int cmp = strcmp(header, "bigfont");
    if (cmp == 0)  // 大字体
    {
        m_fontType = FontType::ShxBigFont;
    }
    else
    {
        memset(header, 0, 8);
        fs.seekg(SHX_HEADER_OFFSET, std::ios::beg);
        fs.read(header, SHX_HEADER_SHAPES_LEN);
        cmp = strcmp(header, "shapes");
        if (cmp == 0)
        {
            // ASCII字体或形文件
            unsigned short minCode;  // 最小字形码
            fs.seekg(SHX_MINIMUM_CODE_OFFSET, std::ios::beg);
            fs.read((char*)&minCode, sizeof(unsigned short));
            if (minCode != 0)  // 形文件
            {
                m_fontType = FontType::Invalid;
            }
            else
            {
                // ASCII字体
                m_fontType = FontType::ShxASCII;
            }
        }
        else
        {
            memset(header, 0, 8);
            fs.seekg(SHX_HEADER_OFFSET, std::ios::beg);
            fs.read(header, SHX_HEADER_UNIFONT_LEN);
            cmp = strcmp(header, "unifont");
            if (cmp == 0)
            {
                // unifont字体
                m_fontType = FontType::ShxUnifont;
            }
            else
            {
                m_fontType = FontType::Invalid;
            }
        }
    }
}

DmCharTemplate* DmFont::generateShxFont(const QString& ch)
{
    // 如果没有加载字体，加载它
    if (!m_bLoaded)
    {
        if (!loadFont())
        {
            return nullptr;
        }
    }

    // 如果是空白符，生成之
    DmCharTemplate* whiteSpaceChar = generateWhiteSpace(ch);
    if (whiteSpaceChar)
    {
        return whiteSpaceChar;
    }

    // 处理%%字符并提取shx字形码
    unsigned short codeNum = 0;
    if (ch.startsWith("%%"))
    {
        QString numStr = ch.right(ch.size() - 2);
        bool ok = false;
        codeNum = numStr.toInt(&ok);
        if (!ok)
        {
            return nullptr;
        }
    }
    else
    {
        QChar c = ch.at(0);
        if (c.unicode() < MAX_ASCII_CHAR_CODE)  // ASCII文字（拓展的）
        {
            codeNum = c.unicode();
        }
        else
        {
            QByteArray arr = ch.toLocal8Bit();
            unsigned char* chp = (unsigned char*)arr.data();
            if (arr.size() == 2)  // 汉字等其他字符
            {
                codeNum = (chp[0] << 8) | chp[1];  // 将2个char解析成unsigned short
            }
            else
            {
                return nullptr;
            }
        }
    }
    if (codeNum == SHX_FONT_SPECIAL_CODE)  // 0代表特殊字码，没有字形
    {
        return nullptr;
    }

    // 查找对应字形码的字形信息，生成字形
    std::map<unsigned short, std::vector<unsigned char>>::iterator it = m_shxRawData.find(codeNum);
    if (it == m_shxRawData.end())
    {
        return nullptr;
    }
    ShxCmdManager shxMgr(it->second);
    DmCharTemplate* letter = shxMgr.generateShxFont(ch);
    if (letter)
    {
        double maxY = letter->getMax().y;
        double minY = letter->getMin().y;
        double scale = 1.0 / (m_shxHeight * m_dScaleToOne);
        double boxx = letter->getWidthHeight().x;
        double boxy = letter->getWidthHeight().y;
        double maxX = letter->getMax().x;
        double whFactor = 1.0;
        if (boxy != 0.0)
        {
            whFactor = maxX / boxy;
        }
        letter->setWHFactor(whFactor);
        letter->setHeight(boxy);  // 高度在后面的scale会再调整
        if (maxY > 0.0 && minY < 0.0)
        {
            letter->setAscenderFactor(maxY / boxy);
        }
        letter->scale(DmVector(0.0, 0.0), DmVector(scale, scale));
        m_letterList.add(ch, letter);
    }
    return letter;
}

void DmFont::readSystem(QString path)
{
    if (library == nullptr)
    {
        FT_Error error = FT_Init_FreeType(&library);
        if (error)
        {
            m_fontType = FontType::Invalid;
            return;
        }
    }

    FT_Error error = FT_New_Face(library, path.toStdString().c_str(), 0, &m_sysFontFace);
    if (error)
    {
        m_fontType = FontType::Invalid;
        return;
    }
    error = FT_Set_Pixel_Sizes(m_sysFontFace, FTFONTSIZE, FTFONTSIZE);
    if (error)
    {
        m_fontType = FontType::Invalid;
        return;
    }
    m_fontType = FontType::System;
    if (readSysFamily())
    {
        // 用一个字符来计算实际高度比例
        QString testChar(FTFONTTESTCHAR);
        generateSysFont(testChar, true);
        m_dDAScale = std::abs((double)m_sysFontFace->descender / (double)m_sysFontFace->ascender);
    }
    else
    {
        m_fontType = FontType::Invalid;
    }
}

/// @brief 获得字体族名记录的优先级
/// @param nameId OpenType名称ID
/// @return 数值越小优先级越高
static int familyNamePriority(FT_UShort nameId)
{
    switch (nameId)
    {
    case TT_NAME_ID_WWS_FAMILY:
        return 0;
    case TT_NAME_ID_FONT_FAMILY:
        return 1;
    case TT_NAME_ID_PREFERRED_FAMILY:
        return 2;
    default:
        return 3;
    }
}

/// @brief 排序字体名称记录，优先取WWS/Legacy Family和Unicode编码
static bool nameComp(const FT_SfntName a, const FT_SfntName b)
{
    int aPriority = familyNamePriority(a.name_id);
    int bPriority = familyNamePriority(b.name_id);
    if (aPriority != bPriority)
    {
        return aPriority < bPriority;
    }

    // sort Unicode platforms first
    // preferring MS platform as it is more likely to
    // not have bogus entries, being the more tested one.
    if (a.platform_id != b.platform_id)
    {
        if (a.platform_id == TT_PLATFORM_MICROSOFT)
        {
            return true;
        }
        else if (b.platform_id == TT_PLATFORM_MICROSOFT)
        {
            return false;
        }
        else if (a.platform_id == TT_PLATFORM_APPLE_UNICODE)
        {
            return true;
        }
        else if (b.platform_id == TT_PLATFORM_APPLE_UNICODE)
        {
            return false;
        }
    }

    // sort Unicode encodings first
    if (a.encoding_id != b.encoding_id)
    {
        if (a.platform_id == TT_PLATFORM_MICROSOFT)
        {
            if (a.encoding_id == TT_MS_ID_UCS_4)
            {
                return true;
            }
            else if (b.encoding_id == TT_MS_ID_UCS_4)
            {
                return false;
            }
            else if (a.encoding_id == TT_MS_ID_UNICODE_CS)
            {
                return true;
            }
            else if (b.encoding_id == TT_MS_ID_UNICODE_CS)
            {
                return false;
            }
        }
    }

    // sort English names first
    if (a.language_id != b.language_id)
    {
        if (a.platform_id == TT_PLATFORM_MICROSOFT)
        {
            if (a.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES)
            {
                return true;
            }
            else if (b.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES)
            {
                return false;
            }
        }
    }

    // the rest is all the same for us
    return false;
}

/// @brief 解码字体名称记录
static QString decodeNameRecord(FT_SfntName name)
{
    QString string;
    QByteArray encoding;
    if (name.platform_id == TT_PLATFORM_APPLE_UNICODE)
    {
        encoding = "UTF-16BE";
    }
    else if (name.platform_id == TT_PLATFORM_MICROSOFT)
    {
        switch (name.encoding_id)
        {
        case TT_MS_ID_SYMBOL_CS:
        case TT_MS_ID_UNICODE_CS:
        case TT_MS_ID_UCS_4:
            encoding = "UTF-16BE";
            break;
        case TT_MS_ID_SJIS:
            encoding = "Shift-JIS";
            break;
        case TT_MS_ID_GB2312:
            encoding = "GB18030-0";
            break;
        case TT_MS_ID_BIG_5:
            encoding = "Big5";
            break;
        default:
            break;
        }
    }

    if (!encoding.isEmpty())
    {
        QTextCodec* codec = QTextCodec::codecForName(encoding);
        QByteArray bytes((const char*)name.string, name.string_len);
        string = codec->toUnicode(bytes);
    }

    return string;
}

/// @brief 获得字体族名
static QString getFamilyName(const FT_Face face)
{
    QString familyName(face->family_name);

    QVector<FT_SfntName> names;

    for (FT_UInt i = 0; i < FT_Get_Sfnt_Name_Count(face); i++)
    {
        FT_SfntName name;

        if (!FT_Get_Sfnt_Name(face, i, &name))
        {
            switch (name.name_id)
            {
            case TT_NAME_ID_WWS_FAMILY:
            case TT_NAME_ID_FONT_FAMILY:
            case TT_NAME_ID_PREFERRED_FAMILY:
            {
                QString decodedName = decodeNameRecord(name);
                if (!decodedName.isEmpty())
                {
                    names.append(name);
                }
            }
            break;
            default:
                break;
            }
        }
    }

    if (!names.isEmpty())
    {
        std::sort(names.begin(), names.end(), nameComp);
        foreach (const FT_SfntName & name, names)
        {
            QString string(decodeNameRecord(name));
            if (!string.isEmpty())
            {
                familyName = string;
                break;
            }
        }
    }

    return familyName;
}

bool DmFont::readSysFamily()
{
    m_strSysFamily = getFamilyName(m_sysFontFace);
    m_strSysStyle = QString::fromUtf8(m_sysFontFace->style_name);
    QString lowerStyle = m_strSysStyle.toLower();
    auto isValidStyle = [](const QString& str) {
        return str == "regular" || str == "bold" || str == "italic" || str == "bold italic";
    };
    if (!isValidStyle(lowerStyle))  // 有时候有写莫名其妙的名字
    {
        m_strSysStyle = m_sysFontFace->style_name;
        lowerStyle = m_strSysStyle.toLower();
        if (!isValidStyle(lowerStyle))
        {
            m_strSysStyle = "Regular";
        }
    }
    if (m_strSysFamily.isEmpty())
    {
        return false;
    }
    return true;
}

int DmFont::moveTo(FT_Vector* to, void* fp)
{
    DmFont* f = static_cast<DmFont*>(fp);
    f->m_moveToIdx.emplace_back((int)f->m_sysFontPts.size());
    f->m_sysFontPts.emplace_back(DmVector(int26p6_to_float(to->x), int26p6_to_float(to->y)));
    return 0;
}

int DmFont::lineTo(const FT_Vector* to, void* user)
{
    DmFont* f = static_cast<DmFont*>(user);
    f->m_sysFontPts.emplace_back(DmVector(int26p6_to_float(to->x), int26p6_to_float(to->y)));
    return 0;
}

int DmFont::conicTo(const FT_Vector* control, const FT_Vector* to, void* user)
{
    DmFont* f = static_cast<DmFont*>(user);
    DmVector p0 = f->m_sysFontPts.back();
    // 二阶贝塞尔：B(t) = (1-t)^2 * P0 + 2t(1-t) * P1 + t^2 * P2
    DmVector p1(int26p6_to_float(control->x), int26p6_to_float(control->y));
    DmVector p2(int26p6_to_float(to->x), int26p6_to_float(to->y));
    // 取4个点
    auto ts = {BEZIER2_SAMPLE_T1, BEZIER2_SAMPLE_T2, BEZIER2_SAMPLE_T3};
    for (auto t : ts)
    {
        DmVector p = p0 * (1.0 - t) * (1.0 - t) + p1 * 2.0 * t * (1.0 - t) + p2 * t * t;
        f->m_sysFontPts.emplace_back(p);
    }
    f->m_sysFontPts.emplace_back(DmVector(int26p6_to_float(to->x), int26p6_to_float(to->y)));
    return 0;
}

int DmFont::cubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
    DmFont* f = static_cast<DmFont*>(user);
    DmVector p0 = f->m_sysFontPts.back();
    // 三阶贝塞尔：B(t) = (1-t)^3 * P0 + 3t(1-t)^2 * P1 + 3*t^2*(1-t) + t^3 * P3
    DmVector p1(int26p6_to_float(control1->x), int26p6_to_float(control1->y));
    DmVector p2(int26p6_to_float(control2->x), int26p6_to_float(control2->y));
    DmVector p3(int26p6_to_float(to->x), int26p6_to_float(to->y));
    // 取8个点
    auto ts = {BEZIER3_SAMPLE_POINTS[0], BEZIER3_SAMPLE_POINTS[1], BEZIER3_SAMPLE_POINTS[2],
               BEZIER3_SAMPLE_POINTS[3], BEZIER3_SAMPLE_POINTS[4], BEZIER3_SAMPLE_POINTS[5],
               BEZIER3_SAMPLE_POINTS[6]};
    for (auto t : ts)
    {
        DmVector p = p0 * (1.0 - t) * (1.0 - t) * (1.0 - t) +
            p1 * 3.0 * t * (1.0 - t) * (1.0 - t) +
            p2 * 3.0 * t * t * (1.0 - t) +
            p3 * t * t * t;
        f->m_sysFontPts.emplace_back(p);
    }
    f->m_sysFontPts.emplace_back(DmVector(int26p6_to_float(to->x), int26p6_to_float(to->y)));
    return 0;
}

DmCharTemplate* DmFont::generateSysFont(const QString& ch, bool setScale)
{
    // 如果没有加载字体，加载它
    if (!m_bLoaded)
    {
        if (!loadFont())
        {
            return nullptr;
        }
    }
    if (m_fontType == FontType::Invalid)
    {
        return nullptr;
    }

    // 如果是空白符，生成之
    DmCharTemplate* whiteSpaceChar = generateWhiteSpace(ch);
    if (whiteSpaceChar)
    {
        return whiteSpaceChar;
    }

    // 处理%%字符并得到unicode
    unsigned short codeNum = 0;
    if (ch.startsWith("%%"))
    {
        QString numStr = ch.right(ch.size() - 2);
        bool ok = false;
        codeNum = numStr.toInt(&ok);
        if (!ok)
        {
            return nullptr;
        }
    }
    else
    {
        codeNum = ch.at(0).unicode();
    }

    // freetype解析字符
    int iGlyphIndex = FT_Get_Char_Index(m_sysFontFace, codeNum);
    if (iGlyphIndex == 0)
    {
        return nullptr;
    }
    FT_Int32 loadflags = FT_LOAD_DEFAULT;
    FT_Error error = FT_Load_Glyph(m_sysFontFace, iGlyphIndex, loadflags);
    if (error != 0)
    {
        return nullptr;
    }
    FT_GlyphSlot pGlyphSlot = m_sysFontFace->glyph;
    FT_Outline* outline = &pGlyphSlot->outline;

    // 分解文字，生成顶点数据
    m_sysFontPts.clear();
    m_moveToIdx.clear();
    FT_Outline_Funcs funcs;
    funcs.move_to = (FT_Outline_MoveTo_Func)&DmFont::moveTo;
    funcs.line_to = (FT_Outline_LineTo_Func)&DmFont::lineTo;
    funcs.conic_to = (FT_Outline_ConicTo_Func)&DmFont::conicTo;
    funcs.cubic_to = (FT_Outline_CubicTo_Func)&DmFont::cubicTo;
    funcs.delta = 0;
    funcs.shift = 0;
    // trace outline of the glyph
    FT_Error err = FT_Outline_Decompose(outline, &funcs, this);
    if (err)
    {
        return nullptr;
    }
    if (m_sysFontPts.size() == 0)
    {
        return nullptr;
    }

    // 取出真包围框
    double minY = m_sysFontPts.front().y;
    double maxY = m_sysFontPts.front().y;
    double minX = m_sysFontPts.front().x;
    double maxX = m_sysFontPts.front().x;
    for (auto it = m_sysFontPts.begin(); it != m_sysFontPts.end(); ++it)
    {
        if (it->y > maxY)
        {
            maxY = it->y;
        }
        if (it->y < minY)
        {
            minY = it->y;
        }
        if (it->x > maxX)
        {
            maxX = it->x;
        }
        if (it->x < minX)
        {
            minX = it->x;
        }
    }
    // 如果需要，计算放缩比例
    double scaleToOne = (maxY - minY) / FTFONTSIZE;
    if (setScale)
    {
        m_dScaleToOne = scaleToOne;
        return nullptr;  // 计算m_dScaleToOne时，不生成文字块
    }
    // 构建文字
    DmCharTemplate* letter = new DmCharTemplate(nullptr, ch, &m_letterList);
    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    letter->setPen(pen);
    letter->setLayer(nullptr);
    double whFactor = 1.0;
    double deltaX = maxX - minX;
    double deltaY = maxY - minY;
    if (deltaY != 0.0)
    {
        whFactor = maxX / deltaY;
    }
    letter->setWHFactor(whFactor);
    letter->setHeight(scaleToOne / m_dScaleToOne);
    if (maxY > 0.0 && minY < 0.0)
    {
        letter->setAscenderFactor(maxY / deltaY);
    }
    const double scale = 1.0 / (FTFONTSIZE * m_dScaleToOne);

    // 用CDT的Constrained Delaunay Triangulation做三角剖分
    std::vector<std::vector<DmVector>> boundaries;
    for (int i = 0; i < m_moveToIdx.size(); i++)
    {
        int startIdx = m_moveToIdx.at(i);
        int endIdx = 0;
        if (i != m_moveToIdx.size() - 1)
        {
            endIdx = m_moveToIdx.at(i + 1) - 1;
        }
        else
        {
            endIdx = m_sysFontPts.size() - 1;
        }
        std::vector<DmVector> pts;
        for (int k = startIdx; k < endIdx; k++)
        {
            auto pt = m_sysFontPts.at(k) * scale;
            pts.emplace_back(pt);
        }
        boundaries.emplace_back(pts);
    }
    std::vector<DmTriangle*> triangles;
    ConstrainedDelaunayTriangulation::trianglulate<DmTriangle*>({}, boundaries, triangles);
    for (auto triangle : triangles)
    {
        triangle->setParent(letter);
        triangle->setPen(pen);
        triangle->setLayer(nullptr);
        letter->addEntity(triangle);
    }

    letter->calculateBorders();
    // 保存到块表
    m_letterList.add(ch, letter);
    return letter;
}

DmCharTemplate* DmFont::generateWhiteSpace(const QString& ch)
{
    static std::set<QString> whiteSpaces = {
        QString(static_cast<ushort>(0x20)) /*空格*/,
        "\n",
        "\t"
    };
    auto it = whiteSpaces.find(ch);
    if (whiteSpaces.end() == it)  // 不是空白符返回空
    {
        return nullptr;
    }

    DmCharTemplate* letter = new DmCharTemplate(nullptr, ch, &m_letterList);
    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    letter->setPen(pen);
    letter->setLayer(nullptr);
    letter->setWHFactor(CHAR_DEFAULT_WIDTH_HEIGHT_FACTOR);
    letter->setHeight(CHAR_DEFAULT_HEIGHT);
    if (ch == QString(static_cast<ushort>(0x20)))
    {
        letter->setWHFactor(SPACE_WIDTH / WHITE_SPACE_HEIGHT);
        letter->setHeight(WHITE_SPACE_HEIGHT);
    }
    else if (ch == "\n")
    {
        letter->setWHFactor(LINEFEED_WIDTH / WHITE_SPACE_HEIGHT);
        letter->setHeight(LINEFEED_WIDTH);
    }
    else if (ch == "\t")
    {
        letter->setWHFactor(TAB_WIDTH / WHITE_SPACE_HEIGHT);
        letter->setHeight(TAB_WIDTH);
    }
    // 保存到块表
    m_letterList.add(ch, letter);
    return letter;
}

float DmFont::int26p6_to_float(int i)
{
    return (float)i / (float)FONT26P6_DIVISOR;
}

bool DmFont::isSystemFont(const QString& fontName)
{
    QString lower = fontName.toLower();
    if (lower.endsWith(TTF_POST) || lower.endsWith(TTC_POST))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool DmFont::isShxFont(const QString& fontName)
{
    QString lower = fontName.toLower();
    if (lower.endsWith(SHX_POST))
    {
        return true;
    }
    else
    {
        return false;
    }
}

QString DmFont::getPredefineFontNameOfLetter(const QString& letter)
{
    // 中文标点符号unicode，参考：https://blog.csdn.net/COCO56/article/details/87618925
    static std::unordered_set<ushort> chinesePunct{
        0x3002 /*。*/,
        0xFF1F /*？*/,
        0xFF01 /*！*/,
        0x3010 /*【*/,
        0x3011 /*】*/,
        0xFF0C /*，*/,
        0x3001 /*、*/,
        0xFF1B /*；*/,
        0xFF1A /*：*/,
        0x300C /*「*/,
        0x300D /*」*/,
        0x300E /*『*/,
        0x300F /*』*/,
        0x2019 /*'*/,
        0x201C /*"*/,
        0x201D /*"*/,
        0x2018 /*'*/,
        0xFF08 /*（*/,
        0xFF09 /*）*/,
        0x3014 /*〔*/,
        0x3015 /*〕*/,
        0x2026 /*...*/,
        0x2013 /*-*/,
        0xFF0E /*．*/,
        0x2014 /*-*/,
        0x300A /*《*/,
        0x300B /*》*/,
        0x3008 /*〈*/,
        0x3009 /*〉*/
    };

    // 汉字Unicode范围
    constexpr ushort CJK_UNIFIED_BEGIN = 0x4E00;
    constexpr ushort CJK_UNIFIED_END = 0x9FA5;
    constexpr ushort CJK_COMPAT_BEGIN = 0xF900;
    constexpr ushort CJK_COMPAT_END = 0xFA2D;
    constexpr ushort ASCII_MAX = 127;

    ushort code = letter.at(0).unicode();
    if ((code >= CJK_UNIFIED_BEGIN && code <= CJK_UNIFIED_END)
        || (code >= CJK_COMPAT_BEGIN && code <= CJK_COMPAT_END)
        || (chinesePunct.find(code) != chinesePunct.end()))
    {
        // 汉字
        return "simsun.ttc";
    }

    if (code <= ASCII_MAX)
    {
        // ASCII字符
        return "arial.ttf";
    }

    // 常用特殊符号
    static std::unordered_set<ushort> symbols{
        0x2248 /*几乎相等*/,
        0x2220,
        0xE100,
        0x2104,
        0x0394,
        0x0278,
        0xE101,
        0x2261,
        0xE102,
        0x2260,
        0x2126,
        0x03A9,
        0x214A,
        0x2082,
        0x00B2,
        0x00B3,
    };
    if (symbols.find(code) != symbols.end())
    {
        return "isocpeur.ttf";
    }
    return "";
}

QString DmFont::getNameWithoutExt(const QString& fontName)
{
    int idx = fontName.indexOf('.');
    if (idx != -1)
    {
        return fontName.mid(0, idx);
    }
    return fontName;
}

DmCharTemplate* DmFont::findLetter(const QString& name)
{
    if (m_fontType == FontType::Invalid)
    {
        return nullptr;
    }

    DmCharTemplate* ret = m_letterList.find(name);
    if (ret)
    {
        return ret;
    }
    if (m_fontType == FontType::System)
    {
        return generateSysFont(name);
    }
    else
    {
        return generateShxFont(name);
    }
}

ShxCmd* ShxCmd::createCmd(const std::vector<unsigned char>& byteData, size_t& idx)
{
    if (idx >= byteData.size())
    {
        return nullptr;
    }
    unsigned char c = byteData.at(idx);
    CmdType cmdType = getCmdTypeOf(c);

    ShxCmd* cmd = new ShxCmd();
    cmd->setCmdType(cmdType);
    if (cmd->hasParameter())
    {
        if (cmdType == _3DevideLength || cmdType == _4MultiplyLength)
        {
            cmd->m_scale = byteData.at(idx + 1);
            idx += 2;
        }
        else if (cmdType == _7InsertSubShape)
        {
            cmd->m_subShapeId = byteData.at(idx + 1);
            idx += 2;
        }
        else if (cmdType == _8Offset)
        {
            char x = (char)byteData.at(idx + 1);
            char y = (char)byteData.at(idx + 2);
            cmd->m_offsets.push_back(Point(x, y));
            idx += 3;
        }
        else if (cmdType == _9Offsets)
        {
            bool end = false;
            bool curX = true;
            char lastX = 0;
            size_t tmpIdx = idx + 1;
            while (tmpIdx < byteData.size())
            {
                if (curX)
                {
                    lastX = byteData.at(tmpIdx);
                }
                else
                {
                    char y = byteData.at(tmpIdx);
                    if (lastX == 0 && y == 0)  // 结束
                    {
                        end = true;
                    }
                    else
                    {
                        cmd->m_offsets.push_back(Point(lastX, y));
                    }
                }
                curX = !curX;
                tmpIdx++;
                if (end)
                {
                    break;
                }
            }
            idx = tmpIdx;
        }
        else if (cmdType == _10Arc)
        {
            constexpr int ARC_CLOCKWISE_SHIFT = 7;
            constexpr int ARC_START_SHIFT = 4;
            constexpr int ARC_SPAN_MASK = 0b00000111;
            constexpr int ARC_START_MASK = 0b01110000;
            cmd->m_arcRadius = byteData.at(idx + 1);
            cmd->m_arcClockwise = (byteData.at(idx + 2) >> ARC_CLOCKWISE_SHIFT) == 0x1;
            cmd->m_arcStart = (byteData.at(idx + 2) & ARC_START_MASK) >> ARC_START_SHIFT;
            cmd->m_arcSpan = byteData.at(idx + 2) & ARC_SPAN_MASK;
            idx += 3;
        }
        else if (cmdType == _11ArcComplex)
        {
            idx += 6;
        }
        else if (cmdType == _12ArcBulge)
        {
            char x = (char)byteData.at(idx + 1);
            char y = (char)byteData.at(idx + 2);
            char bulge = (char)byteData.at(idx + 3);
            cmd->m_offsets.push_back(Point(x, y, bulge));
            idx += 4;
        }
        else if (cmdType == _13ArcsBulge)
        {
            bool end = false;
            int type = 0;  // 0表示x，1表示y，2表示bulge
            char lastX = 0;
            char lastY = 0;
            size_t tmpIdx = idx + 1;
            while (tmpIdx < byteData.size())
            {
                if (type == 0)
                {
                    type = 1;
                    lastX = byteData.at(tmpIdx);
                }
                else if (type == 1)
                {
                    char y = byteData.at(tmpIdx);
                    if (lastX == 0 && y == 0)  // 结束
                    {
                        end = true;
                    }
                    else
                    {
                        lastY = y;
                        type = 2;
                    }
                }
                else
                {
                    char bulge = byteData.at(tmpIdx);
                    cmd->m_offsets.push_back(Point(lastX, lastY, bulge));
                    type = 0;
                }
                tmpIdx++;
                if (end)
                {
                    break;
                }
            }
            idx = tmpIdx;
        }
    }
    else
    {
        if (cmdType == _Normal)
        {
            constexpr int VEC_DIR_SHIFT = 4;
            constexpr int VEC_DIR_MASK = 0x0F;
            constexpr int VEC_LOOKUP_SIZE = 16;
            unsigned char vecSize = c >> VEC_DIR_SHIFT;
            unsigned char vecDir = c & VEC_DIR_MASK;
            const float xOffsets[VEC_LOOKUP_SIZE] = {1, 1, 1, 0.5f, 0, -0.5f, -1, -1,
                -1, -1, -1, -0.5f, 0, 0.5f, 1, 1};
            const float yOffsets[VEC_LOOKUP_SIZE] = {0, 0.5f, 1, 1, 1, 1, 1, 0.5f,
                0, -0.5f, -1, -1, -1, -1, -1, -0.5f};
            char x = (char)round(xOffsets[vecDir] * vecSize);
            char y = (char)round(yOffsets[vecDir] * vecSize);
            cmd->m_offsets.push_back(ShxCmd::Point(x, y));
        }
        idx++;
    }
    return cmd;
}

ShxCmd::CmdType ShxCmd::getCmdTypeOf(unsigned char cmdType)
{
    if (cmdType < CmdType::_Normal)
    {
        return (CmdType)cmdType;
    }
    else
    {
        return CmdType::_Normal;
    }
}

bool ShxCmd::hasParameter()
{
    if (m_cmdType == _3DevideLength || m_cmdType == _4MultiplyLength || m_cmdType == _7InsertSubShape ||
        m_cmdType == _8Offset || m_cmdType == _9Offsets || m_cmdType == _10Arc || m_cmdType == _11ArcComplex ||
        m_cmdType == _12ArcBulge || m_cmdType == _13ArcsBulge)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ShxCmdManager::ShxCmdManager(std::vector<unsigned char>& shxByteData)
    : m_shxByteData(shxByteData)
{
}

DmCharTemplate* ShxCmdManager::generateShxFont(const QString& ch)
{
    // 弧凸度除数
    constexpr double ARC_BULGE_DIVISOR = 127.0;
    // 圆弧跨度除数
    constexpr double ARC_SPAN_DIVISOR = 4.0;
    // 每度弧度
    constexpr double RAD_PER_DEGREE_MULTIPLIER = 45.0;

    size_t idx = 0;
    while (idx < m_shxByteData.size())
    {
        if (m_shxByteData.at(idx) == 0)
        {
            break;
        }
        idx++;
    }
    idx++;
    while (idx < m_shxByteData.size())
    {
        ShxCmd* cmd = ShxCmd::createCmd(m_shxByteData, idx);
        if (cmd != nullptr)
        {
            m_cmds.push_back(std::unique_ptr<ShxCmd>(cmd));
        }
    }

    // 创建文字
    DmCharTemplate* letter = new DmCharTemplate(nullptr, ch);
    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    letter->setPen(pen);
    letter->setLayer(nullptr);
    DmPolyline* pCurPoly = nullptr;
    bool enableDraw = false;
    float scale = 1;
    DmVector curPt(0.0, 0.0);
    idx = 0;
    while (idx < m_cmds.size())
    {
        ShxCmd* cmd = m_cmds.at(idx).get();
        ShxCmd::CmdType cmdType = cmd->getCmdType();
        if (cmdType == ShxCmd::_14OnlyForVertical)
        {
            idx += 2;
            continue;
        }
        if (cmdType == ShxCmd::_1EnableDraw)
        {
            enableDraw = true;
        }
        else if (cmdType == ShxCmd::_2DisableDraw)
        {
            if (pCurPoly)
            {
                pCurPoly->update();
                letter->addEntity(pCurPoly);
                pCurPoly = nullptr;
            }
            enableDraw = false;
        }
        else if (cmdType == ShxCmd::_3DevideLength)
        {
            scale /= cmd->getScale();
        }
        else if (cmdType == ShxCmd::_4MultiplyLength)
        {
            scale *= cmd->getScale();
        }
        else if (cmdType == ShxCmd::_5PushPosition)
        {
            // 暂不处理
        }
        else if (cmdType == ShxCmd::_6PopPosition)
        {
            // 暂不处理
        }
        else if (cmdType == ShxCmd::_7InsertSubShape)
        {
            // 暂不处理
        }
        else if (cmdType == ShxCmd::_8Offset || cmdType == ShxCmd::_Normal)
        {
            if (enableDraw)
            {
                createPolyIfNull(pCurPoly, letter, curPt);
                curPt.x += scale * cmd->getOffset().front().x;
                curPt.y += scale * cmd->getOffset().front().y;
                pCurPoly->appendVertex(curPt);
            }
            else
            {
                curPt.x += scale * cmd->getOffset().front().x;
                curPt.y += scale * cmd->getOffset().front().y;
            }
        }
        else if (cmdType == ShxCmd::_9Offsets)
        {
            if (enableDraw)
            {
                createPolyIfNull(pCurPoly, letter, curPt);
            }
            size_t i = 0;
            std::vector<ShxCmd::Point> pts = cmd->getOffset();
            while (i < pts.size())
            {
                curPt.x += scale * pts.at(i).x;
                curPt.y += scale * pts.at(i).y;
                if (enableDraw)
                {
                    pCurPoly->appendVertex(curPt);
                }
                i++;
            }
        }
        else if (cmdType == ShxCmd::_10Arc)
        {
            bool isClockwise = cmd->isArcClockwise();
            char start = cmd->getArcStart();
            char span = cmd->getArcSpan();
            double radius = cmd->getArcRadius() * scale;
            double radPerDegree = Math2d::deg2rad(1.0);
            double rad45 = radPerDegree * RAD_PER_DEGREE_MULTIPLIER;
            double a = start * rad45;  // 起点对应的角度
            double dx = std::cos(a) * radius;
            double dy = std::sin(a) * radius;
            double centerX = curPt.x - dx;
            double centerY = curPt.y - dy;
            if (span == 0)  // 圆
            {
                if (enableDraw)
                {
                    if (pCurPoly)  // 保存原来的
                    {
                        pCurPoly->update();
                        letter->addEntity(pCurPoly);
                        pCurPoly = nullptr;
                    }
                    CircleData circleData(DmVector(centerX, centerY, 0.0), radius);
                    DmCircle* pCircle = new DmCircle(letter, circleData);
                    pCircle->setPen(pen);
                    pCircle->setLayer(nullptr);
                    letter->addEntity(pCircle);
                }
            }
            else  // 圆弧
            {
                double spanRad = rad45 * span;
                double bulge = std::tan(spanRad / ARC_SPAN_DIVISOR);
                if (isClockwise)
                {
                    bulge = -bulge;
                }
                double b = 0.0;
                if (isClockwise)
                {
                    b = a - spanRad;
                }
                else
                {
                    b = a + spanRad;
                }
                double endX = centerX + std::cos(b) * radius;
                double endY = centerY + std::sin(b) * radius;
                if (enableDraw)
                {
                    createPolyIfNull(pCurPoly, letter, curPt);
                    curPt.x = endX;
                    curPt.y = endY;
                    pCurPoly->appendVertex(curPt, bulge);
                }
                else
                {
                    curPt.x = endX;
                    curPt.y = endY;
                }
            }
        }
        else if (cmdType == ShxCmd::_11ArcComplex)
        {
            // 暂不处理
        }
        else if (cmdType == ShxCmd::_12ArcBulge)
        {
            if (enableDraw)
            {
                createPolyIfNull(pCurPoly, letter, curPt);
                curPt.x += scale * cmd->getOffset().front().x;
                curPt.y += scale * cmd->getOffset().front().y;
                double bulge = cmd->getOffset().front().bulge / ARC_BULGE_DIVISOR;
                pCurPoly->appendVertex(curPt, bulge);
            }
            else
            {
                curPt.x += scale * cmd->getOffset().front().x;
                curPt.y += scale * cmd->getOffset().front().y;
            }
        }
        else if (cmdType == ShxCmd::_13ArcsBulge)
        {
            if (enableDraw)
            {
                createPolyIfNull(pCurPoly, letter, curPt);
            }
            size_t i = 0;
            std::vector<ShxCmd::Point> pts = cmd->getOffset();
            while (i < pts.size())
            {
                curPt.x += scale * pts.at(i).x;
                curPt.y += scale * pts.at(i).y;
                double bulge = pts.at(i).bulge / ARC_BULGE_DIVISOR;
                if (enableDraw)
                {
                    pCurPoly->appendVertex(curPt, bulge);
                }
                i++;
            }
        }
        idx++;
    }

    if (pCurPoly)
    {
        pCurPoly->update();
        letter->addEntity(pCurPoly);
        pCurPoly = nullptr;
    }

    if (letter->isEmpty())
    {
        delete letter;
        letter = nullptr;
        return nullptr;
    }
    else
    {
        letter->calculateBorders();
        return letter;
    }
}

void ShxCmdManager::createPolyIfNull(DmPolyline*& pCurPoly, DmCharTemplate* letter, DmVector firstPt)
{
    if (nullptr == pCurPoly)
    {
        pCurPoly = new DmPolyline(letter, PolylineData());
        DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
        pCurPoly->setPen(pen);
        pCurPoly->setLayer(nullptr);
        pCurPoly->appendVertex(firstPt);
    }
}
