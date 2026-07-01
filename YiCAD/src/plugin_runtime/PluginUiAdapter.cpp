#include "PluginUiAdapter.h"

#include "PluginRegistry.h"
#include "UIActionHandler.h"
#include "UICommandWidget.h"

#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPannel.h"

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStringList>

PluginUiAdapter::PluginUiAdapter(
    SARibbonBar& ribbon,
    PluginRegistry& registry,
    UIActionHandler& actionHandler,
    UICommandWidget& commandWidget)
    : m_ribbon(ribbon),
      m_registry(registry),
      m_actionHandler(actionHandler),
      m_commandWidget(commandWidget)
{
    auto* registryPointer = &m_registry;
    m_actionHandler.setExternalCommandExecutor(
        [registryPointer](const QString& pluginId, const QString& commandId) {
            if (registryPointer->findCommand(pluginId, commandId) == nullptr)
            {
                return false;
            }
            registryPointer->executeCommand(pluginId, commandId);
            return true;
        });
}

PluginUiAdapter::~PluginUiAdapter()
{
    m_actionHandler.setExternalCommandExecutor({});
    m_commandWidget.setExternalCommandStrings({});
}

bool PluginUiAdapter::materialize()
{
    bool success = true;

    const auto& ribbonButtons = m_registry.ribbonButtons();
    for (int index = m_materializedRibbonCount;
         index < ribbonButtons.size();
         ++index)
    {
        const auto& record = ribbonButtons.at(index);

        const auto* command =
            m_registry.findCommand(record.pluginId, record.commandId);
        const auto* plugin = m_registry.findPlugin(record.pluginId);
        if (command == nullptr || plugin == nullptr)
        {
            success = false;
            continue;
        }

        auto* category = m_ribbon.categoryByName(record.tab);
        if (category == nullptr)
        {
            category = m_ribbon.addCategoryPage(record.tab);
        }

        auto* pannel = category->pannelByName(record.group);
        if (pannel == nullptr)
        {
            pannel = category->addPannel(record.group);
        }

        auto* action = new QAction(command->displayName, &m_ribbon);
        action->setObjectName(
            QStringLiteral("plugin:%1/%2")
                .arg(record.pluginId, record.commandId));

        if (!record.iconPath.isEmpty())
        {
            const QFileInfo declaredIcon(record.iconPath);
            const QString iconPath = declaredIcon.isAbsolute()
                ? declaredIcon.absoluteFilePath()
                : QDir(plugin->dllDirectory)
                      .absoluteFilePath(record.iconPath);
            const QFileInfo iconFile(iconPath);
            if (iconFile.exists() && iconFile.isFile())
            {
                const QIcon icon(iconFile.absoluteFilePath());
                if (!icon.isNull())
                {
                    action->setIcon(icon);
                }
            }
        }

        const QString pluginId = record.pluginId;
        const QString commandId = record.commandId;
        auto* registryPointer = &m_registry;
        QObject::connect(
            action,
            &QAction::triggered,
            action,
            [registryPointer, pluginId, commandId]() {
                registryPointer->executeCommand(pluginId, commandId);
            });
        pannel->addLargeAction(action);
    }
    m_materializedRibbonCount = ribbonButtons.size();

    QStringList externalCommands;
    externalCommands.reserve(m_registry.commands().size());
    for (const auto& command : m_registry.commands())
    {
        externalCommands.append(
            QStringLiteral("%1/%2")
                .arg(command.pluginId, command.commandId));
    }
    m_commandWidget.setExternalCommandStrings(externalCommands);

    return success;
}
