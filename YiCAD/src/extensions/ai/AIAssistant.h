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

/// @file AIAssistant.h
/// @brief AI 助手控制器，封装 AIDialog / AIPipeline 创建与信号连接

#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QObject>

class QWidget;
class DmDocument;
class GuiDocumentView;
class AIDialog;
class AIPipeline;

/// @brief AI 助手控制器
///
/// 封装 AI 对话框（AIDialog）与 AI 总调度器（AIPipeline）的
/// 懒初始化、信号连接、配置对话框等业务逻辑。
/// ApplicationWindow 仅需创建实例并通过 show() 委托即可。
class AIAssistant : public QObject
{
    Q_OBJECT

public:
    /// @param parentWindow AIDialog 的父窗口
    /// @param parent       父 QObject
    explicit AIAssistant(QWidget* parentWindow, QObject* parent = nullptr);

    ~AIAssistant() override;

    /// @brief 显示 AI 助手对话框（首次调用时创建 AIDialog + AIPipeline）
    /// @param doc     当前文档指针（可为 nullptr，仅首次调用用于 AIPipeline 初始化）
    /// @param docView 当前文档视图指针（可为 nullptr）
    void show(DmDocument* doc, GuiDocumentView* docView);

private slots:
    void onSendRequested(const QString& text, const QString& mode);
    void onConfigRequested();
    void onNewSessionRequested();
    void onPipelineResponse(const QString& sender, const QString& text);

private:
    void ensureCreated(DmDocument* doc, GuiDocumentView* docView);

    QWidget*            m_parentWindow = nullptr;
    AIDialog*           m_dialog       = nullptr;
    AIPipeline*         m_pipeline     = nullptr;
};

#endif // AIASSISTANT_H
