/// @file FakeExtensionHost.h
/// @brief 扩展框架单测用的 IExtensionHost 替身：Ribbon 用真实的注册表（按扩展限定命名空间
///        的入口也是真实实现），其余服务只记录调用或返回空

#ifndef YICAD_TEST_FAKE_EXTENSION_HOST_H
#define YICAD_TEST_FAKE_EXTENSION_HOST_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CommandRegistry.h"
#include "IExtensionHost.h"
#include "UIRibbonRegistry.h"

namespace yicad_test
{
/// @brief 最小的 IExtensionHost 测试替身：Ribbon 用真实的注册表（按扩展
/// 限定命名空间的入口也是真实实现），其余服务只记录调用。
class FakeExtensionHost : public IExtensionHost
{
public:
    UIRibbonRegistrar& ribbonFor(std::string_view extensionId) override
    {
        auto& registrar = m_scoped[std::string(extensionId)];
        if (!registrar)
        {
            registrar = std::make_unique<UIRibbonScopedRegistrar>(ribbon, extensionId);
        }
        return *registrar;
    }

    QWidget* mainWindow() override { return nullptr; }
    DmDocument* currentDocument() const override { return nullptr; }
    GuiDocumentView* currentDocumentView() const override { return nullptr; }
    UITabDrawWidget* tabDrawWidget() override { return nullptr; }

    bool registerSettingsPage(const QString& id, const QString&, const QString&,
                              std::function<void()>) override
    {
        settingsPages.push_back(id);
        return true;
    }

    bool activateCommand(const QString& commandId) override
    {
        activated.push_back(commandId);
        return CommandRegistry::instance().hasCommand(commandId);
    }

    UIRibbonRegistry ribbon;
    std::vector<QString> settingsPages;
    std::vector<QString> activated;

private:
    std::map<std::string, std::unique_ptr<UIRibbonScopedRegistrar>> m_scoped;
};
}  // namespace yicad_test

#endif  // YICAD_TEST_FAKE_EXTENSION_HOST_H
