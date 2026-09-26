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
/// 宿主只注册类目与面板（占位），按钮全部由扩展注册（IExtensionContext::ribbon），
/// 点击后经 UIActionHandler::activateCommand 按 ID 启动 CommandRegistry 里的命令。
/// 原先宿主自己注册的绘图、修改、测量按钮随内置命令拆进扩展（ext.draw、ext.modify、
/// ext.measure，doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）。写成 ApplicationWindow 的
/// 成员函数，是为了让 tr() 的翻译上下文与迁移前的 createCategory*() 保持一致
/// （ApplicationWindow / QObject），已有译文不受影响。

#include "ApplicationWindow.h"

#include "UIRibbonRegistry.h"

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

    // 文件、导出：只占位，按钮由文件扩展（src/extensions/file/）注册。
    r.addPanel({.id = kPanelFileFile, .categoryId = kCategoryFile, .title = QObject::tr("File"),
                .rows = 1, .iconOnly = true});
    r.addPanel({.id = kPanelFileExport, .categoryId = kCategoryFile, .title = QObject::tr("Export"),
                .rows = 1, .iconOnly = true, .enableFn = documentOpen});
}

void ApplicationWindow::registerRibbonDraw2d(UIRibbonRegistrar& r)
{
    using namespace UIRibbonIds;

    r.addCategory({.id = kCategoryDraw2d, .title = QObject::tr("Draw2d"), .objectName = "categoryDraw2d",
                   .enableFn = UIRibbonCondition::requireAll(UIRibbonRequires::DocumentOpen)});

    // 画线、曲线、多段线、圆、椭圆：只占位，按钮由绘图扩展（src/extensions/draw/）注册，
    // 多段线面板里的添加/追加/删除节点由修改扩展（src/extensions/modify/）注册
    r.addPanel({.id = kPanelDraw2dLine, .categoryId = kCategoryDraw2d, .title = QObject::tr("Line")});
    r.addPanel({.id = kPanelDraw2dCurve, .categoryId = kCategoryDraw2d, .title = QObject::tr("Curve")});
    r.addPanel({.id = kPanelDraw2dPolyline, .categoryId = kCategoryDraw2d, .title = QObject::tr("Polyline")});
    r.addPanel({.id = kPanelDraw2dCircle, .categoryId = kCategoryDraw2d, .title = QObject::tr("Circle")});
    r.addPanel({.id = kPanelDraw2dEllipse, .categoryId = kCategoryDraw2d, .title = QObject::tr("Ellipse")});

    // 标注：只占位，按钮由标注扩展（src/extensions/dim/）注册。
    r.addPanel({.id = kPanelDraw2dDimension, .categoryId = kCategoryDraw2d, .title = QObject::tr("Dimension")});

    // 文字：只占位，按钮由文字扩展（src/extensions/text/）注册。
    r.addPanel({.id = kPanelDraw2dText, .categoryId = kCategoryDraw2d, .title = QObject::tr("Text")});

    // 其他：只占位，插入图片由绘图扩展注册，填充由填充扩展（src/extensions/hatch/）注册
    r.addPanel({.id = kPanelDraw2dOther, .categoryId = kCategoryDraw2d, .title = QObject::tr("Other")});

    // 修改：只占位，按钮由修改扩展（src/extensions/modify/）注册。
    r.addPanel({.id = kPanelDraw2dModify, .categoryId = kCategoryDraw2d, .title = QObject::tr("Modify")});

    // 测量：只占位，按钮由查询扩展（src/extensions/measure/）注册。
    r.addPanel({.id = kPanelDraw2dMeasure, .categoryId = kCategoryDraw2d, .title = QObject::tr("Measure")});

    // 图层：图层下拉框与图层操作按钮是宿主自己的控件（依赖图层列表的刷新逻辑）。
    r.addPanel({.id = kPanelDraw2dLayer, .categoryId = kCategoryDraw2d, .title = QObject::tr("Layer")});
    r.addWidget({.id = "draw2d.layer.table",
                 .panelId = kPanelDraw2dLayer,
                 .factory = [this](QWidget* parent) { return createLayerTable(parent); }});

    // 图块：只占位，按钮由块扩展（src/extensions/block/）注册。
    r.addPanel({.id = kPanelDraw2dBlock, .categoryId = kCategoryDraw2d, .title = QObject::tr("Block")});
}

void ApplicationWindow::registerRibbonOptions(UIRibbonRegistrar& r)
{
    using namespace UIRibbonIds;

    r.addCategory({.id = kCategoryOptions, .title = QObject::tr("Options"), .objectName = "categoryOptions",
                   .enableFn = UIRibbonCondition::requireAll(UIRibbonRequires::DocumentOpen)});

    // 只占位：系统设置、图纸设置两个按钮由选项扩展（src/extensions/options/）注册；
    // 其它扩展经 IExtensionContext::registerSettingsPage 注册的设置页入口也进这个
    // 面板，按扩展的注册顺序排列。
    r.addPanel({.id = kPanelOptionsSettings, .categoryId = kCategoryOptions, .title = QObject::tr("Options"),
                .rows = 1, .iconOnly = true});
}
