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


/// @file DmTextStyle.h
/// @brief 文字样式类，定义文字的字体、高度、宽度因子等属性

#ifndef DMTEXTSTYLE_H
#define DMTEXTSTYLE_H

#include <QString>
#include "DmBlockTable.h"
#include "DmFlags.h"
#include "TextConsts.h"
#include "DmObject.h"

class DmFont;
class DmCharTemplate;
class DmTextStyleTable;
class DmEntityContainer;
class DmChar;
class Writer;
class Reader;
class OutputStream;
class InputStream;

/// @brief 文字样式数据结构
class DmTextStyleData
{
public:
    QString name{""};                       ///< 样式名
    bool isUseBigfont{false};               ///< 是否使用大字体（仅针对shx字体）
    bool isSystemFont{false};               ///< 是否为系统字体
    DmFont* pSysFont{nullptr};              ///< 系统字体
    DmFont* pAsciiFont{nullptr};            ///< ASCII字体
    DmFont* pBigFont{nullptr};              ///< 大字体
    QString sysFontFamily{ "" };            ///< 系统字体族名
    bool isSysFontBold{ false };            ///< 系统字体是否为粗体
    bool isSysFontItalic{ false };          ///< 系统字体是否为斜体
    double defaultHeight{0.0};              ///< 默认高度
    bool isUpsideDown{false};               ///< 是否颠倒
    bool isReverseDirection{false};         ///< 是否反向
    bool isVertical{false};                 ///< 是否垂直
    double widhFactor{1.0};                ///< 宽度因子
    double slashAngle{0.0};                ///< 倾斜角度（弧度）

    QString invalidSysFontFamily{ "" };     ///< 不存在的系统字体族名
    QString invalidAsciiFont{ "" };         ///< 不存在的shx字体名
    QString invalidBigFont{ "" };           ///< 不存在的大字体名
};

/// @brief 文字样式类
class DmTextStyle : public DmObject
{
    TYPESYSTEM_HEADER();
private:
    DmTextStyle(const DmTextStyle& style) = default;
public:
    /// @brief 默认构造函数
    DmTextStyle();

    /// @brief 带名字的构造函数
    DmTextStyle(const QString& name);

    /// @brief 带数据的构造函数
    DmTextStyle(const DmTextStyleData& data);

    /// @brief 带模板和名字的构造函数
    DmTextStyle(const DmTextStyle& temp, const QString name);

    /// @brief 克隆
    DmTextStyle* clone() const;

    /// @brief 获得指定文字（如果不存在则创建）
    /// @param letter 文字字符串
    /// @param widthFactor 宽度因子
    /// @param getDefaultIfNotFound 如果未找到是否获取默认值
    /// @return 字符实体
    DmChar* findOrCreateLetter(QString& letter, const double& widthFactor, const bool& getDefaultIfNotFound);

    /// @brief 获得有效的默认高度（不会为0）
    /// @return 默认高度
    double getValidDefaultHeight() const;

    /// @brief 获得文字样式数据
    DmTextStyleData getData() const;

    /// @brief 设置文字样式数据
    void setData(const DmTextStyleData& data);

    /// @brief 获得文字样式数据常量指针
    const DmTextStyleData* getDataConstPtr() const;

    /// @brief 是否为标准样式
    bool isStandard();

    /// @brief 获得样式名
    QString getName() const;

    /// @brief 设置样式名
    void setName(const QString& name);

    /// @brief 是否有效。有字体文件不存在则无效
    bool isValid() const;

    /// @brief 获得预览实体
    void getPreview(DmEntityContainer* previewContainer) const;

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& rdr, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    std::shared_ptr<DmTextStyleData> m_data;    ///< 文字样式数据
};

#endif
