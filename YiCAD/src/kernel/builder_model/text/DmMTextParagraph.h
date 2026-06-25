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


/// @file DmMTextParagraph.h
/// @brief 多行文字段落实体类，管理段落内的行

#ifndef DMMTEXTPARAGRAPH_H
#define DMMTEXTPARAGRAPH_H

#include "DmEntity.h"

class DmChar;
class DmMTextLine;

/// @brief 多行文字段落实体类
class DmMTextParagraph : public DmEntity
{
public:
    /// @brief 段落对齐方式枚举
    enum class Alignment
    {
        Default,    ///< 默认
        Left,       ///< 左对齐
        Mid,        ///< 居中
        Right,      ///< 右对齐
        Justify,    ///< 对正
        Distribute, ///< 分散对齐
    };

public:
    /// @brief 构造函数
    DmMTextParagraph(DmEntity* parent);

    /// @brief 析构函数
    ~DmMTextParagraph();

    /// @brief 克隆
    DmMTextParagraph* clone() const override;

    /// @brief 获得左上角坐标
    DmVector getLeftTop() const;

    /// @brief 设置左上角坐标
    void setLeftTop(const DmVector& leftTop);

    /// @brief 段落高度。通过统计每行行高获得（包括行间隔）
    double getHeight() const;

    /// @brief 获得除去最后一行下延空白的段落高
    double getHeightWithoutLastDescenderSpace() const;

    /// @brief 获得段落宽度。如果仅有一行，获得实际宽度，如果有多行取文字定义宽度
    double getWidth() const;

    /// @brief 获得段落对齐方式
    DmMTextParagraph::Alignment getAlignment() const;

    /// @brief 设置段落对齐方式
    void setAlignent(const DmMTextParagraph::Alignment& align);

    /// @brief 根据当前对齐方式获得初始定位
    DmVector getStartPosition() const;

    /// @brief 通过坐标定位字符
    void locateChar(const DmVector& pos, const bool force, DmChar*& preChar, DmChar*& postChar, const bool containNewLine = false);

    /// @brief 通过前后字符定位索引
    void locateCharIndex(const DmChar* preChar, const DmChar* postChar, int& index);

    /// @brief 获得字符索引
    int indexOf(const DmChar* c) const;

    /// @brief 在指定索引处插入多个字符（不能包含换行）
    void insertChars(int index, const std::vector<DmChar*>& charsToInsert);

    /// @brief 在前后字符之间插入多个字符
    void insertChars(const DmChar* preChar, const DmChar* postChar, const std::vector<DmChar*>& charsToInsert);

    /// @brief 在前后字符之间插入单个字符
    void insertChar(const DmChar* preChar, const DmChar* postChar, DmChar* charToInsert);

    /// @brief 从索引开始移除指定数量的字符
    void removeChars(int index, int count, std::vector<DmChar*>& originChars);

    /// @brief 更新文字位置
    void updateTextPosition(const double& defineWidth);

    /// @brief 移除指定范围的字符
    void removeRange(DmChar* beginC, DmChar* endC, std::vector<DmChar*>& removedCollect);

    /// @brief 通过索引获得字符
    DmChar* getChar(int index) const;

    /// @brief 获得指定范围的字符
    void getCharsByRange(int index, int count, std::vector<DmChar*>& chars) const;

    /// @brief 获得字符总数
    int getCharsCount(bool countLineFeed) const;

    /// @brief 获得前一行
    DmMTextLine* getPreLine(DmMTextLine* line);
    DmMTextLine* getPreLine(int lineIdx);

    /// @brief 获得后一行
    DmMTextLine* getPostLine(DmMTextLine* line);
    DmMTextLine* getPostLine(int lineIdx);

    /// @brief 释放line但不删除文字
    void clearWithoutDelChars();

    /// @brief 在结尾处添加换行符
    bool appendLineFeed(DmChar* newLineChar);

    /// @brief 移除换行符
    bool removeLineFeed(DmChar*& removedNewLineChar);

    /// @brief 是否存在换行符
    bool hasLineFeed() const;

    /// @brief 判断是否为空段落（没有任何字符，或者仅包含换行符）
    bool isEmptyParagraph() const;

    /// @brief 添加行
    void addLine(DmMTextLine* line);

    /// @brief 通过索引获得行
    DmMTextLine* lineAt(int index) const;

    /// @brief 行数量
    int size() const;

    /// @brief 移除行
    void removeLine(DmMTextLine* line);

    /// @brief 查找行索引
    int findLine(DmMTextLine* line) const;

    /// @brief 第一行
    DmMTextLine* first() const;

    /// @brief 最后一行
    DmMTextLine* last() const;

    std::vector<DmMTextLine*>::const_iterator begin() const;
    std::vector<DmMTextLine*>::const_iterator end() const;
    std::vector<DmMTextLine*>::iterator begin();
    std::vector<DmMTextLine*>::iterator end();

    std::list<DmEntity*> getSubEntities() const override;

    bool isContainer() const override;
    void calculateBorders() override;
    double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr, DM::ResolveLevel level = DM::ResolveNone) const override;
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
    void setVisible(bool v) override;
    bool setSelected(bool select = true) override;
    void setHighlighted(bool highlight = true) override;

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

private:
    DmMTextLine* getPrePostLine(DmMTextLine* line, bool getPre);
    DmMTextLine* getPrePostLine(int lineIdx, bool getPre);

    /// @brief 通过段落的对齐方式设置行文字的位置
    void updateLineXPosition(std::vector<DmMTextLine*>& lines, const double& defineWidth);

    /// @brief 居中对齐更新x位置
    void updateLineXPosition_Mid(std::vector<DmMTextLine*>& lines, const double& defineWidth);

    /// @brief 右对齐更新x位置
    void updateLineXPosition_Right(std::vector<DmMTextLine*>& lines, const double& defineWidth);

    /// @brief 分散对齐更新x位置
    void updateLineXPosition_Distribute(std::vector<DmMTextLine*>& lines, const double& defineWidth);

private:
    Alignment m_eAlign;                     ///< 对齐方式
    DmVector m_leftTop;                     ///< 左上角坐标
    std::vector<DmMTextLine*> lines;        ///< 行列表
};

#endif //!DMMTEXTPARAGRAPH_H
