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


/// @file ActionModifyMText.h
/// @brief 编辑多行文字属性的交互动作类声明

#ifndef ACTIONMODIFYMTEXT_H
#define ACTIONMODIFYMTEXT_H

#include "ActionInterface.h"

class DmMText;
class DmDocument;
class UIMTextModifyOptions;
class DmTextStyle;

/// @brief 编辑多行文字属性命令
///
/// 处理多行文字（MText）的字体、高度、旋转角度、行间距等属性的编辑操作。
/// 与普通实体修改不同，该动作使用非模态选项面板，不可中断。
class ActionModifyMText : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyMText(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 判断是否可被其他Action中断
    /// @return 始终返回false，此动作不可中断
    bool canBeInterrupt() override;

    /// @brief 初始化动作，创建非模态选项面板
    /// @param [in] status 初始状态，默认为0
    void init(int status = 0) override;

    /// @brief 获取当前文档指针
    /// @return 文档指针
    DmDocument* getDocument();

    /// @brief 设置当前编辑的多行文字
    /// @param [in] editingText 多行文字实体指针
    void setText(DmMText* editingText);

    /// @brief 结束当前动作
    /// @param [in] updateTB 是否更新工具栏，默认为true
    void finish(bool updateTB = true) override;

    /// @brief 设置文字高度
    /// @param [in] height 文字高度
    void setHeight(const double height);

    /// @brief 获取当前文字高度
    /// @return 文字高度
    double getHeight() const;

    /// @brief 设置文字样式
    /// @param [in] textStyle 文字样式指针
    void setStyle(DmTextStyle* textStyle);

    /// @brief 获取当前文字样式
    /// @return 文字样式指针
    DmTextStyle* getStyle() const;

    /// @brief 设置行间距因子
    /// @param [in] factor 行间距因子
    void setLineSpaceFatctor(const double factor);

    /// @brief 获取当前行间距因子
    /// @return 行间距因子
    double getLineSpaceFatctor() const;

    /// @brief 设置旋转角度
    /// @param [in] angle 旋转角度
    void setAngle(const double angle);

    /// @brief 获取当前旋转角度
    /// @return 旋转角度
    double getAngle() const;

    /// @brief 设置行间距
    /// @param [in] lineSpace 行间距
    void setLineSpace(const double lineSpace);

    /// @brief 获取当前行间距
    /// @return 行间距
    double getLineSpace() const;

public:
    /// @brief 鼠标移动事件处理
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 鼠标双击事件处理
    /// @param [in] e 鼠标事件指针
    void mouseDoubleClickEvent(QMouseEvent* e) override;

private:
    /// @brief 如果多行文字内容为空，更新其内容
    /// @param [in] theText 多行文字实体指针
    void updateContentIfEmpty(DmMText* theText);

private:
    DmMText* m_pMText = nullptr;            ///< 当前编辑的多行文字实体
    UIMTextModifyOptions* m_option = nullptr; ///< 非模态选项面板
    QWidget* m_optionBack = nullptr;        ///< 选项面板的父窗口
};

#endif // ACTIONMODIFYMTEXT_H
