/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file MTextEditCmd.cpp
/// @brief 多行文字编辑器命令实现

#include "MTextEditCmd.h"
#include "DmMText.h"
#include "DmMTextParagraph.h"
#include "DmMTextLine.h"
#include "DmChar.h"
#include "MTextEditWidget.h"
#include "DmMTextContentCmd.h"
#include "DmFontList.h"

/// @brief 执行添加字符
void MTextEdit_AddCharsCmd::execute()
{
    m_pPara->insertChars(m_iAddIdx, m_addedChars);
    ICmd::execute();
}

/// @brief 撤销添加字符
void MTextEdit_AddCharsCmd::undo()
{
    std::vector<DmChar*> nouse;
    m_pPara->removeChars(m_iAddIdx, static_cast<int>(m_addedChars.size()), nouse);
    ICmd::undo();
}

/// @brief 重做添加字符
void MTextEdit_AddCharsCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_AddCharsCmd::clear()
{
    if (!m_isExecuted)
    {
        for (auto e : m_addedChars)
        {
            delete e;
        }
        m_addedChars.clear();
    }
}

/// @brief 执行删除字符
void MTextEdit_RemoveCharsCmd::execute()
{
    m_pPara->removeChars(m_iRemoveIdx, m_iRemoveCount, m_removedChars);
    ICmd::execute();
}

/// @brief 撤销删除字符
void MTextEdit_RemoveCharsCmd::undo()
{
    m_pPara->insertChars(m_iRemoveIdx, m_removedChars);
    m_removedChars.clear();
    ICmd::undo();
}

/// @brief 重做删除字符
void MTextEdit_RemoveCharsCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_RemoveCharsCmd::clear()
{
    if (m_isExecuted)
    {
        for (auto e : m_removedChars)
        {
            delete e;
        }
        m_removedChars.clear();
    }
}

/// @brief 执行删除段落和字符
void MTextEdit_RemoveParasAndCharsCmd::execute()
{
    int paraCount = m_pMText->size();
    DmChar* firstChar = m_pMText->getCharByIndex(m_iRemoveIdx);
    DmChar* lastChar = m_pMText->getCharByIndex(m_iRemoveIdx + m_iRemoveCount - 1);
    DmMTextParagraph* firstPara = static_cast<DmMTextParagraph*>(firstChar->getParent()->getParent());
    DmMTextParagraph* lastPara = static_cast<DmMTextParagraph*>(lastChar->getParent()->getParent());
    int firstParaIdx = m_pMText->findPara(firstPara);
    int lastParaIdx = m_pMText->findPara(lastPara);

    //同一段落
    if (lastParaIdx - firstParaIdx == 0)
    {
        //删除换行
        if (lastChar->isNewLine())
        {
            int paraCharCount = firstPara->getCharsCount(true);
            //删除该段落
            if (m_iRemoveCount == paraCharCount)
            {
                MTextEdit_RemoveParaCmd* cmd = new MTextEdit_RemoveParaCmd(m_pMText, firstParaIdx);
                cmd->execute();
                m_cmds.emplace_back(cmd);
            }
            //删除部分文字，包括换行
            else
            {
                //把下一段落文字提出放入这个段落，删除下一个段落
                //删除本段落后面的文字，文字个数为m_iRemoveCount-1
                MTextEdit_RemoveCharsCmd* removeCharsCmds = new MTextEdit_RemoveCharsCmd(m_pMText, firstPara, m_iRemoveIdx, m_iRemoveCount);
                removeCharsCmds->execute();
                m_cmds.emplace_back(removeCharsCmds);
                //删除下一段落
                DmMTextParagraph* nextPara = m_pMText->paraAt(lastParaIdx + 1);
                int nextParaCharCount = nextPara->getCharsCount(true);
                std::vector<DmChar*> chs;
                nextPara->getCharsByRange(0, nextParaCharCount, chs);
                MTextEdit_AddCharsCmd* addCharsCmd = new MTextEdit_AddCharsCmd(m_pMText, firstPara, chs, paraCharCount - 1);
                addCharsCmd->execute();
                m_cmds.emplace_back(addCharsCmd);
                MTextEdit_RemoveParaCmd* cmd = new MTextEdit_RemoveParaCmd(m_pMText, lastParaIdx + 1);
                cmd->execute();
                m_cmds.emplace_back(cmd);
            }
        }
        //仅删除段落内文字（不含换行），同MTextEdit_RemoveCharsCmd
        else
        {
            MTextEdit_RemoveCharsCmd* cmd = new MTextEdit_RemoveCharsCmd(m_pMText, firstPara, m_iRemoveIdx, m_iRemoveCount);
            cmd->execute();
            m_cmds.emplace_back(cmd);
        }
    }
    //跨越段落
    else
    {
        DmChar* paraFirstChar = firstPara->getChar(0);
        //从段落开头开始
        if (firstChar == paraFirstChar)
        {
            //从起始到结束全部段落删除
            if (lastChar->isNewLine())
            {
                int count = lastParaIdx - firstParaIdx + 1;
                MTextEdit_RemoveParasCmd* cmd = new MTextEdit_RemoveParasCmd(m_pMText, firstParaIdx, count);
                cmd->execute();
                m_cmds.emplace_back(cmd);
            }
            //最后段落剩下字符
            else
            {
                int count = lastParaIdx - firstParaIdx;
                MTextEdit_RemoveParasCmd* cmd = new MTextEdit_RemoveParasCmd(m_pMText, firstParaIdx, count);
                cmd->execute();
                m_cmds.emplace_back(cmd);
                int idx = lastPara->indexOf(lastChar);
                MTextEdit_RemoveCharsCmd* removeCharsCmd = new MTextEdit_RemoveCharsCmd(m_pMText, lastPara, 0, idx + 1);
                removeCharsCmd->execute();
                m_cmds.emplace_back(removeCharsCmd);
            }
        }
        //从段落内开始（起始段落还剩下字符）
        else
        {
            //起始段落还剩下字符，结束段落全部删除
            if (lastChar->isNewLine())
            {
                int idx = firstPara->indexOf(firstChar);
                int totalCount = firstPara->getCharsCount(true);
                int count = totalCount - idx - 1;
                MTextEdit_RemoveCharsCmd* removeCharsCmd = new MTextEdit_RemoveCharsCmd(m_pMText, firstPara, idx, count);
                removeCharsCmd->execute();
                m_cmds.emplace_back(removeCharsCmd);

                int removeCount = lastParaIdx - firstParaIdx;
                MTextEdit_RemoveParasCmd* cmd = new MTextEdit_RemoveParasCmd(m_pMText, firstParaIdx + 1, removeCount);
                cmd->execute();
                m_cmds.emplace_back(cmd);
            }
            //起始段落还剩下字符，结束段落剩下字符
            else
            {
                //删除起始段落部分文字（包括换行）
                int idx = firstPara->indexOf(firstChar);
                int totalCount = firstPara->getCharsCount(true);
                int count = totalCount - idx;
                MTextEdit_RemoveCharsCmd* removeCharsCmd = new MTextEdit_RemoveCharsCmd(m_pMText, firstPara, idx, count);
                removeCharsCmd->execute();
                m_cmds.emplace_back(removeCharsCmd);

                // 删除中间段落
                int removeCount = lastParaIdx - firstParaIdx - 1;
                if (removeCount > 0)
                {
                    MTextEdit_RemoveParasCmd* cmd = new MTextEdit_RemoveParasCmd(m_pMText, firstParaIdx + 1, removeCount);
                    cmd->execute();
                    m_cmds.emplace_back(cmd);
                }

                // 提取结束段落剩下的文字（可能包括换行）
                int lastIdx = lastPara->indexOf(lastChar);
                int lastTotalCount = lastPara->getCharsCount(true);
                std::vector<DmChar*> chs;
                lastPara->getCharsByRange(lastIdx + 1, lastTotalCount - lastIdx - 1, chs);
                //将这些字符添加到起始段落
                int addIdx = firstPara->getCharsCount(true);
                MTextEdit_AddCharsCmd* addCharsCmd = new MTextEdit_AddCharsCmd(m_pMText, firstPara, chs, addIdx);
                addCharsCmd->execute();
                m_cmds.emplace_back(addCharsCmd);

                //删除结束段落
                MTextEdit_RemoveParasCmd* removeParaCmd = new MTextEdit_RemoveParasCmd(m_pMText, firstParaIdx + 1, 1);
                removeParaCmd->execute();
                m_cmds.emplace_back(removeParaCmd);
            }
        }
    }

    ICmd::execute();
}

/// @brief 撤销删除段落和字符
void MTextEdit_RemoveParasAndCharsCmd::undo()
{
    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->undo();
    }
    ICmd::undo();
}

/// @brief 重做删除段落和字符
void MTextEdit_RemoveParasAndCharsCmd::redo()
{
    for (auto cmd : m_cmds)
    {
        cmd->redo();
    }
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_RemoveParasAndCharsCmd::clear()
{
    if (m_isExecuted)
    {
        for (auto it = m_cmds.end(); it != m_cmds.begin();)
        {
            --it; // 先递减迭代器
            (*it)->clear();
            delete *it;
        }
    }
    else
    {
        for (auto cmd : m_cmds)
        {
            cmd->clear();
        }
    }
    m_cmds.clear();
}

/// @brief 执行添加段落
void MTextEdit_AddParaCmd::execute()
{
    m_pMText->insertPara(m_iAddIdx, m_pPara);
    ICmd::execute();
}

/// @brief 撤销添加段落
void MTextEdit_AddParaCmd::undo()
{
    DmMTextParagraph* para;
    m_pMText->removePara(m_iAddIdx, para);
    ICmd::undo();
}

/// @brief 重做添加段落
void MTextEdit_AddParaCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_AddParaCmd::clear()
{
    if (!m_isExecuted)
    {
        if (m_pPara)
        {
            delete m_pPara;
            m_pPara = nullptr;
        }
    }
}

/// @brief 执行删除段落
void MTextEdit_RemoveParaCmd::execute()
{
    m_pMText->removePara(m_iRemoveIdx, m_pPara);
    ICmd::execute();
}

/// @brief 撤销删除段落
void MTextEdit_RemoveParaCmd::undo()
{
    m_pMText->insertPara(m_iRemoveIdx, m_pPara);
    ICmd::undo();
}

/// @brief 重做删除段落
void MTextEdit_RemoveParaCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_RemoveParaCmd::clear()
{
    if (m_isExecuted)
    {
        if (m_pPara)
        {
            delete m_pPara;
            m_pPara = nullptr;
        }
    }
}

/// @brief 执行删除多个段落
void MTextEdit_RemoveParasCmd::execute()
{
    m_pParas.clear();
    m_pMText->removeParas(m_iRemoveIdx, m_iRemoveCount, m_pParas);
    ICmd::execute();
}

/// @brief 撤销删除多个段落
void MTextEdit_RemoveParasCmd::undo()
{
    m_pMText->insertParas(m_iRemoveIdx, m_pParas);
    ICmd::undo();
}

/// @brief 重做删除多个段落
void MTextEdit_RemoveParasCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_RemoveParasCmd::clear()
{
    if (m_isExecuted)
    {
        for (auto para : m_pParas)
        {
            delete para;
        }
        m_pParas.clear();
    }
}

/// @brief 执行添加换行符
void MTextEdit_AppendNewLineCharCmd::execute()
{
    m_pPara->appendLineFeed(m_newLineChar);
    ICmd::execute();
}

/// @brief 撤销添加换行符
void MTextEdit_AppendNewLineCharCmd::undo()
{
    DmChar* nouse;
    m_pPara->removeLineFeed(nouse);
    ICmd::undo();
}

/// @brief 重做添加换行符
void MTextEdit_AppendNewLineCharCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_AppendNewLineCharCmd::clear()
{
    if (!m_isExecuted)
    {
        if (m_newLineChar)
        {
            delete m_newLineChar;
            m_newLineChar = nullptr;
        }
    }
}

/// @brief 执行删除换行符
void MTextEdit_RemoveNewLineCharCmd::execute()
{
    m_pPara->removeLineFeed(m_newLineChar);
    ICmd::execute();
}

/// @brief 撤销删除换行符
void MTextEdit_RemoveNewLineCharCmd::undo()
{
    m_pPara->appendLineFeed(m_newLineChar);
    ICmd::undo();
}

/// @brief 重做删除换行符
void MTextEdit_RemoveNewLineCharCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_RemoveNewLineCharCmd::clear()
{
    if (m_isExecuted)
    {
        if (m_newLineChar)
        {
            delete m_newLineChar;
            m_newLineChar = nullptr;
        }
    }
}

/// @brief 构造清空选中区域命令
MTextEdit_SetSelectBeginEndToNull_Cmd::MTextEdit_SetSelectBeginEndToNull_Cmd(DmMText *mtext, MTextEditWidget* editWidget)
    : m_pMText(mtext)
    , m_editWidget(editWidget)
{
    m_originSelectBeginPreChar = editWidget->m_selectBeginPreChar;
    m_originSelectBeginPostChar = editWidget->m_selectBeginPostChar;
    m_originSelectEndPreChar = editWidget->m_selectEndPreChar;
    m_originSelectEndPostChar = editWidget->m_selectEndPostChar;
}

/// @brief 执行清空选中区域
void MTextEdit_SetSelectBeginEndToNull_Cmd::execute()
{
    //重置选择的实体
    m_editWidget->m_selectBeginPreChar = nullptr;
    m_editWidget->m_selectBeginPostChar = nullptr;
    m_editWidget->m_selectEndPreChar = nullptr;
    m_editWidget->m_selectEndPostChar = nullptr;
    ICmd::execute();
}

/// @brief 撤销清空选中区域
void MTextEdit_SetSelectBeginEndToNull_Cmd::undo()
{
    m_editWidget->m_selectBeginPreChar = m_originSelectBeginPreChar;
    m_editWidget->m_selectBeginPostChar = m_originSelectBeginPostChar;
    m_editWidget->m_selectEndPreChar = m_originSelectEndPreChar;
    m_editWidget->m_selectEndPostChar = m_originSelectEndPostChar;
    ICmd::undo();
}

/// @brief 重做清空选中区域
void MTextEdit_SetSelectBeginEndToNull_Cmd::redo()
{
    //重置选择的实体
    m_editWidget->m_selectBeginPreChar = nullptr;
    m_editWidget->m_selectBeginPostChar = nullptr;
    m_editWidget->m_selectEndPreChar = nullptr;
    m_editWidget->m_selectEndPostChar = nullptr;
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_SetSelectBeginEndToNull_Cmd::clear()
{
}

/// @brief 执行修改段落对齐
void MTextEdit_SetParaAlignmentCmd::execute()
{
    m_pPara->setAlignent(m_newAlignment);
    ICmd::execute();
}

/// @brief 撤销修改段落对齐
void MTextEdit_SetParaAlignmentCmd::undo()
{
    m_pPara->setAlignent(m_originAlignment);
    ICmd::undo();
}

/// @brief 重做修改段落对齐
void MTextEdit_SetParaAlignmentCmd::redo()
{
    m_pPara->setAlignent(m_newAlignment);
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_SetParaAlignmentCmd::clear()
{
}

/// @brief 执行插入换行符
void MTextEdit_InsertLineFeedCmd::execute()
{
    QString newLineStr("\n");
    DmChar* newLineChar = m_editWidget->createChar(newLineStr, m_preChar, m_postChar);

    DmMTextParagraph* newPara = nullptr;
    DmMTextParagraph* lastPara = nullptr;
    bool newParaAfter = true;   // 是否在当前段落后面追加段落
    DmChar* preChar = m_preChar;
    DmChar* postChar = m_postChar;
    bool isLastParaEnd = false; //是否最后一个段落末尾
    if (postChar == nullptr)
    {
        if (preChar == nullptr)
        {
            isLastParaEnd = true;
        }
        else
        {
            auto line = static_cast<DmMTextLine*>(preChar->getParent());
            auto para = static_cast<DmMTextParagraph*>(line->getParent());
            int lineIdx = para->findLine(line);
            if (lineIdx == para->size() - 1)
            {
                isLastParaEnd = true;
            }
            else
            {
                isLastParaEnd = false;
            }
        }
    }
    else
    {
        isLastParaEnd = false;
    }
    //在最后段落的末尾添加新行
    if (isLastParaEnd)
    {
        //在最后段落添加换行符
        lastPara = m_pMText->paraAt(m_pMText->size() - 1);
        MTextEdit_AppendNewLineCharCmd* appendNECmd = new MTextEdit_AppendNewLineCharCmd(m_pMText, lastPara, newLineChar);
        appendNECmd->execute();
        m_cmds.emplace_back(appendNECmd);

        //后面追加一个空段落
        newPara = new DmMTextParagraph(m_pMText);
        newPara->setAlignent(lastPara->getAlignment());
        newPara->lineAt(0)->setHeight(newLineChar->getNominalHeight());
        MTextEdit_AddParaCmd* addParaCmd = new MTextEdit_AddParaCmd(m_pMText, newPara, m_pMText->size());
        addParaCmd->execute();
        m_cmds.emplace_back(addParaCmd);
    }
    // 在非最后段落结尾处添加新行
    else if (postChar != nullptr && postChar->isNewLine())
    {
        //在该段落后面插入新段落
        lastPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
        int lastParaIdx = m_pMText->findPara(lastPara);
        double lastHeight = postChar->getNominalHeight();
        newPara = new DmMTextParagraph(m_pMText);
        newPara->setAlignent(lastPara->getAlignment());
        newPara->appendLineFeed(newLineChar);
        MTextEdit_AddParaCmd* addParaCmd = new MTextEdit_AddParaCmd(m_pMText, newPara, lastParaIdx + 1);
        addParaCmd->execute();
        m_cmds.emplace_back(addParaCmd);
    }
    // 在段落中间处添加新行
    else
    {
        //在段落开头处换行
        if (preChar == nullptr)
        {
            //在该段落前面插入空段落
            lastPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
            int lastParaIdx = m_pMText->findPara(lastPara);
            //double lastHeight = postChar->getNominalHeight();
            newPara = new DmMTextParagraph(m_pMText);
            newPara->setAlignent(lastPara->getAlignment());
            newPara->appendLineFeed(newLineChar);
            MTextEdit_AddParaCmd* addParaCmd = new MTextEdit_AddParaCmd(m_pMText, newPara, lastParaIdx);
            addParaCmd->execute();
            m_cmds.emplace_back(addParaCmd);
            newParaAfter = false;
        }
        //在段落中间处换行
        else
        {
            // 分割段落
            lastPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
            int lastParaIdx = m_pMText->findPara(lastPara);
            //double lastHeight = preChar->getNominalHeight();

            //把当前段落光标前的字符删除，将这些字符放入新段落
            newPara = new DmMTextParagraph(m_pMText);
            newPara->setAlignent(lastPara->getAlignment());
            DmMTextLine* newLine = newPara->lineAt(0);
            std::vector<DmChar*> removedChs;
            int count = lastPara->indexOf(preChar) + 1;
            lastPara->getCharsByRange(0, count, removedChs);
            MTextEdit_RemoveCharsCmd* removeCharsCmd = new MTextEdit_RemoveCharsCmd(m_pMText, lastPara, 0, count);
            removeCharsCmd->execute();
            m_cmds.emplace_back(removeCharsCmd);

            for (auto c : removedChs)
            {
                c->setParent(newLine);
                newLine->addChar(c);
            }
            newPara->appendLineFeed(newLineChar);

            //在该段落前插入新段落
            MTextEdit_AddParaCmd* addParaCmd = new MTextEdit_AddParaCmd(m_pMText, newPara, lastParaIdx);
            addParaCmd->execute();
            m_cmds.emplace_back(addParaCmd);
            newParaAfter = false;
        }
    }

    m_afterPara = newParaAfter ? newPara : lastPara; //形成的后面那个段落

    ICmd::execute();
}

/// @brief 撤销插入换行符
void MTextEdit_InsertLineFeedCmd::undo()
{
    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->undo();
    }
    ICmd::undo();
}

/// @brief 重做插入换行符
void MTextEdit_InsertLineFeedCmd::redo()
{
    for (auto cmd : m_cmds)
    {
        cmd->redo();
    }
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_InsertLineFeedCmd::clear()
{
    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->clear();
        delete *it;
    }
    m_cmds.clear();
}

/// @brief 执行粘贴内容
void MTextEdit_ContentPasteCmd::execute()
{
    //参考MTextContentCmdMgr::insertContentAtPos
    MTextContentCmdMgr mgr(m_pMText);
    //解析出命令
    std::vector<std::pair<bool, std::vector<MTextContentCmd*>>> cmdsList;
    mgr.parseContentToCmds(m_strPasteContent, cmdsList);

    //由命令生成文字
    insertEntitiesByCmds(m_originPreChar, m_originPostChar, cmdsList);

    // 释放命令列表
    for (auto item : cmdsList)
    {
        for (auto cmd : item.second)
        {
            delete cmd;
        }
    }

    ICmd::execute();
}

/// @brief 撤销粘贴内容
void MTextEdit_ContentPasteCmd::undo()
{
    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->undo();
    }
    ICmd::undo();
}

/// @brief 重做粘贴内容
void MTextEdit_ContentPasteCmd::redo()
{
    for (auto cmd : m_cmds)
    {
        cmd->redo();
    }
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_ContentPasteCmd::clear()
{
    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->clear();
        delete *it;
    }
    m_cmds.clear();
}

/// @brief 根据命令列表插入实体
void MTextEdit_ContentPasteCmd::insertEntitiesByCmds(const DmChar *preChar, const DmChar *postChar,
                                                     const std::vector<std::pair<bool, std::vector<MTextContentCmd *>>> &cmdsList)
{
    //参考MTextContentCmdMgr::insertEntitiesByCmds
    auto style = m_pMText->getDataConstPtr()->getTextStyle();
    double height = 0.0;
    DmColor color;
    QString fontName;
    bool isBold = false;
    bool isItalic = false;
    double widthFactor = 0.0;
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

    //根据命令生成文字
    DmChar* preCharNew = const_cast<DmChar*>(preChar);
    DmChar* postCharNew = const_cast<DmChar*>(postChar);
    for (auto& cmds : cmdsList)
    {
        for (auto& cmd : cmds.second)
        {
            generateEntitiesByOneCmd(cmd, preCharNew, postCharNew, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        }
    }

    m_newPreChar = preCharNew;
    m_newPostChar = postCharNew;
}

/// @brief 处理单个命令（生成实体或设置当前信息）
void
MTextEdit_ContentPasteCmd::generateEntitiesByOneCmd(const MTextContentCmd *cmd, DmChar *&preChar, DmChar *&postChar,
                                                    const DmTextStyle *style, DmColor &color, double &height,
                                                    double &widthFactor, double &slashAngle, QString &fontName,
                                                    bool &isBold, bool &isItalic, bool &hasUnderline,
                                                    bool &hasStrikethrough, bool &hasOverline)
{
    //参考MTextContentCmdMgr::generateEntitiesByOneCmd
    DmMTextParagraph* thePara = nullptr;	//所插入的段落
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

    auto cmdType = cmd->getCmdType();
    switch (cmdType)
    {
        case MTextContentCmd::CmdType::Value:
        {
            QString str = cmd->getStrValue();
            insertStringEntities(thePara, str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        }
            break;
        case MTextContentCmd::CmdType::UndefinedCmd:
            break;
        case MTextContentCmd::CmdType::FontShx:
        {
            bool nouse;
            int nousei;
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
            bool bold;
            bool italic;
            int nousei;
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
            //参考：MTextEditWidget::keyPressEvent_Enter()
            MTextEdit_InsertLineFeedCmd* insertCmd = new MTextEdit_InsertLineFeedCmd(m_pMText, preChar, postChar, m_editWidget);
            insertCmd->execute();
            m_cmds.emplace_back(insertCmd);
            DmMTextParagraph* afterPara = insertCmd->getAfterPara();
            auto firstLine = afterPara->lineAt(0);
            preChar = nullptr;
            postChar = firstLine->charAt(0);
        }
            break;
        case MTextContentCmd::CmdType::AddSlash:
        {
            QString str("\\");
            insertStringEntities(thePara, str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        }
            break;
        case MTextContentCmd::CmdType::AddBraceOpen:
        {
            QString str("{");
            insertStringEntities(thePara, str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        }
            break;
        case MTextContentCmd::CmdType::AddBraceClose:
        {
            QString str("}");
            insertStringEntities(thePara, str, preChar, postChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignCenter:
        {
            MTextEdit_SetParaAlignmentCmd* alignCmd = new MTextEdit_SetParaAlignmentCmd(m_pMText, thePara, DmMTextParagraph::Alignment::Mid);
            alignCmd->execute();
            m_cmds.emplace_back(alignCmd);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignRight:
        {
            MTextEdit_SetParaAlignmentCmd* alignCmd = new MTextEdit_SetParaAlignmentCmd(m_pMText, thePara, DmMTextParagraph::Alignment::Right);
            alignCmd->execute();
            m_cmds.emplace_back(alignCmd);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignDistribute:
        {
            MTextEdit_SetParaAlignmentCmd* alignCmd = new MTextEdit_SetParaAlignmentCmd(m_pMText, thePara, DmMTextParagraph::Alignment::Distribute);
            alignCmd->execute();
            m_cmds.emplace_back(alignCmd);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignLeft:
        {
            MTextEdit_SetParaAlignmentCmd* alignCmd = new MTextEdit_SetParaAlignmentCmd(m_pMText, thePara, DmMTextParagraph::Alignment::Left);
            alignCmd->execute();
            m_cmds.emplace_back(alignCmd);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignDefault:
        {
            MTextEdit_SetParaAlignmentCmd* alignCmd = new MTextEdit_SetParaAlignmentCmd(m_pMText, thePara, DmMTextParagraph::Alignment::Default);
            alignCmd->execute();
            m_cmds.emplace_back(alignCmd);
        }
            break;
        case MTextContentCmd::CmdType::ParagraphAlignJustify:
        {
            MTextEdit_SetParaAlignmentCmd* alignCmd = new MTextEdit_SetParaAlignmentCmd(m_pMText, thePara, DmMTextParagraph::Alignment::Justify);
            alignCmd->execute();
            m_cmds.emplace_back(alignCmd);
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

/// @brief 插入字符串实体到段落
void MTextEdit_ContentPasteCmd::insertStringEntities(DmMTextParagraph* para, const QString &str, DmChar *&preChar, DmChar *&postChar,
                                                     const DmTextStyle *style, const DmColor &color,
                                                     const double &height, const double &widthFactor,
                                                     const double &slashAngle, const QString &fontName,
                                                     const bool &isBold, const bool &isItalic, const bool &hasUnderline,
                                                     const bool &hasStrikethrough, const bool &hasOverline)
{
    //创建文字
    std::vector<DmChar*> chs;
    for (auto ch : str)
    {
        QString charStr(ch);
        DmChar* c = DmMText::createChar(charStr, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        if (nullptr == c)
        {
            continue;
        }
        //m_pMText->insertChar(preChar, postChar, c);
        //preChar = c;
        chs.emplace_back(c);
    }

    //添加执行命令
    if (chs.size() != 0)
    {
        int idx;
        para->locateCharIndex(preChar, postChar, idx);
        MTextEdit_AddCharsCmd* cmd = new MTextEdit_AddCharsCmd(m_pMText, para, chs, idx);
        cmd->execute();
        m_cmds.emplace_back(cmd);
        preChar = chs.back();
    }
}

/// @brief 构造调整大小命令
MTextEdit_ResizeCmd::MTextEdit_ResizeCmd(MTextEditWidget *editWidget, int newWidth, int newHeight)
    : m_editWidget(editWidget)
{
    m_originRightBottom  = m_editWidget->m_rightBottom;
    GuiDocumentView* docView = m_editWidget->m_pDocumentView;
    double worldWidth = docView->toGraphDX(newWidth);
    double worldHeight = docView->toGraphDY(newHeight);
    m_newRightBottom = m_editWidget->m_leftTop + DmVector(worldWidth, -worldHeight);
}

/// @brief 执行调整大小
void MTextEdit_ResizeCmd::execute()
{
    m_editWidget->m_rightBottom = m_newRightBottom;
    m_editWidget->updateGeometry();
    ICmd::execute();
}

/// @brief 撤销调整大小
void MTextEdit_ResizeCmd::undo()
{
    m_editWidget->m_rightBottom = m_originRightBottom;
    m_editWidget->updateGeometry();
    ICmd::undo();
}

/// @brief 重做调整大小
void MTextEdit_ResizeCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_ResizeCmd::clear()
{
}

/// @brief 构造改变文字样式命令
MTextEdit_ChangeStyleCmd::MTextEdit_ChangeStyleCmd(DmMText *mtext, DmTextStyle *textStyle)
    : m_mtext(mtext)
    , m_newTextStyle(textStyle)
{
    m_originTextStyle = mtext->getTextStyle();
    //mtext->updateContent();
    //与updateContent()类似，规避了startModify()
    MTextContentCmdMgr mgr(mtext);
    m_content = mgr.generateContent();

    for (auto para : *mtext)
    {
         m_originParas.emplace_back(para);
    }
}

/// @brief 执行改变文字样式
void MTextEdit_ChangeStyleCmd::execute()
{
    std::vector<DmMTextParagraph*> nouse;
    m_mtext->removeParas(0, m_mtext->size(), nouse); //update()会调用clear()，导致释放，这里先解除从属关系

    auto data = m_mtext->getData();
    data.setTextStyle(m_newTextStyle);
    data.setTextString(m_content);
    m_mtext->setData(data);
    m_mtext->update(); //TODO :对于有角度的多行文字须验证一下

    for (auto para : *m_mtext)
    {
        m_newParas.emplace_back(para);
    }

    ICmd::execute();
}

/// @brief 撤销改变文字样式
void MTextEdit_ChangeStyleCmd::undo()
{
    std::vector<DmMTextParagraph*> nouse;
    m_mtext->removeParas(0, m_mtext->size(), nouse);
    m_mtext->insertParas(0, m_originParas);
    ICmd::undo();
}

/// @brief 重做改变文字样式
void MTextEdit_ChangeStyleCmd::redo()
{
    std::vector<DmMTextParagraph*> nouse;
    m_mtext->removeParas(0, m_mtext->size(), nouse);
    m_mtext->insertParas(0, m_newParas);
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_ChangeStyleCmd::clear()
{
    if (m_isExecuted)
    {
        for (auto para : m_originParas)
        {
            delete para;
        }
        m_originParas.clear();
    }
    else
    {
        for (auto para : m_newParas)
        {
            delete para;
        }
        m_newParas.clear();
    }
}

/// @brief 构造缩放字符命令
MTextEdit_ModifyCharCmd::MTextEdit_ModifyCharCmd(DmChar *theChar, double scale)
    : m_char(theChar)
    , m_newData(scale)
    , m_subCmdType(SubCmdType::Scale)
{
}

/// @brief 构造改变颜色命令
MTextEdit_ModifyCharCmd::MTextEdit_ModifyCharCmd(DmChar *theChar, DmColor color)
    : m_char(theChar)
    , m_newData(color)
    , m_subCmdType(SubCmdType::ChangeColor)
{
    m_originData = m_char->getPen(false).getColor();
}

/// @brief 构造格式修改命令
MTextEdit_ModifyCharCmd::MTextEdit_ModifyCharCmd(DmChar *theChar, MTextEdit_ModifyCharCmd::SubCmdType cmdType)
    : m_char(theChar)
    , m_subCmdType(cmdType)
{
}

/// @brief 执行修改字符格式
void MTextEdit_ModifyCharCmd::execute()
{
    switch (m_subCmdType)
    {
        case SubCmdType::Scale:
        {
            double scale = std::get<double>(m_newData);
            m_char->scale(DmVector(0.0, 0.0), DmVector(scale, scale));
        }
            break;
        case SubCmdType::ChangeColor:
        {
            DmColor color = std::get<DmColor>(m_newData);
            DmPen pen = m_char->getPen(false);
            pen.setColor(color);
            m_char->setPen(pen);
        }
            break;
        case SubCmdType::AddUnderline:
        {
            m_char->addUnderline();
        }
            break;
        case SubCmdType::RemoveUnderline:
        {
            m_char->removeUnderline();
        }
            break;
        case SubCmdType::AddStrikethrough:
        {
            m_char->addStrikethrough();
        }
            break;
        case SubCmdType::RemoveStrikethrough:
        {
            m_char->removeStrikethrough();
        }
            break;
        case SubCmdType::AddOverline:
        {
            m_char->addOverline();
        }
            break;
        case SubCmdType::RemoveOverline:
        {
            m_char->removeOverline();
        }
            break;
    }

    ICmd::execute();
}

/// @brief 撤销修改字符格式
void MTextEdit_ModifyCharCmd::undo()
{
    switch (m_subCmdType)
    {
        case SubCmdType::Scale:
        {
            double scale = 1.0 / std::get<double>(m_newData);
            m_char->scale(DmVector(0.0, 0.0), DmVector(scale, scale));
        }
            break;
        case SubCmdType::ChangeColor:
        {
            DmColor color = std::get<DmColor>(m_originData);
            DmPen pen = m_char->getPen(false);
            pen.setColor(color);
            m_char->setPen(pen);
        }
            break;
        case SubCmdType::AddUnderline:
        {
            m_char->removeUnderline();
        }
            break;
        case SubCmdType::RemoveUnderline:
        {
            m_char->addUnderline();
        }
            break;
        case SubCmdType::AddStrikethrough:
        {
            m_char->removeStrikethrough();
        }
            break;
        case SubCmdType::RemoveStrikethrough:
        {
            m_char->addStrikethrough();
        }
            break;
        case SubCmdType::AddOverline:
        {
            m_char->removeOverline();
        }
            break;
        case SubCmdType::RemoveOverline:
        {
            m_char->addOverline();
        }
            break;
    }

    ICmd::undo();
}

/// @brief 重做修改字符格式
void MTextEdit_ModifyCharCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_ModifyCharCmd::clear()
{
}

/// @brief 构造替换字符命令
MTextEdit_ReplaceCharCmd::MTextEdit_ReplaceCharCmd(DmMText *pMText, DmChar *originChar, DmChar *newChar)
    : m_pMText(pMText)
    , m_originChar(originChar)
    , m_newChar(newChar)
{
    m_para = static_cast<DmMTextParagraph*>(originChar->getParent()->getParent());
    m_index = m_para->indexOf(originChar);
}

/// @brief 执行替换字符
void MTextEdit_ReplaceCharCmd::execute()
{
    MTextEdit_RemoveCharsCmd* cmd = new MTextEdit_RemoveCharsCmd(m_pMText, m_para, m_index, 1);
    cmd->execute();
    m_cmds.emplace_back(cmd);

    MTextEdit_AddCharsCmd* addCmd = new MTextEdit_AddCharsCmd(m_pMText, m_para, {m_newChar}, m_index);
    addCmd->execute();
    m_cmds.emplace_back(addCmd);

    ICmd::execute();
}

/// @brief 撤销替换字符
void MTextEdit_ReplaceCharCmd::undo()
{
    for (auto it = m_cmds.end(); it != m_cmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->undo();
    }
    ICmd::undo();
}

/// @brief 重做替换字符
void MTextEdit_ReplaceCharCmd::redo()
{
    for (auto cmd : m_cmds)
    {
        cmd->redo();
    }
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_ReplaceCharCmd::clear()
{
    if (m_isExecuted)
    {
        for (auto it = m_cmds.end(); it != m_cmds.begin();)
        {
            --it; // 先递减迭代器
            (*it)->clear();
            delete *it;
        }
    }
    else
    {
        for (auto cmd : m_cmds)
        {
            cmd->clear();
        }
    }
    m_cmds.clear();
}

/// @brief 构造修改对正命令
MTextEdit_ModifyJustificationCmd::MTextEdit_ModifyJustificationCmd(DmMText *mtext, EMTextMode justification)
    : m_pMText(mtext)
    , m_newJustification(justification)
{
    m_originJustification = mtext->getJustification();
}

/// @brief 执行修改对正
void MTextEdit_ModifyJustificationCmd::execute()
{
    auto data = m_pMText->getDataConstPtr();
    const_cast<MTextData*>(data)->setJustification(m_newJustification);
    ICmd::execute();
}

/// @brief 撤销修改对正
void MTextEdit_ModifyJustificationCmd::undo()
{
    m_pMText->setJustification(m_originJustification);
    ICmd::undo();
}

/// @brief 重做修改对正
void MTextEdit_ModifyJustificationCmd::redo()
{
    execute();
    ICmd::redo();
}

/// @brief 清理资源
void MTextEdit_ModifyJustificationCmd::clear()
{
}

/// @brief 构造段落对齐命令
MTextEdit_ParasAlignCmd::MTextEdit_ParasAlignCmd(DmMText *mtext, const std::vector<DmMTextParagraph *>& paras,
                                                 DmMTextParagraph::Alignment newAlign)
    : m_pMText(mtext)
    , m_paras(paras)
    , m_newAlign(newAlign)
{
     for (auto para : m_paras)
     {
         m_originAligns.emplace_back(para->getAlignment());
     }
}

/// @brief 执行段落对齐
void MTextEdit_ParasAlignCmd::execute()
{
    for (auto para : m_paras)
    {
        para->setAlignent(m_newAlign);
    }
}

/// @brief 撤销段落对齐
void MTextEdit_ParasAlignCmd::undo()
{
    int count = static_cast<int>(m_paras.size());
    for (int i = 0; i < count; i++)
    {
        m_paras.at(i)->setAlignent(m_originAligns.at(i));
    }
}

/// @brief 重做段落对齐
void MTextEdit_ParasAlignCmd::redo()
{
    execute();
}

/// @brief 清理资源
void MTextEdit_ParasAlignCmd::clear()
{
}
