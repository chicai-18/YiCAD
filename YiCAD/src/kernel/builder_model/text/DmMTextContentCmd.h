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


/// @file DmMTextContentCmd.h
/// @brief 多行文字内容命令类，解析和生成MText格式化内容

#ifndef DMMTEXTCONTENTCMD_H
#define DMMTEXTCONTENTCMD_H

#include "DmColor.h"
#include "DmMTextParagraph.h"

class DmMText;
class DmTextStyle;

/// @brief 多行文字内容的命令
class MTextContentCmd
{
public:
    /// @brief 命令类型枚举
    enum class CmdType
    {
        Value = 0,          ///< 仅仅是值，区别于以"\"开始的命令
        UndefinedCmd,       ///< 以"\"开始的命令，但是程序无法识别

        FontShx,            ///< shx字体，\fSimSun|b0|i0|c134|p2;
        FontSystem,         ///< 系统字体，\Ftxt|c134;
        ColorIndex,         ///< 索引颜色，\C1;
        ColorValue,         ///< 值颜色，\c14510626;
        Height,             ///< 值高度，\H0.5;
        HeightFactor,       ///< 倍数高度，\H0.5x;
        OverlineOpen,       ///< 开启上划线，\O
        OverlineClose,      ///< 关闭上划线，\o
        UnderlineOpen,      ///< 开启下划线，\L
        UnderlineClose,     ///< 关闭下划线，\l
        StrikethroughOpen,  ///< 开启删除线，\K
        StrikethroughClose, ///< 关闭删除线，\k
        Oblique,            ///< 倾斜角度，\Q20;
        WidthFactor,        ///< 宽度因子，\W2;
        AddParagraph,       ///< 新启段落，\P
        AddSlash,           ///< 添加反斜杠，"\\"
        AddBraceOpen,       ///< 添加左大括号，\{
        AddBraceClose,      ///< 添加右大括号，\}
        ParagraphAlignCenter,    ///< 段落居中，\pxqc;
        ParagraphAlignRight,     ///< 段落右对齐，\pqr;
        ParagraphAlignDistribute,///< 段落分散对齐，\pqd;
        ParagraphAlignLeft,      ///< 段落左对齐，\pql;
        ParagraphAlignDefault,   ///< 段落默认对齐，\px*;
        ParagraphAlignJustify,   ///< 段落对正，\pxj;
        NonBreakingSpace,        ///< 不间断空格（暂时处理成空格），\~

        /*以下情况暂不考虑*/
        Stack,          ///< 堆叠，\S...^...;
        CharSpacing,    ///< 字符之间的间距，\T2;
        Alignment,      ///< 对齐方式，\A1;
        LineSpace,      ///< 行距（倍数），\pxsm1.5;a\Pb
        ItemNumber,     ///< 项目符号和编号，\pxi-3,l3,t3;1.

        // 分栏信息在文字信息里，不在此处
        // 字段的行为尚未弄清楚
    };

public:
    /// @brief 构造函数
    /// @param cmdType 命令类型
    /// @param fullCmdString 完整命令字符串
    MTextContentCmd(MTextContentCmd::CmdType cmdType, const QString& fullCmdString = "");

    /// @brief 获得命令类型
    MTextContentCmd::CmdType getCmdType() const { return m_cmdType; }

    /// @brief 获得完整命令字符串
    QString getFullCmdString() const { return m_strFullCmdString; }

    /// @brief 获得字符串值
    QString getStrValue() const { return m_strVal; }

    /// @brief 设置字符串值
    void setStrValue(const QString& val) { m_strVal = val; }

    /// @brief 获得整数值
    int getIntValue() const { return m_iVal; }

    /// @brief 设置整数值
    void setIntValue(const int& iVal) { m_iVal = iVal; }

    /// @brief 获得浮点值
    double getDoubleValue() const { return m_dVal; }

    /// @brief 设置浮点值
    void setDoubleValue(const double& dVal) { m_dVal = dVal; }

    /// @brief 获得颜色
    DmColor getColor() const { return m_color; }

    /// @brief 设置颜色
    void setColor(const DmColor& color) { m_color = color; }

    /// @brief 获得字体信息
    /// @param fontName 输出：字体名
    /// @param isBold 输出：是否粗体
    /// @param isItalic 输出：是否斜体
    /// @param codePage 输出：代码页
    /// @param pitch 输出：字符间距
    void getFontValue(QString& fontName, bool& isBold, bool& isItalic, int& codePage, int& pitch) const;

    /// @brief 设置字体信息
    /// @param fontName 字体名
    /// @param isBold 是否粗体
    /// @param isItalic 是否斜体
    /// @param codePage 代码页
    /// @param pitch 字符间距
    void setFontValue(const QString& fontName, const bool& isBold, const bool& isItalic, const int& codePage, const int& pitch);

    /// @brief 计算本命令的字符串表示
    /// @return 命令字符串
    QString computeCmdString();

    /// @brief 命令类型是否需要以分号结束
    /// @param cmdType 命令类型
    /// @return 需要分号返回true
    static bool CmdNeedSemicolon(MTextContentCmd::CmdType cmdType);

    /// @brief 将段落对齐方式转换为命令类型
    /// @param align 段落对齐方式
    /// @return 对应的命令类型
    static MTextContentCmd::CmdType ParaAlignTypeToCmdType(DmMTextParagraph::Alignment align);

private:
    CmdType     m_cmdType;              ///< 命令类型
    QString     m_strFullCmdString;     ///< 完整命令字符串
    QString     m_strVal;               ///< 字符串值
    int         m_iVal;                 ///< 整数值
    double      m_dVal;                 ///< 浮点值

    // 颜色
    DmColor     m_color;                ///< 颜色

    // 字体专属
    QString     m_strFontName;          ///< 对于shx字体为文件名，对于系统字体为字体族名
    bool        m_bBold;                ///< 是否粗体
    bool        m_bItalic;              ///< 是否斜体
    int         m_iCodePage;            ///< 代码页
    int         m_iPitch;               ///< 字符间距
};


/// @brief 多行文字内容的命令管理器
class MTextContentCmdMgr
{
public:
    /// @brief 构造函数
    /// @param mtext 多行文字实体指针
    MTextContentCmdMgr(DmMText* mtext);

    /// @brief 通过多行文字实体生成内容
    /// @return 生成的内容字符串
    QString generateContent();

    /// @brief 通过多行文字内容生成实体
    void generateEntities();

    /// @brief 释放命令列表的内存
    /// @param cmdsList 命令列表
    void freeCmdList(std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList);

    /// @brief 生成指定范围的文字内容
    /// @param startChar 起始字符
    /// @param endChar 结束字符
    /// @return 内容字符串
    QString generateContentOfRange(const DmChar* startChar, const DmChar* endChar);

    /// @brief 在指定位置插入内容
    /// @param preChar 前一个字符
    /// @param postChar 后一个字符
    /// @param content 内容字符串
    /// @return 最后插入的字符
    DmChar* insertContentAtPos(const DmChar* preChar, const DmChar* postChar, const QString& content);

    /// @brief 解析整个文字的内容为命令
    /// @param content 内容字符串
    /// @param cmds 输出：命令列表
    void parseContentToCmds(const QString& content, std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmds);

private:
    /// @brief 通过反斜线后一个字符判断命令类型。不完全准确，但可初步判断出是否是已定义的命令
    /// @param c 字符
    /// @return 命令类型
    MTextContentCmd::CmdType getCmdTypeOfChar(const QString& c);

    /// @brief 通过命令字符串及预处理的命令类型，生成命令
    /// @param cmdStr 命令字符串
    /// @param preprocessedType 预处理类型
    /// @return 成功识别返回指定类型的命令，失败返回空
    MTextContentCmd* generateCmd(const QString& cmdStr, MTextContentCmd::CmdType preprocessedType);

    /// @brief 根据花括号将文字的"内容"分割
    /// @param content 内容字符串
    /// @return 分割后的字符串列表
    QStringList splitContent(const QString& content) const;

    /// @brief 处理花括号及非花括号的内容
    /// @param subContent 子内容字符串
    /// @param cmds 输出：命令列表
    void parseSubContent(const QString& subContent, std::vector<MTextContentCmd*>& cmds);

    /// @brief 由命令生成文字实体
    /// @param cmdsList 命令列表
    void generateEntitiesByCmds(const std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList);

    /// @brief 根据命令插入字符
    /// @param preChar 前一个字符
    /// @param postChar 后一个字符
    /// @param cmdsList 命令列表
    /// @return 返回最后插入的字符，没有插入字符返回空
    DmChar* insertEntitiesByCmds(const DmChar* preChar, const DmChar* postChar, const std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList);

    /// @brief 处理单个命令（生成实体或设置当前信息）
    void generateEntitiesByOneCmd(const MTextContentCmd* cmd, bool insert, DmChar*& preChar, DmChar*& postChar, const DmTextStyle* style, DmColor& color, double& height, double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline);

    /// @brief 由当前信息及字符生成文字实体
    void addStringEntities(const QString& str, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline);

    /// @brief 在指定位置插入字符串实体
    void insertStringEntities(const QString& str, DmChar*& preChar, DmChar*& postChar, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline);

    /// @brief 字符的格式是否匹配文字样式
    /// @param c 字符指针
    /// @return 匹配返回true
    bool isCharMatchTextStyle(const DmChar* c);

    /// @brief 多行文字实体生成命令
    /// @param cmds 输出：命令列表
    void parseMTextToCmds(std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmds);

    /// @brief 指定范围的多行文字生成命令
    void parseMTextRangeToCmds(std::vector<std::vector<MTextContentCmd*>>& cmds, const DmChar* startChar, const DmChar* endChar);

    /// @brief 生成字符的非格式命令
    /// @param c 字符指针
    /// @param cmds 输出：命令列表
    void generateNoFormatCmdsOfChar(const DmChar* c, std::vector<MTextContentCmd*>& cmds);

    /// @brief 命令生成内容
    /// @param cmdsList 命令列表
    /// @return 内容字符串
    QString parseCmdsToContent(const std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList);

    /// @brief 根据指定文字与当前文字格式（的区别）生成命令
    void generateCmdsOfCharFormat(const DmChar* c, const bool& init, std::vector<MTextContentCmd*>& cmds, DmColor& color, double& height, double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline);

    /// @brief 根据2种不同的格式信息（不含字体）生成命令
    void generateCmdsOfDataChange(const bool& init, std::vector<MTextContentCmd*>& cmds, const DmColor& newColor, const double& newHeight, const double& newWidthFactor, const double& newSlashAngle, const bool& newHasUnderline, const bool& newHasStrikethrough, const bool& newHasOverline,
                                  DmColor& color, double& height, double& widthFactor, double& slashAngle, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline);

    /// @brief 生成到文字样式格式的命令
    void generateCmdsOfFormatChangeToTextStyle(std::vector<MTextContentCmd*>& cmds, DmColor& color, double& height, double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline);

    /// @brief 将连续的值命令组合
    /// @param cmds 命令列表
    void combineContinuousValueCmds(std::vector<MTextContentCmd*>& cmds);

private:
    DmMText* m_pMText;                          ///< 多行文字实体指针
    std::vector<MTextContentCmd*> m_cmds;       ///< 命令列表
};

#endif //DMMTEXTCONTENTCMD_H
