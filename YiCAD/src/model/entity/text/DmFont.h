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


/// @file DmFont.h
/// @brief 字体类，代表一个字体文件的信息（shx字体或系统字体），包含ShxCmd/ShxCmdManager等辅助类

#ifndef DMFONT_H
#define DMFONT_H

#include <QMap>
#include <QStringList>
#include <iosfwd>
#include <memory>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "DmArc.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "DmSolid.h"
#include "DmCharTemplateList.h"

class DmCharTemplate;

/// @brief 字体类型枚举
enum class FontType
{
    ShxASCII,    ///< shx一般字体
    ShxUnifont,  ///< unifont字体
    ShxBigFont,  ///< shx大字体
    System,      ///< 系统字体
    Invalid      ///< shx形文件或其他无效文件
};

/// @brief 代表一个字体文件的信息。实现为DmDocument带名称（字体名）及若干块（每个字母一个块）
class DmFont
{
public:
    /// @brief 构造字体
    /// @param name 字体文件名
    DmFont(const QString& name);

    /// @brief 析构函数
    ~DmFont();

    /// @brief 获得字体文件名
    /// @return 字体文件名
    QString getFileName() const;

    /// @brief 获得字体文件许可证
    /// @return 许可证信息
    QString getFileLicense() const;

    /// @brief 获得字体创建日期
    /// @return 创建日期
    QString getFileCreate() const;

    /// @brief 获得字体编码
    /// @return 编码名称
    QString getEncoding() const;

    /// @brief 获得字体别名列表
    /// @return 常量引用别名列表
    const QStringList& getNames() const;

    /// @brief 获得字体作者列表
    /// @return 常量引用作者列表
    const QStringList& getAuthors() const;

    /// @brief 获得默认字母间距
    /// @return 字母间距值
    double getLetterSpacing();

    /// @brief 获得默认词间距
    /// @return 词间距值
    double getWordSpacing();

    /// @brief 获得默认行距因子
    /// @return 行距因子
    double getLineSpacingFactor();

    /// @brief 加载字体文件
    /// @return 加载成功返回true
    bool loadFont();

    /// @brief 是否已加载
    /// @return 已加载返回true
    bool isLoaded() const;

    /// @brief 加载shx前缀，以判断是大字体还是一般字体
    /// @return 加载成功返回true
    bool loadPrefix();

    /// @brief 获得文字模板列表
    /// @return 模板列表指针
    DmCharTemplateList* getLetterList();

    /// @brief 获得字体高度
    /// @return 高度值
    double getHeight() const;

    /// @brief 获得样式名
    /// @return 样式名
    QString getStyle() const;

    /// @brief 获得族名
    /// @return 族名
    QString getFamily() const;

    /// @brief 查找并生成一个文字模板
    /// @param name 文字名称
    /// @return 生成的文字模板
    DmCharTemplate* findLetter(const QString& name);

    /// @brief 获得字体类型
    /// @return 字体类型
    FontType getFontType() const;

    /// @brief 是否为shx字体
    /// @return 是返回true
    bool isShxFont() const;

    /// @brief 是否为系统字体
    /// @return 是返回true
    bool isSystemFont() const;

    /// @brief 是否有效
    /// @return 有效返回true
    bool isValid() const;

    /// @brief 释放freetype库
    static void freeLibrary();

    friend class DmFontList;

private:
    /// @brief 读取shx字体
    /// @param path 文件路径
    void readShx(QString path);

    /// @brief 读取shx前缀
    /// @param path 文件路径
    void readShxPrefix(QString path);

    /// @brief 生成shx文字模板
    /// @param ch 文字字符串
    /// @return 生成的文字模板
    DmCharTemplate* generateShxFont(const QString& ch);

    /// @brief 读取系统字体
    /// @param path 文件路径
    void readSystem(QString path);

    /// @brief 获得字体的族及样式
    /// @return 成功返回true
    bool readSysFamily();

    /// @brief 获得系统字体的字符
    /// @param ch 文字字符串
    /// @param setScale 是否设置缩放比例，为true时仅计算并设置放缩比例，返回空，为false时生成字符
    /// @return 生成的文字模板
    DmCharTemplate* generateSysFont(const QString& ch, bool setScale = false);

    /// @brief 如果是空白符，生成空白符，否则返回空
    /// @param ch 文字字符串
    /// @return 空白符模板或空
    DmCharTemplate* generateWhiteSpace(const QString& ch);

    /// @brief 将freetype 26.6格式转成float值
    /// @param i 26.6格式的整数值
    /// @return 浮点值
    static float int26p6_to_float(int i);

public:
    /// @brief 判断是否为系统字体
    /// @param fontFileName 字体文件名
    /// @return 是返回true
    static bool isSystemFont(const QString& fontFileName);

    /// @brief 判断是否为shx字体
    /// @param fontFileName 字体文件名
    /// @return 是返回true
    static bool isShxFont(const QString& fontFileName);

    /// @brief 通过字符获得一个默认的字体文件
    /// @param letter 字符
    /// @return 预定义的字体文件名
    static QString getPredefineFontNameOfLetter(const QString& letter);

    /// @brief 获得不带后缀的字体名
    /// @param fontName 字体名
    /// @return 不带后缀的名字
    static QString getNameWithoutExt(const QString& fontName);

    // 以下是系统字体解析时的回调函数
    static int moveTo(FT_Vector* to, void* fp);
    static int lineTo(const FT_Vector* to, void* user);
    static int conicTo(const FT_Vector* control, const FT_Vector* to, void* user);
    static int cubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user);

private:
    // shx字体数据
    std::map<unsigned short, std::vector<unsigned char>> m_shxRawData;
    int m_shxAbove{0};       ///< shx文字基线以上高度
    int m_shxBelow{0};       ///< shx文字基线以下高度
    int m_shxHeight{0};      ///< shx文字高度。实际生成的文字块都是1.0高度，这个是shx文件读取的高度，内部使用

    // 系统字体数据
    static FT_Library library;
    FT_Face m_sysFontFace = nullptr;
    QString m_strSysFamily;  ///< 系统字体族名
    QString m_strSysStyle;   ///< 系统字体样式名

    FontType m_fontType = FontType::Invalid; ///< 初步判断的字体类型

    std::vector<DmVector> m_sysFontPts; ///< 系统字体轮廓的顶点，按笔画顺序存储
    std::vector<int> m_moveToIdx;       ///< 系统字体轮廓生成时，记录moveTo时顶点索引

    DmCharTemplateList m_letterList;  ///< 保存检索过的文字模板

    QString m_strFileName;       ///< 字体文件，可能是全路径或相对路径
    QString m_strFullFileName;   ///< 字体文件的全路径，带后缀
    QString m_strFileLicense;    ///< Font file license
    QString m_strFileCreate;     ///< Font file creation date

    QString m_strEncoding;       ///< Font encoding

    QStringList m_names;         ///< Font names
    QStringList m_authors;       ///< Authors

    bool m_bLoaded = false;                    ///< 是否已加载，仅读取前缀不算加载
    double m_dScaleToOne = 1.0;                ///< shx固定0.75，系统字体初始化设置
    double m_dCharHeight = 0.0;                ///< 文字平均高度。1.0/m_dScaleToOne
    double m_dDAScale = (1.0 / 3.0);           ///< descender/ascender比例

    double m_dLetterSpacing = 3.0;             ///< Default letter spacing
    double m_dWordSpacing = 6.75;              ///< Default word spacing
    double m_dLineSpacingFactor = 1.0;         ///< Default line spacing factor
};

/// @brief shx字体的笔画命令
class ShxCmd
{
public:
    /// @brief 命令类型枚举
    enum CmdType
    {
        _0End,               ///< 开始/结束
        _1EnableDraw,        ///< 落笔
        _2DisableDraw,       ///< 抬笔
        _3DevideLength,      ///< 矢量长度除以下一个字节
        _4MultiplyLength,    ///< 矢量长度乘以下一个字节
        _5PushPosition,      ///< 将当前位置入栈
        _6PopPosition,       ///< 将当前位置出栈
        _7InsertSubShape,    ///< 插入子形
        _8Offset,            ///< xy偏移
        _9Offsets,           ///< 连续xy偏移
        _10Arc,              ///< 绘制圆弧
        _11ArcComplex,       ///< 绘制复杂圆弧（暂不处理）
        _12ArcBulge,         ///< xy偏移及凸度绘制圆弧
        _13ArcsBulge,        ///< 连续xy偏移及凸度绘制圆弧
        _14OnlyForVertical,  ///< 仅当文字垂直时处理
        _Normal              ///< 矢量绘制命令
    };

public:
    /// @brief shx命令中的点坐标
    struct Point
    {
        /// @brief 带凸度的构造
        Point(char px, char py, char pBulge = 0) : x(px), y(py), bulge(pBulge) {}
        /// @brief 默认构造
        Point() : x(0), y(0), bulge(0) {}
        char x;
        char y;
        char bulge;
    };

    /// @brief 从字节数据创建命令
    /// @param byteData 字节数据
    /// @param idx 当前索引
    /// @return 创建的命令
    static ShxCmd* createCmd(const std::vector<unsigned char>& byteData, size_t& idx);

    /// @brief 通过字节获得命令类型
    static CmdType getCmdTypeOf(unsigned char cmdType);

    /// @brief 获得命令类型
    CmdType getCmdType() { return m_cmdType; }

    /// @brief 设置命令类型
    void setCmdType(CmdType cmdType) { m_cmdType = cmdType; }

    /// @brief 是否有参数
    bool hasParameter();

    /// @brief 获得缩放
    unsigned char getScale() const { return m_scale; }

    /// @brief 获得偏移向量
    std::vector<Point> getOffset() const { return m_offsets; }

    /// @brief 获得圆弧半径
    unsigned char getArcRadius() { return m_arcRadius; }

    /// @brief 圆弧是否顺时针
    bool isArcClockwise() { return m_arcClockwise; }

    /// @brief 获得圆弧起始位置
    char getArcStart() { return m_arcStart; }

    /// @brief 获得圆弧跨度
    char getArcSpan() { return m_arcSpan; }

    ~ShxCmd() = default;

private:
    ShxCmd() = default;

private:
    CmdType m_cmdType;
    unsigned char m_scale{1};
    unsigned char m_subShapeId = 0;
    std::vector<Point> m_offsets;

    // 下面这些参数针对_10Arc
    unsigned char m_arcRadius = 0;
    bool m_arcClockwise{false};
    char m_arcStart = 0;
    char m_arcSpan = 0;
};

class DmPolyline;
class DmVector;

/// @brief shx字体笔画管理，可生成shx文字模板
class ShxCmdManager
{
public:
    std::vector<std::unique_ptr<ShxCmd>> m_cmds;
    std::vector<unsigned char> m_shxByteData;

    /// @brief 构造笔画管理器
    /// @param shxByteData shx字节数据
    ShxCmdManager(std::vector<unsigned char>& shxByteData);

    /// @brief 生成shx文字模板
    /// @param ch 文字字符串
    /// @return 生成的文字模板
    DmCharTemplate* generateShxFont(const QString& ch);

private:
    /// @brief 创建多段线（如果为空）
    void createPolyIfNull(DmPolyline*& poly, DmCharTemplate* letter, DmVector firstPt);
};

/// @brief 系统字体字符点
struct SysCharPoint
{
    double x = 0.0;
    double y = 0.0;
    int type = 1;  ///< 1表示一般的点，2表示三阶贝塞尔控制点，0表示二阶贝塞尔控制点
};

#endif //DMFONT_H
