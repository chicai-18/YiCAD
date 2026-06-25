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

/// @file MTextEditCmd.h
/// @brief 多行文字编辑器命令，包含字符/段落添加、删除、格式修改等Undo/Redo命令

#ifndef MTEXTEDITCMD_H
#define MTEXTEDITCMD_H

#include "Cmd.h"
#include <vector>
#include "DmMTextParagraph.h"
#include "EntityDataDef.h"
#include <variant>

class DmMText;
class DmMTextParagraph;
class DmChar;
class DmEntityContainer;
class MTextEditWidget;
class MTextContentCmd;
class DmTextStyle;

/// @brief 给段落添加字符（不能有换行）
class MTextEdit_AddCharsCmd : public ICmd
{
public:
    /// @brief 构造添加字符命令
    /// @param mtext 多行文字对象
    /// @param para 目标段落
    /// @param addedChars 要添加的字符列表
    /// @param addIdx 插入索引
    MTextEdit_AddCharsCmd(DmMText* mtext, DmMTextParagraph* para, const std::vector<DmChar*>& addedChars, int addIdx)
        : m_pMText(mtext)
        , m_pPara(para)
        , m_addedChars(addedChars)
        , m_iAddIdx(addIdx)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                      ///< 多行文字对象
    DmMTextParagraph* m_pPara;              ///< 目标段落
    std::vector<DmChar*> m_addedChars;      ///< 要添加的字符列表
    int m_iAddIdx;                          ///< 插入的索引
};

/// @brief 给段落删除字符（仅删除该段落的字符）
class MTextEdit_RemoveCharsCmd : public ICmd
{
public:
    /// @brief 构造删除字符命令
    /// @param mtext 多行文字对象
    /// @param para 目标段落
    /// @param removeIdx 从该段落的起始计数索引
    /// @param removedCount 删除字符数
    MTextEdit_RemoveCharsCmd(DmMText* mtext, DmMTextParagraph* para, int removeIdx, int removedCount)
        : m_pMText(mtext)
        , m_pPara(para)
        , m_iRemoveIdx(removeIdx)
        , m_iRemoveCount(removedCount)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                      ///< 多行文字对象
    DmMTextParagraph* m_pPara;              ///< 目标段落
    std::vector<DmChar*> m_removedChars;    ///< 被删除的字符列表
    int m_iRemoveIdx;                       ///< 删除起始索引
    int m_iRemoveCount;                     ///< 删除字符数
};

/// @brief 删除指定个数的字符，如果整个段落文字都删除了，删除该段落
class MTextEdit_RemoveParasAndCharsCmd : public ICmd
{
public:
    /// @brief 构造删除段落和字符命令
    /// @param mtext 多行文字对象
    /// @param removeIdx 从文字首段落开始计数的索引
    /// @param removeCount 删除字符数
    MTextEdit_RemoveParasAndCharsCmd(DmMText* mtext, int removeIdx, int removeCount)
        : m_pMText(mtext)
        , m_iRemoveIdx(removeIdx)
        , m_iRemoveCount(removeCount)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                      ///< 多行文字对象
    int m_iRemoveIdx;                       ///< 删除起始字符索引
    int m_iRemoveCount;                     ///< 删除字符数
    std::vector<ICmd*> m_cmds;              ///< 子命令
};

/// @brief 插入一个段落
class MTextEdit_AddParaCmd : public ICmd
{
public:
    /// @brief 构造添加段落命令
    /// @param mtext 多行文字对象
    /// @param para 要添加的段落
    /// @param addIdx 插入的索引
    MTextEdit_AddParaCmd(DmMText* mtext, DmMTextParagraph* para, int addIdx)
        : m_pMText(mtext)
        , m_pPara(para)
        , m_iAddIdx(addIdx)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                      ///< 多行文字对象
    DmMTextParagraph* m_pPara;              ///< 要添加的段落
    int m_iAddIdx;                          ///< 插入的索引
};

/// @brief 删除一个段落
class MTextEdit_RemoveParaCmd : public ICmd
{
public:
    /// @brief 构造删除段落命令
    /// @param mtext 多行文字对象
    /// @param removeIdx 删除的索引
    MTextEdit_RemoveParaCmd(DmMText* mtext, int removeIdx)
        : m_pMText(mtext)
        , m_iRemoveIdx(removeIdx)
        , m_pPara(nullptr)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                      ///< 多行文字对象
    DmMTextParagraph* m_pPara;              ///< 删除的段落
    int m_iRemoveIdx;                       ///< 删除的索引
};

/// @brief 删除多个段落
class MTextEdit_RemoveParasCmd : public ICmd
{
public:
    /// @brief 构造删除多个段落命令
    /// @param mtext 多行文字对象
    /// @param removeIdx 删除的起始索引
    /// @param removeCount 删除段落数
    MTextEdit_RemoveParasCmd(DmMText* mtext, int removeIdx, int removeCount)
        : m_pMText(mtext)
        , m_iRemoveIdx(removeIdx)
        , m_iRemoveCount(removeCount)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                          ///< 多行文字对象
    std::vector<DmMTextParagraph*> m_pParas;    ///< 删除的段落
    int m_iRemoveIdx;                           ///< 删除的索引
    int m_iRemoveCount;                         ///< 删除段落数
};

/// @brief 给段落添加换行符
class MTextEdit_AppendNewLineCharCmd : public ICmd
{
public:
    /// @brief 构造添加换行符命令
    /// @param mtext 多行文字对象
    /// @param para 目标段落
    /// @param newLineChar 换行符
    MTextEdit_AppendNewLineCharCmd(DmMText* mtext, DmMTextParagraph* para, DmChar* newLineChar)
        : m_pMText(mtext)
        , m_pPara(para)
        , m_newLineChar(newLineChar)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;              ///< 多行文字对象
    DmMTextParagraph* m_pPara;      ///< 目标段落
    DmChar* m_newLineChar;          ///< 换行符
};

/// @brief 删除段落的换行符
class MTextEdit_RemoveNewLineCharCmd : public ICmd
{
public:
    /// @brief 构造删除换行符命令
    /// @param mtext 多行文字对象
    /// @param para 目标段落
    MTextEdit_RemoveNewLineCharCmd(DmMText* mtext, DmMTextParagraph* para)
        : m_pMText(mtext)
        , m_pPara(para)
        , m_newLineChar(nullptr)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;              ///< 多行文字对象
    DmMTextParagraph* m_pPara;      ///< 目标段落
    DmChar* m_newLineChar;          ///< 换行符
};

/// @brief 将多行文字编辑器的选中起始字符设为空
class MTextEdit_SetSelectBeginEndToNull_Cmd : public ICmd
{
public:
    /// @brief 构造清空选中区域命令
    /// @param mtext 多行文字对象
    /// @param editWidget 编辑控件
    MTextEdit_SetSelectBeginEndToNull_Cmd(DmMText* mtext, MTextEditWidget* editWidget);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                              ///< 多行文字对象
    MTextEditWidget* m_editWidget;                  ///< 编辑控件
    DmChar* m_originSelectBeginPreChar;             ///< 原来选择起始前置字符
    DmChar* m_originSelectBeginPostChar;            ///< 原来选择起始后置字符
    DmChar* m_originSelectEndPreChar;               ///< 原来选择结束前置字符
    DmChar* m_originSelectEndPostChar;              ///< 原来选择结束后置字符
};

/// @brief 修改段落的对齐方式
class MTextEdit_SetParaAlignmentCmd : public ICmd
{
public:
    /// @brief 构造修改段落对齐命令
    /// @param mtext 多行文字对象
    /// @param para 目标段落
    /// @param newAlignment 新对齐方式
    MTextEdit_SetParaAlignmentCmd(DmMText* mtext, DmMTextParagraph* para, DmMTextParagraph::Alignment newAlignment)
        : m_pMText(mtext)
        , m_pPara(para)
        , m_newAlignment(newAlignment)
    {
        m_originAlignment = para->getAlignment();
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                              ///< 多行文字对象
    DmMTextParagraph* m_pPara;                      ///< 目标段落
    DmMTextParagraph::Alignment m_newAlignment;     ///< 新对齐方式
    DmMTextParagraph::Alignment m_originAlignment;  ///< 原始对齐方式
};

/// @brief 插入换行符
class MTextEdit_InsertLineFeedCmd : public ICmd
{
public:
    /// @brief 构造插入换行符命令
    /// @param mtext 多行文字对象
    /// @param preChar 插入位置前置字符
    /// @param postChar 插入位置后置字符
    /// @param editWidget 编辑控件
    MTextEdit_InsertLineFeedCmd(DmMText* mtext, DmChar* preChar, DmChar* postChar, MTextEditWidget* editWidget)
        : m_pMText(mtext)
        , m_preChar(preChar)
        , m_postChar(postChar)
        , m_editWidget(editWidget)
        , m_afterPara(nullptr)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

    /// @brief 获取插入换行后后面那个段落
    /// @return 换行后后面那个段落
    DmMTextParagraph* getAfterPara() const { return m_afterPara; }

protected:
    DmMText* m_pMText;                      ///< 多行文字对象
    DmChar* m_preChar;                      ///< 插入的位置前置字符
    DmChar* m_postChar;                     ///< 插入的位置后置字符
    DmMTextParagraph* m_afterPara;          ///< 插入换行后后面那个段落（供外部使用）
    std::vector<ICmd*> m_cmds;              ///< 子命令
    MTextEditWidget* m_editWidget;          ///< 编辑控件
};

/// @brief 粘贴内容
class MTextEdit_ContentPasteCmd : public ICmd
{
public:
    /// @brief 构造粘贴内容命令
    /// @param mtext 多行文字对象
    /// @param preChar 粘贴位置前置字符
    /// @param postChar 粘贴位置后置字符
    /// @param pasteContent 粘贴内容字符串
    /// @param editWidget 编辑控件
    MTextEdit_ContentPasteCmd(DmMText* mtext, DmChar* preChar, DmChar* postChar, const QString& pasteContent, MTextEditWidget* editWidget)
        : m_pMText(mtext)
        , m_originPreChar(preChar)
        , m_originPostChar(postChar)
        , m_strPasteContent(pasteContent)
        , m_editWidget(editWidget)
        , m_newPreChar(nullptr)
        , m_newPostChar(nullptr)
    {
    }

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

    /// @brief 获取粘贴后的新前置字符
    /// @return 新前置字符
    DmChar* getNewPreChar() const { return m_newPreChar; }

    /// @brief 获取粘贴后的新后置字符
    /// @return 新后置字符
    DmChar* getNewPostChar() const { return m_newPostChar; }

private:
    /// @brief 根据命令列表插入实体
    /// @note 修改下列函数需要同步修改MTextContentCmdMgr的相关函数
    /// @param preChar 前置字符
    /// @param postChar 后置字符
    /// @param cmdsList 命令列表
    void insertEntitiesByCmds(const DmChar* preChar, const DmChar* postChar, const std::vector<std::pair<bool, std::vector<MTextContentCmd*>>>& cmdsList);

    /// @brief 处理单个命令（生成实体或设置当前信息）
    /// @param cmd 内容命令
    /// @param preChar 前置字符（输入输出）
    /// @param postChar 后置字符（输入输出）
    /// @param style 文字样式
    /// @param color 颜色（输入输出）
    /// @param height 字高（输入输出）
    /// @param widthFactor 宽度因子（输入输出）
    /// @param slashAngle 倾斜角（输入输出）
    /// @param fontName 字体名称（输入输出）
    /// @param isBold 是否粗体（输入输出）
    /// @param isItalic 是否斜体（输入输出）
    /// @param hasUnderline 是否有下划线（输入输出）
    /// @param hasStrikethrough 是否有删除线（输入输出）
    /// @param hasOverline 是否有上划线（输入输出）
    void generateEntitiesByOneCmd(const MTextContentCmd* cmd, DmChar*& preChar, DmChar*& postChar, const DmTextStyle* style, DmColor& color, double& height, double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic, bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline);

    /// @brief 插入字符串实体到段落
    /// @param para 目标段落
    /// @param str 要插入的字符串
    /// @param preChar 前置字符（输入输出）
    /// @param postChar 后置字符（输入输出）
    /// @param style 文字样式
    /// @param color 颜色
    /// @param height 字高
    /// @param widthFactor 宽度因子
    /// @param slashAngle 倾斜角
    /// @param fontName 字体名称
    /// @param isBold 是否粗体
    /// @param isItalic 是否斜体
    /// @param hasUnderline 是否有下划线
    /// @param hasStrikethrough 是否有删除线
    /// @param hasOverline 是否有上划线
    void insertStringEntities(DmMTextParagraph* para, const QString& str, DmChar*& preChar, DmChar*& postChar, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline);

protected:
    DmMText* m_pMText;                      ///< 多行文字对象
    std::vector<ICmd*> m_cmds;              ///< 子命令
    QString m_strPasteContent;              ///< 粘贴内容字符串
    MTextEditWidget* m_editWidget;          ///< 编辑控件
    DmChar* m_originPreChar;                ///< 原始前置字符
    DmChar* m_originPostChar;               ///< 原始后置字符
    DmChar* m_newPreChar;                   ///< 粘贴后的新前置字符
    DmChar* m_newPostChar;                  ///< 粘贴后的新后置字符
};

/// @brief 编辑框改变大小
class MTextEdit_ResizeCmd : public ICmd
{
public:
    /// @brief 构造调整大小命令
    /// @param editWidget 编辑控件
    /// @param newWidth 新宽度
    /// @param newHeight 新高度
    MTextEdit_ResizeCmd(MTextEditWidget* editWidget, int newWidth, int newHeight);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    MTextEditWidget* m_editWidget;          ///< 编辑控件
    DmVector m_originRightBottom;           ///< 原始的编辑框右下角世界坐标（不保存UI的宽高）
    DmVector m_newRightBottom;              ///< 新编辑框右下角世界坐标
};

/// @brief 改变文字样式
class MTextEdit_ChangeStyleCmd : public ICmd
{
public:
    /// @brief 构造改变文字样式命令
    /// @param mtext 多行文字对象
    /// @param textStyle 新文字样式
    MTextEdit_ChangeStyleCmd(DmMText* mtext, DmTextStyle* textStyle);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_mtext;                               ///< 多行文字对象
    DmTextStyle* m_newTextStyle;                    ///< 新文字样式
    DmTextStyle* m_originTextStyle;                 ///< 原始文字样式
    std::vector<DmMTextParagraph*> m_originParas;   ///< 原始段落列表
    std::vector<DmMTextParagraph*> m_newParas;      ///< 新段落列表
    QString m_content;                              ///< 文字内容
};

/// @brief 改变DmChar颜色或比例等格式信息
class MTextEdit_ModifyCharCmd : public ICmd
{
public:
    /// @brief 子命令类型
    enum class SubCmdType
    {
        Scale,                  ///< 缩放
        ChangeColor,            ///< 改变颜色
        AddUnderline,           ///< 添加下划线
        RemoveUnderline,        ///< 移除下划线
        AddStrikethrough,       ///< 添加删除线
        RemoveStrikethrough,    ///< 移除删除线
        AddOverline,            ///< 添加上划线
        RemoveOverline          ///< 移除上划线
    };

    /// @brief 构造缩放字符命令
    /// @param theChar 目标字符
    /// @param scale 缩放比例
    MTextEdit_ModifyCharCmd(DmChar* theChar, double scale);

    /// @brief 构造改变颜色命令
    /// @param theChar 目标字符
    /// @param color 新颜色
    MTextEdit_ModifyCharCmd(DmChar* theChar, DmColor color);

    /// @brief 构造格式修改命令
    /// @param theChar 目标字符
    /// @param cmdType 子命令类型
    MTextEdit_ModifyCharCmd(DmChar* theChar, SubCmdType cmdType);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    std::variant<double, DmColor> m_originData; ///< 原始数据
    std::variant<double, DmColor> m_newData;    ///< 新数据
    DmChar* m_char;                             ///< 目标字符
    SubCmdType m_subCmdType;                    ///< 子命令类型
};

/// @brief 替换字符命令
class MTextEdit_ReplaceCharCmd : public ICmd
{
public:
    /// @brief 构造替换字符命令
    /// @param pMText 多行文字对象
    /// @param originChar 原始字符
    /// @param newChar 新字符
    MTextEdit_ReplaceCharCmd(DmMText* pMText, DmChar* originChar, DmChar* newChar);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                  ///< 多行文字对象
    DmMTextParagraph* m_para;           ///< 文字所在段落
    DmChar* m_originChar;               ///< 原始字符
    DmChar* m_newChar;                  ///< 新字符
    int m_index;                        ///< 文字在段落上的索引
    std::vector<ICmd*> m_cmds;          ///< 子命令
};

/// @brief 修改对正方式命令
class MTextEdit_ModifyJustificationCmd : public ICmd
{
public:
    /// @brief 构造修改对正命令
    /// @param mtext 多行文字对象
    /// @param justification 新对正方式
    MTextEdit_ModifyJustificationCmd(DmMText* mtext, EMTextMode justification);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                  ///< 多行文字对象
    EMTextMode m_originJustification;   ///< 原始对正方式
    EMTextMode m_newJustification;      ///< 新对正方式
};

/// @brief 多个段落对齐命令
class MTextEdit_ParasAlignCmd : public ICmd
{
public:
    /// @brief 构造段落对齐命令
    /// @param mtext 多行文字对象
    /// @param paras 目标段落列表
    /// @param newAlign 新对齐方式
    MTextEdit_ParasAlignCmd(DmMText* mtext, const std::vector<DmMTextParagraph*>& paras, DmMTextParagraph::Alignment newAlign);

    void execute() override;
    void undo() override;
    void redo() override;
    void clear() override;

protected:
    DmMText* m_pMText;                                  ///< 多行文字对象
    std::vector<DmMTextParagraph*> m_paras;             ///< 目标段落列表
    std::vector<DmMTextParagraph::Alignment> m_originAligns; ///< 原始对齐方式列表
    DmMTextParagraph::Alignment m_newAlign;             ///< 新对齐方式
};

#endif //MTEXTEDITCMD_H
