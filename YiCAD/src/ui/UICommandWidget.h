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

/// @file UICommandWidget.h
/// @brief 命令行输入控件，提供命令输入、自动补全和历史记录功能

#ifndef UICOMMANDWIDGETNEW
#define UICOMMANDWIDGETNEW

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QToolButton>
#include <QStringList>
#include "DmVector.h"
#include "MDIWindow.h"
#include "qevent.h"
#include "UITabDrawWidget.h"

class UIActionHandler;
class QPropertyAnimation;

class UICommandWidget : public QWidget
{
    Q_OBJECT
public:
    UICommandWidget(QWidget* parent, UITabDrawWidget* tabDrawWidget);
    ~UICommandWidget();

    void createTriangleBtn();
    void createLineEdit();
    void createTempWin(QLineEdit* e);

    void setCompleterStrings(const QStringList& strs);

    /// @brief 设置外部规范命令，并与内置命令共同用于自动补全。
    void setExternalCommandStrings(const QStringList& commands);
    QWidget* createTempEdit();
    QWidget* getCommandWidget();
    QWidget* getInfoWidget();
    QWidget* getTempWidget();
    QWidget* getTipWidget();

    void appCmdTempText(const QString text);

    void setActionHandler(UIActionHandler* pActionHandler);

    void doNothing();

    QLineEdit* getEditline();
    QLineEdit* getEdit();
    QTextEdit* getTipWin();

private slots:
    void pressShowTextEdit();
    void pressShowLineEdit();
    void btnShowTextEdit();

private:
    void updateCompleterModel();

    QCompleter*                     m_pCompleter = nullptr;
    QLineEdit*                      m_editline = nullptr;
    QLineEdit*                      m_pEdit = nullptr;                      ///< 输入栏
    QTextEdit*                      m_pTipWin = nullptr;

    QWidget*                        m_pWidget = nullptr;
    QWidget*                        m_cmdWin = nullptr;                     ///< 输入栏背板
    QWidget*                        m_infoWin = nullptr;                    ///< 信息保存栏
    QWidget*                        m_pCmdTempWin = nullptr;                ///< 临时信息显示栏
    QTextEdit*                      m_pTempTextEdit = nullptr;
    QTextEdit*                      m_infoTextEdit = nullptr;
    QString                         m_LineText;
    QString                         m_pLastText;                            ///< 记录上一次输入的值
    QString                         m_cmdInfo;
    UIActionHandler*                m_pActionHandler = nullptr;
    QWidget*                        editWidget = nullptr;
    QWidget*                        m_pTipWidget = nullptr;
    QString                         m_Coord;
    UITabDrawWidget*                m_pTabDrawWidget = nullptr;
    std::unique_ptr<QPropertyAnimation> m_pAnimation;
    QStringList                     m_completerStrings;
    QStringList                     m_externalCommandStrings;
};
#endif // UICOMMANDWIDGETNEW
