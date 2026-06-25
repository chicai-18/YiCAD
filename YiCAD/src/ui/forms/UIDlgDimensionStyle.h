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

/// @file UIDlgDimensionStyle.h
/// @brief 标注样式对话框

#ifndef UIDLGDIMENSIONSTYLE_H
#define UIDLGDIMENSIONSTYLE_H

#include "ui_UIDlgDimensionStyle.h"
#include "DmDimensionStyle.h"
#include <memory>

class DmDimensionStyle;
class DmDocument;

class UIDlgDimensionStyle : public QDialog, public Ui::UIDlgDimensionStyle
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIDlgDimensionStyle(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIDlgDimensionStyle() = default;

    /// @brief 初始化对话框
    /// @param [in] pStyle 标注样式指针
    /// @param [in] pDocument 文档指针
    void init(DmDimensionStyle* pStyle, DmDocument* pDocument);

protected:
    /// @brief 显示事件
    /// @param [in] e 显示事件
    virtual void showEvent(QShowEvent* e) override;

    /// @brief 大小变化事件
    /// @param [in] event 大小变化事件
    virtual void resizeEvent(QResizeEvent* event) override;

public slots:
    /// @brief 确认操作
    virtual void accept() override;

    /// @brief 取消操作
    virtual void reject() override;

    /// @brief 标签页切换槽
    /// @param [in] index 标签页索引
    void slotCurrentTabChanged(int index);

private slots:
    //线
    void slotColorChanged(DmColor color);
    void slotLineTypeChanged(DmLineType* lineType);
    void slotWidthChanged(DM::LineWidth width);
    void slotChkChanged(int state);
    void slotSpinBoxChanged(double value);
    //箭头
    void slotArrowChanged(DM::ArrowType arrowType);
    //文字
    void slotComboCurrentChanged(int index);
    void slotTextStyleChanged();
    //单位
    void slotLinearUnitFormatChanged(int idx);
    void slotAngleUnitFormatChanged(int idx);
    void slotTextChanged(QString text);

private:
    /// @brief 初始化信号连接和界面数据
    void init();

    /// @brief 获取线性标注精度列表
    /// @param [in] unitFormat 单位格式
    /// @return 精度字符串列表
    QStringList getPrecisionOfLinear(DmDimensionStyleUnitData::UnitFormat unitFormat) const;

    /// @brief 获取角度标注精度列表
    /// @param [in] unitFormat 角度单位格式
    /// @return 精度字符串列表
    QStringList getPrecisionOfAngle(DmDimensionStyleUnitData::AngleUnitFormat unitFormat) const;

    /// @brief 更新预览视图
    void updatePreview();

private:
    DmDimensionStyle*       m_pStyle;       ///< 标注样式指针
    DmDocument*             m_pDocument;    ///< 文档指针
    DmDimensionStyleData    m_tempData;     ///< 临时修改数据
    DmDimensionStyleData    m_originData;   ///< 原始数据备份
    DmEntityContainer*      m_pPreview;     ///< 预览容器
};

#endif
