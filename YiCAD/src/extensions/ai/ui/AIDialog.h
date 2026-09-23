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

/// @file AIDialog.h
/// @brief AI 助手对话框，提供问答与建模一体化入口界面

#ifndef AIDIALOG_H
#define AIDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

/// @brief AI 助手对话框，modeless 方式显示，提供问答/建模/自动三种模式
class AIDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    explicit AIDialog(QWidget* parent = nullptr);

    /// @brief 析构函数
    ~AIDialog() override = default;

    /// @brief 获取当前模式
    /// @return 模式文本："问答" / "建模" / "自动"
    QString currentMode() const;

    /// @brief 获取当前模式索引
    /// @return 0=问答, 1=建模, 2=自动
    int modeIndex() const;

    /// @brief 设置模式
    /// @param [in] mode 模式索引：0=问答, 1=建模, 2=自动
    void setMode(int mode);

    /// @brief 向对话显示区追加一条消息（对外公开，供 DeepSeekProvider 等回写）
    /// @param [in] sender 发送者标识（如 "AI", "System"）
    /// @param [in] message 消息内容
    void appendMessage(const QString& sender, const QString& message);

    /// @brief 清空对话显示区（用于加载历史会话前清理旧内容）
    void clearChatView();

    /// @brief 设置发送按钮是否可用（AI 处理期间禁用）
    void setSendEnabled(bool enabled);

signals:
    /// @brief 用户点击发送按钮，携带输入文本和当前模式
    /// @param [in] text 用户输入的文本
    /// @param [in] mode 当前模式："问答" / "建模" / "自动"
    void sendRequested(const QString& text, const QString& mode);

    /// @brief 用户点击新会话按钮
    void newSessionRequested();

    /// @brief 用户点击配置按钮
    void configRequested();

protected:
    /// @brief 关闭事件，隐藏而非销毁（modeless 行为）
    /// @param [in] event 关闭事件
    void closeEvent(QCloseEvent* event) override;

private slots:
    /// @brief 发送按钮点击槽
    void slotSendClicked();

    /// @brief 新会话按钮点击槽
    void slotNewSessionClicked();

    /// @brief 配置按钮点击槽
    void slotConfigClicked();

    /// @brief 模式切换响应
    /// @param [in] index 模式索引
    void slotModeChanged(int index);

private:
    /// @brief 初始化 UI 布局
    void setupUI();

    /// @brief 初始化信号连接（内部占位）
    void setupConnections();


private:
    QLabel*         m_pModeLabel = nullptr;         ///< 模式标签
    QComboBox*      m_pModeCombo = nullptr;          ///< 模式下拉框
    QTextEdit*      m_pChatView = nullptr;           ///< 对话显示区（只读）
    QTextEdit*      m_pInputEdit = nullptr;          ///< 输入框
    QPushButton*    m_pBtnSend = nullptr;            ///< 发送按钮
    QPushButton*    m_pBtnNewSession = nullptr;      ///< 新会话按钮（右上角）
    QPushButton*    m_pBtnConfig = nullptr;          ///< 配置按钮（右上角）
};

#endif  // AIDIALOG_H
