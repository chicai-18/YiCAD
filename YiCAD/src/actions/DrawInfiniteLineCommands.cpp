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

/// @file DrawInfiniteLineCommands.cpp
/// @brief 射线命令 draw.ray（原 ActionDrawRay）与构造线命令 draw.xline（原 ActionDrawXline）：
///        指定基点，再指定方向
///
/// 两个原 Action 除实体类型、事务名与 help 之外逐行相同，这里共用一个工具模板。
/// 原 Action 没有 Q_OBJECT，译文在 ActionInterface 的翻译上下文里。

#include <memory>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BasePlaceTool.h"
#include "CommandPreview.h"
#include "CommandRegistry.h"
#include "Commands.h"
#include "DmDocument.h"
#include "DmRay.h"
#include "DmXline.h"
#include "EntityTable.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "PlaceCommand.h"
#include "Transaction.h"

namespace
{
/// @brief 射线、构造线命令的公共部分
/// @tparam Entity 实体类型（DmRay、DmXline）
/// @tparam Data 实体数据类型（RayData、XLineData）
template <typename Entity, typename Data>
class InfiniteLineCommand : public PlaceCommand
{
public:
    /// @brief 预览
    void previewLine(const Data& data)
    {
        preview().clear();
        Entity* entity = new Entity(preview().entities().getEntityContainer(), data);
        entity->setDocument(document());
        preview().entities().addEntity(entity);
        preview().draw();
    }

    /// @brief 提交，相对零点移到基点
    void commitLine(const Data& data)
    {
        preview().clear();
        Transaction t(transactionName().toStdString(), document());
        t.start();
        Entity* entity = new Entity(nullptr, data);
        entity->setDocument(document());
        document()->getEntityTable()->add(entity);
        t.commit();
        view()->moveRelativeZero(data.getBasePoint());
    }

    /// @brief 基点状态下输入 help 时是否列出命令（原射线列出，构造线不理会）
    virtual bool listsHelp() const = 0;
    virtual QString firstPointHint() const = 0;
    virtual QString directionHint() const = 0;
    virtual QString cancelHint() const = 0;
    virtual QString backHint() const = 0;

protected:
    virtual QString transactionName() const = 0;
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 射线、构造线工具：指定基点，再指定方向
template <typename Entity, typename Data>
class InfiniteLineTool : public BasePlaceTool
{
public:
    /// @brief 交互状态
    enum Status
    {
        SetBasePoint, ///< 设置基点
        SetDir        ///< 设置方向
    };

    using Command = InfiniteLineCommand<Entity, Data>;

    InfiniteLineTool(Command& command, DmDocument* doc, IDocumentView* view)
        : BasePlaceTool(command, doc, view)
        , m_command(command)
    {
    }

protected:
    void updateHints() override
    {
        // 与原 Action 一致：其它状态不刷新提示
        switch (status())
        {
        case SetBasePoint:
            GUIDIALOGFACTORY->updateMouseWidget(m_command.firstPointHint(), m_command.cancelHint());
            break;
        case SetDir:
            GUIDIALOGFACTORY->updateMouseWidget(m_command.directionHint(), m_command.backHint());
            break;
        default:
            break;
        }
    }

    void onMouseMove(QMouseEvent* e) override
    {
        DmVector mouse = snapper()->snapPoint(e);
        if (status() == SetDir && m_data.getBasePoint().valid)
        {
            m_data.setDirection(mouse - m_data.getBasePoint());
            m_command.previewLine(m_data);
        }
    }

    void onMouseRelease(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
        {
            onCoordinate(snapper()->snapPoint(e));
        }
        else if (e->button() == Qt::RightButton)
        {
            m_command.preview().clear();
            if (status() == SetDir)
            {
                // 与原 Action 一致：只退回状态，不重新初始化捕捉器，保留基点
                setStatus(SetBasePoint);
            }
            else
            {
                command().finish();
            }
        }
    }

    void onCoordinate(const DmVector& pos) override
    {
        switch (status())
        {
        case SetBasePoint:
            m_data.setBasePoint(pos);
            setStatus(SetDir);
            view()->moveRelativeZero(pos);
            break;
        case SetDir:
            m_data.setDirection(pos - m_data.getBasePoint());
            m_command.commitLine(m_data);
            break;
        default:
            break;
        }
    }

    void onCommand(GuiCommandEvent* e) override
    {
        // 与原射线一致：help 只列出命令（没有可用命令），不接受
        if (m_command.listsHelp() && status() == SetBasePoint
            && Commands::checkCommand("help", e->getCommand().toLower()))
        {
            GUIDIALOGFACTORY->commandMessage(Commands::msgAvailableCommands());
        }
    }

private:
    Command& m_command;
    Data m_data; ///< 基点与方向
};

template <typename Entity, typename Data>
std::unique_ptr<BasePlaceTool> InfiniteLineCommand<Entity, Data>::createTool()
{
    return std::make_unique<InfiniteLineTool<Entity, Data>>(*this, document(), view());
}

/// @brief 射线命令
class DrawRayCommand : public InfiniteLineCommand<DmRay, RayData>
{
    Q_DECLARE_TR_FUNCTIONS(DrawRayCommand)

public:
    bool listsHelp() const override { return true; }
    QString firstPointHint() const override { return tr("Specify first point"); }
    QString directionHint() const override { return tr("Specify direction"); }
    QString cancelHint() const override { return tr("Cancel"); }
    QString backHint() const override { return tr("Back"); }

protected:
    QString transactionName() const override { return tr("Draw Ray"); }
};

/// @brief 构造线命令
class DrawXlineCommand : public InfiniteLineCommand<DmXline, XLineData>
{
    Q_DECLARE_TR_FUNCTIONS(DrawXlineCommand)

public:
    bool listsHelp() const override { return false; }
    QString firstPointHint() const override { return tr("Specify first point"); }
    QString directionHint() const override { return tr("Specify direction"); }
    QString cancelHint() const override { return tr("Cancel"); }
    QString backHint() const override { return tr("Back"); }

protected:
    QString transactionName() const override { return tr("Draw Construction Line"); }
};

const bool g_registeredRay = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("draw.ray"), exclusiveCommandFactory<DrawRayCommand>());

const bool g_registeredXline = CommandRegistry::instance().registerExclusiveCommand(
    QStringLiteral("draw.xline"), exclusiveCommandFactory<DrawXlineCommand>());
}  // namespace
