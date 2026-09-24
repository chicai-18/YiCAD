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

/// @file ModifyMTextCommand.h
/// @brief 多行文字属性面板命令 ext.text.modify_mtext，取代原 ActionModifyMText：空闲态选中
///        一段多行文字（或修改实体属性点了多行文字）时在主窗口左上显示属性面板
///        （UIMTextModifyOptions），每次改动即提交；在画布上单击取消选中并结束

#ifndef MODIFYMTEXTCOMMAND_H
#define MODIFYMTEXTCOMMAND_H

#include <memory>

#include <QCoreApplication>
#include <QPointer>

#include "PlaceCommand.h"

class DmDocument;
class DmMText;
class DmTextStyle;
class QWidget;

/// @brief 多行文字属性面板命令；交互由 ModifyMTextTool 驱动
class ModifyMTextCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyMTextCommand)

public:
    /// @param text 要修改的多行文字
    explicit ModifyMTextCommand(DmMText* text);
    ~ModifyMTextCommand() override;

    /// @brief 要修改的多行文字
    DmMText* text() const { return m_pMText; }

    // ---- 属性面板（UIMTextModifyOptions）：每次改动一个事务 ----

    DmDocument* getDocument() const { return document(); }
    void setHeight(double height);
    double getHeight() const;
    void setStyle(DmTextStyle* textStyle);
    DmTextStyle* getStyle() const;
    void setLineSpaceFatctor(double factor);
    double getLineSpaceFatctor() const;
    void setAngle(double angle);
    double getAngle() const;
    void setLineSpace(double lineSpace);
    double getLineSpace() const;

    // ---- 工具调用 ----

    /// @brief 在画布上单击：取消选中文字，结束，随后通知选择变化
    void deselectAndFinish();
    /// @brief 关闭属性面板（结束时）
    void closePanel();

    /// @brief 不可打断：撤销、删除等即时命令先结束它，免得继续修改被删除的文字
    bool isUninterruptible() const override { return true; }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    /// @brief 显示属性面板
    bool onStarted() override;

private:
    /// @brief 文字内容为空时按属性重新生成内容（原 updateContentIfEmpty）
    static void updateContentIfEmpty(DmMText* text);

    DmMText* m_pMText = nullptr;      ///< 要修改的多行文字
    QPointer<QWidget> m_optionBack;   ///< 属性面板的底板
};

#endif  // MODIFYMTEXTCOMMAND_H
