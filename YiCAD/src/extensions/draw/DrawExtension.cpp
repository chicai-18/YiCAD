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

/// @file DrawExtension.cpp

#include "DrawExtension.h"

#include <QCoreApplication>

#include "DmSystem.h"
#include "DrawCommands.h"
#include "IExtensionContext.h"
#include "UIArcOptions.h"
#include "UIArcTangentialOptions.h"
#include "UICircleTan2Options.h"
#include "UICloudLineOptions.h"
#include "UIImageOptions.h"
#include "UILineBisectorOptions.h"
#include "UILineOptions.h"
#include "UILinePolygonOptions.h"
#include "UIPolylineOptions.h"
#include "UIRibbonRegistry.h"
#include "UISplineOptions.h"

namespace
{
/// @brief 构造选项条并交给命令
template <typename Options>
ExclusiveCommandOptionsFactory optionsFactory()
{
    return [](QWidget* parent, IExclusiveCommand* command, bool update) -> QWidget*
    {
        auto* options = new Options(parent);
        options->setCommand(command, update);
        return options;
    };
}

/// @brief 画直线的选项条：setCommand() 没有 update 参数（原先对话框工厂也不传）
ExclusiveCommandOptionsFactory lineOptionsFactory()
{
    return [](QWidget* parent, IExclusiveCommand* command, bool) -> QWidget*
    {
        auto* options = new UILineOptions(parent);
        options->setCommand(command);
        return options;
    };
}

/// @brief 一条绘图命令：命令 ID、按钮所在面板（为空表示没有按钮）、按钮文字、图标、工厂、选项条
struct DrawCommand
{
    const char* id;
    const char* panelId;
    const char* text;
    const char* iconPath;
    ExclusiveCommandFactory factory;
    ExclusiveCommandOptionsFactory optionsFactory = {};
    int optionsHeight = 23; ///< 选项条容器的高度，见 CommandInfo::commandOptionsHeight
};
}  // namespace

void DrawExtension::OnRegister(IExtensionContext& ctx)
{
    // 本扩展的翻译包（src/extensions/draw/ts/），必须早于下面的 translate() 调用。
    DMSYSTEM->loadExtensionTranslation(QStringLiteral("draw"));

    using namespace UIRibbonIds;
    // 按钮按原先宿主注册的顺序；多段线面板里的添加/追加/删除节点归修改扩展
    const DrawCommand commands[] = {
        {"ext.draw.line", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "2 Points"),
         ":/ribbon/draw2d/line_2p.svg", DrawCommands::line(), lineOptionsFactory()},
        {"ext.draw.line_rectangle", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Rectangle"),
         ":/ribbon/draw2d/line_square.svg", DrawCommands::lineRectangle()},
        {"ext.draw.line_bisector", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Bisector"),
         ":/ribbon/draw2d/line_bi_angle.svg", DrawCommands::lineBisector(), optionsFactory<UILineBisectorOptions>()},
        {"ext.draw.line_tangent1", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Tangent (P,C)"),
         ":/ribbon/draw2d/line_tangent_pt_circle.svg", DrawCommands::lineTangent1()},
        {"ext.draw.line_tangent2", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Tangent (C,C)"),
         ":/ribbon/draw2d/line_tangent_c_c.svg", DrawCommands::lineTangent2()},
        {"ext.draw.line_orth_tan", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Tangent Orthogonal"),
         ":/ribbon/draw2d/line_tan_orthognal.svg", DrawCommands::lineOrthTan()},
        {"ext.draw.line_polygon_cen_cor", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Polygon (Cen,Cor)"),
         ":/ribbon/draw2d/line_polygon_cen_cor.svg", DrawCommands::linePolygonCenCor(),
         optionsFactory<UILinePolygonOptions>()},
        {"ext.draw.line_polygon_cen_tan", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Polygon (Cen,Tan)"),
         ":/ribbon/draw2d/line_polygon_cen_tan.svg", DrawCommands::linePolygonCenTan(),
         optionsFactory<UILinePolygonOptions>()},
        {"ext.draw.ray", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Ray"), ":/ribbon/draw2d/line_ray.svg",
         DrawCommands::ray()},
        {"ext.draw.xline", kPanelDraw2dLine, QT_TRANSLATE_NOOP("DrawExtension", "Xline"),
         ":/ribbon/draw2d/line_xline.svg", DrawCommands::xline()},

        {"ext.draw.arc", kPanelDraw2dCurve, QT_TRANSLATE_NOOP("DrawExtension", "Center, Point, Angles"),
         ":/ribbon/draw2d/curve_arc_center_angle.svg", DrawCommands::arc(), optionsFactory<UIArcOptions>()},
        {"ext.draw.arc_3p", kPanelDraw2dCurve, QT_TRANSLATE_NOOP("DrawExtension", "3 Points"),
         ":/ribbon/draw2d/curve_arc_3p.svg", DrawCommands::arc3p()},
        {"ext.draw.arc_tangential", kPanelDraw2dCurve, QT_TRANSLATE_NOOP("DrawExtension", "Arc Tangential"),
         ":/ribbon/draw2d/curve_arc_tang.svg", DrawCommands::arcTangential(),
         optionsFactory<UIArcTangentialOptions>()},
        {"ext.draw.spline", kPanelDraw2dCurve, QT_TRANSLATE_NOOP("DrawExtension", "Spline"),
         ":/ribbon/draw2d/curve_spline.svg", DrawCommands::spline(), optionsFactory<UISplineOptions>(), 26},
        {"ext.draw.spline_points", kPanelDraw2dCurve, QT_TRANSLATE_NOOP("DrawExtension", "Spline through points"),
         ":/ribbon/draw2d/curve_spline_ft_pt.svg", DrawCommands::splinePoints(), optionsFactory<UISplineOptions>(),
         26},
        {"ext.draw.line_free", kPanelDraw2dCurve, QT_TRANSLATE_NOOP("DrawExtension", "Freehand Line"),
         ":/ribbon/draw2d/curve_freehand_line.svg", DrawCommands::lineFree()},

        {"ext.draw.polyline", kPanelDraw2dPolyline, QT_TRANSLATE_NOOP("DrawExtension", "Polyline"),
         ":/ribbon/draw2d/polyline.svg", DrawCommands::polyline(), optionsFactory<UIPolylineOptions>()},
        {"ext.draw.cloud_line_rectangle", kPanelDraw2dPolyline,
         QT_TRANSLATE_NOOP("DrawExtension", "Create cloud line by rectangle"), ":/ribbon/draw2d/cloudline_rectangle.svg",
         DrawCommands::cloudLineRectangle(), optionsFactory<UICloudLineOptions>()},
        {"ext.draw.cloud_line_polygon", kPanelDraw2dPolyline,
         QT_TRANSLATE_NOOP("DrawExtension", "Create cloud line by polygon"), ":/ribbon/draw2d/cloudline_polygon.svg",
         DrawCommands::cloudLinePolygon(), optionsFactory<UICloudLineOptions>()},
        {"ext.draw.cloud_line_free", kPanelDraw2dPolyline,
         QT_TRANSLATE_NOOP("DrawExtension", "Create cloud line by free"), ":/ribbon/draw2d/cloudline_free.svg",
         DrawCommands::cloudLineFree(), optionsFactory<UICloudLineOptions>()},

        {"ext.draw.circle", kPanelDraw2dCircle, QT_TRANSLATE_NOOP("DrawExtension", "Center, Point"),
         ":/ribbon/draw2d/circle_center_pt.svg", DrawCommands::circle()},
        {"ext.draw.circle_2p", kPanelDraw2dCircle, QT_TRANSLATE_NOOP("DrawExtension", "2 Points"),
         ":/ribbon/draw2d/circle_2p.svg", DrawCommands::circle2p()},
        {"ext.draw.circle_3p", kPanelDraw2dCircle, QT_TRANSLATE_NOOP("DrawExtension", "3 Points"),
         ":/ribbon/draw2d/circle_3p.svg", DrawCommands::circle3p()},
        {"ext.draw.circle_tan2", kPanelDraw2dCircle, QT_TRANSLATE_NOOP("DrawExtension", "Tangential 2 Circles, Radius"),
         ":/ribbon/draw2d/circle_tan_radius.svg", DrawCommands::circleTan2(), optionsFactory<UICircleTan2Options>()},
        {"ext.draw.circle_tan3", kPanelDraw2dCircle, QT_TRANSLATE_NOOP("DrawExtension", "Tangential 3 Circles"),
         ":/ribbon/draw2d/circle_tan_3c.svg", DrawCommands::circleTan3()},

        {"ext.draw.ellipse_axis", kPanelDraw2dEllipse, QT_TRANSLATE_NOOP("DrawExtension", "Ellipse(Axis)"),
         ":/ribbon/draw2d/ellipe_2seg.svg", DrawCommands::ellipseAxis()},
        {"ext.draw.ellipse_inscribe", kPanelDraw2dEllipse, QT_TRANSLATE_NOOP("DrawExtension", "Ellipse Inscribed"),
         ":/ribbon/draw2d/ellipse_incrib.svg", DrawCommands::ellipseInscribe()},

        // 填充按钮由填充扩展注册，排在插入图片之后
        {"ext.draw.image", kPanelDraw2dOther, QT_TRANSLATE_NOOP("DrawExtension", "Insert Image"),
         ":/ribbon/draw2d/insert_image.svg", DrawCommands::image(), optionsFactory<UIImageOptions>()},

        // 没有按钮：点（原先经 UIActionHandler::slotDrawPoint，没有调用方）、椭圆弧
        {"ext.draw.point", nullptr, nullptr, nullptr, DrawCommands::point()},
        {"ext.draw.ellipse_arc_axis", nullptr, nullptr, nullptr, DrawCommands::ellipseArcAxis()},
    };

    for (const DrawCommand& command : commands)
    {
        ctx.registerExclusiveCommand(command.id, command.factory,
                                     {.commandOptionsFactory = command.optionsFactory,
                                      .commandOptionsHeight = command.optionsHeight});
        if (command.panelId)
        {
            ctx.ribbon().addAction({
                .panelId = command.panelId,
                .text = QCoreApplication::translate("DrawExtension", command.text),
                .iconPath = command.iconPath,
                .commandId = command.id,
            });
        }
    }
}
