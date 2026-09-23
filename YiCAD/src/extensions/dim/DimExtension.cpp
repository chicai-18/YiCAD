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

#include <QCoreApplication>
#include <QStringList>

#include "ActionDimAligned.h"
#include "ActionDimAngular.h"
#include "ActionDimBaseline.h"
#include "ActionDimDiametric.h"
#include "ActionDimLeader.h"
#include "ActionDimLinear.h"
#include "ActionDimRadial.h"
#include "ActionDimStyle.h"
#include "DmSystem.h"
#include "IExtensionContext.h"
#include "UIDimLinearOptions.h"
#include "UIRibbonRegistry.h"

namespace
{
/// @brief 构造某个 Action 类型的命令工厂。
template <typename TAction>
CommandFactory factoryOf()
{
    return [](const CommandContext& ctx) -> ActionInterface* { return new TAction(ctx.document, ctx.view); };
}

/// @brief 一条标注命令：命令 ID、按钮文字（兼作命令行提示里的说明）、图标、命令行别名。
struct DimCommand
{
    const char* id;
    const char* text;
    const char* iconPath;
    QStringList aliases;
    CommandFactory factory;
    CommandOptionsFactory optionsFactory;
};
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
         {"dimaligned", "da", "dqbx", "dq"}, factoryOf<ActionDimAligned>(), {}},
        {"ext.dim.linear", QT_TRANSLATE_NOOP("DimExtension", "Linear"), ":/extensions/dim/dim_linear.svg",
         {"dimlinear", "dl", "dr", "xxbz", "xx"}, factoryOf<ActionDimLinear>(),
         [](QWidget* parent, ActionInterface* action, bool update) -> QWidget*
         {
             auto* options = new UIDimLinearOptions(parent);
             options->setAction(action, update);
             return options;
         }},
        {"ext.dim.radial", QT_TRANSLATE_NOOP("DimExtension", "Radial"), ":/extensions/dim/dim_radius.svg",
         {"dimradial", "dimradius", "bjbz", "bj"}, factoryOf<ActionDimRadial>(), {}},
        {"ext.dim.diametric", QT_TRANSLATE_NOOP("DimExtension", "Diametric"), ":/extensions/dim/dim_diam.svg",
         {"dimdiametric", "dimdiameter", "dd", "zjbz", "zj"}, factoryOf<ActionDimDiametric>(), {}},
        {"ext.dim.angular", QT_TRANSLATE_NOOP("DimExtension", "Angular"), ":/extensions/dim/dim_angle.svg",
         {"dimangular", "dan", "jdbz", "jd"}, factoryOf<ActionDimAngular>(), {}},
        {"ext.dim.leader", QT_TRANSLATE_NOOP("DimExtension", "Leader"), ":/extensions/dim/dim_leader.svg",
         {"dimleader", "ld", "yxbz", "yx"}, factoryOf<ActionDimLeader>(), {}},
        {"ext.dim.baseline", QT_TRANSLATE_NOOP("DimExtension", "Baseline"), ":/extensions/dim/dim_baseline.svg",
         {}, factoryOf<ActionDimBaseline>(), {}},
        {"ext.dim.style", QT_TRANSLATE_NOOP("DimExtension", "Dimension style"), ":/extensions/dim/dim_style.svg",
         {}, factoryOf<ActionDimStyle>(), {}},
    };

    for (const DimCommand& command : commands)
    {
        const QString text = QCoreApplication::translate("DimExtension", command.text);
        ctx.registerCommand(command.id, command.factory,
                            {.description = text, .aliases = command.aliases, .optionsFactory = command.optionsFactory});
        ctx.ribbon().addAction({
            .panelId = UIRibbonIds::kPanelDraw2dDimension,
            .text = text,
            .iconPath = command.iconPath,
            .commandId = command.id,
        });
    }
}
