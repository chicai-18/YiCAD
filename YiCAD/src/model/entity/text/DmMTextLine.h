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


/// @file DmMTextLine.h
/// @brief 多行文字行实体类，管理一行内的字符

#ifndef DMMTEXTLINE_H
#define DMMTEXTLINE_H

#include "DmEntity.h"

class DmChar;

/// @brief 多行文字行实体类
class DmMTextLine : public DmEntity
{
public:
    /// @brief 构造函数
    DmMTextLine(DmEntity* parent);

    /// @brief 析构函数
    ~DmMTextLine();

    /// @brief 拷贝构造函数
    DmMTextLine(const DmMTextLine& line) = default;

    /// @brief 克隆
    DmMTextLine* clone() const override;

    /// @brief 更新行高
    void updateHeight();

    /// @brief 获得行高
    double getHeight() const;

    /// @brief 设置行高
    void setHeight(double height);

    /// @brief 计算带行间距的行高
    double getHeightWithSpace() const;

    /// @brief 获得基线以下最大高度
    double getDescender() const;

    /// @brief 更新行宽
    void updateWidth();

    /// @brief 获得行宽
    double getWidth() const;

    /// @brief 获得左上角坐标
    DmVector getLeftTop() const;

    /// @brief 设置左上角坐标
    void setLeftTop(const DmVector& leftTop);

    /// @brief 通过坐标定位字符
    void locateChar(const DmVector& pos, const bool force, DmChar*& preChar, DmChar*& postChar, const bool containNewLine = false);

    /// @brief 清空但不删除字符
    void clearWithoutDelChars();

    /// @brief 获得最后一个字符
    /// @param includeLineFeed 是否可以是换行符
    DmChar* getLastChar(bool includeLineFeed) const;

    /// @brief 获得第一个字符
    /// @param includeLineFeed 是否可以是换行符
    DmChar* getFirstChar(bool includeLineFeed);

    /// @brief 此行结尾是否包含换行
    bool hasLineFeed();

    /// @brief 在指定索引处插入多个字符
    void insertChars(int index, const std::vector<DmChar*>& chs);

    /// @brief 在指定索引处插入字符
    void insertChar(int index, DmChar* c);

    /// @brief 添加字符到末尾
    void addChar(DmChar* c);

    /// @brief 移除字符
    /// @param c 要移除的字符
    /// @param del 是否删除内存
    void removeChar(DmChar* c, bool del);

    /// @brief 从索引开始移除指定数量的字符
    /// @param index 起始索引
    /// @param count 数量
    /// @param originChars 输出：移除的字符
    void removeChars(int index, int count, std::vector<DmChar*>& originChars);

    /// @brief 获得指定范围的字符
    /// @param index 起始索引
    /// @param count 数量
    /// @param chars 输出：获得的字符
    void getCharsByRange(int index, int count, std::vector<DmChar*>& chars) const;

    /// @brief 通过索引获得字符
    DmChar* charAt(int index) const;

    /// @brief 字符数量
    int size() const;

    /// @brief 查找字符索引
    int findChar(const DmChar* c) const;

    /// @brief 第一个字符
    DmChar* first();

    /// @brief 最后一个字符
    DmChar* last();

    /// @brief 是否为空
    bool isEmpty() const;

    std::vector<DmChar*>::const_iterator begin() const;
    std::vector<DmChar*>::const_iterator end() const;
    std::vector<DmChar*>::iterator begin();
    std::vector<DmChar*>::iterator end();

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
    double m_dHeight{0.0};              ///< 行高（最大的文字基线以上高度，最小取名义高度，不包括间隔）
    double m_dDescender{0.0};           ///< 最大的文字基线以下高度（大于等于0）
    double m_dWidth{0.0};               ///< 行宽
    DmVector m_leftTop;                 ///< 左上角坐标
    std::vector<DmChar*> chars;         ///< 字符列表
};

#endif //!DMMTEXTLINE_H
