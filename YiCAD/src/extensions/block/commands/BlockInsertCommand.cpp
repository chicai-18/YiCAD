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

/// @file BlockInsertCommand.cpp
/// @brief BlockInsertCommand 与插入块工具的实现

#include "BlockInsertCommand.h"

#include <list>

#include <QDialog>
#include <QMouseEvent>
#include <QStringList>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "Commands.h"
#include "DmAttribute.h"
#include "DmAttributeDefinition.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmColor.h"
#include "DmDocument.h"
#include "DmLineTypeTable.h"
#include "DmPen.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"
#include "UIBlockListWidget.h"
#include "UIDialogRunner.h"
#include "UIDlgEditAttributes.h"

namespace
{
constexpr double DEFAULT_SCALE = 1.0; ///< 默认比例与阵列间距
constexpr double DEFAULT_ANGLE = 0.0; ///< 默认旋转角
constexpr int DEFAULT_COUNT = 1;      ///< 默认阵列行列数

constexpr int BLOCK_LIST_WIDTH = 300;       ///< 块列表对话框宽度
constexpr int BLOCK_LIST_HEIGHT = 600;      ///< 块列表对话框高度
constexpr double DIALOG_CENTER_RATIO = 0.5; ///< 块列表垂直居中的位置系数

/// @brief 默认的块参照数据（原 ActionBlocksInsert::reset）
std::unique_ptr<DmBlockReferenceData> defaultData()
{
    return std::make_unique<DmBlockReferenceData>("", DmVector(0.0, 0.0), DmVector(DEFAULT_SCALE, DEFAULT_SCALE),
                                                  DEFAULT_ANGLE, DEFAULT_COUNT, DEFAULT_COUNT,
                                                  DmVector(DEFAULT_SCALE, DEFAULT_SCALE), nullptr, DM::Update);
}

/// @brief 插入块工具：选块阶段等待块列表，放置阶段指定插入点，命令行可改选项
class BlockInsertTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        ChooseBlock,      ///< 在块列表里选块（原"插入准备"）
        SetTargetPoint,   ///< 指定插入点
        SetAngle,         ///< 在命令行设置角度
        SetFactor,        ///< 在命令行设置比例
        SetColumns,       ///< 在命令行设置列数
        SetRows,          ///< 在命令行设置行数
        SetColumnSpacing, ///< 在命令行设置列间距
        SetRowSpacing     ///< 在命令行设置行间距
    };

    BlockInsertTool(BlockInsertCommand& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

    /// @brief 开始放置
    void startPlacing() { restart(SetTargetPoint); }
    /// @brief 回到选块
    void startChoosing() { restart(ChooseBlock); }

    std::optional<DM::CursorType> getCursor() const override
    {
        return status() == ChooseBlock ? DM::ArrowCursor : DM::CadCursor;
    }

protected:
    void updateHints() override;

    void onMouseMove(QMouseEvent* e) override
    {
        if (status() == ChooseBlock)
        {
            // 原"插入准备"：不画捕捉标记
            snapper()->deleteSnapper();
            return;
        }
        if (status() == SetTargetPoint)
        {
            m_command.previewInsert(snapper()->snapPoint(e));
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (status() == ChooseBlock)
        {
            // 原"插入准备"：在画布上单击（任意键）结束
            command().finish();
            return;
        }
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            if (status() == SetTargetPoint)
            {
                m_command.backToChoosing();
            }
            else
            {
                restart(status() - 1);
            }
        }
    }

    /// @brief 放置阶段的任何一步都以该点插入（与原 Action 一致）
    void onCoordinate(const DmVector& pos) override
    {
        if (status() != ChooseBlock)
        {
            m_command.commitInsert(pos);
        }
    }

    void onCommand(GuiCommandEvent* e) override;

    void onFinish() override { m_command.closeBlockList(); }

private:
    QStringList availableCommands() const;

    /// @brief 进入某个命令行输入步骤，记下原来的一步
    void enterInput(Status s)
    {
        m_command.preview().clear();
        m_lastStatus = static_cast<Status>(status());
        setStatus(s);
    }

    /// @brief 命令行输入完成：刷新选项条，回到原来的一步
    void leaveInput()
    {
        GUIDIALOGFACTORY->requestCommandOptions(&m_command, true, true);
        setStatus(m_lastStatus);
    }

    BlockInsertCommand& m_command;
    Status m_lastStatus = SetTargetPoint; ///< 进入命令行输入前的一步
};

void BlockInsertTool::updateHints()
{
    switch (status())
    {
    case ChooseBlock:
        // 原"插入准备"没有按键提示；清空，免得留着放置阶段的提示
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    case SetTargetPoint:
        GUIDIALOGFACTORY->updateMouseWidget(BlockInsertCommand::tr("Specify reference point"),
                                            BlockInsertCommand::tr("Cancel"));
        break;
    case SetAngle:
        GUIDIALOGFACTORY->updateMouseWidget(BlockInsertCommand::tr("Enter angle:"), "");
        break;
    case SetFactor:
        GUIDIALOGFACTORY->updateMouseWidget(BlockInsertCommand::tr("Enter factor:"), "");
        break;
    case SetColumns:
        GUIDIALOGFACTORY->updateMouseWidget(BlockInsertCommand::tr("Enter columns:"), "");
        break;
    case SetRows:
        GUIDIALOGFACTORY->updateMouseWidget(BlockInsertCommand::tr("Enter rows:"), "");
        break;
    case SetColumnSpacing:
        GUIDIALOGFACTORY->updateMouseWidget(BlockInsertCommand::tr("Enter column spacing:"), "");
        break;
    case SetRowSpacing:
        GUIDIALOGFACTORY->updateMouseWidget(BlockInsertCommand::tr("Enter row spacing:"), "");
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @note 与原 Action 一样不接受命令行文本：文本随后还会被当作新命令解析
void BlockInsertTool::onCommand(GuiCommandEvent* e)
{
    const QString c = e->getCommand().toLower();

    if (Commands::checkCommand("help", c))
    {
        GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands() + availableCommands().join(", "));
        return;
    }

    bool ok = false;
    switch (status())
    {
    case SetTargetPoint:
        // Commands::checkCommand 对 help/close/undo 以外的关键字都返回 true：
        // 排在前面的 "angle" 接住了所有文本（原有行为）
        if (Commands::checkCommand("angle", c))
        {
            enterInput(SetAngle);
        }
        else if (Commands::checkCommand("factor", c))
        {
            enterInput(SetFactor);
        }
        else if (Commands::checkCommand("columns", c))
        {
            enterInput(SetColumns);
        }
        else if (Commands::checkCommand("rows", c))
        {
            enterInput(SetRows);
        }
        else if (Commands::checkCommand("columnspacing", c))
        {
            enterInput(SetColumnSpacing);
        }
        else if (Commands::checkCommand("rowspacing", c))
        {
            enterInput(SetRowSpacing);
        }
        break;

    case SetAngle:
    {
        const double a = Math2d::eval(c, &ok);
        if (ok)
        {
            m_command.setAngle(Math2d::deg2rad(a));
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(BlockInsertCommand::tr("Not a valid expression"));
        }
        leaveInput();
        break;
    }

    case SetFactor:
    {
        const double f = Math2d::eval(c, &ok);
        if (ok)
        {
            m_command.setFactor(f);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(BlockInsertCommand::tr("Not a valid expression"));
        }
        leaveInput();
        break;
    }

    case SetColumns:
    {
        const int cols = static_cast<int>(Math2d::eval(c, &ok));
        if (ok)
        {
            m_command.setColumns(cols);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(BlockInsertCommand::tr("Not a valid expression"));
        }
        leaveInput();
        break;
    }

    case SetRows:
    {
        const int rows = static_cast<int>(Math2d::eval(c, &ok));
        if (ok)
        {
            m_command.setRows(rows);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(BlockInsertCommand::tr("Not a valid expression"));
        }
        leaveInput();
        break;
    }

    case SetColumnSpacing:
    {
        const double cs = Math2d::eval(c, &ok);
        if (ok)
        {
            m_command.setColumnSpacing(cs);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(BlockInsertCommand::tr("Not a valid expression"));
        }
        leaveInput();
        break;
    }

    case SetRowSpacing:
    {
        const double rs = Math2d::eval(c, &ok);
        if (ok)
        {
            m_command.setRowSpacing(rs);
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(BlockInsertCommand::tr("Not a valid expression"));
        }
        leaveInput();
        break;
    }

    default:
        break;
    }
}

QStringList BlockInsertTool::availableCommands() const
{
    QStringList cmd;
    if (status() == SetTargetPoint)
    {
        cmd += Commands::command("angle");
        cmd += Commands::command("factor");
        cmd += Commands::command("columns");
        cmd += Commands::command("rows");
        cmd += Commands::command("columnspacing");
        cmd += Commands::command("rowspacing");
    }
    return cmd;
}
}  // namespace

BlockInsertCommand::BlockInsertCommand()
    : m_data(defaultData())
{
}

BlockInsertCommand::~BlockInsertCommand()
{
    closeBlockList();
}

double BlockInsertCommand::angle() const
{
    return m_data->angle;
}

void BlockInsertCommand::setAngle(double a)
{
    m_data->angle = a;
}

double BlockInsertCommand::factor() const
{
    return m_data->scaleFactor.x;
}

void BlockInsertCommand::setFactor(double f)
{
    m_data->scaleFactor = DmVector(f, f);
}

int BlockInsertCommand::columns() const
{
    return m_data->cols;
}

void BlockInsertCommand::setColumns(int c)
{
    m_data->cols = c;
}

int BlockInsertCommand::rows() const
{
    return m_data->rows;
}

void BlockInsertCommand::setRows(int r)
{
    m_data->rows = r;
}

double BlockInsertCommand::columnSpacing() const
{
    return m_data->spacing.x;
}

void BlockInsertCommand::setColumnSpacing(double cs)
{
    m_data->spacing.x = cs;
}

double BlockInsertCommand::rowSpacing() const
{
    return m_data->spacing.y;
}

void BlockInsertCommand::setRowSpacing(double rs)
{
    m_data->spacing.y = rs;
}

std::unique_ptr<BasePlaceTool> BlockInsertCommand::createTool()
{
    return std::make_unique<BlockInsertTool>(*this, document(), view());
}

bool BlockInsertCommand::onStarted()
{
    // 块列表挂在主窗口上、靠右居中（原 ActionBlockInsertPrepare::init）；测试用的假视图没有窗口
    QWidget* canvas = view()->asQObject() ? qobject_cast<QWidget*>(view()->asQObject()) : nullptr;
    QWidget* window = canvas ? canvas->window() : nullptr;
    if (!window)
    {
        return true;
    }
    auto* dialog = new QDialog(window);
    dialog->setWindowTitle(QObject::tr("Block List"));
    dialog->setFixedSize(BLOCK_LIST_WIDTH, BLOCK_LIST_HEIGHT);
    dialog->move(window->width() - dialog->width(),
                 static_cast<int>((window->height() - dialog->height()) * DIALOG_CENTER_RATIO));
    auto* list = new UIBlockListWidget([this](DmBlock* block) { chooseBlock(block); }, dialog, "Block");
    list->setBlockList(document()->getBlockTable(), view()->graphicsModel());
    list->resize(dialog->size());
    dialog->show();
    m_blockList = dialog;
    return true;
}

void BlockInsertCommand::showOptions()
{
    if (m_block)
    {
        GUIDIALOGFACTORY->requestCommandOptions(this, true);
    }
}

void BlockInsertCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

void BlockInsertCommand::chooseBlock(DmBlock* block)
{
    auto* tool = static_cast<BlockInsertTool*>(placeTool());
    if (!isActive() || !tool || !block)
    {
        return;
    }
    document()->getBlockTable()->activate(block);
    preview().clear();

    // 原 ActionBlocksInsert::init：选项复位，随后选项条按保存的设置重新写入
    m_data = defaultData();
    m_block = block;
    const QString blockName = block->getName();
    m_data->name = blockName;
    const QStringList chain = block->findNestedInsert(blockName);
    if (!chain.empty())
    {
        GUIDIALOGFACTORY->commandMessage(blockName + tr(" has nested insert of current block in:\n") +
                                         chain.join("->") + tr("\nThis block cannot be inserted."));
        backToChoosing();
        return;
    }
    tool->startPlacing();
    showOptions();
}

void BlockInsertCommand::backToChoosing()
{
    m_block = nullptr;
    hideOptions();
    preview().clear();
    if (auto* tool = static_cast<BlockInsertTool*>(placeTool()))
    {
        tool->startChoosing();
    }
}

void BlockInsertCommand::closeBlockList()
{
    if (m_blockList)
    {
        m_blockList->hide();
        m_blockList->deleteLater();
        m_blockList.clear();
    }
}

void BlockInsertCommand::previewInsert(const DmVector& pos)
{
    preview().clear();
    if (!m_block)
    {
        return;
    }
    m_data->insertionPoint = pos;
    m_data->updateMode = DM::PreviewUpdate;
    auto* ref = new DmBlockReference(nullptr, *m_data);
    ref->setDocument(document());
    if (m_block->hasAttributeDefinitions())
    {
        std::list<DmAttribute*> previewAttrs;
        for (DmAttributeDefinition* def : m_block->getAttributeDefinitions())
        {
            auto* attr = new DmAttribute(nullptr, def->getData(), AttributeData(def->getTag()));
            attr->setPen(DmPen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous));
            attr->setLayer(nullptr);
            attr->setText(def->getText());
            attr->update();
            attr->move(m_data->insertionPoint);
            attr->scale(m_data->insertionPoint, m_data->scaleFactor);
            attr->rotateAngle(m_data->insertionPoint, m_data->angle);
            previewAttrs.emplace_back(attr);
        }
        ref->addAttributes(previewAttrs);
    }
    ref->update();
    preview().entities().addEntity(ref);
    m_data->updateMode = DM::Update;
    preview().draw();
}

void BlockInsertCommand::commitInsert(const DmVector& pos)
{
    preview().clear();
    if (!m_block)
    {
        return;
    }
    m_data->insertionPoint = pos;

    std::list<DmAttribute*> attrs;
    if (m_block->hasAttributeDefinitions())
    {
        std::list<DmAttributeDefinition*> attrDefs = m_block->getAttributeDefinitions();
        UIDlgEditAttributes::editAttributes(UIDialogRunner::parentOf(view()), m_block->getName(), attrDefs, attrs);
    }
    for (DmAttribute* attr : attrs)
    {
        attr->move(m_data->insertionPoint);
        attr->scale(m_data->insertionPoint, m_data->scaleFactor);
        attr->rotateAngle(m_data->insertionPoint, m_data->angle);
    }

    Transaction t("Insert Block", document());
    t.start();
    m_data->updateMode = DM::Update;
    auto* ref = new DmBlockReference(nullptr, *m_data);
    ref->addAttributes(attrs);
    ref->setDocument(document());
    ref->update();
    document()->getEntityTable()->add(ref);
    t.commit();

    view()->redraw();
}
