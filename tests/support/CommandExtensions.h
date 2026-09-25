/// @file CommandExtensions.h
/// @brief 在用例期间启动原内置命令所在的五个扩展
///
/// 业务工具化第四步把原 src/actions/ 的内置命令拆进了绘图、修改、查询、编辑、视图五个扩展
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节），命令不再自注册，要经扩展启动才在
/// CommandRegistry 里。ExtensionManager 是单例，启动之后不能再登记别的扩展，所以需要
/// 同时启动其它扩展的用例先调用 registerCommandExtensions()，再自己 BootAll()。

#ifndef YICAD_TEST_COMMAND_EXTENSIONS_H
#define YICAD_TEST_COMMAND_EXTENSIONS_H

#include <memory>

#include "DrawExtension.h"
#include "EditExtension.h"
#include "ExtensionManager.h"
#include "MeasureExtension.h"
#include "ModifyExtension.h"
#include "ViewExtension.h"
#include "support/FakeExtensionHost.h"

namespace yicad_test
{
/// @brief 登记五个扩展，在 ExtensionManager::BootAll() 之前调用；用例结束时由 Shutdown() 注销
inline void registerCommandExtensions()
{
    ExtensionManager& manager = ExtensionManager::instance();
    manager.Register(std::make_unique<DrawExtension>());
    manager.Register(std::make_unique<ModifyExtension>());
    manager.Register(std::make_unique<MeasureExtension>());
    manager.Register(std::make_unique<EditExtension>());
    manager.Register(std::make_unique<ViewExtension>());
}

/// @brief 构造时启动五个扩展，析构时注销
struct CommandExtensionsScope
{
    FakeExtensionHost host;

    CommandExtensionsScope()
    {
        registerCommandExtensions();
        ExtensionManager::instance().BootAll(host);
    }
    ~CommandExtensionsScope() { ExtensionManager::instance().Shutdown(); }

    CommandExtensionsScope(const CommandExtensionsScope&) = delete;
    CommandExtensionsScope& operator=(const CommandExtensionsScope&) = delete;
};
}  // namespace yicad_test

#endif  // YICAD_TEST_COMMAND_EXTENSIONS_H
