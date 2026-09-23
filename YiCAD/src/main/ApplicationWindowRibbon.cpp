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

/// @file ApplicationWindowRibbon.cpp
/// @brief 内置 Ribbon 类目的注册（文件、绘图、设置）。
///
/// 按钮只声明外观、位置、可用条件与命令 ID，由 UIRibbonManager 统一装配，
/// 点击后经 UIActionHandler::activateCommand 按 ID 启动 CommandRegistry 里
/// 的命令。写成 ApplicationWindow 的成员函数，是为了让 tr() 的翻译上下文
/// 与迁移前的 createCategory*() 保持一致（ApplicationWindow / QObject），
/// 已有译文不受影响。

#include "ApplicationWindow.h"

#include "UIRibbonRegistry.h"

namespace
{
/// @brief 注册一个启动命令的按钮。
void addCommand(UIRibbonRegistrar& r, const char* panelId, const QString& text, const char* iconPath,
                const char* commandId, UIRibbonEnableFn enableFn = {})
{
    r.addAction(UIRibbonActionDef{
        .panelId = panelId,
        .text = text,
        .iconPath = iconPath,
        .commandId = commandId,
        .enableFn = std::move(enableFn),
    });
}
}  // namespace

void ApplicationWindow::registerBuiltinRibbon(UIRibbonRegistrar& r)
{
    registerRibbonFile(r);
    registerRibbonDraw2d(r);
    registerRibbonOptions(r);
}

void ApplicationWindow::registerRibbonFile(UIRibbonRegistrar& r)
{
    using namespace UIRibbonIds;
    const UIRibbonEnableFn documentOpen = UIRibbonCondition::requireAll(UIRibbonRequires::DocumentOpen);

    r.addCategory({.id = kCategoryFile, .title = QObject::tr("File"), .objectName = "categoryFile"});

    // 新建、打开始终可用；其余需要打开的文档。
    r.addPanel({.id = kPanelFileFile, .categoryId = kCategoryFile, .title = QObject::tr("File"),
                .rows = 1, .iconOnly = true});
    addCommand(r, kPanelFileFile, QObject::tr("new"), ":/ribbon/file/new.svg", "file.new");
    addCommand(r, kPanelFileFile, QObject::tr("open"), ":/ribbon/file/open.svg", "file.open");
    addCommand(r, kPanelFileFile, QObject::tr("save"), ":/ribbon/file/save.svg", "file.save", documentOpen);
    addCommand(r, kPanelFileFile, QObject::tr("save as"), ":/ribbon/file/save_as.svg", "file.save_as",
               documentOpen);

    r.addPanel({.id = kPanelFileExport, .categoryId = kCategoryFile, .title = QObject::tr("Export"),
                .rows = 1, .iconOnly = true, .enableFn = documentOpen});
    addCommand(r, kPanelFileExport, QObject::tr("Export Image"), ":/ribbon/file/export_image.svg",
               "file.export_image");
}

void ApplicationWindow::registerRibbonDraw2d(UIRibbonRegistrar& r)
{
    using namespace UIRibbonIds;

    r.addCategory({.id = kCategoryDraw2d, .title = QObject::tr("Draw2d"), .objectName = "categoryDraw2d",
                   .enableFn = UIRibbonCondition::requireAll(UIRibbonRequires::DocumentOpen)});

    // 画线
    r.addPanel({.id = kPanelDraw2dLine, .categoryId = kCategoryDraw2d, .title = QObject::tr("Line")});
    addCommand(r, kPanelDraw2dLine, tr("2 Points"), ":/ribbon/draw2d/line_2p.svg", "draw.line");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Rectangle"), ":/ribbon/draw2d/line_square.svg",
               "draw.line_rectangle");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Bisector"), ":/ribbon/draw2d/line_bi_angle.svg",
               "draw.line_bisector");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Tangent (P,C)"), ":/ribbon/draw2d/line_tangent_pt_circle.svg",
               "draw.line_tangent1");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Tangent (C,C)"), ":/ribbon/draw2d/line_tangent_c_c.svg",
               "draw.line_tangent2");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Tangent Orthogonal"), ":/ribbon/draw2d/line_tan_orthognal.svg",
               "draw.line_orth_tan");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Polygon (Cen,Cor)"), ":/ribbon/draw2d/line_polygon_cen_cor.svg",
               "draw.line_polygon_cen_cor");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Polygon (Cen,Tan)"), ":/ribbon/draw2d/line_polygon_cen_tan.svg",
               "draw.line_polygon_cen_tan");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Ray"), ":/ribbon/draw2d/line_ray.svg", "draw.ray");
    addCommand(r, kPanelDraw2dLine, QObject::tr("Xline"), ":/ribbon/draw2d/line_xline.svg", "draw.xline");

    // 曲线
    r.addPanel({.id = kPanelDraw2dCurve, .categoryId = kCategoryDraw2d, .title = QObject::tr("Curve")});
    addCommand(r, kPanelDraw2dCurve, QObject::tr("Center, Point, Angles"),
               ":/ribbon/draw2d/curve_arc_center_angle.svg", "draw.arc");
    addCommand(r, kPanelDraw2dCurve, QObject::tr("3 Points"), ":/ribbon/draw2d/curve_arc_3p.svg", "draw.arc_3p");
    addCommand(r, kPanelDraw2dCurve, QObject::tr("Arc Tangential"), ":/ribbon/draw2d/curve_arc_tang.svg",
               "draw.arc_tangential");
    addCommand(r, kPanelDraw2dCurve, QObject::tr("Spline"), ":/ribbon/draw2d/curve_spline.svg", "draw.spline");
    addCommand(r, kPanelDraw2dCurve, QObject::tr("Spline through points"), ":/ribbon/draw2d/curve_spline_ft_pt.svg",
               "draw.spline_points");
    addCommand(r, kPanelDraw2dCurve, QObject::tr("Freehand Line"), ":/ribbon/draw2d/curve_freehand_line.svg",
               "draw.line_free");

    // 多段线
    r.addPanel({.id = kPanelDraw2dPolyline, .categoryId = kCategoryDraw2d, .title = QObject::tr("Polyline")});
    addCommand(r, kPanelDraw2dPolyline, QObject::tr("Polyline"), ":/ribbon/draw2d/polyline.svg", "draw.polyline");
    addCommand(r, kPanelDraw2dPolyline, QObject::tr("Add node"), ":/ribbon/draw2d/polyline_add_node.svg",
               "polyline.add");
    addCommand(r, kPanelDraw2dPolyline, QObject::tr("Append node"), ":/ribbon/draw2d/polyline_append_node.svg",
               "polyline.append");
    addCommand(r, kPanelDraw2dPolyline, QObject::tr("Delete node"), ":/ribbon/draw2d/polyline_delete_node.svg",
               "polyline.del");
    addCommand(r, kPanelDraw2dPolyline, QObject::tr("Create cloud line by rectangle"),
               ":/ribbon/draw2d/cloudline_rectangle.svg", "draw.cloud_line_rectangle");
    addCommand(r, kPanelDraw2dPolyline, QObject::tr("Create cloud line by polygon"),
               ":/ribbon/draw2d/cloudline_polygon.svg", "draw.cloud_line_polygon");
    addCommand(r, kPanelDraw2dPolyline, QObject::tr("Create cloud line by free"),
               ":/ribbon/draw2d/cloudline_free.svg", "draw.cloud_line_free");

    // 画圆
    r.addPanel({.id = kPanelDraw2dCircle, .categoryId = kCategoryDraw2d, .title = QObject::tr("Circle")});
    addCommand(r, kPanelDraw2dCircle, QObject::tr("Center, Point"), ":/ribbon/draw2d/circle_center_pt.svg",
               "draw.circle");
    addCommand(r, kPanelDraw2dCircle, QObject::tr("2 Points"), ":/ribbon/draw2d/circle_2p.svg", "draw.circle_2p");
    addCommand(r, kPanelDraw2dCircle, QObject::tr("3 Points"), ":/ribbon/draw2d/circle_3p.svg", "draw.circle_3p");
    addCommand(r, kPanelDraw2dCircle, QObject::tr("Tangential 2 Circles, Radius"),
               ":/ribbon/draw2d/circle_tan_radius.svg", "draw.circle_tan2");
    addCommand(r, kPanelDraw2dCircle, QObject::tr("Tangential 3 Circles"), ":/ribbon/draw2d/circle_tan_3c.svg",
               "draw.circle_tan3");

    // 椭圆
    r.addPanel({.id = kPanelDraw2dEllipse, .categoryId = kCategoryDraw2d, .title = QObject::tr("Ellipse")});
    addCommand(r, kPanelDraw2dEllipse, QObject::tr("Ellipse(Axis)"), ":/ribbon/draw2d/ellipe_2seg.svg",
               "draw.ellipse_axis");
    addCommand(r, kPanelDraw2dEllipse, QObject::tr("Ellipse Inscribed"), ":/ribbon/draw2d/ellipse_incrib.svg",
               "draw.ellipse_inscribe");

    // 标注
    r.addPanel({.id = kPanelDraw2dDimension, .categoryId = kCategoryDraw2d, .title = QObject::tr("Dimension")});
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Aligned"), ":/ribbon/draw2d/dim_align.svg", "dim.aligned");
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Linear"), ":/ribbon/draw2d/dim_linear.svg", "dim.linear");
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Radial"), ":/ribbon/draw2d/dim_radius.svg", "dim.radial");
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Diametric"), ":/ribbon/draw2d/dim_diam.svg", "dim.diametric");
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Angluar"), ":/ribbon/draw2d/dim_angle.svg", "dim.angular");
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Leader"), ":/ribbon/draw2d/dim_leader.svg", "dim.leader");
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Baseline"), ":/ribbon/draw2d/dim_baseline.svg", "dim.baseline");
    addCommand(r, kPanelDraw2dDimension, QObject::tr("Dimension style"), ":/ribbon/draw2d/dim_style.svg", "dim.style");

    // 文字
    r.addPanel({.id = kPanelDraw2dText, .categoryId = kCategoryDraw2d, .title = QObject::tr("Text")});
    addCommand(r, kPanelDraw2dText, QObject::tr("Single line text"), ":/ribbon/draw2d/text.svg", "draw.text");
    addCommand(r, kPanelDraw2dText, QObject::tr("Multiline text"), ":/ribbon/draw2d/mtext.svg", "draw.mtext");
    addCommand(r, kPanelDraw2dText, QObject::tr("Text style"), ":/ribbon/draw2d/text_style.svg", "text.style");

    // 其他
    r.addPanel({.id = kPanelDraw2dOther, .categoryId = kCategoryDraw2d, .title = QObject::tr("Other")});
    addCommand(r, kPanelDraw2dOther, QObject::tr("Hatch"), ":/ribbon/draw2d/hatch.svg", "draw.hatch");
    addCommand(r, kPanelDraw2dOther, QObject::tr("Insert Image"), ":/ribbon/draw2d/insert_image.svg", "draw.image");

    // 修改
    r.addPanel({.id = kPanelDraw2dModify, .categoryId = kCategoryDraw2d, .title = QObject::tr("Modify")});
    addCommand(r, kPanelDraw2dModify, QObject::tr("Copy"), ":/ribbon/draw2d/modify_copy.svg", "modify.copy");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Move"), ":/ribbon/draw2d/modify_move.svg", "modify.move");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Rotate"), ":/ribbon/draw2d/modify_rotate.svg", "modify.rotate");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Scale"), ":/ribbon/draw2d/modify_zoom.svg", "modify.scale");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Mirror"), ":/ribbon/draw2d/modify_mirror.svg", "modify.mirror");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Trim"), ":/ribbon/draw2d/modify_trim.svg", "modify.trim");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Lengthen"), ":/ribbon/draw2d/modify_lengthen.svg",
               "modify.extend");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Offset"), ":/ribbon/draw2d/modify_offset.svg",
               "modify.single_offset");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Bevel"), ":/ribbon/draw2d/modify_bevel.svg", "modify.bevel");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Fillet"), ":/ribbon/draw2d/modify_fillet.svg", "modify.round");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Divide"), ":/ribbon/draw2d/modify_divide.svg", "modify.cut");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Divide_2P"), ":/ribbon/draw2d/modify_twopoints_break.svg",
               "modify.cut_2p");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Properties"), ":/ribbon/draw2d/modify_attributes.svg",
               "modify.entity");
    addCommand(r, kPanelDraw2dModify, QObject::tr("Explode"), ":/ribbon/draw2d/modify_explode.svg",
               "modify.explode");

    // 测量
    r.addPanel({.id = kPanelDraw2dMeasure, .categoryId = kCategoryDraw2d, .title = QObject::tr("Measure")});
    addCommand(r, kPanelDraw2dMeasure, QObject::tr("Distance Point to Point"), ":/ribbon/draw2d/info_dist_pt_pt.svg",
               "info.dist");
    addCommand(r, kPanelDraw2dMeasure, QObject::tr("Angle between two lines"),
               ":/ribbon/draw2d/info_angle_2lines.svg", "info.angle");
    addCommand(r, kPanelDraw2dMeasure, QObject::tr("Total length of selected entities"),
               ":/ribbon/draw2d/info_length_entity.svg", "info.total_length");
    addCommand(r, kPanelDraw2dMeasure, QObject::tr("Polygonal Area"), ":/ribbon/draw2d/info_area_polygon.svg",
               "info.area");

    // 图层：图层下拉框与图层操作按钮是宿主自己的控件（依赖图层列表的刷新逻辑）。
    r.addPanel({.id = kPanelDraw2dLayer, .categoryId = kCategoryDraw2d, .title = QObject::tr("Layer")});
    r.addWidget({.id = "draw2d.layer.table",
                 .panelId = kPanelDraw2dLayer,
                 .factory = [this](QWidget* parent) { return createLayerTable(parent); }});

    // 图块
    r.addPanel({.id = kPanelDraw2dBlock, .categoryId = kCategoryDraw2d, .title = QObject::tr("Block")});
    addCommand(r, kPanelDraw2dBlock, QObject::tr("Create Block"), ":/ribbon/block/block_create.svg", "blocks.create");
    addCommand(r, kPanelDraw2dBlock, QObject::tr("Insert the active block"), ":/ribbon/block/block_insert.svg",
               "blocks.insert_prepare");
    addCommand(r, kPanelDraw2dBlock, QObject::tr("save the block to a file"), ":/ribbon/block/block_save.svg",
               "blocks.save_as");
    addCommand(r, kPanelDraw2dBlock, QObject::tr("Define attributes"), ":/ribbon/block/define_attribute.svg",
               "blocks.define_attributes");
    addCommand(r, kPanelDraw2dBlock, QObject::tr("Delete Block"), ":/ribbon/block/block_delete.svg",
               "blocks.delete");
    addCommand(r, kPanelDraw2dBlock, QObject::tr("Edit Block"), ":/ribbon/block/block_edit.svg", "blocks.edit");
    addCommand(r, kPanelDraw2dBlock, QObject::tr("Import Block"), ":/ribbon/file/import_block.svg",
               "blocks.import");
}

void ApplicationWindow::registerRibbonOptions(UIRibbonRegistrar& r)
{
    using namespace UIRibbonIds;

    r.addCategory({.id = kCategoryOptions, .title = QObject::tr("Options"), .objectName = "categoryOptions",
                   .enableFn = UIRibbonCondition::requireAll(UIRibbonRequires::DocumentOpen)});

    // 扩展经 IExtensionContext::registerSettingsPage 注册的设置页入口也进这个
    // 面板，排在下面两个内置按钮之后。
    r.addPanel({.id = kPanelOptionsSettings, .categoryId = kCategoryOptions, .title = QObject::tr("Options"),
                .rows = 1, .iconOnly = true});
    addCommand(r, kPanelOptionsSettings, QObject::tr("System Setting"), ":/ribbon/options/settings.svg",
               "options.general");
    addCommand(r, kPanelOptionsSettings, QObject::tr("Draw Setting"), ":/ribbon/options/draw_settings.svg",
               "options.drawing");
}
