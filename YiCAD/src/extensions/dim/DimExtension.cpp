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

/// @file DimExtension.cpp

#include "DimExtension.h"

#include <algorithm>
#include <iterator>

#include <QCoreApplication>
#include <QInputDialog>
#include <QStringList>

#include "DimCommands.h"
#include "DmDimension.h"
#include "DmDocument.h"
#include "DmSystem.h"
#include "EntityTable.h"
#include "IExtensionContext.h"
#include "Transaction.h"
#include "UIDialogRunner.h"
#include "UIDimLinearOptions.h"
#include "UIDlgDimensionStyleMgr.h"
#include "UIRibbonRegistry.h"

namespace
{
/// @brief 一条标注命令：命令 ID、按钮文字（兼作命令行提示里的说明）、图标、命令行别名。
struct DimCommand
{
    const char* id;
    const char* text;
    const char* iconPath;
    QStringList aliases;
    ExclusiveCommandFactory factory;
    ExclusiveCommandOptionsFactory optionsFactory;
};

/// @brief 登记属性编辑命令 ext.dim.properties 的标注类型
const DM::EntityType kDimensionTypes[] = {DM::EntityDimAligned, DM::EntityDimAngular, DM::EntityDimDiametric,
                                          DM::EntityDimRadial, DM::EntityDimLinear};

/// @brief 标注的属性编辑：修改标注文字（原 UIDialogFactory::requestModifyEntityDialog 的标注分支）；
///        上下文的 entity 为要修改的标注
void editDimensionText(const CommandContext& ctx)
{
    if (!ctx.entity || std::find(std::begin(kDimensionTypes), std::end(kDimensionTypes),
                                 ctx.entity->getEntityType()) == std::end(kDimensionTypes))
    {
        return;
    }
    auto* dim = static_cast<DmDimension*>(ctx.entity);

    // 与原先的 QInputDialog::getText() 相同，改为自己构造以便经 UIDialogRunner 运行
    QInputDialog dlg(UIDialogRunner::parentOf(ctx.view));
    dlg.setWindowTitle(QCoreApplication::translate("DimExtension", "Modify dimension text"));
    dlg.setLabelText(QCoreApplication::translate("DimExtension", "New dimension text:"));
    dlg.setTextEchoMode(QLineEdit::Normal);
    dlg.setTextValue(dim->getLabel());
    if (UIDialogRunner::exec(dlg) != QDialog::Accepted)
    {
        return;
    }

    Transaction t(QCoreApplication::translate("DimExtension", "Modify dimension").toStdString(), dim->getDocument());
    t.start();
    dim->getDocument()->getEntityTable()->startModify(dim);
    dim->setLabel(dlg.textValue());
    dim->update();
    t.commit();
}
}  // namespace

void DimExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/dim/ts/ → <exe 目录>/resources/qm/dim_*.qm），
    // 必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("dim"));

    // 别名取自原 keyconfig.xml 里"默认""拼音简写"两组的并集：扩展命令不在
    // keyconfig.xml 里，两组的别名同时生效；与内置命令重名的别名由内置命令优先。
    const DimCommand commands[] = {
        {"ext.dim.aligned", QT_TRANSLATE_NOOP("DimExtension", "Aligned"), ":/extensions/dim/dim_align.svg",
         {"dimaligned", "da", "dqbx", "dq"}, exclusiveCommandFactory<DimAlignedCommand>(), {}},
        {"ext.dim.linear", QT_TRANSLATE_NOOP("DimExtension", "Linear"), ":/extensions/dim/dim_linear.svg",
         {"dimlinear", "dl", "dr", "xxbz", "xx"}, exclusiveCommandFactory<DimLinearCommand>(),
         [](QWidget* parent, IExclusiveCommand* command, bool update) -> QWidget*
         {
             auto* options = new UIDimLinearOptions(parent);
             options->setCommand(command, update);
             return options;
         }},
        {"ext.dim.radial", QT_TRANSLATE_NOOP("DimExtension", "Radial"), ":/extensions/dim/dim_radius.svg",
         {"dimradial", "dimradius", "bjbz", "bj"}, exclusiveCommandFactory<DimRadialCommand>(), {}},
        {"ext.dim.diametric", QT_TRANSLATE_NOOP("DimExtension", "Diametric"), ":/extensions/dim/dim_diam.svg",
         {"dimdiametric", "dimdiameter", "dd", "zjbz", "zj"}, exclusiveCommandFactory<DimDiametricCommand>(), {}},
        {"ext.dim.angular", QT_TRANSLATE_NOOP("DimExtension", "Angular"), ":/extensions/dim/dim_angle.svg",
         {"dimangular", "dan", "jdbz", "jd"}, exclusiveCommandFactory<DimAngularCommand>(), {}},
        {"ext.dim.leader", QT_TRANSLATE_NOOP("DimExtension", "Leader"), ":/extensions/dim/dim_leader.svg",
         {"dimleader", "ld", "yxbz", "yx"}, exclusiveCommandFactory<DimLeaderCommand>(), {}},
        {"ext.dim.baseline", QT_TRANSLATE_NOOP("DimExtension", "Baseline"), ":/extensions/dim/dim_baseline.svg",
         {}, exclusiveCommandFactory<DimBaselineCommand>(), {}},
    };

    for (const DimCommand& command : commands)
    {
        const QString text = QCoreApplication::translate("DimExtension", command.text);
        ctx.registerExclusiveCommand(command.id, command.factory,
                                     {.description = text,
                                      .aliases = command.aliases,
                                      .commandOptionsFactory = command.optionsFactory});
        ctx.ribbon().addAction({
            .panelId = UIRibbonIds::kPanelDraw2dDimension,
            .text = text,
            .iconPath = command.iconPath,
            .commandId = command.id,
        });
    }

    // 标注样式管理（原 ActionDimStyle）：即时命令，对话框挂在主窗口上
    const QString styleText = QCoreApplication::translate("DimExtension", "Dimension style");
    IExtensionContext* context = &ctx;
    ctx.registerInstantCommand(
        QStringLiteral("ext.dim.style"),
        [context](const CommandContext& c)
        {
            if (!c.document)
            {
                return;
            }
            UIDlgDimensionStyleMgr dlg(context->mainWindow(), true);
            dlg.init(c.document->getDimStyleTable(), c.document);
            dlg.exec();
        },
        {.description = styleText});
    ctx.ribbon().addAction({
        .panelId = UIRibbonIds::kPanelDraw2dDimension,
        .text = styleText,
        .iconPath = QStringLiteral(":/extensions/dim/dim_style.svg"),
        .commandId = QStringLiteral("ext.dim.style"),
    });

    // 属性编辑（"修改实体属性"与选择层双击）：修改标注文字，不打断正在运行的命令
    ctx.registerInstantCommand(QStringLiteral("ext.dim.properties"), editDimensionText,
                               {.instantInterrupt = InstantInterrupt::KeepAll});
    for (DM::EntityType type : kDimensionTypes)
    {
        ctx.registerPropertyEditor(type, QStringLiteral("ext.dim.properties"));
    }
}
