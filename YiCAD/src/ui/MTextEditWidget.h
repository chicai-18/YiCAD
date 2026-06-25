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

/// @file MTextEditWidget.h
/// @brief 多行文字编辑控件，提供图形界面中多行文字的编辑、格式设置和光标管理

#ifndef MTEXTEDITWIDGET_H
#define MTEXTEDITWIDGET_H

#include "GuiPreviewWidget.h"
#include "GuiDocumentView.h"
#include "DmMText.h"
#include "MTextEditCmdManager.h"

class ActionDrawMText;
class ActionDrawMTextContext;
class QTimer;
class DmChar;

class MTextEditWidget : public GuiPreviewWidget
{
    Q_OBJECT
public:
    enum class Position
    {
        Bottom,
        Right,
        ButtomRight,
        Other
    };
    enum class Status
    {
        Normal,     ///< 正常编辑状态
        Resizing,   ///< 调整编辑框尺寸状态
        Selecting,  ///< 选择文字状态
    };
    MTextEditWidget(DmMText* mtext, GuiDocumentView* docView, ActionDrawMText* action, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    virtual ~MTextEditWidget();
    /// @brief 创建（格式刷）光标
    QCursor* createSvgCursor(const QString& svgPath, const QSize& size, const QPoint& hotSpot);
    void setCornersForNew(const DmVector& leftTop, const DmVector& rightBottom);
    void setCornersForModify(const DmVector& leftTop, const DmVector& rightBottom, const DmVector& clickPt);

    void updateCorners();
    void updateGeometry();
    /// @brief 如果编辑框高度不够，调整以能够放下文字，同时更新文字定义高度
    void increaseHeightIfNotEnough();
    bool isDrawCursor() const;
    bool isMatching() const;
    /// @brief 通过上下文创建字符
    DmChar* createChar(QString& charStr);
    /// @brief 通过前后字符的格式创建字符
    DmChar* createChar(QString& charStr, const DmChar* preChar, const DmChar* postChar);
    /// @brief 将选择的起点终点设置为null（采用命令，支持undo）
    void setSelectBeginEndToNull(bool needUndo);
    void insertTextAtCursor(const QString& str);
    /// @brief 根据指定的上下文（当前格式或格式刷的上下文）设置选择的文字
    void matchSelectedChars(const ActionDrawMTextContext* context);
    /// @brief 指定的文字是否需要替换。如果后面的参数与文字格式不匹配，则需要替换
    bool charShouldBeReplacedInMatching(DmChar* c, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic) const;
    /// @brief 按指定的格式设置一个文字
    void matchChar(std::unordered_map<DmChar*, DmChar*>& origin_replaced_map, DmChar* startChar, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& bold, const bool& italic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline);
    /// @brief 按指定的格式设置指定范围的文字
    void matchChars(DmChar* startChar, DmChar* endChar, const std::initializer_list<DmChar**>& refChars, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& bold, const bool& italic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline);
    /// @brief 获得选择的段落
    void getSelectedParas(std::vector<DmMTextParagraph*>& paras);
    void locateChar(bool needUndo = false);
    DmVector getCursorCenter() const;
    double toGuiX(double x) const;
    double toGuiY(double y) const;
    double toGraphX(double x) const;
    double toGraphY(double y) const;
    void setCursorByPosition(Position pos);
    void resizing(double x, double y);
    void selecting(double x, double y);
    void drawEditCursorImmediately(bool draw);
    /// @brief 重新生成选择遮罩
    void drawSelectCoverImmediately();
    /// @brief 立即绘制光标或选择遮罩
    void drawCursorOrCoverBySelectStateImmediately();
    /// @brief 移除光标
    void removeEditCursor(bool notShowAnymore);
    /// @brief 让光标重新闪烁
    void resumeEditCursor();
    /// @brief 移除选择遮罩
    void removeSelectCover();

    void deleteSelected();
    bool hasSelectedChar() const;
    void saveSelectedCharsToClipboard();
protected:
    void wheelEvent(QWheelEvent* e) override;
    //void enterEvent(QEvent* e) override;
    //void leaveEvent(QEvent* e) override;
    //void focusOutEvent(QFocusEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void inputMethodEvent(QInputMethodEvent* e) override;
    void closeEvent(QCloseEvent* event) override;
    //QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
protected:
    void keyPressEvent_Undo();
    void keyPressEvent_Redo();
    void keyPressEvent_Copy();
    void keyPressEvent_Cut();
    void keyPressEvent_Paste();
    void keyPressEvent_Enter();
    void keyPressEvent_Left();
    void keyPressEvent_Right();
    void keyPressEvent_Down();
    void keyPressEvent_Up();
    void keyPressEvent_Delete();
    void keyPressEvent_Backspace();
    void keyPressEvent_Escape();
protected:
    void resizeGL(int w, int h) override;
    /// @brief 为了能接受tab按键
    bool focusNextPrevChild(bool next) override;
private:
    Position getPosition(int x, int y);
    void setStatus(Status s);
    Status getStatus() const;
    void setEditCursorPos(const int x, const int y, const bool init = false);
    /// @brief 更新当前光标坐标信息并绘制光标
    /// @param updateToOption [in] 是否更新到选项。根据需要设置，避免造成死循环
    void setEditCursorPos(const bool& updateToOption = true);
    void updateCharFormatToContext(const DmChar* c);
    void setSelectBegin();
    void setSelectEnd();
private slots:
    void slotViewChanged();
    void slotTimerTrigger();
private slots:
    // 选项编辑修改后的响应
    void slotTextStyleChanged();
    /// @brief 选项中一般格式变化的响应
    void slotFormatChanged();
    void slotJustificationChanged();
    void slotParaAlignmentChanged();
    void slotAddSymbol(const QString& symbol);
    void slotMatch();
    /// @brief 选项中触发undo
    void slotUndo();
    /// @brief 选项中触发redo
    void slotRedo();
    /// @brief 发生命令commit或undo，redo时响应
    void slotCmdChanged();

    friend class MTextEdit_SetSelectBeginEndToNull_Cmd;
    friend class MTextEdit_ResizeCmd;

private:
    GuiDocumentView*                    m_pDocumentView = nullptr;
    DmMText*                            m_pMText = nullptr;
    ActionDrawMText*                    m_pAction = nullptr;
    ActionDrawMTextContext*             m_pContext = nullptr;                           ///< 当前上下文，所有权在ActionDrawMText
    std::unique_ptr<ActionDrawMTextContext> m_matchContext;                             ///< 格式刷上下文
    DmVector                            m_leftTop;                                      ///< 编辑窗体左上角世界坐标
    DmVector                            m_rightBottom;                                  ///< 编辑窗体右下角世界坐标
    Status                              m_curStatus = Status::Normal;
    Position                            m_lastPos = Position::Other;

    DmEntityContainer*                  m_pCursor = nullptr;                            ///< 光标实体
    DmEntityContainer*                  m_pSelectCover = nullptr;                       ///< 选择文字时的遮罩实体
    std::unique_ptr<QTimer>             m_timer;                                        ///< 显示光标用的定时器
    std::unique_ptr<QCursor>            m_matchCurxor;                                  ///< 格式刷光标
    DmVector                            m_currentCursorPos;                             ///< 当前光标的位置（下部中心）
    DmChar*                             m_cursorPreChar = nullptr;                      ///< 光标位置处前一个文字
    DmChar*                             m_cursorPostChar = nullptr;                     ///< 光标位置处后一个文字
    //int m_cursorIdx;                    //光标位置的索引
    DmChar*                             m_selectBeginPreChar = nullptr;                 ///< 选择起始位置前一字符
    DmChar*                             m_selectBeginPostChar = nullptr;                ///< 选择起始位置后一字符
    DmChar*                             m_selectEndPreChar = nullptr;                   ///< 选择结束位置前一字符
    DmChar*                             m_selectEndPostChar = nullptr;                  ///< 选择结束位置后一字符

    MTextEditCmdManager                 m_cmdManager;                                   ///< 命令管理器

    // 画布系数相关
    DmVector                            m_factor;                                       ///< 单位世界距离占用的像素距离
    double                              m_dMaxScale = 0;                                ///< 画布最大允许放缩比例

};
#endif // MTEXTEDITWIDGET_H