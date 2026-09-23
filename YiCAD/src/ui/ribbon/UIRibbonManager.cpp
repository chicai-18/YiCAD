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

/// @file UIRibbonManager.cpp

#include "UIRibbonManager.h"

#include <utility>

#include <QAction>
#include <QDebug>
#include <QGridLayout>
#include <QIcon>
#include <QToolButton>

#include "SARibbonBar.h"
#include "SARibbonButtonGroupWidget.h"
#include "SARibbonCategory.h"
#include "SARibbonPannel.h"

namespace
{
constexpr int kRibbonButtonGroupMargin = 5;
constexpr int kRibbonButtonGroupSpacing = 5;
}  // namespace

UIRibbonManager::UIRibbonManager(SARibbonBar& bar, const UIRibbonRegistry& registry,
                                 CommandActivator activator, ContextProvider contextProvider)
    : m_bar(bar)
    , m_registry(registry)
    , m_activator(std::move(activator))
    , m_contextProvider(std::move(contextProvider))
{
}

SARibbonButtonGroupWidget* UIRibbonManager::createButtonGroup(QWidget* parent, int rows)
{
    auto* group = new SARibbonButtonGroupWidget(parent);
    if (parent && parent->inherits("SARibbonPannel"))
    {
        delete group->layout();
        auto* layout = new QGridLayout(group);
        layout->setDefaultPositioning(rows, Qt::Vertical);
        layout->setContentsMargins(kRibbonButtonGroupMargin, kRibbonButtonGroupMargin,
                                   kRibbonButtonGroupMargin, kRibbonButtonGroupMargin);
        layout->setSpacing(kRibbonButtonGroupSpacing);
        group->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    }
    return group;
}

QAction* UIRibbonManager::createAction(const UIRibbonActionDef& def)
{
    auto* act = new QAction(&m_bar);
    act->setText(def.text);
    if (!def.iconPath.isEmpty())
    {
        act->setIcon(QIcon(def.iconPath));
    }
    act->setToolTip(def.toolTip.isEmpty() ? def.text : def.toolTip);
    act->setObjectName(def.objectName.isEmpty() ? def.text : def.objectName);

    if (!def.commandId.isEmpty())
    {
        QObject::connect(act, &QAction::triggered, act,
                         [activator = m_activator, commandId = def.commandId, act]()
                         {
                             if (activator)
                             {
                                 activator(commandId, act);
                             }
                         });
    }
    else
    {
        QObject::connect(act, &QAction::triggered, act, [trigger = def.trigger]() { trigger(); });
    }

    m_actions[def.id] = act;
    if (def.enableFn)
    {
        m_tracked.push_back({def.enableFn, act});
    }
    return act;
}

void UIRibbonManager::install()
{
    if (m_installed)
    {
        qWarning("UIRibbonManager: install called more than once");
        return;
    }
    if (!m_registry.isFinalized())
    {
        qWarning("UIRibbonManager: registry must be finalized before install");
        return;
    }
    m_installed = true;

    for (const auto& categoryDef : m_registry.categories())
    {
        std::vector<std::pair<const UIRibbonPanelDef*, std::vector<const UIRibbonEntry*>>> panels;
        for (const UIRibbonPanelDef* panelDef : m_registry.panelsOf(categoryDef.id))
        {
            auto entries = m_registry.entriesOf(panelDef->id);
            if (!entries.empty())
            {
                panels.emplace_back(panelDef, std::move(entries));
            }
        }
        if (panels.empty())
        {
            continue;
        }

        SARibbonCategory* category = m_bar.addCategoryPage(categoryDef.title);
        category->setObjectName(categoryDef.objectName.isEmpty() ? categoryDef.id : categoryDef.objectName);
        m_categories[categoryDef.id] = category;
        if (categoryDef.enableFn)
        {
            m_tracked.push_back({categoryDef.enableFn, category});
        }

        for (const auto& [panelDef, entries] : panels)
        {
            SARibbonPannel* panel = category->addPannel(panelDef->title);
            panel->setObjectName(panelDef->id);
            if (panelDef->enableFn)
            {
                m_tracked.push_back({panelDef->enableFn, panel});
            }

            // 连续的按钮排进同一个按钮组；遇到自定义控件时先把已有按钮组放进
            // 面板，保持注册顺序。
            SARibbonButtonGroupWidget* group = nullptr;
            auto flushGroup = [&group, panel]()
            {
                if (group)
                {
                    panel->addLargeWidget(group);
                    group = nullptr;
                }
            };

            for (const UIRibbonEntry* entry : entries)
            {
                if (const auto* actionDef = std::get_if<UIRibbonActionDef>(entry))
                {
                    if (!group)
                    {
                        group = createButtonGroup(panel, panelDef->rows);
                    }
                    QAction* act = createAction(*actionDef);
                    if (panelDef->iconOnly)
                    {
                        auto* button = new QToolButton(group);
                        button->setDefaultAction(act);
                        button->setProperty("yiCadStandaloneRibbonButton", true);
                        group->addWidget(button);
                    }
                    else
                    {
                        group->addAction(act);
                    }
                    continue;
                }

                flushGroup();
                const auto& widgetDef = std::get<UIRibbonWidgetDef>(*entry);
                if (QWidget* widget = widgetDef.factory(panel))
                {
                    panel->addLargeWidget(widget);
                    if (widgetDef.enableFn)
                    {
                        m_tracked.push_back({widgetDef.enableFn, widget});
                    }
                }
            }
            flushGroup();
        }
    }

    for (const UIRibbonEntry* entry : m_registry.entriesOf(QString::fromLatin1(UIRibbonIds::kPanelRightButtons)))
    {
        if (const auto* actionDef = std::get_if<UIRibbonActionDef>(entry))
        {
            m_bar.rightButtonGroup()->addAction(createAction(*actionDef));
        }
        else
        {
            qWarning("UIRibbonManager: widget '%s' is not supported in the right button group; skipped",
                     qUtf8Printable(std::get<UIRibbonWidgetDef>(*entry).id));
        }
    }

    evaluateActivation();
}

void UIRibbonManager::evaluateActivation()
{
    const UIRibbonContext ctx = m_contextProvider ? m_contextProvider() : UIRibbonContext{};
    for (const auto& tracked : m_tracked)
    {
        if (!tracked.target)
        {
            continue;
        }
        const bool enabled = tracked.enableFn(ctx);
        if (auto* widget = qobject_cast<QWidget*>(tracked.target.data()))
        {
            widget->setEnabled(enabled);
        }
        else if (auto* act = qobject_cast<QAction*>(tracked.target.data()))
        {
            act->setEnabled(enabled);
        }
    }
}

SARibbonCategory* UIRibbonManager::category(const QString& id) const
{
    auto it = m_categories.find(id);
    return it == m_categories.end() ? nullptr : it->second;
}

QAction* UIRibbonManager::action(const QString& id) const
{
    auto it = m_actions.find(id);
    return it == m_actions.end() ? nullptr : it->second;
}
