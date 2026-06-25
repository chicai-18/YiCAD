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


/// @file DmMTextContentCmd.cpp
/// @brief 多行文字内容命令类实现，解析和生成MText格式化内容

#include "DmMTextContentCmd.h"
#include "DmMText.h"
#include "DmChar.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmMTextLine.h"
#include "DmTextStyle.h"
#include "DmCharTemplate.h"

// 颜色通道位偏移常量
constexpr int COLOR_BLUE_SHIFT = 16;
constexpr int COLOR_GREEN_SHIFT = 8;
constexpr int COLOR_CHANNEL_MASK = 0xFF;
constexpr int COLOR_BYBLOCK_INDEX = 0;
constexpr int COLOR_BYLAYER_INDEX = 256;

// 字符串条件判断字符位置
constexpr int CMD_CHAR_POS_BOLD_ITALIC = 1;
constexpr int CMD_CHAR_POS_ZERO = 0;

MTextContentCmd::MTextContentCmd(MTextContentCmd::CmdType cmdType, const QString& fullCmdString /*= ""*/)
    : m_cmdType(cmdType)
    , m_strFullCmdString(fullCmdString)
    , m_strVal("")
    , m_iVal(0)
    , m_dVal(0.0)
    , m_strFontName("")
    , m_bBold(false)
    , m_bItalic(false)
    , m_iCodePage(0)
    , m_iPitch(0)
{
}

void MTextContentCmd::getFontValue(QString& fontName, bool& isBold, bool& isItalic, int& codePage, int& pitch) const
{
    fontName = m_strFontName;
    isBold = m_bBold;
    isItalic = m_bItalic;
    codePage = m_iCodePage;
    pitch = m_iPitch;
}

void MTextContentCmd::setFontValue(const QString& fontName, const bool& isBold, const bool& isItalic, const int& codePage, const int& pitch)
{
    m_strFontName = fontName;
    m_bBold = isBold;
    m_bItalic = isItalic;
    m_iCodePage = codePage;
    m_iPitch = pitch;
}

QString MTextContentCmd::computeCmdString()
{
    QString res("");
    switch (m_cmdType)
    {
        case MTextContentCmd::CmdType::Value:
        {
            res = m_strVal;
        }
            break;
        case MTextContentCmd::CmdType::UndefinedCmd:
            break;
        case MTextContentCmd::CmdType::FontShx:
        {
            res = QString("\\F%1|c%2").arg(m_strFontName).arg(m_iPitch);
        }
            break;
        case MTextContentCmd::CmdType::FontSystem:
        {
            res = QString("\\f%1|b%2|i%3|c%4|p%5").arg(m_strFontName).arg(m_bBold).arg(m_bItalic).arg(m_iCodePage).arg(m_iPitch);
        }
            break;
            // TODO : 暂不支持
        case MTextContentCmd::CmdType::ColorIndex:
            break;
        case MTextContentCmd::CmdType::ColorValue:
        {
            if (m_color.isByLayer())
            {
                res = "\\C256";
            }
            else if (m_color.isByBlock())
            {
                res = "\\C0";
            }
            else
            {
                int red = m_color.red();
                int green = m_color.green();
                int blue = m_color.blue();
                int c = (blue << COLOR_BLUE_SHIFT) | (green << COLOR_GREEN_SHIFT) | red;
                res = QString("\\c%1").arg(QString::number(c));
            }
        }
            break;
            // TODO : 不知道什么时候使用
        case MTextContentCmd::CmdType::Height:
            break;
        case MTextContentCmd::CmdType::HeightFactor:
        {
            QString valStr = QString::number(m_dVal);
            res = QString("\\H%1x").arg(valStr);
        }
            break;
        case MTextContentCmd::CmdType::OverlineOpen:
        {
            res = "\\O";
        }
            break;
        case MTextContentCmd::CmdType::OverlineClose:
        {
            res = "\\o";
        }
            break;
        case MTextContentCmd::CmdType::UnderlineOpen:
        {
            res = "\\L";
        }
            break;
        case MTextContentCmd::CmdType::UnderlineClose:
        {
            res = "\\l";
        }
            break;
        case MTextContentCmd::CmdType::StrikethroughOpen:
        {
            res = "\\K";
        }
            break;
        case MTextContentCmd::CmdType::StrikethroughClose:
        {
            res = "\\k";
        }
            break;
        case MTextContentCmd::CmdType::Oblique:
        {
            QString valStr = QString::number(m_dVal);
            res = QString("\\Q%1").arg(valStr);
        }
            break;
        case MTextContentCmd::CmdType::WidthFactor:
        {
            QString valStr = QString::number(m_dVal);
            res = QString("\\W%1").arg(valStr);
        }
            break;
        case MTextContentCmd::CmdType::AddParagraph:
        {
            res = "\\P";
        }
            break;
        case MTextContentCmd::CmdType::AddSlash:
        {
            res = "\\";
        }
            break;
        case MTextContentCmd::CmdType::AddBraceOpen:
        {
            res = "{";
        }
            break;
        case MTextContentCmd::CmdType::AddBraceClose:
        {
            res = "}";
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignCenter:
        {
            res = "\\pqc";
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignRight:
        {
            res = "\\pqr";
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignDistribute:
        {
            res = "\\pqd";
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignLeft:
        {
            res = "\\pql";
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignDefault:
        {
            res = "\\pq*";
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignJustify:
        {
            res = "\\pxj";
        }
            break;
        case MTextContentCmd::CmdType::NonBreakingSpace:
        {
            res = "\\~";
        }
            break;
            // 其他的不支持
        case MTextContentCmd::CmdType::Stack:
            break;
        case MTextContentCmd::CmdType::CharSpacing:
            break;
        case MTextContentCmd::CmdType::Alignment:
            break;
        case MTextContentCmd::CmdType::LineSpace:
            break;
        case MTextContentCmd::CmdType::ItemNumber:
            break;
        default:
            break;
    }

    if (res.isEmpty())
    {
        return res;
    }
    if (MTextContentCmd::CmdNeedSemicolon(m_cmdType))
    {
        res.append(";");
    }
    return res;
}

bool MTextContentCmd::CmdNeedSemicolon(MTextContentCmd::CmdType cmdType)
{
    bool need = true;
    switch (cmdType)
    {
        case MTextContentCmd::CmdType::UndefinedCmd:
        case MTextContentCmd::CmdType::FontShx:
        case MTextContentCmd::CmdType::FontSystem:
        case MTextContentCmd::CmdType::ColorIndex:
        case MTextContentCmd::CmdType::ColorValue:
        case MTextContentCmd::CmdType::Height:
        case MTextContentCmd::CmdType::HeightFactor:
        case MTextContentCmd::CmdType::Oblique:
        case MTextContentCmd::CmdType::WidthFactor:
        case MTextContentCmd::CmdType::ParagraphAlignCenter:
        case MTextContentCmd::CmdType::ParagraphAlignRight:
        case MTextContentCmd::CmdType::ParagraphAlignDistribute:
        case MTextContentCmd::CmdType::ParagraphAlignLeft:
        case MTextContentCmd::CmdType::ParagraphAlignDefault:
        case MTextContentCmd::CmdType::ParagraphAlignJustify:
        case MTextContentCmd::CmdType::Stack:
        case MTextContentCmd::CmdType::CharSpacing:
        case MTextContentCmd::CmdType::Alignment:
        case MTextContentCmd::CmdType::LineSpace:
        case MTextContentCmd::CmdType::ItemNumber:
            need = true;
            break;
        case MTextContentCmd::CmdType::OverlineOpen:
        case MTextContentCmd::CmdType::OverlineClose:
        case MTextContentCmd::CmdType::UnderlineOpen:
        case MTextContentCmd::CmdType::UnderlineClose:
        case MTextContentCmd::CmdType::StrikethroughOpen:
        case MTextContentCmd::CmdType::StrikethroughClose:
        case MTextContentCmd::CmdType::Value:
        case MTextContentCmd::CmdType::AddParagraph:
        case MTextContentCmd::CmdType::AddSlash:
        case MTextContentCmd::CmdType::AddBraceOpen:
        case MTextContentCmd::CmdType::AddBraceClose:
        case MTextContentCmd::CmdType::NonBreakingSpace:
            need = false;
            break;
        default:
            break;
    }
    return need;
}

MTextContentCmd::CmdType MTextContentCmd::ParaAlignTypeToCmdType(DmMTextParagraph::Alignment align)
{
    MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::ParagraphAlignDefault;
    switch (align)
    {
        case DmMTextParagraph::Alignment::Default:
            cmdType = MTextContentCmd::CmdType::ParagraphAlignDefault;
            break;
        case DmMTextParagraph::Alignment::Left:
            cmdType = MTextContentCmd::CmdType::ParagraphAlignLeft;
            break;
        case DmMTextParagraph::Alignment::Mid:
            cmdType = MTextContentCmd::CmdType::ParagraphAlignCenter;
            break;
        case DmMTextParagraph::Alignment::Right:
            cmdType = MTextContentCmd::CmdType::ParagraphAlignRight;
            break;
        case DmMTextParagraph::Alignment::Justify:
            cmdType = MTextContentCmd::CmdType::ParagraphAlignJustify;
            break;
        case DmMTextParagraph::Alignment::Distribute:
            cmdType = MTextContentCmd::CmdType::ParagraphAlignDistribute;
            break;
        default:
            break;
    }
    return cmdType;
}

MTextContentCmdMgr::MTextContentCmdMgr(DmMText* mtext)
{
    m_pMText = mtext;
}

QString MTextContentCmdMgr::generateContent()
{
    // 生成命令
    std::vector<std::pair<bool, std::vector<MTextContentCmd*>>> cmdsList;
    parseMTextToCmds(cmdsList);

    // 由命令生成内容
    QString content = parseCmdsToContent(cmdsList);

    // 释放命令列表
    freeCmdList(cmdsList);

    return content;
}

void MTextContentCmdMgr::generateEntities()
{
    // 解析出命令
    QString content = m_pMText->getDataConstPtr()->getTextString();
    std::vector<std::pair<bool, std::vector<MTextContentCmd*>>> cmdsList;
    parseContentToCmds(content, cmdsList);

    // 由命令生成文字
    generateEntitiesByCmds(cmdsList);

    // 释放命令列表
    freeCmdList(cmdsList);
}

void MTextContentCmdMgr::freeCmdList(std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList)
{
    for (auto item : cmdsList)
    {
        for (auto cmd : item.second)
        {
            delete cmd;
        }
    }
}

QString MTextContentCmdMgr::generateContentOfRange(const DmChar* startChar, const DmChar* endChar)
{
    // 生成命令
    std::vector<std::vector<MTextContentCmd*>> cmdsList;
    parseMTextRangeToCmds(cmdsList, startChar, endChar);

    // 由命令生成内容
    std::vector<std::pair<bool, std::vector<MTextContentCmd*>>> newCmdsList;
    for (auto& cmds : cmdsList)
    {
        auto item = std::make_pair(true, cmds);
        newCmdsList.emplace_back(item);
    }
    QString content = parseCmdsToContent(newCmdsList);

    // 释放命令列表
    freeCmdList(newCmdsList);

    return content;
}

DmChar* MTextContentCmdMgr::insertContentAtPos(const DmChar* preChar, const DmChar* postChar, const QString& content)
{
    // 解析出命令
    std::vector<std::pair<bool, std::vector<MTextContentCmd*>>> cmdsList;
    parseContentToCmds(content, cmdsList);

    // 由命令生成文字
    DmChar* lastInsertedChar = insertEntitiesByCmds(preChar, postChar, cmdsList);

    // 释放命令列表
    freeCmdList(cmdsList);
    return lastInsertedChar;
}

MTextContentCmd::CmdType MTextContentCmdMgr::getCmdTypeOfChar(const QString& c)
{
    static std::unordered_map<QString, MTextContentCmd::CmdType> charCmdMap {
            {"f", MTextContentCmd::CmdType::FontSystem},
            {"F", MTextContentCmd::CmdType::FontShx},
            {"C", MTextContentCmd::CmdType::ColorIndex},
            {"c", MTextContentCmd::CmdType::ColorValue},
            {"H", MTextContentCmd::CmdType::Height},    // 需要继续分析
            {"O", MTextContentCmd::CmdType::OverlineOpen},
            {"o", MTextContentCmd::CmdType::OverlineClose},
            {"L", MTextContentCmd::CmdType::UnderlineOpen},
            {"l", MTextContentCmd::CmdType::UnderlineClose},
            {"K", MTextContentCmd::CmdType::StrikethroughOpen},
            {"k", MTextContentCmd::CmdType::StrikethroughClose},
            {"Q", MTextContentCmd::CmdType::Oblique},
            {"W", MTextContentCmd::CmdType::WidthFactor},
            {"P", MTextContentCmd::CmdType::AddParagraph},
            {"\\", MTextContentCmd::CmdType::AddSlash},
            {"{", MTextContentCmd::CmdType::AddBraceOpen},
            {"}", MTextContentCmd::CmdType::AddBraceClose},
            {"p", MTextContentCmd::CmdType::ParagraphAlignDefault},  // 需要继续分析
            {"S", MTextContentCmd::CmdType::Stack},
            {"~", MTextContentCmd::CmdType::NonBreakingSpace},
            {"T", MTextContentCmd::CmdType::CharSpacing},
            {"A", MTextContentCmd::CmdType::Alignment},
    };
    auto it = charCmdMap.find(c);
    if (it != charCmdMap.end())
    {
        return it->second;
    }
    return MTextContentCmd::CmdType::UndefinedCmd;
}

MTextContentCmd* MTextContentCmdMgr::generateCmd(const QString& cmdStr, MTextContentCmd::CmdType preprocessedType)
{
    if (preprocessedType == MTextContentCmd::CmdType::UndefinedCmd)
    {
        return nullptr;
    }
    if (preprocessedType == MTextContentCmd::CmdType::Value)
    {
        MTextContentCmd::CmdType cmdType = preprocessedType;
        MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
        cmd->setStrValue(cmdStr);
        return cmd;
    }
    // "\p"打头的命令需要继续分析
    else if (preprocessedType == MTextContentCmd::CmdType::ParagraphAlignDefault)
    {
        MTextContentCmd::CmdType cmdType = preprocessedType;
        if (cmdStr == "\\pqc;" || cmdStr == "\\pxqc;")
        {
            cmdType = MTextContentCmd::CmdType::ParagraphAlignCenter;
        }
        else if (cmdStr == "\\pqr;" || cmdStr == "\\pxqr;")
        {
            cmdType = MTextContentCmd::CmdType::ParagraphAlignRight;
        }
        else if (cmdStr == "\\pqd;" || cmdStr == "\\pxqd;")
        {
            cmdType = MTextContentCmd::CmdType::ParagraphAlignDistribute;
        }
        else if (cmdStr == "\\pql;" || cmdStr == "\\pxql;")
        {
            cmdType = MTextContentCmd::CmdType::ParagraphAlignLeft;
        }
        else if (cmdStr == "\\pq*;" || cmdStr == "\\pxq*;")
        {
            cmdType = MTextContentCmd::CmdType::ParagraphAlignDefault;
        }
        else if (cmdStr == "\\pqj;" || cmdStr == "\\pxqj;")
        {
            cmdType = MTextContentCmd::CmdType::ParagraphAlignJustify;
        }
        else
        {
            // 其他的暂不处理
            return nullptr;
        }
        MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
        return cmd;
    }
    // "\H"打头的命令需要继续分析
    else if (preprocessedType == MTextContentCmd::CmdType::Height)
    {
        MTextContentCmd::CmdType cmdType = preprocessedType;
        int xIdx = cmdStr.size() - 2;
        QString strFactor;
        if (cmdStr.at(xIdx) == 'x')
        {
            cmdType = MTextContentCmd::CmdType::HeightFactor;
            strFactor = cmdStr.mid(2, xIdx - 2);
        }
        else
        {
            cmdType = MTextContentCmd::CmdType::Height;
            strFactor = cmdStr.mid(2, cmdStr.count() - 2);
        }
        bool ok = false;
        double dVal = strFactor.toDouble(&ok);
        if (ok)
        {
            MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
            cmd->setDoubleValue(dVal);
            return cmd;
        }
        else
        {
            return nullptr;
        }
    }
    // 其他情况命令是正确的
    else
    {
        MTextContentCmd::CmdType cmdType = preprocessedType;
        switch (cmdType)
        {
            case MTextContentCmd::CmdType::FontShx:
            {
                // \Ftxt|c134;
                QString subStr = cmdStr.mid(2, cmdStr.size() - 3);
                QStringList list = subStr.split('|');
                QString name = list.first();
                MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
                cmd->setFontValue(name, false, false, 0, 0);
                return cmd;
            }
                break;
            case MTextContentCmd::CmdType::FontSystem:
            {
                // \fSimSun|b0|i0|c134|p2;
                QString subStr = cmdStr.mid(2, cmdStr.size() - 3);
                QStringList list = subStr.split('|');
                QString name = list.first();
                bool bold = false;
                bool italic = false;
                for (int i = 1; i < list.size(); i++)
                {
                    QString sub = list.at(i);
                    if (sub.startsWith("b"))
                    {
                        if (sub.at(CMD_CHAR_POS_BOLD_ITALIC) != '0')
                        {
                            bold = true;
                        }
                    }
                    else if (sub.startsWith("i"))
                    {
                        if (sub.at(CMD_CHAR_POS_BOLD_ITALIC) != '0')
                        {
                            italic = true;
                        }
                    }
                }
                MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
                cmd->setFontValue(name, bold, italic, 0, 0);
                return cmd;
            }
                break;
            case MTextContentCmd::CmdType::ColorIndex:
            {
                // \C1;      索引色
                // \C0       随块      特殊处理
                // \C256     随层      特殊处理
                QString subStr = cmdStr.mid(2, cmdStr.size() - 3);
                int idx = subStr.toInt();
                DmColor color;
                if (idx == COLOR_BYBLOCK_INDEX) // 随块
                {
                    color = DmColor(DM::FlagByBlock);
                }
                else if (idx == COLOR_BYLAYER_INDEX) // 随层
                {
                    color = DmColor(DM::FlagByLayer);
                }
                else
                {
                    color = DM::indexColors[idx];
                }
                MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
                cmd->setColor(color);
                return cmd;
            }
                break;
            case MTextContentCmd::CmdType::ColorValue:
            {
                // \c14510626;
                QString subStr = cmdStr.mid(2, cmdStr.size() - 3);
                int intColor = subStr.toInt();
                int red = intColor & COLOR_CHANNEL_MASK;
                int green = intColor >> COLOR_GREEN_SHIFT & COLOR_CHANNEL_MASK;
                int blue = intColor >> COLOR_BLUE_SHIFT & COLOR_CHANNEL_MASK;
                DmColor color = DmColor(red, green, blue);
                MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
                cmd->setColor(color);
                return cmd;
            }
                break;
            case MTextContentCmd::CmdType::OverlineOpen:
            case MTextContentCmd::CmdType::OverlineClose:
            case MTextContentCmd::CmdType::UnderlineOpen:
            case MTextContentCmd::CmdType::UnderlineClose:
            case MTextContentCmd::CmdType::StrikethroughOpen:
            case MTextContentCmd::CmdType::StrikethroughClose:
            case MTextContentCmd::CmdType::AddParagraph:
            case MTextContentCmd::CmdType::AddSlash:
            case MTextContentCmd::CmdType::AddBraceOpen:
            case MTextContentCmd::CmdType::AddBraceClose:
            case MTextContentCmd::CmdType::NonBreakingSpace:
            {
                MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
                return cmd;
            }
                break;
            case MTextContentCmd::CmdType::Oblique:
            case MTextContentCmd::CmdType::WidthFactor:
            {
                QString subStr = cmdStr.mid(2, cmdStr.count() - 3);
                double dVal = subStr.toDouble();
                MTextContentCmd* cmd = new MTextContentCmd(cmdType, cmdStr);
                cmd->setDoubleValue(dVal);
                return cmd;
            }
                break;
            case MTextContentCmd::CmdType::Stack:
            case MTextContentCmd::CmdType::CharSpacing:
            case MTextContentCmd::CmdType::Alignment:
            case MTextContentCmd::CmdType::LineSpace:
            case MTextContentCmd::CmdType::ItemNumber:
            {
                return nullptr;
            }
                break;
            default:
                break;
        }
    }
    return nullptr;
}

QStringList MTextContentCmdMgr::splitContent(const QString& content) const
{
    QStringList list;
    bool setLeft = false;
    int lastIdx = 0;
    int size = (int)content.size();
    for (int i = 0; i < size; i++)
    {
        QChar c = content.at(i);
        if (i - 1 >= 0)
        {
            QChar preChar = content.at(i - 1);
            // "\\{"表示可见花括号字符
            if (preChar == "\\")
            {
                continue;
            }
        }
        if (c == '{')
        {
            setLeft = true;
            if (lastIdx != i)
            {
                list.append(content.mid(lastIdx, i - lastIdx));
            }
            lastIdx = i;
        }
        else if (c == '}')
        {
            if (setLeft)
            {
                list.append(content.mid(lastIdx, i - lastIdx + 1));
            }
            lastIdx = i + 1;
            setLeft = false;
        }
        else if (i == content.size() - 1)
        {
            list.append(content.mid(lastIdx));
        }
    }
    return list;
}

void MTextContentCmdMgr::parseSubContent(const QString& subContent, std::vector<MTextContentCmd*>& cmds)
{
    int valueCmdStartIdx = -1;
    int size = subContent.size();
    int i = 0;
    while (i < size)
    {
        QChar c = subContent.at(i);
        // 命令开始
        if (c == '\\')
        {
            int count = i - valueCmdStartIdx;
            if (valueCmdStartIdx != -1 && i - 1 >= 0 && count > 0)
            {
                QString valueString = subContent.mid(valueCmdStartIdx, count);
                MTextContentCmd* cmd = generateCmd(valueString, MTextContentCmd::CmdType::Value);
                cmds.emplace_back(cmd);
            }
            if (i + 1 < size)
            {
                QChar nextC = subContent.at(i + 1);
                MTextContentCmd::CmdType type = getCmdTypeOfChar(nextC);
                if (type == MTextContentCmd::CmdType::UndefinedCmd)
                {
                    // 不能识别的命令，到分号或下一个命令结束
                    int semicolonIdx = subContent.indexOf(';', i + 1);
                    int nextSlashIdx = subContent.indexOf('\\', i + 1);
                    if (semicolonIdx > i && semicolonIdx != -1)
                    {
                        i = semicolonIdx;
                        continue;
                    }
                    if (nextSlashIdx > i && nextSlashIdx != -1)
                    {
                        i = nextSlashIdx;
                        continue;
                    }
                    i++;
                }
                else
                {
                    if (MTextContentCmd::CmdNeedSemicolon(type))
                    {
                        // 命令以分号结束
                        int semicolonIdx = subContent.indexOf(';', i + 1);
                        if (semicolonIdx == -1) // 没找到分号，意外结束
                        {
                            break;
                        }
                        else
                        {
                            int count = semicolonIdx - i + 1;
                            QString cmdString = subContent.mid(i, count);
                            MTextContentCmd* cmd = generateCmd(cmdString, type);
                            if (nullptr != cmd)
                            {
                                cmds.emplace_back(cmd);
                            }
                            i = semicolonIdx + 1;
                            valueCmdStartIdx = i;
                        }
                    }
                    else
                    {
                        // 命令不以分号结束，取单个字符作为命令
                        QString cmdString = subContent.mid(i, 2);
                        MTextContentCmd* cmd = generateCmd(cmdString, type);
                        if (nullptr != cmd)
                        {
                            cmds.emplace_back(cmd);
                        }
                        i += 2;
                        valueCmdStartIdx = i;
                    }
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            i++;
            if (valueCmdStartIdx == -1)
            {
                valueCmdStartIdx = 0;
            }
        }
    }

    // 最后一个值
    if (valueCmdStartIdx != -1)
    {
        QString valueString = subContent.mid(valueCmdStartIdx);
        if (valueString.isEmpty())
        {
            return;
        }
        MTextContentCmd* cmd = generateCmd(valueString, MTextContentCmd::CmdType::Value);
        cmds.emplace_back(cmd);
    }
}

void MTextContentCmdMgr::parseContentToCmds(const QString& content, std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList)
{
    QStringList list = splitContent(content);
    cmdsList.reserve(list.size());
    for (int i = 0; i < list.size(); i++)
    {
        QString item = list.at(i);
        bool inBrace = false;
        if (item.startsWith("{") && item.endsWith("}"))
        {
            // 带花括号的文字块创建
            item = item.mid(1, item.size() - 2);
            inBrace = true;
        }
        else
        {
            // 花括号外的文字可直接由文字样式创建
        }
        if (item.isEmpty())
        {
            continue;
        }
        std::vector<MTextContentCmd*> cmds;
        parseSubContent(item, cmds);
        cmdsList.emplace_back(std::make_pair(inBrace, cmds));
    }
}

void MTextContentCmdMgr::generateEntitiesByCmds(const std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList)
{
    auto style = m_pMText->getDataConstPtr()->getTextStyle();
    double height = 0.0;
    DmColor color;
    QString fontName;
    bool isBold = false;
    bool isItalic = false;
    double widthFactor = 1.0;
    double slashAngle = 0.0;
    bool hasUnderline = false;
    bool hasStrikethrough = false;
    bool hasOverline = false;
    DmMText::initInfoFromStyle(style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
    height = m_pMText->getDataConstPtr()->getCharHeight();

    // 初始化文字
    if (m_pMText->size() == 0)
    {
        m_pMText->init(height);
    }

    // 根据命令生成文字
    DmChar* nouse = nullptr;
    for (auto& cmds : cmdsList)
    {
        bool inBrace = cmds.first;
        if (!inBrace) // 不在花括号内的文字，他的字体在content中不会指定，所以重新从文字样式中获得设置
        {
            DmMText::initInfoFromStyle(style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            height = m_pMText->getDataConstPtr()->getCharHeight();
        }
        for (auto& cmd : cmds.second)
        {
            generateEntitiesByOneCmd(cmd, false, nouse, nouse, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        }
    }
    m_pMText->updateTextPosition();
}

DmChar* MTextContentCmdMgr::insertEntitiesByCmds(const DmChar* preChar, const DmChar* postChar, const std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList)
{
    auto style = m_pMText->getDataConstPtr()->getTextStyle();
    double height = 0.0;
    DmColor color;
    QString fontName;
    bool isBold = false;
    bool isItalic = false;
    double widthFactor = 1.0;
    double slashAngle = 0.0;
    bool hasUnderline = false;
    bool hasStrikethrough = false;
    bool hasOverline = false;
    DmMText::initInfoFromStyle(style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
    height = m_pMText->getDataConstPtr()->getCharHeight();

    // 初始化文字
    if (m_pMText->size() == 0)
    {
        m_pMText->init(height);
    }

    // 根据命令生成文字
    DmChar* preCharNew = const_cast<DmChar*>(preChar);
    DmChar* postCharNew = const_cast<DmChar*>(postChar);
    for (auto& cmds : cmdsList)
    {
        for (auto& cmd : cmds.second)
        {
            generateEntitiesByOneCmd(cmd, true, preCharNew, postCharNew, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        }
    }
    m_pMText->updateTextPosition();
    if (preChar != preCharNew)
    {
        return preCharNew;
    }
    return nullptr;
}

void MTextContentCmdMgr::generateEntitiesByOneCmd(const MTextContentCmd* cmd, bool insert, DmChar*& preChar, DmChar*& postChar, const DmTextStyle* style, DmColor& color, double& height, double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline)
{
    DmMTextParagraph* thePara = nullptr; // 所插入的段落
    if (insert)
    {
        if (preChar == nullptr && postChar == nullptr)
        {
            thePara = static_cast<DmMTextParagraph*>(m_pMText->last());
        }
        else if (preChar == nullptr)
        {
            thePara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        }
        else
        {
            thePara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
        }
    }
    else
    {
        thePara = static_cast<DmMTextParagraph*>(m_pMText->last());
    }
    auto cmdType = cmd->getCmdType();
    switch (cmdType)
    {
        case MTextContentCmd::CmdType::Value:
        {
            QString str = cmd->getStrValue();
            if (!insert)
            {
                addStringEntities(str, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
            else
            {
                insertStringEntities(str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
        }
            break;
        case MTextContentCmd::CmdType::UndefinedCmd:
            break;
        case MTextContentCmd::CmdType::FontShx:
        {
            bool nouse = false;
            int nousei = 0;
            QString name;
            cmd->getFontValue(name, nouse, nouse, nousei, nousei);
            if (nullptr != DMFONTLIST->requestFont(name, false))
            {
                fontName = name;
                isBold = false;
                isItalic = false;
            }
        }
            break;
        case MTextContentCmd::CmdType::FontSystem:
        {
            QString nameNoPost;
            bool bold = false;
            bool italic = false;
            int nousei = 0;
            cmd->getFontValue(nameNoPost, bold, italic, nousei, nousei);
            auto& trans = DMFONTLIST->FamilyToTranslateMap();
            auto itTrans = trans.find(nameNoPost);
            if (itTrans != trans.end())
            {
                nameNoPost = itTrans->second;
            }
            if (nullptr != DMFONTLIST->requestSysFontCloset(nameNoPost, bold, italic))
            {
                fontName = nameNoPost;
                isBold = bold;
                isItalic = italic;
            }
        }
            break;
        case MTextContentCmd::CmdType::ColorIndex:
        case MTextContentCmd::CmdType::ColorValue:
        {
            color = cmd->getColor();
        }
            break;
        case MTextContentCmd::CmdType::Height:
        {
            height = cmd->getDoubleValue();
        }
            break;
        case MTextContentCmd::CmdType::HeightFactor:
        {
            height *= cmd->getDoubleValue();
        }
            break;
        case MTextContentCmd::CmdType::OverlineOpen:
        {
            hasOverline = true;
        }
            break;
        case MTextContentCmd::CmdType::OverlineClose:
        {
            hasOverline = false;
        }
            break;
        case MTextContentCmd::CmdType::UnderlineOpen:
        {
            hasUnderline = true;
        }
            break;
        case MTextContentCmd::CmdType::UnderlineClose:
        {
            hasUnderline = false;
        }
            break;
        case MTextContentCmd::CmdType::StrikethroughOpen:
        {
            hasStrikethrough = true;
        }
            break;
        case MTextContentCmd::CmdType::StrikethroughClose:
        {
            hasStrikethrough = false;
        }
            break;
        case MTextContentCmd::CmdType::Oblique:
        {
            slashAngle = cmd->getDoubleValue();
        }
            break;
        case MTextContentCmd::CmdType::WidthFactor:
        {
            widthFactor = cmd->getDoubleValue();
        }
            break;
        case MTextContentCmd::CmdType::AddParagraph:
        {
            // 参考：MTextEditWidget::keyPressEvent_Enter()
            QString newLineStr("\n");
            DmChar* newLineChar = DmMText::createChar(newLineStr, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            auto afterPara = m_pMText->newParagraph(preChar, postChar, newLineChar);
            auto firstLine = afterPara->lineAt(0);
            preChar = nullptr;
            postChar = firstLine->charAt(0);
        }
            break;
        case MTextContentCmd::CmdType::AddSlash:
        {
            QString str("\\");
            if (!insert)
            {
                addStringEntities(str, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
            else
            {
                insertStringEntities(str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
        }
            break;
        case MTextContentCmd::CmdType::AddBraceOpen:
        {
            QString str("{");
            if (!insert)
            {
                addStringEntities(str, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
            else
            {
                insertStringEntities(str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
        }
            break;
        case MTextContentCmd::CmdType::AddBraceClose:
        {
            QString str("}");
            if (!insert)
            {
                addStringEntities(str, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
            else
            {
                insertStringEntities(str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            }
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignCenter:
        {
            thePara->setAlignent(DmMTextParagraph::Alignment::Mid);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignRight:
        {
            thePara->setAlignent(DmMTextParagraph::Alignment::Right);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignDistribute:
        {
            thePara->setAlignent(DmMTextParagraph::Alignment::Distribute);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignLeft:
        {
            thePara->setAlignent(DmMTextParagraph::Alignment::Left);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignDefault:
        {
            thePara->setAlignent(DmMTextParagraph::Alignment::Default);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignJustify:
        {
            thePara->setAlignent(DmMTextParagraph::Alignment::Justify);
        }
            break;
            // 其余暂不处理
        case MTextContentCmd::CmdType::NonBreakingSpace:
            break;
        case MTextContentCmd::CmdType::Stack:
            break;
        case MTextContentCmd::CmdType::CharSpacing:
            break;
        case MTextContentCmd::CmdType::Alignment:
            break;
        case MTextContentCmd::CmdType::LineSpace:
            break;
        case MTextContentCmd::CmdType::ItemNumber:
            break;
        default:
            break;
    }
}

void MTextContentCmdMgr::addStringEntities(const QString& str, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline)
{
    DmMTextParagraph* lastPara = static_cast<DmMTextParagraph*>(m_pMText->last());
    for (auto ch : str)
    {
        QString charStr(ch);
        DmChar* c = DmMText::createChar(charStr, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        if (nullptr == c)
        {
            continue;
        }
        DmMTextLine* lastLine = lastPara->last();
        lastLine->addChar(c);
        c->setParent(lastLine);
    }
}

void MTextContentCmdMgr::insertStringEntities(const QString& str, DmChar*& preChar, DmChar*& postChar, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline)
{
    // 插入文字（TODO: 用insertChars替代）
    for (auto ch : str)
    {
        QString charStr(ch);
        DmChar* c = DmMText::createChar(charStr, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        if (nullptr == c)
        {
            continue;
        }
        m_pMText->insertChar(preChar, postChar, c);
        preChar = c;
    }
}

bool MTextContentCmdMgr::isCharMatchTextStyle(const DmChar* c)
{
    if (c->getUnderline() != nullptr || c->getStrikethrough() != nullptr || c->getOverline() != nullptr)
    {
        return false;
    }
    DmMText* mtext = static_cast<DmMText*>(c->getParent()->getParent()->getParent());
    auto data = mtext->getDataConstPtr();
    DmTextStyle* style = data->getTextStyle();
    auto styleData = style->getDataConstPtr();
    double charHeight = data->getCharHeight();
    double widthFactor = styleData->widhFactor;
    double slashAngle = styleData->slashAngle;
    bool heightMatch = std::abs(charHeight - c->getNominalHeight()) < DISTANCE_TOLRANCE;
    if (!heightMatch)
    {
        return false;
    }
    bool widthFactorMatch = std::abs(widthFactor - c->getWidthFactor()) < DISTANCE_TOLRANCE;
    if (!widthFactorMatch)
    {
        return false;
    }
    bool slashAngleMatch = std::abs(slashAngle - c->getSlashAngle()) < DISTANCE_TOLRANCE;
    if (!slashAngleMatch)
    {
        return false;
    }
    DmPen pen = c->getPen(false);
    DmColor color = pen.getColor();
    if (!color.isByLayer())
    {
        return false;
    }

    DmCharTemplate* templ = c->getCharTemplate();
    auto owner = templ->getOwner();
    if (nullptr != templ)
    {
        DmFont* font = owner->getFont();
        if (font != nullptr)
        {
            if (font == styleData->pAsciiFont || font == styleData->pBigFont || font == styleData->pSysFont)
            {
                return true;
            }
        }
    }
    return false;
}

void MTextContentCmdMgr::parseMTextToCmds(std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList)
{
    // 从文字样式初始化信息
    auto style = m_pMText->getDataConstPtr()->getTextStyle();
    double height = 0.0;
    DmColor color;
    QString fontName;
    bool isBold = false;
    bool isItalic = false;
    double widthFactor = 1.0;
    double slashAngle = 0.0;
    bool hasUnderline = false;
    bool hasStrikethrough = false;
    bool hasOverline = false;
    DmMText::initInfoFromStyle(style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
    height = m_pMText->getDataConstPtr()->getCharHeight();

    // 由文字实体生成命令
    bool isFirstPara = true;
    std::vector<MTextContentCmd*> cmds;
    bool lastInBrace = false;
    for (auto& paraEnt : *m_pMText)
    {
        DmMTextParagraph* para = static_cast<DmMTextParagraph*>(paraEnt);
        DmMTextParagraph::Alignment curParaAlign = para->getAlignment();
        if (isFirstPara) // 第一个段落格式
        {
            if (curParaAlign != DmMTextParagraph::Alignment::Default)
            {
                MTextContentCmd* cmd = new MTextContentCmd(MTextContentCmd::ParaAlignTypeToCmdType(curParaAlign));
                cmds.emplace_back(cmd);
            }
        }
        for (auto& lineEnt : *para)
        {
            DmMTextLine* line = static_cast<DmMTextLine*>(lineEnt);
            for (auto& charEnt : *line)
            {
                // 生成转换到当前格式的命令
                DmChar* c = static_cast<DmChar*>(charEnt);
                // 匹配文字样式，在花括号外
                if (isCharMatchTextStyle(c))
                {
                    if (lastInBrace)
                    {
                        // 添加转换到文字样式状态的命令（除了字体）
                        generateCmdsOfFormatChangeToTextStyle(cmds, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
                        auto item = std::make_pair(lastInBrace, cmds);
                        cmdsList.emplace_back(item);
                        cmds.clear();
                    }
                    lastInBrace = false;
                }
                // 不匹配文字样式格式，在花括号内
                else
                {
                    generateCmdsOfCharFormat(c, false, cmds, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
                    lastInBrace = true;
                }
                // 非格式产生的命令
                generateNoFormatCmdsOfChar(c, cmds);
            }
        }

        if (isFirstPara)
        {
            isFirstPara = false;
        }
    }
    // 剩下的命令
    if (cmds.size() > 0)
    {
        auto item = std::make_pair(lastInBrace, cmds);
        cmdsList.emplace_back(item);
        cmds.clear();
    }

    // 将连续的值命令组合成一个
    for (auto kv : cmdsList)
    {
        combineContinuousValueCmds(kv.second);
    }
}

void MTextContentCmdMgr::parseMTextRangeToCmds(std::vector<std::vector<MTextContentCmd*>>& cmdsList, const DmChar* startChar, const DmChar* endChar)
{
    double height = 0.0;
    DmColor color;
    QString fontName;
    bool isBold = false;
    bool isItalic = false;
    double widthFactor = 1.0;
    double slashAngle = 0.0;
    bool hasUnderline = false;
    bool hasStrikethrough = false;
    bool hasOverline = false;
    DmMText::initInfoFromChar(startChar, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
    height = m_pMText->getDataConstPtr()->getCharHeight();

    // 由文字实体生成命令
    std::vector<MTextContentCmd*> cmds;
    // 仅选择了一个字符
    if (startChar == endChar)
    {
        generateCmdsOfCharFormat(startChar, true, cmds, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        generateNoFormatCmdsOfChar(startChar, cmds);
    }
    // 选择了多个字符
    else
    {
        generateCmdsOfCharFormat(startChar, true, cmds, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        generateNoFormatCmdsOfChar(startChar, cmds);
        DmChar* c = const_cast<DmChar*>(startChar);
        c = DmMText::getPostChar(c, true, true);
        while (c && c != endChar)
        {
            generateCmdsOfCharFormat(c, false, cmds, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            generateNoFormatCmdsOfChar(c, cmds);
            c = DmMText::getPostChar(c, true, true);
        }
        if (c && c == endChar)
        {
            generateCmdsOfCharFormat(c, false, cmds, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            generateNoFormatCmdsOfChar(c, cmds);
        }
    }

    // 保存命令
    if (cmds.size() > 0)
    {
        // 将连续的值命令组合成一个
        combineContinuousValueCmds(cmds);
        cmdsList.emplace_back(cmds);
        cmds.clear();
    }
}

void MTextContentCmdMgr::generateNoFormatCmdsOfChar(const DmChar* c, std::vector<MTextContentCmd*>& cmds)
{
    DmMTextParagraph* para = static_cast<DmMTextParagraph*>(c->getParent()->getParent());
    DmMTextParagraph::Alignment curParaAlign = para->getAlignment();
    // 特殊字符命令
    if (c->isNewLine())
    {
        MTextContentCmd* cmd = new MTextContentCmd(MTextContentCmd::CmdType::AddParagraph);
        cmds.emplace_back(cmd);
        DmMTextParagraph* nextPara = m_pMText->getPostParagraph(para);
        if (nullptr != nextPara)
        {
            DmMTextParagraph::Alignment nextParaAlign = nextPara->getAlignment();
            if (curParaAlign != nextParaAlign)
            {
                MTextContentCmd* alignCmd = new MTextContentCmd(MTextContentCmd::ParaAlignTypeToCmdType(nextParaAlign));
                cmds.emplace_back(alignCmd);
            }
        }
        return;
    }
    // 其他特殊字符
    MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::UndefinedCmd;
    QString cStr = c->getName();
    if (cStr == "\\")
    {
        cmdType = MTextContentCmd::CmdType::AddSlash;
    }
    if (cStr == "{")
    {
        cmdType = MTextContentCmd::CmdType::AddBraceOpen;
    }
    if (cStr == "}")
    {
        cmdType = MTextContentCmd::CmdType::AddBraceClose;
    }
    if (cmdType != MTextContentCmd::CmdType::UndefinedCmd)
    {
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmds.emplace_back(cmd);
    }
    // 保存值
    else
    {
        MTextContentCmd* cmd = new MTextContentCmd(MTextContentCmd::CmdType::Value);
        cmd->setStrValue(cStr);
        cmds.emplace_back(cmd);
    }
}

QString MTextContentCmdMgr::parseCmdsToContent(const std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList)
{
    QString content;
    for (auto item : cmdsList)
    {
        bool inBrace = item.first;
        QString cmdsStr;
        for (auto cmd : item.second)
        {
            QString cmdStr = cmd->computeCmdString();
            cmdsStr.append(cmdStr);
        }
        if (inBrace)
        {
            cmdsStr.insert(0, "{");
            cmdsStr.append("}");
        }
        content.append(cmdsStr);
    }
    return content;
}

void MTextContentCmdMgr::generateCmdsOfCharFormat(const DmChar* c, const bool& init, std::vector<MTextContentCmd*>& cmds, DmColor& color, double& height, double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline)
{
    // 字体变化
    DmCharTemplate* templ = c->getCharTemplate();
    auto owner = templ->getOwner();
    DmFont* f = owner->getFont();
    if (f != nullptr)
    {
        bool cBold, cItalic;
        QString cFontName = DMFONTLIST->getFontFamilyName(f, cBold, cItalic);
        if (cFontName != fontName || init)
        {
            MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::FontShx;
            if (f->getFontType() == FontType::System)
            {
                cmdType = MTextContentCmd::CmdType::FontSystem;
            }
            MTextContentCmd* cmd = new MTextContentCmd(cmdType);
            cmd->setFontValue(cFontName, cBold, cItalic, 0, 0);
            cmds.emplace_back(cmd);
            fontName = cFontName;
            isBold = cBold;
            isItalic = cItalic;
        }
    }

    // 其他
    double cHeight = c->getNominalHeight();
    double cSlashAngle = c->getSlashAngle();
    double cWidthFactor = c->getWidthFactor();
    bool cHasUnderline = c->getUnderline() != nullptr;
    bool cHasStrikethrough = c->getStrikethrough() != nullptr;
    bool cHasOverline = c->getOverline() != nullptr;
    DmColor cColor = c->getPen(false).getColor();
    generateCmdsOfDataChange(init, cmds, cColor, cHeight, cWidthFactor, cSlashAngle, cHasUnderline, cHasStrikethrough, cHasOverline,
                             color, height, widthFactor, slashAngle, hasUnderline, hasStrikethrough, hasOverline);
}

void MTextContentCmdMgr::generateCmdsOfDataChange(const bool& init, std::vector<MTextContentCmd*>& cmds, const DmColor& newColor, const double& newHeight, const double& newWidthFactor, const double& newSlashAngle, const bool& newHasUnderline, const bool& newHasStrikethrough, const bool& newHasOverline, DmColor& color, double& height, double& widthFactor, double& slashAngle, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline)
{
    // 高度
    double hScale = newHeight / height;
    if (std::abs(hScale - 1.0) > SCALE_TOLRANCE || init)
    {
        MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::HeightFactor;
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmd->setDoubleValue(hScale);
        cmds.emplace_back(cmd);
        height = newHeight;
    }
    // 倾斜角度
    if (std::abs(newSlashAngle - slashAngle) > ANGLE_TOLRANCE || init)
    {
        MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::Oblique;
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmd->setDoubleValue(newSlashAngle);
        cmds.emplace_back(cmd);
        slashAngle = newSlashAngle;
    }
    // 宽度系数
    double wfScale = newWidthFactor / widthFactor;
    if (std::abs(wfScale - 1.0) > SCALE_TOLRANCE || init)
    {
        MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::WidthFactor;
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmd->setDoubleValue(wfScale);
        cmds.emplace_back(cmd);
        widthFactor = newWidthFactor;
    }
    // 下划线
    if (newHasUnderline != hasUnderline || init)
    {
        MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::UnderlineOpen;
        if (!newHasUnderline)
        {
            cmdType = MTextContentCmd::CmdType::UnderlineClose;
        }
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmds.emplace_back(cmd);
        hasUnderline = newHasUnderline;
    }
    // 删除线
    if (newHasStrikethrough != hasStrikethrough || init)
    {
        MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::StrikethroughOpen;
        if (!newHasStrikethrough)
        {
            cmdType = MTextContentCmd::CmdType::StrikethroughClose;
        }
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmds.emplace_back(cmd);
        hasStrikethrough = newHasStrikethrough;
    }
    // 上划线
    if (newHasOverline != hasOverline || init)
    {
        MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::OverlineOpen;
        if (!newHasOverline)
        {
            cmdType = MTextContentCmd::CmdType::OverlineClose;
        }
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmds.emplace_back(cmd);
        hasOverline = newHasOverline;
    }
    // 颜色
    // TODO : 能否判断是否为索引色
    if (newColor != color || init)
    {
        MTextContentCmd::CmdType cmdType = MTextContentCmd::CmdType::ColorValue;
        MTextContentCmd* cmd = new MTextContentCmd(cmdType);
        cmd->setColor(newColor);
        cmds.emplace_back(cmd);
        color = newColor;
    }
}

void MTextContentCmdMgr::generateCmdsOfFormatChangeToTextStyle(std::vector<MTextContentCmd*>& cmds, DmColor& color, double& height, double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline)
{
    // 文字样式的信息
    auto style = m_pMText->getDataConstPtr()->getTextStyle();
    double sHeight = 0.0;
    DmColor sColor;
    QString sFontName;
    bool sIsBold = false;
    bool sIsItalic = false;
    double sWidthFactor = 1.0;
    double sSlashAngle = 0.0;
    bool sHasUnderline = false;
    bool sHasStrikethrough = false;
    bool sHasOverline = false;
    DmMText::initInfoFromStyle(style, sColor, sHeight, sWidthFactor, sSlashAngle, sFontName, sIsBold, sIsItalic, sHasUnderline, sHasStrikethrough, sHasOverline);
    sHeight = m_pMText->getDataConstPtr()->getCharHeight();

    fontName = sFontName;
    isBold = sIsBold;
    isItalic = sIsItalic;
    generateCmdsOfDataChange(false, cmds, sColor, sHeight, sWidthFactor, sSlashAngle, sHasUnderline, sHasStrikethrough, sHasOverline,
                             color, height, widthFactor, slashAngle, hasUnderline, hasStrikethrough, hasOverline);
}

void MTextContentCmdMgr::combineContinuousValueCmds(std::vector<MTextContentCmd*>& cmds)
{
    // 获得包含连续的值命令的索引
    std::vector<std::pair<int, int>> continuousIdx;
    int lastIdx = -1;
    for (int i = 0; i < (int)cmds.size(); i++)
    {
        auto cmd = cmds.at(i);
        if (cmd->getCmdType() == MTextContentCmd::CmdType::Value)
        {
            if (lastIdx == -1)
            {
                lastIdx = i;
            }
        }
        else
        {
            if (lastIdx != -1)
            {
                if (i - lastIdx > 1)
                {
                    continuousIdx.emplace_back(std::make_pair(lastIdx, i));
                }
                lastIdx = -1;
            }
        }
    }

    // 从后往前合并
    for (auto rit = continuousIdx.rbegin(); rit != continuousIdx.rend(); ++rit)
    {
        int startIdx = rit->first;
        int endIdx = rit->second;
        QString strVal;
        for (int i = startIdx; i < endIdx; i++)
        {
            auto cmd = cmds.at(i);
            strVal.append(cmd->getStrValue());
        }
        cmds.erase(cmds.begin() + startIdx, cmds.begin() + endIdx);
        MTextContentCmd* newCmd = new MTextContentCmd(MTextContentCmd::CmdType::Value);
        newCmd->setStrValue(strVal);
        cmds.insert(cmds.begin() + startIdx, newCmd);
    }
}
