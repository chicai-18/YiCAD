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

/// @file DrawEntityProperties.cpp
/// @brief 即时命令 ext.draw.properties：点、直线、圆弧、圆、椭圆、样条、多段线、图片的属性对话框
///
/// 登记为这几类实体的属性编辑命令（CommandRegistry::registerPropertyEditor），由"修改实体属性"
/// 与选择层双击运行，上下文的 entity 为要编辑的实体。取代原 UIDialogFactory::requestModifyEntityDialog
/// 的对应分支（doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）。

#include <QDialog>

#include "DmArc.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmImage.h"
#include "DmLine.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmSpline.h"
#include "DrawCommands.h"
#include "UIDialogRunner.h"
#include "UIDlgArc.h"
#include "UIDlgCircle.h"
#include "UIDlgEllipse.h"
#include "UIDlgImage.h"
#include "UIDlgLine.h"
#include "UIDlgPoint.h"
#include "UIDlgPolyline.h"
#include "UIDlgSpline.h"

namespace
{
/// @brief 用 Dialog 编辑 entity：set 把实体填进对话框，确认后 update 写回实体（写回自带事务）
template <typename Dialog, typename Entity>
void editWith(QWidget* parent, DmEntity* entity, void (Dialog::*set)(Entity&), void (Dialog::*update)())
{
    Dialog dlg(parent);
    (dlg.*set)(*static_cast<Entity*>(entity));
    if (UIDialogRunner::exec(dlg) == QDialog::Accepted)
    {
        (dlg.*update)();
    }
}
}  // namespace

const std::vector<DM::EntityType>& DrawCommands::propertyEntityTypes()
{
    static const std::vector<DM::EntityType> types = {DM::EntityPoint,   DM::EntityLine,    DM::EntityArc,
                                                      DM::EntityCircle,  DM::EntityEllipse, DM::EntitySpline,
                                                      DM::EntityPolyline, DM::EntityImage};
    return types;
}

InstantCommand DrawCommands::properties()
{
    return [](const CommandContext& ctx)
    {
        DmEntity* entity = ctx.entity;
        if (!entity)
        {
            return;
        }
        QWidget* parent = UIDialogRunner::parentOf(ctx.view);
        switch (entity->getEntityType())
        {
        case DM::EntityPoint:
            editWith(parent, entity, &UIDlgPoint::setPoint, &UIDlgPoint::updatePoint);
            break;
        case DM::EntityLine:
            editWith(parent, entity, &UIDlgLine::setLine, &UIDlgLine::updateLine);
            break;
        case DM::EntityArc:
            editWith(parent, entity, &UIDlgArc::setArc, &UIDlgArc::updateArc);
            break;
        case DM::EntityCircle:
            editWith(parent, entity, &UIDlgCircle::setCircle, &UIDlgCircle::updateCircle);
            break;
        case DM::EntityEllipse:
            editWith(parent, entity, &UIDlgEllipse::setEllipse, &UIDlgEllipse::updateEllipse);
            break;
        case DM::EntitySpline:
            // 原先样条的属性对话框不设父窗口
            editWith(nullptr, entity, &UIDlgSpline::setSpline, &UIDlgSpline::updateSpline);
            break;
        case DM::EntityPolyline:
            editWith(parent, entity, &UIDlgPolyline::setPolyline, &UIDlgPolyline::updatePolyline);
            break;
        case DM::EntityImage:
            editWith(parent, entity, &UIDlgImage::setImage, &UIDlgImage::updateImage);
            break;
        default:
            break;
        }
    };
}
