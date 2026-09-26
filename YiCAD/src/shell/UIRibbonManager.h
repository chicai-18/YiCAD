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

/// @file UIRibbonManager.h
/// @brief 把 UIRibbonRegistry 里的注册数据装配到 SARibbonBar，并按上下文
/// 重算可用状态（参考 DS 的 RibbonManager::installEntry/evaluateActivation）。

#ifndef UIRIBBONMANAGER_H
#define UIRIBBONMANAGER_H

#include <functional>
#include <map>
#include <vector>

#include <QPointer>
#include <QString>

#include "UIRibbonRegistry.h"

class QAction;
class QObject;
class QWidget;
class SARibbonBar;
class SARibbonButtonGroupWidget;
class SARibbonCategory;

class UIRibbonManager
{
public:
    /// @brief 按命令 ID 启动命令；source 是被点击的 QAction。
    using CommandActivator = std::function<void(const QString& commandId, QObject* source)>;
    /// @brief 构造当前上下文快照。
    using ContextProvider = std::function<UIRibbonContext()>;

    UIRibbonManager(SARibbonBar& bar, const UIRibbonRegistry& registry,
                    CommandActivator activator, ContextProvider contextProvider);

    /// @brief 按注册表创建类目、面板、按钮与控件。注册表必须已 finalize；
    /// 没有任何条目的面板不装配。只能调用一次。
    void install();

    /// @brief 按当前上下文重算类目、面板、按钮与控件的可用状态。
    void evaluateActivation();

    /// @brief 按 ID 查找已装配的类目；未找到返回 nullptr。
    SARibbonCategory* category(const QString& id) const;

    /// @brief 按 ID 查找已装配的按钮；未找到返回 nullptr。
    QAction* action(const QString& id) const;

    /// @brief 创建 Ribbon 面板里用的按钮组，rows 行竖向排列。
    /// 宿主自定义控件（如图层面板）也用它，保持与注册按钮一致的边距。
    static SARibbonButtonGroupWidget* createButtonGroup(QWidget* parent, int rows);

private:
    QAction* createAction(const UIRibbonActionDef& def);

    struct Tracked
    {
        UIRibbonEnableFn enableFn;
        QPointer<QObject> target;  ///< QWidget 或 QAction
    };

    SARibbonBar& m_bar;
    const UIRibbonRegistry& m_registry;
    CommandActivator m_activator;
    ContextProvider m_contextProvider;
    bool m_installed = false;

    std::map<QString, SARibbonCategory*> m_categories;
    std::map<QString, QAction*> m_actions;
    std::vector<Tracked> m_tracked;
};

#endif  // UIRIBBONMANAGER_H
