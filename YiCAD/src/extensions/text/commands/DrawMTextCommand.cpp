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

/// @file DrawMTextCommand.cpp
/// @brief DrawMTextCommand 与多行文字工具的实现（提交、放弃与界面的收放从原 ActionDrawMText 搬来）

#include "DrawMTextCommand.h"

#include <algorithm>

#include <QMessageBox>
#include <QMouseEvent>
#include <QWidget>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "DmColor.h"
#include "DmDocument.h"
#include "DmLineTypeTable.h"
#include "DmMText.h"
#include "DmPen.h"
#include "DmPolyline.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "MTextEditContext.h"
#include "MTextEditWidget.h"
#include "Preview.h"
#include "SelectionSet.h"
#include "TextConsts.h"
#include "Transaction.h"
#include "UIMTextOptions.h"

namespace
{
constexpr int BOUNDING_BOX_PREVIEW_RGB = 255; ///< 编辑框预览的颜色分量（白色）
constexpr double DEFAULT_DEFINE_WIDTH = 100.0; ///< 就地编辑时定义宽度为 0 的文字取的宽度

constexpr int OPTION_PANEL_X = 1;      ///< 选项条在主窗口里的位置与大小（与原 Action 相同）
constexpr int OPTION_PANEL_Y = 186;
constexpr int OPTION_PANEL_WIDTH = 655;
constexpr int OPTION_PANEL_HEIGHT = 90;

/// @brief 多行文字工具：拉编辑框的两个角点，编辑中在画布上按下提交
class DrawMTextTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        DrawingBoundingBox, ///< 拉编辑框
        Editing             ///< 编辑中
    };

    DrawMTextTool(DrawMTextCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    /// @brief 进入编辑
    void setEditing() { setStatus(Editing); }
    bool isEditing() const { return status() == Editing; }

    /// @brief 编辑时用系统箭头光标（十字光标由命令关掉）
    std::optional<DM::CursorType> getCursor() const override
    {
        return status() == Editing ? DM::ArrowCursor : DM::CadCursor;
    }

protected:
    /// @brief 原 Action 只在拉编辑框时有提示；编辑时保持原样
    void updateHints() override
    {
        if (status() != DrawingBoundingBox)
        {
            return;
        }
        if (!m_first.valid)
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawMTextCommand::tr("Specify first point of edit box"),
                                                DrawMTextCommand::tr("Cancel"));
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(DrawMTextCommand::tr("Specify second point of edit box"),
                                                DrawMTextCommand::tr("Cancel"));
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() == DrawingBoundingBox && m_first.valid)
        {
            m_command.previewBox(m_first, snapper()->snapPoint(e));
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (status() != DrawingBoundingBox)
        {
            return;
        }
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            command().finish();
        }
    }

    /// @brief 编辑中在画布上按下（编辑框里的按下归编辑框）：提交并结束
    void onMousePress(QMouseEvent*) override
    {
        if (status() == Editing)
        {
            m_command.commitAndFinish();
        }
    }

    void onCoordinate(const DmVector& coord) override
    {
        if (status() != DrawingBoundingBox)
        {
            return;
        }
        if (!m_first.valid)
        {
            m_first = coord;
            updateHints();
            return;
        }
        // 左上、右下角点
        const DmVector topLeft(std::min(m_first.x, coord.x), std::max(m_first.y, coord.y));
        const DmVector bottomRight(std::max(m_first.x, coord.x), std::min(m_first.y, coord.y));
        m_command.startEditing(topLeft, bottomRight);
    }

    /// @brief 结束时关掉编辑框与选项条（任何结束方式都经过这里）
    void onFinish() override;

private:
    DrawMTextCommand& m_command;
    DmVector m_first{false}; ///< 编辑框的第一个角点
};
}  // namespace

DrawMTextCommand::DrawMTextCommand()
    : m_isModify(false)
{
}

DrawMTextCommand::DrawMTextCommand(DmMText* originText, const DmVector& clickPt)
    : m_isModify(true)
    , m_clickPt(clickPt)
    , m_pOriginText(originText)
{
    m_pEditingText = static_cast<DmMText*>(originText->clone());
    // 文字不正时放正后编辑，提交时再转回去
    const double angle = m_pEditingText->getDataConstPtr()->getAngle();
    if (angle != 0.0)
    {
        const DmVector pos = m_pEditingText->getDataConstPtr()->getPosition();
        m_pEditingText->moveEntities(-pos);
        m_pEditingText->rotateEntities(DmVector(0.0, 0.0), -angle);
        m_pEditingText->moveEntities(pos);
    }
}

DrawMTextCommand::~DrawMTextCommand()
{
    freeUI();
    // 提交或放弃时已交给文档或释放；否则（如没有进入编辑）在这里释放
    delete m_pEditingText;
}

std::unique_ptr<BasePlaceTool> DrawMTextCommand::createTool()
{
    return std::make_unique<DrawMTextTool>(*this, document(), view());
}

bool DrawMTextCommand::onStarted()
{
    if (m_isModify)
    {
        initDisplayDialogs();
    }
    return true;
}

MTextEditContext* DrawMTextCommand::getContext()
{
    return m_context.get();
}

void DrawMTextCommand::focusEditWidget()
{
    if (m_pEditWidget)
    {
        m_pEditWidget->setFocus();
    }
}

bool DrawMTextCommand::isEditing() const
{
    auto* tool = static_cast<DrawMTextTool*>(placeTool());
    return tool && tool->isEditing() && !m_done;
}

void DrawMTextCommand::previewBox(const DmVector& corner1, const DmVector& corner2)
{
    preview().clear();
    const DmColor color(BOUNDING_BOX_PREVIEW_RGB, BOUNDING_BOX_PREVIEW_RGB, BOUNDING_BOX_PREVIEW_RGB);
    auto* poly = new DmPolyline(nullptr, PolylineData());
    poly->setPen(DmPen(color, DM::Width00, DmLineTypeTable::Continuous));
    poly->appendVertex(corner1);
    poly->appendVertex(DmVector(corner2.x, corner1.y));
    poly->appendVertex(corner2);
    poly->appendVertex(DmVector(corner1.x, corner2.y));
    poly->setClosed(true);
    poly->update();
    preview().entities().addEntity(poly);
    preview().draw();
}

void DrawMTextCommand::startEditing(const DmVector& topLeft, const DmVector& bottomRight)
{
    m_pos = topLeft;
    m_secPos = bottomRight;
    initDisplayDialogs();
}

void DrawMTextCommand::initDisplayDialogs()
{
    auto* docView = dynamic_cast<GuiDocumentView*>(view());
    if (!docView)
    {
        // 没有真正的画布（测试用的假视图）：无法编辑
        m_done = true;
        finish();
        return;
    }
    preview().clear();

    // 隐藏原来的文字
    if (m_isModify)
    {
        m_trans = std::make_shared<TransactionGroup>(tr("Modify MText").toStdString(), document());
        m_trans->start();
        Transaction t(tr("Hind origin MText").toStdString(), document());
        t.start();
        document()->getEntityTable()->startModify(m_pOriginText);
        m_pOriginText->setVisible(false);
        t.commit();
    }
    else
    {
        m_trans = std::make_shared<TransactionGroup>(tr("Create MText").toStdString(), document());
        m_trans->start();
    }

    m_context = std::make_unique<MTextEditContext>();
    m_context->init(document());
    // Esc 在编辑框的按键处理里发出；结束请求在分发之外，由总线延后执行
    QObject::connect(m_context.get(), &MTextEditContext::escPressed, [this](bool save) { onEscPressed(save); });

    // 选项条挂在主窗口上
    m_pOptionBack = new QWidget(docView->window());
    m_pOptionBack->setObjectName("mTextOptionBackWidget");
    auto* options = new UIMTextOptions(m_pOptionBack);
    options->setCommand(this);
    m_pOptionBack->setGeometry(OPTION_PANEL_X, OPTION_PANEL_Y, OPTION_PANEL_WIDTH, OPTION_PANEL_HEIGHT);
    m_pOptionBack->show();

    // 编辑框
    if (m_isModify)
    {
        const DmVector pos = m_pEditingText->getPosition();
        m_pos = pos;
        auto* mtextData = const_cast<MTextData*>(m_pEditingText->getDataConstPtr());
        double defineHeight = mtextData->getDefineHeight();
        const double minHeight = m_pEditingText->calculateHeight();
        if (defineHeight < minHeight)
        {
            // 定义高度比较小时用真实高度代替
            defineHeight = minHeight;
            mtextData->setDefineHeight(defineHeight);
        }
        double defineWidth = mtextData->getDefineWidth();
        if (defineWidth == 0.0)
        {
            defineWidth = DEFAULT_DEFINE_WIDTH;
            mtextData->setDefineWidth(defineWidth);
        }
        m_secPos = pos + DmVector(defineWidth, -defineHeight);
        m_pEditWidget = new MTextEditWidget(m_pEditingText, docView, this, docView);
        m_pEditWidget->setCornersForModify(m_pos, m_secPos, m_clickPt);
    }
    else
    {
        DmTextStyle* activeStyle = document()->getTextStyleTable()->getActive();
        const double defaultHeight = activeStyle->getValidDefaultHeight();
        const double defineWidth = m_secPos.x - m_pos.x;
        MTextData data(m_pos, defaultHeight, EMTextVertMode::kTextTop, EMTextHorzMode::kTextLeft,
                       LINE_HEIGHT_PER_CHAR_HEIGHT * defaultHeight, defineWidth, "", activeStyle, 0.0);
        m_pEditingText = new DmMText(nullptr, data);
        m_pEditingText->setDocument(document());
        m_pEditWidget = new MTextEditWidget(m_pEditingText, docView, this, docView);
        m_pEditWidget->setCornersForNew(m_pos, m_secPos);
    }

    m_pEditWidget->setFocus();
    m_pEditWidget->show();
    // 系统箭头光标，关掉渲染绘制的十字光标
    view()->setCursor(Qt::ArrowCursor);
    view()->setIsDrawCursor(false);
    static_cast<DrawMTextTool*>(placeTool())->setEditing();
}

void DrawMTextCommand::commit()
{
    preview().clear();

    // 文字不正时编辑前放正了，现在转回去
    const double angle = m_pEditingText->getDataConstPtr()->getAngle();
    if (angle != 0.0)
    {
        const DmVector pos = m_pEditingText->getDataConstPtr()->getPosition();
        m_pEditingText->moveEntities(-pos);
        m_pEditingText->rotateEntities(DmVector(0.0, 0.0), angle);
        m_pEditingText->moveEntities(pos);
    }

    if (!m_pEditingText->isEmptyText())
    {
        if (!m_isModify)
        {
            Transaction t(tr("Create MText").toStdString(), document());
            t.start();
            m_pEditingText->updateContent();
            m_pEditingText->update();
            document()->getEntityTable()->add(m_pEditingText);
            t.commit();
            // 文字已交给文档
            m_pEditingText = nullptr;
        }
        else
        {
            Transaction t(tr("Modify MText").toStdString(), document());
            t.start();
            m_pEditingText->updateContent();
            document()->getEntityTable()->startModify(m_pOriginText);
            m_pOriginText->setData(m_pEditingText->getData());
            m_pOriginText->setVisible(true);
            selection()->remove(m_pOriginText);
            m_pOriginText->update();
            t.commit();
            delete m_pEditingText;
            m_pEditingText = nullptr;
        }
    }
    else
    {
        // 文字为空：修改时删除原文字，新建时什么也不做
        if (m_isModify)
        {
            Transaction t(tr("Delete MText").toStdString(), document());
            t.start();
            document()->getEntityTable()->remove(m_pOriginText);
            t.commit();
        }
        delete m_pEditingText;
        m_pEditingText = nullptr;
    }

    m_trans->commit();
    m_done = true;
}

void DrawMTextCommand::cancel()
{
    if (m_pOriginText && m_trans)
    {
        m_trans->rollback();
    }
    delete m_pEditingText;
    m_pEditingText = nullptr;
    m_done = true;
}

void DrawMTextCommand::commitAndFinish()
{
    if (!isEditing())
    {
        return;
    }
    commit();
    freeUI();
    finish();
}

void DrawMTextCommand::onEscPressed(bool save)
{
    if (!isEditing())
    {
        return;
    }
    if (save)
    {
        commit();
    }
    else
    {
        cancel();
    }
    freeUI();
    finish();
}

bool DrawMTextCommand::onEndRequested(CommandEndReason)
{
    if (isEditing())
    {
        const auto button = QMessageBox::critical(nullptr, tr("Tips"), tr("Save the changes?"),
                                                  QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                                                  QMessageBox::StandardButton::Yes);
        if (button == QMessageBox::StandardButton::Yes)
        {
            commit();
        }
        else
        {
            cancel();
        }
        freeUI();
        // 结束全部命令时块编辑可能否决；已经提交或放弃，自己结束
        finish();
    }
    return true;
}

void DrawMTextCommand::freeUI()
{
    const bool hadUi = m_pEditWidget || m_pOptionBack;
    if (m_pEditWidget)
    {
        // 编辑框设了 WA_DeleteOnClose，关闭即释放（直接删除不行）
        m_pEditWidget->close();
        m_pEditWidget.clear();
    }
    if (m_pOptionBack)
    {
        m_pOptionBack->close();
        m_pOptionBack->deleteLater();
        m_pOptionBack.clear();
    }
    if (hadUi && view())
    {
        view()->setIsDrawCursor(true);
    }
}

void DrawMTextTool::onFinish()
{
    m_command.freeUI();
}
