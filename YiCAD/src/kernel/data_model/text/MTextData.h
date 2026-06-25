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

/// @file MTextData.h
/// @brief 多行文字数据类

#ifndef MTEXTDATA_H
#define MTEXTDATA_H

#include "EntityData.h"
#include "DmVector.h"
#include <QString>

class DmTextStyle;

/// @brief 多行文字数据
class MTextData : public EntityData
{
public:
    MTextData();

    /// @brief 带参数的多行文字构造函数
    /// @param [in] insertionPoint 插入点
    /// @param [in] charHeight 字符高度
    /// @param [in] valign 纵向对齐模式
    /// @param [in] halign 横向对齐模式
    /// @param [in] lineSpace 行间距
    /// @param [in] defineWidth 定义宽度
    /// @param [in] text 文字内容
    /// @param [in] style 文字样式
    /// @param [in] angle 旋转角度
    /// @param [in] updateMode 更新模式
    MTextData(const DmVector& insertionPoint, double charHeight, EMTextVertMode valign, EMTextHorzMode halign,
        double lineSpace, double defineWidth, const QString& text, DmTextStyle* style, double angle,
        EUpdateMode updateMode = EUpdateMode::Update);

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

    /// @brief 获取横向对齐模式
    /// @return 横向对齐模式
    EMTextHorzMode getTextHorzMode() const;

    /// @brief 设置横向对齐模式
    /// @param [in] eTextHorzMode 横向对齐模式
    void setTextHorzMode(EMTextHorzMode eTextHorzMode);

    /// @brief 获取纵向对齐模式
    /// @return 纵向对齐模式
    EMTextVertMode getTextVertMode() const;

    /// @brief 设置纵向对齐模式
    /// @param [in] eTextVertMode 纵向对齐模式
    void setTextVertMode(EMTextVertMode eTextVertMode);

    /// @brief 获取行间距比例因子
    /// @return 行间距比例因子
    double getLineSpacingFactor() const;

    /// @brief 设置行间距比例因子
    /// @param [in] dLineSpacingFactor 行间距比例因子
    void setLineSpacingFactor(const double& dLineSpacingFactor);

    /// @brief 获取行间距
    /// @return 行间距
    double getLineSpace() const;

    /// @brief 设置行间距
    /// @param [in] dLineSpace 行间距
    void setLineSpace(const double& dLineSpace);

    /// @brief 获取字符高度
    /// @return 字符高度
    double getCharHeight() const;

    /// @brief 设置字符高度
    /// @param [in] height 字符高度
    void setCharHeight(double height);

    /// @brief 获取对正方式
    /// @return 对正方式
    EMTextMode getJustification() const;

    /// @brief 设置对正方式
    /// @param [in] justification 对正方式
    void setJustification(EMTextMode justification);

    /// @brief 获取定义宽度
    /// @return 定义宽度
    double getDefineWidth() const;

    /// @brief 设置定义宽度
    /// @param [in] width 定义宽度
    void setDefineWidth(double width);

    /// @brief 获取定义高度
    /// @return 定义高度
    double getDefineHeight() const;

    /// @brief 设置定义高度
    /// @param [in] height 定义高度
    void setDefineHeight(double height);

    /// @brief 获取更新模式
    /// @return 更新模式
    EUpdateMode getUpdateMode() const;

    /// @brief 设置更新模式
    /// @param [in] updateMode 更新模式
    void setUpdateMode(EUpdateMode updateMode);

private:
    QString         m_strTextString;        ///< 文字内容
    DmTextStyle*    m_pTextStyle;           ///< 字体样式
    double          m_dAngle;               ///< 旋转角度（弧度）
    DmVector        m_ptPosition;           ///< 定位点
    EMTextHorzMode  m_eTextHorzMode;        ///< 横向对齐
    EMTextVertMode  m_eTextVertMode;        ///< 纵向对齐
    double          m_dLineSpacingFactor;   ///< 行间距比例因子。在Autocad中这个参数与行间距关联，当值为1.0时，一行总高为5.0/3.0名义高度
    double          m_dLineSpace;           ///< 行间距。字高修改后，此参数按比例修改
    double          m_dCharHeight;          ///< 字高（单个文字的高度）
    EMTextMode      m_eJustification;       ///< 对正（暂只支持左上）
    double          m_dDefineWidth;         ///< 定义宽度
    double          m_dDefineHeight;        ///< 定义高度（这个参数修改并不影响显示）
    EUpdateMode     m_updateMode;           ///< 更新模式
};

#endif // MTEXTDATA_H
