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

/// @file LLMSettingsService.h
/// @brief LLM 配置服务 —— 普通配置走 DmSettings，密钥走 SecretStorage
///
/// 职责：
///   - 统一管理 AI/LLM 相关的配置项读写
///   - 普通配置（Provider、BaseUrl、Model、Timeout、Temperature）落在 DmSettings
///   - API Key 通过 SecretStorage 安全存储，不落 QSettings 明文
///   - 提供默认值
///
/// 使用方式：
///   @code
///     LLMSettingsService* svc = LLMSettingsService::instance();
///     svc->init(companyKey, appKey);
///     svc->setBaseUrl("https://api.deepseek.com/v1");
///     QString key = svc->apiKey();
///   @endcode
///
/// 注意：
///   - 必须先调用 init() 再使用，init() 内部会创建 SecretStorage 实例
///   - 单例生命周期由调用方管理（建议在主窗口析构时调用 shutdown()）

#ifndef LLMSETTINGSSERVICE_H
#define LLMSETTINGSSERVICE_H

#include <QString>
#include <memory>

class SecretStorage;

// ---------------------------------------------------------------------------
// 默认值
// ---------------------------------------------------------------------------

namespace LLMDefaults
{
    constexpr char kProvider[]    = "deepseek";
    constexpr char kBaseUrl[]     = "https://api.deepseek.com/v1";
    constexpr char kModel[]       = "deepseek-chat";
    constexpr int  kTimeoutSecs   = 30;
    constexpr double kTemperature = 0.7;
    constexpr char kSecretKey[]   = "llm_api_key";  // SecretStorage 中使用的 key
}  // namespace LLMDefaults

// ---------------------------------------------------------------------------
// LLMSettingsService
// ---------------------------------------------------------------------------

class LLMSettingsService
{
public:
    ~LLMSettingsService();

    /// @brief 获取单例（调用前需先执行 init()）
    static LLMSettingsService* instance();

    /// @brief 初始化服务，创建 SecretStorage
    /// @param companyKey DmSettings 中使用的公司标识
    /// @param appKey     DmSettings 中使用的应用标识
    void init(const QString& companyKey, const QString& appKey);

    /// @brief 销毁单例，释放 SecretStorage
    static void shutdown();

    /// @brief 是否已完成初始化
    bool isInitialized() const { return m_initialized; }

    // ---- 普通配置（走 DmSettings, group = "AI/LLM"） ----

    QString provider() const;
    void    setProvider(const QString& v);

    QString baseUrl() const;
    void    setBaseUrl(const QString& v);

    QString model() const;
    void    setModel(const QString& v);

    int     timeoutSecs() const;
    void    setTimeoutSecs(int v);

    double  temperature() const;
    void    setTemperature(double v);

    // ---- 密钥（走 SecretStorage） ----

    /// @brief 获取 API Key，不存在则返回空字符串
    QString apiKey() const;

    /// @brief 设置 API Key，空字符串视为删除
    void    setApiKey(const QString& key);

    // ---- 整体校验 ----

    /// @brief 检查是否已配置 API Key（非空即视为已配置）
    bool hasApiKey() const;

    /// @brief 检查关键配置项是否齐全
    bool isConfigured() const;

private:
    LLMSettingsService();
    LLMSettingsService(const LLMSettingsService&) = delete;
    LLMSettingsService& operator=(const LLMSettingsService&) = delete;

    static LLMSettingsService* s_instance;

    bool                        m_initialized;
    std::unique_ptr<SecretStorage> m_secretStorage;

    // DmSettings 使用的 group 名
    static constexpr char kSettingsGroup[] = "AI/LLM";
};

#endif // LLMSETTINGSSERVICE_H
