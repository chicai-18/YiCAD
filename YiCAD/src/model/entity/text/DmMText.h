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


/// @file DmMText.h
/// @brief 多行文字实体类，管理段落/行/字符的层级结构

#ifndef DMMTEXT_H
#define DMMTEXT_H

#include "DmEntity.h"
#include "MTextData.h"
#include "DmColor.h"

class DmChar;
class DmMTextParagraph;
class DmSolid;
class DmTextStyle;

/// @brief 多行文字实体类
class DmMText : public DmEntity
{
    TYPESYSTEM_HEADER();
public:
    /// @brief 默认构造函数
    DmMText(DmEntity* parent = nullptr);

    /// @brief 带数据构造函数
    DmMText(DmEntity* parent, const MTextData& d);

    /// @brief 析构函数
    virtual ~DmMText();

    /// @brief 获得数据
    MTextData getData() const;

    /// @brief 设置数据
    void setData(const MTextData& data) { m_data.reset(new MTextData(data)); }

    /// @brief 克隆
    DmEntity* clone() const override;

    /// @brief 获得实体类型
    virtual DM::EntityType getEntityType() const override;

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    void moveEntities(const DmVector& offset);
    void rotateEntities(const DmVector& center, const double& angle);

    /// @brief 获得选择字符个数
    int getSelectCharCount(const DmChar* preCharBegin, const DmChar* postCharBegin,
        const DmChar* preCharEnd, const DmChar* postCharEnd);

    /// @brief 获得字符的索引（换行符包括在内）
    int indexOf(const DmChar* c) const;

    /// @brief 通过索引获得字符（换行符包括在内）
    DmChar* getCharByIndex(int idx) const;

    /// @brief 用新字符替换所有文字，同时获得原始字符
    void replaceByChars(const std::vector<DmChar*>& newChars, std::vector<DmChar*>& originChars);

    /// @brief 清空但不释放字符
    void clearWithoutDelChars();

    /// @brief 获得所有字符，同时将字符父实体设为null
    void getChars(std::vector<DmChar*>& chs);

    /// @brief 插入多个字符，不能有换行符（用于一次输入多个汉字）
    void insertChars(const DmChar* preChar, const DmChar* postChar,
        const std::vector<DmChar*>& charsToInsert);

    /// @brief 在索引处插入字符
    void insertChars(int index, const std::vector<DmChar*>& chs);

    /// @brief 从索引开始删除指定个数字符，获得删除的字符
    void removeChars(int index, int count, std::vector<DmChar*>& originChars);

    /// @brief 通过索引定位前后字符
    /// @details 例如："ABC"，光标在A前面索引为0，A后面为1
    void locateByIndex(int index, DmChar*& preChar, DmChar*& postChar) const;

    /// @brief 通过前后字符获得索引
    void locateCharIndex(const DmChar* preChar, const DmChar* postChar, int& index);

    /// @brief 通过坐标定位前后字符
    void locateChar(const DmVector& pos, bool force, DmChar*& preChar, DmChar*& postChar,
        bool containNewLine = false);

    /// @brief 获得指定位置所在段落
    DmMTextParagraph* getParagraph(const DmChar* preChar, const DmChar* postChar);

    /// @brief 在指定位置插入字符
    void insertChar(const DmChar* preChar, const DmChar* postChar, DmChar* charToInsert);

    /// @brief 更新段落，使每个段落至多有一个换行
    void updateParas();

    /// @brief 从起始段落开始往后更新文字位置
    void updateTextPosition();

    /// @brief 根据对正方式更新文字位置
    void updateTextPositionByJustification();

    /// @brief 新建段落，返回形成的后面那个段落
    DmMTextParagraph* newParagraph(const DmChar* preChar, const DmChar* postChar, DmChar* newLineChar);

    /// @brief 退格操作
    void backspace(const DmChar* preChar, const DmChar* postChar,
        DmChar*& newPreChar, DmChar*& newPostChar);

    /// @brief delete操作
    void del(const DmChar* preChar, const DmChar* postChar,
        DmChar*& newPreChar, DmChar*& newPostChar);

    /// @brief 获得前一个位置（左方向键的响应）
    void getPreviousChar(const DmChar* preChar, const DmChar* postChar,
        DmChar*& newPreChar, DmChar*& newPostChar);

    /// @brief 获得后一个位置（右方向键的响应）
    void getPostChar(const DmChar* preChar, const DmChar* postChar,
        DmChar*& newPreChar, DmChar*& newPostChar);

    /// @brief 获得上一行x坐标最接近的位置（上方向键的响应）
    void getUpChar(const DmChar* preChar, const DmChar* postChar,
        DmChar*& newPreChar, DmChar*& newPostChar);

    /// @brief 获得下一行x坐标最接近的位置（下方向键的响应）
    void getDownChar(const DmChar* preChar, const DmChar* postChar,
        DmChar*& newPreChar, DmChar*& newPostChar);

    /// @brief 根据起始终止位置获得选择遮罩
    DmEntityContainer* getSelectedCover(const DmChar* preCharBegin, const DmChar* postCharBegin,
        const DmChar* preCharEnd, const DmChar* postCharEnd);

    /// @brief 获得一行的选择遮罩
    DmSolid* getSelectedCoverOfLine(DmChar* startChar, DmChar* endChar);

        /// @brief 给2个位置排序
    void getOrderedPos(const DmChar* preCharBegin, const DmChar* postCharBegin,
        const DmChar* preCharEnd, const DmChar* postCharEnd,
        const DmChar*& preCharBeginNew, const DmChar*& postCharBeginNew,
        const DmChar*& preCharEndNew, const DmChar*& postCharEndNew);

    /// @brief 通过有序的位置获得起始终止字符
    void getStartEndCharByOrderedPos(DmChar*& startChar, DmChar*& endChar,
        const DmChar* preCharBeginNew, const DmChar* postCharBeginNew,
        const DmChar* preCharEndNew, const DmChar* postCharEndNew);

    /// @brief 通过位置获得起始终止字符
    void getStartEndCharByPos(DmChar*& startChar, DmChar*& endChar,
        const DmChar* preCharBegin, const DmChar* postCharBegin,
        const DmChar* preCharEnd, const DmChar* postCharEnd);

    /// @brief 删除选择的字符
    void delSelectedChars(const DmChar* preCharBegin, const DmChar* postCharBegin,
        const DmChar* preCharEnd, const DmChar* postCharEnd);

    /// @brief 删除单个字符
    void delChar(DmChar* theChar, bool& paraRemoved, bool& lineRemoved);

    DmMTextParagraph* getPreParagraph(DmMTextParagraph* para);
    DmMTextParagraph* getPreParagraph(int paraIdx);
    DmMTextParagraph* getPostParagraph(DmMTextParagraph* para);
    DmMTextParagraph* getPostParagraph(int paraIdx);

    /// @brief 获得指定字符的后一个字符
    /// @param containNewLine 是否可以返回换行符
    /// @param crossPara 是否跨段落查找
    static DmChar* getPostChar(const DmChar* c, bool containNewLine, bool crossPara);

    /// @brief 获得数据常量指针
    const MTextData* getDataConstPtr() const;

    /// @brief 计算文字高度
    double calculateHeight() const;

    /// @brief 创建字符
    static DmChar* createChar(const QString& charStr, const DmTextStyle* style,
        const DmColor& color, const double& height, const double& widthFactor,
        const double& slashAngle, const QString& fontName, const bool& isBold,
        const bool& isItalic, const bool& hasUnderline, const bool& hasStrikethrough,
        const bool& hasOverline);

    /// @brief 替换字符
    static void replaceChar(DmChar* originChar, DmChar* replaceChar, const bool& delOrigin);

    /// @brief 从样式初始化信息
    static void initInfoFromStyle(const DmTextStyle* style, DmColor& color, double& height,
        double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic,
        bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline);

    /// @brief 从字符初始化信息
    static void initInfoFromChar(const DmChar* c, DmColor& color, double& height,
        double& widthFactor, double& slashAngle, QString& fontName, bool& isBold, bool& isItalic,
        bool& hasUnderline, bool& hasStrikethrough, bool& hasOverline);

    std::list<DmEntity*> getSubEntities() const override;

    /// @brief 初始化文字，用于新建文字时
    void init(const DmVector& pos, const double width, const double height, const double charHeight);

    /// @brief 初始化文字，用于已知文字位置宽高时
    void init(const double charHeight);

    /// @brief 文字是否仅包含空白符
    bool isEmptyText() const;

    bool isJustifyAlignHLeft() const;
    bool isJustifyAlignHCenter() const;
    bool isJustifyAlignHRight() const;
    bool isJustifyAlignVTop() const;
    bool isJustifyAlignVMid() const;
    bool isJustifyAlignVBottom() const;

    DmVector getPosition() const;
    void setPosition(const DmVector& pos);
    double getWidth() const;
    void setWidth(const double width);
    double calculateWidth() const;
    double getHeight() const;
    void setHeight(const double height);
    double getCharHeight() const;
    void setCharHeight(const double height);
    EMTextMode getJustification() const;
    void setJustification(EMTextMode justification);
    DmTextStyle* getTextStyle() const;
    void setTextStyle(DmTextStyle* style);
    double getLineSpacingFactor() const;
    void setLineSpacingFactor(const double factor);
    double getAngle() const;
    void setAngle(const double angle);
    double getLineSpace() const;
    void setLineSpace(const double lineSpace);
    QString getContent() const;
    void setContent(const QString& content);
    void setStyle(DmTextStyle* pStyle);
    DmTextStyle* getStyle() const;
    int size() const;
    DmMTextParagraph* first() const;
    DmMTextParagraph* last() const;
    DmMTextParagraph* paraAt(int index);
    void addPara(DmMTextParagraph* para);
    int findPara(DmMTextParagraph* para) const;
    void insertPara(int index, DmMTextParagraph* para);
    void insertParas(int index, const std::vector<DmMTextParagraph*>& paras);
    void removePara(int index, DmMTextParagraph*& para);
    void removeParas(int index, int count, std::vector<DmMTextParagraph*>& paras);
    void getParas(int index, int count, std::vector<DmMTextParagraph*>& paras);
    void removePara(DmMTextParagraph* para);
    void clear();
    std::vector<DmMTextParagraph*>::const_iterator begin() const;
    std::vector<DmMTextParagraph*>::const_iterator end() const;
    std::vector<DmMTextParagraph*>::iterator begin();
    std::vector<DmMTextParagraph*>::iterator end();

    bool isContainer() const override;
    void calculateBorders() override;
    double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr,
        DM::ResolveLevel level = DM::ResolveNone) const override;
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity = true,
        double* dist = nullptr, DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr,
        int middlePoints = 1) const override;
    void setVisible(bool v) override;
    bool setSelected(bool select = true) override;
    void setHighlighted(bool highlight = true) override;

    /// @brief 更新以生成文字实体
    void update();

    /// @brief 根据实体更新内容
    void updateContent();

    /// @brief 获得范围内文字内容
    QString getContentByRange(const DmChar* startChar, const DmChar* endChar) const;

    /// @brief 在指定位置插入内容
    DmChar* insertContentAtPos(const DmChar* preChar, const DmChar* postChar, const QString& content);

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

private:
    DmMTextParagraph* getPrePostParagraph(DmMTextParagraph* para, bool getPre);
    DmMTextParagraph* getPrePostParagraph(int paraIdx, bool getPre);

protected:
    std::vector<DmMTextParagraph*> paragraphs; ///< 段落列表
    std::shared_ptr<MTextData> m_data; ///< 多行文字数据
};

#endif //!DMMTEXT_H
