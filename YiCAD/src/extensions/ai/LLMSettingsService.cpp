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

/// @file LLMSettingsService.cpp
/// @brief LLMSettingsService 实现

#include "LLMSettingsService.h"
#include "SecretStorage.h"

#include "DmSettings.h"

LLMSettingsService* LLMSettingsService::s_instance = nullptr;

LLMSettingsService::LLMSettingsService()
    : m_initialized(false)
{
}

LLMSettingsService::~LLMSettingsService()
{
    // unique_ptr 自动释放 SecretStorage
}

LLMSettingsService* LLMSettingsService::instance()
{
    if (!s_instance)
    {
        s_instance = new LLMSettingsService();
    }
    return s_instance;
}

void LLMSettingsService::init(const QString& companyKey,
                              const QString& appKey)
{
    if (m_initialized)
    {
        return;
    }

    // 创建 SecretStorage（通过工厂函数，后续替换点在此）
    m_secretStorage.reset(createSecretStorage(companyKey, appKey));
    m_initialized = true;
}

void LLMSettingsService::shutdown()
{
    if (s_instance)
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

// ---------------------------------------------------------------------------
// 普通配置 —— 复用 DmSettings, group = "AI/LLM"
// ---------------------------------------------------------------------------

QString LLMSettingsService::provider() const
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    QString val = DMSETTINGS->readEntry("Provider",
                                        QLatin1String(LLMDefaults::kProvider));
    DMSETTINGS->endGroup();
    return val;
}

void LLMSettingsService::setProvider(const QString& v)
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    DMSETTINGS->writeEntry("Provider", v);
    DMSETTINGS->endGroup();
}

QString LLMSettingsService::baseUrl() const
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    QString val = DMSETTINGS->readEntry("BaseUrl",
                                        QLatin1String(LLMDefaults::kBaseUrl));
    DMSETTINGS->endGroup();
    return val;
}

void LLMSettingsService::setBaseUrl(const QString& v)
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    DMSETTINGS->writeEntry("BaseUrl", v);
    DMSETTINGS->endGroup();
}

QString LLMSettingsService::model() const
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    QString val = DMSETTINGS->readEntry("Model",
                                        QLatin1String(LLMDefaults::kModel));
    DMSETTINGS->endGroup();
    return val;
}

void LLMSettingsService::setModel(const QString& v)
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    DMSETTINGS->writeEntry("Model", v);
    DMSETTINGS->endGroup();
}

int LLMSettingsService::timeoutSecs() const
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    int val = DMSETTINGS->readNumEntry("Timeout", LLMDefaults::kTimeoutSecs);
    DMSETTINGS->endGroup();
    return val;
}

void LLMSettingsService::setTimeoutSecs(int v)
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    DMSETTINGS->writeEntry("Timeout", v);
    DMSETTINGS->endGroup();
}

double LLMSettingsService::temperature() const
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    QString val = DMSETTINGS->readEntry("Temperature",
                                        QString::number(LLMDefaults::kTemperature, 'f', 2));
    DMSETTINGS->endGroup();
    return val.toDouble();
}

void LLMSettingsService::setTemperature(double v)
{
    DMSETTINGS->beginGroup(QLatin1String(kSettingsGroup));
    DMSETTINGS->writeEntry("Temperature", v);
    DMSETTINGS->endGroup();
}

// ---------------------------------------------------------------------------
// 密钥 —— 走 SecretStorage
// ---------------------------------------------------------------------------

QString LLMSettingsService::apiKey() const
{
    if (!m_secretStorage)
    {
        return QString();
    }
    return m_secretStorage->retrieve(
        QLatin1String(LLMDefaults::kSecretKey));
}

void LLMSettingsService::setApiKey(const QString& key)
{
    if (!m_secretStorage)
    {
        return;
    }

    if (key.isEmpty())
    {
        m_secretStorage->remove(QLatin1String(LLMDefaults::kSecretKey));
    }
    else
    {
        m_secretStorage->store(QLatin1String(LLMDefaults::kSecretKey), key);
    }
}

// ---------------------------------------------------------------------------
// 校验
// ---------------------------------------------------------------------------

bool LLMSettingsService::hasApiKey() const
{
    return !apiKey().isEmpty();
}

bool LLMSettingsService::isConfigured() const
{
    return hasApiKey() && !baseUrl().isEmpty();
}
