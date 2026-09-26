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

/// @file TextData.h
/// @brief 单行文字数据类

#ifndef TEXTDATA_H
#define TEXTDATA_H

#include <QString>
#include "EntityData.h"
#include "DmVector.h"
#include "DmColor.h"

class DmTextStyle;

/// @brief 单行文字
class TextData : public EntityData
{
public:
    TextData();

    /// @brief 带参数的单行文字构造函数
    /// @param [in] insertionPoint 插入点
    /// @param [in] height 文字高度
    /// @param [in] valign 纵向对齐模式
    /// @param [in] halign 横向对齐模式
    /// @param [in] text 文字内容
    /// @param [in] style 文字样式
    /// @param [in] angle 旋转角度
    /// @param [in] updateMode 更新模式
    TextData(const DmVector& insertionPoint, double height, ETextVertMode valign, ETextHorzMode halign,
        const QString& text, DmTextStyle* style, double angle, EUpdateMode updateMode = EUpdateMode::Update);

    /// @brief 获取文字内容
    /// @return 文字内容
    QString getTextString() const;

    /// @brief 设置文字内容
    /// @param [in] strTextString 文字内容
    void setTextString(const QString& strTextString);

    /// @brief 获取文字样式
    /// @return 文字样式指针
    DmTextStyle* getTextStyle() const;

    /// @brief 设置文字样式
    /// @param [in] pStyle 文字样式指针
    void setTextStyle(DmTextStyle* pStyle);

    /// @brief 获取旋转角度
    /// @return 旋转角度（弧度）
    double getAngle() const;

    /// @brief 设置旋转角度
    /// @param [in] dAngle 旋转角度（弧度）
    void setAngle(const double& dAngle);

    /// @brief 获取定位点
    /// @return 定位点坐标
    DmVector getPosition() const;

    /// @brief 设置定位点
    /// @param [in] pt 定位点坐标
    void setPosition(const DmVector& pt);

    /// @brief 获取对正点
    /// @return 对正点坐标
    DmVector getAlignment() const;

    /// @brief 设置对正点
    /// @param [in] pt 对正点坐标
    void setAlignment(const DmVector& pt);

    /// @brief 获取横向对齐模式
    /// @return 横向对齐模式
    ETextHorzMode getTextHorzMode() const;

    /// @brief 设置横向对齐模式
    /// @param [in] eTextHorzMode 横向对齐模式
    void setTextHorzMode(ETextHorzMode eTextHorzMode);

    /// @brief 获取纵向对齐模式
    /// @return 纵向对齐模式
    ETextVertMode getTextVertMode() const;

    /// @brief 设置纵向对齐模式
    /// @param [in] eTextVertMode 纵向对齐模式
    void setTextVertMode(ETextVertMode eTextVertMode);

    /// @brief 获取文字高度
    /// @return 文字高度
    double getTextHeight() const;

    /// @brief 设置文字高度
    /// @param [in] dHeight 文字高度
    void setTextHeight(const double& dHeight);

    /// @brief 获取更新模式
    /// @return 更新模式
    EUpdateMode getUpdateMode() const;

    /// @brief 设置更新模式
    /// @param [in] updateMode 更新模式
    void setUpdateMode(EUpdateMode updateMode);

    /// @brief 获取是否颠倒
    /// @return 是否颠倒
    bool getUpsideDown() const;

    /// @brief 设置是否颠倒
    /// @param [in] upsideDown 是否颠倒
    void setUpsideDown(const bool& upsideDown);

    /// @brief 获取是否反向
    /// @return 是否反向
    bool getReverseDirection() const;

    /// @brief 设置是否反向
    /// @param [in] reverseDirection 是否反向
    void setReverseDirection(const bool& reverseDirection);

    /// @brief 获取宽度因子
    /// @return 宽度因子
    double getWidthFactor() const;

    /// @brief 设置宽度因子
    /// @param [in] widthFactor 宽度因子
    void setWidthFactor(const double& widthFactor);

    /// @brief 获取倾斜角度
    /// @return 倾斜角度（弧度）
    double getSlashAngle() const;

    /// @brief 设置倾斜角度
    /// @param [in] slashAngle 倾斜角度（弧度）
    void setSlashAngle(const double& slashAngle);

    /// @brief 获取文字对正模式
    /// @return 文字对正模式
    ETextMode getTextMode() const;

    /// @brief 设置文字对正模式
    /// @param [in] align 文字对正模式
    void setTextMode(const ETextMode& align);

    /// @brief 将横向和纵向对齐转换为统一的对正模式
    /// @param [in] hAlign 横向对齐
    /// @param [in] vAlign 纵向对齐
    /// @return 统一的对正模式
    static ETextMode HVToAlignment(ETextHorzMode hAlign, ETextVertMode vAlign);

    /// @brief 将对正模式分解为横向和纵向对齐
    /// @param [in] align 对正模式
    /// @param [out] hAlign 横向对齐
    /// @param [out] vAlign 纵向对齐
    static void AlignmentToHV(ETextMode align, ETextHorzMode& hAlign, ETextVertMode& vAlign);

private:
    QString         m_strTextString;    ///< 文字内容
    DmTextStyle*    m_pTextStyle;       ///< 字体样式
    double          m_dAngle;           ///< 旋转角度（弧度）
    DmVector        m_ptPosition;       ///< 位置
    DmVector        m_ptAlignment;      ///< 对正点
    ETextHorzMode   m_eTextHorzMode;    ///< 横向对齐
    ETextVertMode   m_eTextVertMode;    ///< 纵向对齐
    double          m_dTextHeight;      ///< 字高
    EUpdateMode     m_updateMode;       ///< 更新模式

    // 以下为文字替代（文字样式）属性
    bool   m_bUpsideDown{ false };       ///< 是否颠倒
    bool   m_bReverseDirection{ false }; ///< 是否反向
    double m_dWidthFactor{ 1.0 };        ///< 宽度因子
    double m_dSlashAngle{ 0.0 };         ///< 倾斜角度（弧度）
};

#endif // TEXTDATA_H
