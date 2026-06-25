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


/// @file ActionBlocksEdit.h
/// @brief 编辑块定义的动作类头文件

#ifndef ACTIONBLOCKSEDIT_H
#define ACTIONBLOCKSEDIT_H

#include "ActionInterface.h"
#include "DmBlockReference.h"

class MacroCmd;
class DmBlock;


/// @brief 编辑块定义的动作类
class ActionBlocksEdit : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum EStatus
    {
        eEditing = 0 ///< 编辑中
    };

    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    /// @param selectedRef 选中的块参照，默认为nullptr
    ActionBlocksEdit(DmDocument* doc, GuiDocumentView* docView,
        DmBlockReference* selectedRef = nullptr);

    /// @brief 析构函数
    ~ActionBlocksEdit() override;

    /// @brief 初始化动作
    /// @param status 状态参数，默认为0
    void init(int status = 0) override;

    /// @brief 触发动作执行
    void trigger() override;

    /// @brief 鼠标移动事件处理
    /// @param e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标按下事件处理
    /// @param e 鼠标事件指针
    void mousePressEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 键盘按下事件处理
    /// @param e 键盘事件指针
    void keyPressEvent(QKeyEvent* e) override;

    /// @brief 检查是否可以中断
    /// @return true表示可以中断，false表示不可以中断
    bool canBeInterrupt() override;

    /// @brief 检查是否为独占动作
    /// @return true表示独占，false表示非独占
    bool isExclusive() override;

    /// @brief 检查是否为子动作
    /// @return true表示是子动作，false表示不是子动作
    bool isSubAction() override;

    /// @brief 显示选项栏
    void showOptions() override;

    /// @brief 隐藏选项栏
    void hideOptions() override;

    /// @brief 更新鼠标按钮提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标
    void updateMouseCursor() override;

    /// @brief 完成编辑
    /// @param save 是否保存修改（更新块参照）
    void completeEditing(bool save);

    /// @brief 取消编辑，丢弃所有修改
    void cancelEditing();

    /// @brief 获取正在编辑的块名称
    /// @return 块名称
    QString getBlockName() const { return m_blockName; }

    /// @brief 是否为嵌套块编辑
    bool isNestedEdit() const { return m_isNestedEdit; }

    /// @brief 获取顶层块名称（嵌套编辑时为用户选中的块参照对应的块名）
    QString getTopLevelBlockName() const { return m_topLevelBlockName; }

    /// @brief 检测自进入编辑以来是否有修改
    /// @return true表示有修改
    bool hasModifications() const;

private:
    /// @brief 正常进入编辑模式
    /// @param blockRef 块参照指针
    void enterEditing(DmBlockReference* blockRef);

    /// @brief 重新进入编辑模式（通过Undo/Redo）
    /// @param block 块定义指针
    void reenterEditing(DmBlock* block);

private:
    DmBlockReference* m_blockRefBeingEdited; ///< 被选中的块参照（初始进入时使用）
    QString m_blockName;                     ///< 编辑的块名称

    MacroCmd* m_enterMacroCmd = nullptr;     ///< BlockEditEnterCmd 所在 MacroCmd（用于 cancel 定位）
    size_t m_undoCountAtEnter = 0;           ///< 进入编辑时的 undo 栈深度（用于检测修改）
    DmBlock* m_editingBlock = nullptr;       ///< 缓存的块定义指针
    bool m_isReentry = false;               ///< 是否通过 undo/redo 重新进入
    bool m_isNestedEdit = false;            ///< 是否为嵌套块编辑
    QString m_topLevelBlockName;            ///< 顶层块名称（嵌套编辑时有效）
};

#endif
