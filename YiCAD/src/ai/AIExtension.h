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

/// @file AIExtension.h
/// @brief ai/ 模块作为进程内扩展的入口。
///
/// 阶段4第二阶段的试点扩展（doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4
/// §7.4任务④）：把原先硬编码在 ApplicationWindow 构造函数里的 AI 按钮
/// 创建、`LLMSettingsService::init()` 调用、以及新增的设置页入口注册，
/// 都收进这一个 `IExtension` 实现里。

#ifndef AIEXTENSION_H
#define AIEXTENSION_H

#include <memory>

#include "IExtension.h"

class IExtensionContext;
class QWidget;
class AIAssistant;

class AIExtension : public IExtension
{
public:
    // 构造/析构都声明在这里、定义放到 .cpp——m_assistant 是
    // unique_ptr<AIAssistant>，这里只前置声明了 AIAssistant。隐式生成的
    // 构造/析构函数会在任何调用 std::make_unique<AIExtension>() 的翻译
    // 单元里实例化（比如 ApplicationWindow.cpp，它不需要也不包含
    // AIAssistant.h），那时 AIAssistant 还是不完整类型，编译不过。两者
    // 都显式声明 + 挪到 AIExtension.cpp 定义，把实例化点固定在真正
    // #include "AIAssistant.h" 的地方（标准的 pimpl 惯用法）。
    AIExtension();
    ~AIExtension() override;

    void OnRegister(IExtensionContext& ctx) override;
    void OnShutdown() override;
    std::string_view Id() const override { return "ext.ai"; }

private:
    /// @brief OnRegister 期间存下来的上下文指针，供后续（点击回调、
    /// 设置页回调）现场取当前文档/视图。安全性依赖
    /// ApplicationWindowExtensionContext 的具体生命周期保证（活到
    /// ExtensionManager::Shutdown() 为止），不是 IExtensionContext
    /// 接口本身的通用契约——见 IExtension.h 的说明。
    IExtensionContext* m_ctx = nullptr;
    QWidget* m_mainWindow = nullptr;
    std::unique_ptr<AIAssistant> m_assistant;
};

#endif  // AIEXTENSION_H
