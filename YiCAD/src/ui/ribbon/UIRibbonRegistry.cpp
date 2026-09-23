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

/// @file UIRibbonRegistry.cpp

#include "UIRibbonRegistry.h"

#include <algorithm>
#include <utility>

#include <QDebug>

#include "CommandRegistry.h"
#include "ExtensionNamespace.h"

namespace
{
/// @brief 条目的 ID（按钮的 ID 为空时取 commandId）。
const QString& entryId(const UIRibbonEntry& entry)
{
    if (const auto* action = std::get_if<UIRibbonActionDef>(&entry))
    {
        return action->id;
    }
    return std::get<UIRibbonWidgetDef>(entry).id;
}

const QString& entryPanelId(const UIRibbonEntry& entry)
{
    if (const auto* action = std::get_if<UIRibbonActionDef>(&entry))
    {
        return action->panelId;
    }
    return std::get<UIRibbonWidgetDef>(entry).panelId;
}
}  // namespace

UIRibbonRequires deriveSatisfied(const UIRibbonContext& ctx)
{
    UIRibbonRequires satisfied = UIRibbonRequires::None;
    if (ctx.document)
    {
        satisfied = satisfied | UIRibbonRequires::DocumentOpen;
    }
    return satisfied;
}

UIRibbonEnableFn UIRibbonCondition::requireAll(UIRibbonRequires flags)
{
    return [flags](const UIRibbonContext& ctx) { return (deriveSatisfied(ctx) & flags) == flags; };
}

// ── UIRibbonRegistry ───────────────────────────────────────────────────────

bool UIRibbonRegistry::acceptAdd(const char* kind, const QString& id) const
{
    if (m_finalized)
    {
        qWarning("UIRibbonRegistry: %s '%s' added after finalize; skipped", kind, qUtf8Printable(id));
        return false;
    }
    if (id.isEmpty())
    {
        qWarning("UIRibbonRegistry: %s with empty id; skipped", kind);
        return false;
    }
    return true;
}

bool UIRibbonRegistry::hasEntry(const QString& id) const
{
    return std::any_of(m_entries.begin(), m_entries.end(),
                       [&id](const UIRibbonEntry& entry) { return entryId(entry) == id; });
}

bool UIRibbonRegistry::addCategory(UIRibbonCategoryDef def)
{
    if (!acceptAdd("category", def.id))
    {
        return false;
    }
    if (std::any_of(m_categories.begin(), m_categories.end(),
                    [&def](const UIRibbonCategoryDef& c) { return c.id == def.id; }))
    {
        qWarning("UIRibbonRegistry: duplicate category id '%s'; skipped", qUtf8Printable(def.id));
        return false;
    }
    m_categories.push_back(std::move(def));
    return true;
}

bool UIRibbonRegistry::addPanel(UIRibbonPanelDef def)
{
    if (!acceptAdd("panel", def.id))
    {
        return false;
    }
    if (std::any_of(m_panels.begin(), m_panels.end(),
                    [&def](const UIRibbonPanelDef& p) { return p.id == def.id; }))
    {
        qWarning("UIRibbonRegistry: duplicate panel id '%s'; skipped", qUtf8Printable(def.id));
        return false;
    }
    m_panels.push_back(std::move(def));
    return true;
}

bool UIRibbonRegistry::addAction(UIRibbonActionDef def)
{
    if (def.id.isEmpty())
    {
        def.id = def.commandId;
    }
    if (!acceptAdd("action", def.id))
    {
        return false;
    }
    if (def.commandId.isEmpty() == !def.trigger)
    {
        qWarning("UIRibbonRegistry: action '%s' must have exactly one of commandId/trigger; skipped",
                 qUtf8Printable(def.id));
        return false;
    }
    if (hasEntry(def.id))
    {
        qWarning("UIRibbonRegistry: duplicate entry id '%s'; skipped", qUtf8Printable(def.id));
        return false;
    }
    m_entries.emplace_back(std::move(def));
    return true;
}

bool UIRibbonRegistry::addWidget(UIRibbonWidgetDef def)
{
    if (!acceptAdd("widget", def.id))
    {
        return false;
    }
    if (!def.factory)
    {
        qWarning("UIRibbonRegistry: widget '%s' without factory; skipped", qUtf8Printable(def.id));
        return false;
    }
    if (hasEntry(def.id))
    {
        qWarning("UIRibbonRegistry: duplicate entry id '%s'; skipped", qUtf8Printable(def.id));
        return false;
    }
    m_entries.emplace_back(std::move(def));
    return true;
}

void UIRibbonRegistry::finalize()
{
    if (m_finalized)
    {
        qWarning("UIRibbonRegistry: finalize called more than once");
        return;
    }

    auto hasCategory = [this](const QString& id)
    {
        return std::any_of(m_categories.begin(), m_categories.end(),
                           [&id](const UIRibbonCategoryDef& c) { return c.id == id; });
    };
    m_panels.erase(std::remove_if(m_panels.begin(), m_panels.end(),
                                  [&hasCategory](const UIRibbonPanelDef& p)
                                  {
                                      if (hasCategory(p.categoryId))
                                      {
                                          return false;
                                      }
                                      qWarning("UIRibbonRegistry: panel '%s' references missing category '%s'; skipped",
                                               qUtf8Printable(p.id), qUtf8Printable(p.categoryId));
                                      return true;
                                  }),
                   m_panels.end());

    auto hasPanel = [this](const QString& id)
    {
        return id == QLatin1String(UIRibbonIds::kPanelRightButtons)
            || std::any_of(m_panels.begin(), m_panels.end(),
                           [&id](const UIRibbonPanelDef& p) { return p.id == id; });
    };
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                                   [&hasPanel](const UIRibbonEntry& entry)
                                   {
                                       if (!hasPanel(entryPanelId(entry)))
                                       {
                                           qWarning("UIRibbonRegistry: entry '%s' references missing panel '%s'; skipped",
                                                    qUtf8Printable(entryId(entry)),
                                                    qUtf8Printable(entryPanelId(entry)));
                                           return true;
                                       }
                                       const auto* action = std::get_if<UIRibbonActionDef>(&entry);
                                       if (action && !action->commandId.isEmpty()
                                           && !CommandRegistry::instance().hasCommand(action->commandId))
                                       {
                                           qWarning("UIRibbonRegistry: action '%s' references unregistered command '%s'; skipped",
                                                    qUtf8Printable(action->id), qUtf8Printable(action->commandId));
                                           return true;
                                       }
                                       return false;
                                   }),
                    m_entries.end());

    m_finalized = true;
}

std::vector<const UIRibbonPanelDef*> UIRibbonRegistry::panelsOf(const QString& categoryId) const
{
    std::vector<const UIRibbonPanelDef*> out;
    for (const auto& panel : m_panels)
    {
        if (panel.categoryId == categoryId)
        {
            out.push_back(&panel);
        }
    }
    return out;
}

std::vector<const UIRibbonEntry*> UIRibbonRegistry::entriesOf(const QString& panelId) const
{
    std::vector<const UIRibbonEntry*> out;
    for (const auto& entry : m_entries)
    {
        if (entryPanelId(entry) == panelId)
        {
            out.push_back(&entry);
        }
    }
    return out;
}

// ── UIRibbonScopedRegistrar ────────────────────────────────────────────────

UIRibbonScopedRegistrar::UIRibbonScopedRegistrar(UIRibbonRegistrar& target, std::string_view extensionId)
    : m_target(target), m_extensionId(extensionId)
{
}

bool UIRibbonScopedRegistrar::owns(const char* kind, const QString& id) const
{
    if (isInExtensionNamespace(m_extensionId, id))
    {
        return true;
    }
    qWarning("UIRibbonRegistry: %s '%s' is outside extension namespace '%s.'; skipped", kind,
             qUtf8Printable(id), m_extensionId.c_str());
    return false;
}

bool UIRibbonScopedRegistrar::addCategory(UIRibbonCategoryDef def)
{
    return owns("category", def.id) && m_target.addCategory(std::move(def));
}

bool UIRibbonScopedRegistrar::addPanel(UIRibbonPanelDef def)
{
    return owns("panel", def.id) && m_target.addPanel(std::move(def));
}

bool UIRibbonScopedRegistrar::addAction(UIRibbonActionDef def)
{
    const QString& id = def.id.isEmpty() ? def.commandId : def.id;
    return owns("action", id) && m_target.addAction(std::move(def));
}

bool UIRibbonScopedRegistrar::addWidget(UIRibbonWidgetDef def)
{
    return owns("widget", def.id) && m_target.addWidget(std::move(def));
}
