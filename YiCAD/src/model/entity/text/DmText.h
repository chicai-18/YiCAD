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


/// @file DmText.h
/// @brief 单行文字实体类，管理单行文字的字符

#ifndef DMTEXT_H
#define DMTEXT_H

#include "DmEntity.h"
#include "TextData.h"

class DmTextStyleData;
class DmCharTemplate;
class DmChar;

/// @brief 单行文字实体类
class DmText : public DmEntity
{
    TYPESYSTEM_HEADER();
public:
    /// @brief 默认构造函数
    DmText() = default;

    /// @brief 带数据构造函数
    /// @param parent 父实体
    /// @param d 文字数据
    DmText(DmEntity* parent, const TextData& d);

    /// @brief 拷贝构造函数
    DmText(const DmText& text);

    /// @brief 析构函数
    ~DmText();

    /// @brief 克隆
    DmEntity* clone() const override;

    /// @brief 获得实体类型
    virtual DM::EntityType getEntityType() const override;

    /// @brief 获得文字数据
    TextData getData() const;

    /// @brief 获得文字数据常量指针
    const TextData* getDataConstPtr() const;

    /// @brief 更新文字实体
    void update() override;

    /// @brief 获得位置
    DmVector getPosition() const;

    /// @brief 获得对齐点
    DmVector getAlignment() const;

    /// @brief 设置对齐点
    void setAlignment(const DmVector& pt);

    /// @brief 获得文字高度
    double getHeight() const;

    /// @brief 设置文字高度
    void setHeight(double h);

    /// @brief 获得垂直对齐方式
    ETextVertMode getVAlign() const;

    /// @brief 设置垂直对齐方式
    void setVAlign(ETextVertMode va);

    /// @brief 获得水平对齐方式
    ETextHorzMode getHAlign() const;

    /// @brief 设置水平对齐方式
    void setHAlign(ETextHorzMode ha);

    /// @brief 设置文字内容
    void setText(const QString& t);

    /// @brief 获得文字内容
    QString getText() const;

    /// @brief 设置文字样式
    void setStyle(DmTextStyle* style);

    /// @brief 获得文字样式
    DmTextStyle* getStyle() const;

    /// @brief 设置旋转角度
    void setAngle(double a);

    /// @brief 获得旋转角度
    double getAngle() const;

    /// @brief 是否颠倒
    bool getUpsideDown() const;

    /// @brief 设置是否颠倒
    void setUpsideDown(const bool& upsideDown);

    /// @brief 是否反向
    bool getReverseDirection() const;

    /// @brief 设置是否反向
    void setReverseDirection(const bool& reverseDirection);

    /// @brief 获得宽度因子
    double getWidthFactor() const;

    /// @brief 设置宽度因子
    void setWidthFactor(const double& widthFactor);

    /// @brief 获得倾斜角度
    double getSlashAngle() const;

    /// @brief 设置倾斜角度
    void setSlashAngle(const double& slashAngle);

    /// @brief 获得已使用的文字宽度
    double getUsedTextWidth() const;

    /// @brief 获得已使用的文字高度
    double getUsedTextHeight() const;

    /// @brief 通过重写为DmEntity的方法来实现关键点拖拽
    virtual DmVector getNearestRef(const DmVector& coord, double* dist = nullptr) const override;
    virtual DmVector getNearestSelectedRef(const DmVector& coord, double* dist = nullptr) const override;
    virtual DmVectorSolutions getRefPoints() const override;

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

    /// @brief 清空字符
    void clear();

    /// @brief 添加字符
    void addChar(DmChar* c);

    virtual void move(const DmVector& offset) override;
    virtual void rotate(const DmVector& center, const DmVector& angleVector) override;
    virtual void scale(const DmVector& center, const DmVector& factor) override;
    virtual void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;
    virtual void moveRef(const DmVector& ref, const DmVector& offset) override;
    virtual bool hasEndpointsWithinWindow(const DmVector& v1, const DmVector& v2) override;

    std::list<DmEntity*> getSubEntities() const override;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& rdr, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    /// @brief 根据文字内容添加实体
    void addEntitiesOfText(const QString& text);

    /// @brief 移动所有字符
    void moveChars(const DmVector& offset);

    /// @brief 旋转所有字符
    void rotateChars(const DmVector& center, const double& angle);

    /// @brief 镜像所有字符
    void mirrorChars(const DmVector& axisPoint1, const DmVector& axisPoint2);

    /// @brief 缩放所有字符
    void scaleChars(const DmVector& center, const DmVector& factor);

private:
    /// @brief 根据对齐方式生成水平文字
    void createTextHorizontal(const QString& text, DmTextStyle* pStyle);

    /// @brief 按指定宽度系数生成水平文字。文字待放缩
    void createTextHorizontal(const QString& text, DmTextStyle* pStyle, const double& widthFactor);

    /// @brief 计算水平放置的文字宽度。"A"字母实际高度为1.0——以此为基准，不是真实的高度
    double getTextWidthHorizontal(const QString& text, DmTextStyle* pStyle, const double& widthFactor);

    /// @brief 生成垂直文字
    void createTextVertical(const QString& text, DmTextStyle* pStyle);

    /// @brief 按指定宽度系数生成垂直文字
    void createTextVertical(const QString& text, DmTextStyle* pStyles, const double& widthFactor);

    /// @brief 计算垂直放置的文字高度。"A"字母实际高度为1.0——以此为基准，不是真实的高度
    double getTextHeightVertical(const QString& text, DmTextStyle* pStyle);

    /// @brief 在创建文字过程中，处理%%特殊字符
    void handlePercentChar(const QChar& ch, const QString& text, const DmVector& spaceVec, QString& letterText, DmVector& letterPos, int& currentIdx);

protected:
    std::shared_ptr<TextData>   m_data;             ///< 文字数据
    std::vector<DmChar*>        chars;              ///< 字符列表

    // Text width used by the current contents of this text entity.
    // This property is updated by the update method.
    double usedTextWidth{0.0};                      ///< 已使用的文字宽度
    double usedTextHeight{0.0};                     ///< 已使用的文字高度
};

#endif
