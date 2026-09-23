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

/// @file AIDialog.cpp
/// @brief AI 助手对话框实现

#include "AIDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCloseEvent>
#include <QScrollBar>
#include <QPainter>

AIDialog::AIDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("AI Assistant"));
    setMinimumSize(480, 600);
    resize(520, 680);

    // modeless: 关闭时隐藏而非销毁
    setAttribute(Qt::WA_DeleteOnClose, false);

    setupUI();
    setupConnections();
}

QString AIDialog::currentMode() const
{
    return m_pModeCombo->currentText();
}

int AIDialog::modeIndex() const
{
    return m_pModeCombo ? m_pModeCombo->currentIndex() : 2;
}

void AIDialog::setMode(int mode)
{
    if (mode >= 0 && mode < m_pModeCombo->count())
    {
        m_pModeCombo->setCurrentIndex(mode);
    }
}

void AIDialog::closeEvent(QCloseEvent* event)
{
    // modeless: 关闭时隐藏
    hide();
    event->ignore();
}

void AIDialog::setupUI()
{
    auto* pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(8, 8, 8, 8);
    pMainLayout->setSpacing(6);

    // ========== 顶部：模式状态栏 ==========
    auto* pModeLayout = new QHBoxLayout();
    m_pModeLabel = new QLabel(tr("Mode:"), this);
    m_pModeCombo = new QComboBox(this);
    m_pModeCombo->addItem(tr("Q&A"));       // 0: 问答
    m_pModeCombo->addItem(tr("Modeling"));   // 1: 建模
    m_pModeCombo->addItem(tr("Auto"));       // 2: 自动
    m_pModeCombo->setCurrentIndex(2);        // 默认自动
    pModeLayout->addWidget(m_pModeLabel);
    pModeLayout->addWidget(m_pModeCombo);
    pModeLayout->addStretch();

    // 新会话按钮（右上角，配置按钮左侧）
    m_pBtnNewSession = new QPushButton(this);
    m_pBtnNewSession->setToolTip(tr("New Session"));
    {
        // 绘制新会话图标：带折角的文档 + 加号
        QPixmap pixmap(18, 18);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(painter.pen().color(), 1.5);
        painter.setPen(pen);
        // 文档主体
        painter.drawRoundedRect(2, 1, 11, 14, 2, 2);
        // 折角
        painter.drawLine(QPointF(8.5, 1), QPointF(8.5, 4.5));
        painter.drawLine(QPointF(8.5, 4.5), QPointF(13, 4.5));
        // 加号
        const qreal cx = 7.5, cy = 9.5, half = 3;
        painter.drawLine(QPointF(cx - half, cy), QPointF(cx + half, cy));
        painter.drawLine(QPointF(cx, cy - half), QPointF(cx, cy + half));
        painter.end();
        m_pBtnNewSession->setIcon(QIcon(pixmap));
    }
    pModeLayout->addWidget(m_pBtnNewSession);

    // 配置按钮放在右上角
    m_pBtnConfig = new QPushButton(tr("Config"), this);
    pModeLayout->addWidget(m_pBtnConfig);

    pMainLayout->addLayout(pModeLayout);

    // ========== 中部：对话显示区 ==========
    m_pChatView = new QTextEdit(this);
    m_pChatView->setReadOnly(true);
    m_pChatView->setPlaceholderText(tr("AI conversation will be displayed here..."));
    m_pChatView->setMinimumHeight(200);
    pMainLayout->addWidget(m_pChatView, 1);  // stretch factor = 1

    // ========== 底部：输入区 ==========
    auto* pInputGroup = new QGroupBox(tr("Input"), this);
    auto* pInputLayout = new QVBoxLayout(pInputGroup);

    // 输入框 + 发送按钮
    auto* pInputRow = new QHBoxLayout();
    m_pInputEdit = new QTextEdit(this);
    m_pInputEdit->setPlaceholderText(tr("Enter your question or modeling command..."));
    m_pInputEdit->setMaximumHeight(80);
    m_pInputEdit->setAcceptRichText(false);
    m_pInputEdit->setTabChangesFocus(true);  // Tab 切换焦点而非插入制表符

    m_pBtnSend = new QPushButton(tr("Send"), this);
    m_pBtnSend->setDefault(true);            // 回车键触发
    m_pBtnSend->setMinimumWidth(70);
    m_pBtnSend->setMaximumHeight(80);        // 与输入框高度一致

    pInputRow->addWidget(m_pInputEdit, 1);
    pInputRow->addWidget(m_pBtnSend);
    pInputLayout->addLayout(pInputRow);

    pMainLayout->addWidget(pInputGroup);
}

void AIDialog::setupConnections()
{
    // 发送按钮 → 发射 sendRequested 信号
    connect(m_pBtnSend, &QPushButton::clicked, this, &AIDialog::slotSendClicked);

    // 新会话按钮 → 发射 newSessionRequested 信号
    connect(m_pBtnNewSession, &QPushButton::clicked, this, &AIDialog::slotNewSessionClicked);

    // 配置按钮 → 发射 configRequested 信号
    connect(m_pBtnConfig, &QPushButton::clicked, this, &AIDialog::slotConfigClicked);

    // 模式切换
    connect(m_pModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AIDialog::slotModeChanged);
}

void AIDialog::clearChatView()
{
    m_pChatView->clear();
}

void AIDialog::setSendEnabled(bool enabled)
{
    m_pBtnSend->setEnabled(enabled);
    if (enabled)
    {
        m_pBtnSend->setCursor(Qt::ArrowCursor);
        m_pBtnSend->setToolTip(QString());
    }
    else
    {
        m_pBtnSend->setCursor(Qt::ForbiddenCursor);
        m_pBtnSend->setToolTip(tr("AI is processing, please wait..."));
    }
}

void AIDialog::appendMessage(const QString& sender, const QString& message)
{
    m_pChatView->moveCursor(QTextCursor::End);

    // 发送者标签
    QTextCharFormat senderFmt;
    senderFmt.setFontWeight(QFont::Bold);
    senderFmt.setForeground(sender == tr("User") ? QColor(0x2D, 0x53, 0x66) : QColor(0x66, 0x66, 0x66));
    m_pChatView->setCurrentCharFormat(senderFmt);
    m_pChatView->insertPlainText(sender + "\n");

    // 消息内容
    QTextCharFormat msgFmt;
    msgFmt.setFontWeight(QFont::Normal);
    m_pChatView->setCurrentCharFormat(msgFmt);
    m_pChatView->insertPlainText(message + "\n\n");

    // 滚动到底部
    auto* sb = m_pChatView->verticalScrollBar();
    sb->setValue(sb->maximum());
}

// ==================== 槽函数 ====================

void AIDialog::slotSendClicked()
{
    QString text = m_pInputEdit->toPlainText().trimmed();
    if (text.isEmpty())
    {
        return;
    }

    QString mode = currentMode();

    // 在对话区显示用户消息
    appendMessage(tr("User"), text);

    // 清空输入框
    m_pInputEdit->clear();
    m_pInputEdit->setFocus();

    // 发射信号（外部连接 DeepSeekProvider / IntentRouter 后处理）
    emit sendRequested(text, mode);
}

void AIDialog::slotNewSessionClicked()
{
    emit newSessionRequested();
}

void AIDialog::slotConfigClicked()
{
    emit configRequested();
}

void AIDialog::slotModeChanged(int index)
{
    Q_UNUSED(index);
    // 占位：后续可用于 UI 状态切换
}
