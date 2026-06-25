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

/// @file SecretStorage.h
/// @brief 密钥安全存储接口
///
/// 设计意图：
///   - 抽象密钥存储，使上层代码不关心底层实现
///   - 调用方通过接口写入/读取敏感数据（API Key 等）
///
/// 平台实现：
///   - Windows：DpapiSecretStorage，使用 CryptProtectData / CryptUnprotectData
///   - 其他平台：XorSecretStorage（回退，仅混淆，非安全存储）
///
/// 后续替换点（Linux/macOS）：
///   - 使用 libsecret (GNOME Keyring) 或 macOS Keychain
///   - 同样通过工厂函数切换

#ifndef SECRETSTORAGE_H
#define SECRETSTORAGE_H

#include <QString>

/// @brief 密钥存储抽象接口
class SecretStorage
{
public:
    virtual ~SecretStorage() = default;

    /// @brief 检查当前后端是否可用
    virtual bool isAvailable() const = 0;

    /// @brief 存储密钥
    /// @param key 密钥标识（如 "llm_api_key"）
    /// @param secret 明文密钥数据
    /// @return true 成功
    virtual bool store(const QString& key, const QString& secret) = 0;

    /// @brief 读取密钥
    /// @param key 密钥标识
    /// @return 明文密钥，不存在则返回空字符串
    virtual QString retrieve(const QString& key) const = 0;

    /// @brief 删除密钥
    /// @param key 密钥标识
    /// @return true 成功
    virtual bool remove(const QString& key) = 0;
};

// ---------------------------------------------------------------------------
// 工厂函数 —— 根据平台返回对应实现
// ---------------------------------------------------------------------------

/// @brief 根据当前平台创建合适的 SecretStorage 实现
///
/// Windows 下返回 DpapiSecretStorage，
/// 其他平台返回 XorSecretStorage（回退）。
SecretStorage* createSecretStorage(const QString& companyKey,
                                   const QString& appKey);

// ---------------------------------------------------------------------------
// Windows DPAPI 实现
// ---------------------------------------------------------------------------

#ifdef Q_OS_WIN

/// @brief 基于 Windows DPAPI 的安全密钥存储
///
/// 使用 CryptProtectData / CryptUnprotectData 进行加密，
/// 密文以 Base64 存入 QSettings。
/// 加密绑定到当前用户，其他用户无法解密。
class DpapiSecretStorage : public SecretStorage
{
public:
    DpapiSecretStorage(const QString& companyKey, const QString& appKey);
    ~DpapiSecretStorage() override = default;

    bool isAvailable() const override;
    bool store(const QString& key, const QString& secret) override;
    QString retrieve(const QString& key) const override;
    bool remove(const QString& key) override;

private:
    QString m_companyKey;
    QString m_appKey;
};

#endif // Q_OS_WIN

// ---------------------------------------------------------------------------
// 临时实现：XorSecretStorage（非 Windows 平台回退）
// ---------------------------------------------------------------------------

/// @brief 基于 XOR 混淆 + Base64 + QSettings 的临时密钥存储
///
/// 【安全警告】
///   此实现不提供真正的加密保护。XOR 混淆可被轻易逆向。
///   仅用于非 Windows 平台的回退实现。
class XorSecretStorage : public SecretStorage
{
public:
    XorSecretStorage(const QString& companyKey, const QString& appKey);
    ~XorSecretStorage() override = default;

    bool isAvailable() const override;
    bool store(const QString& key, const QString& secret) override;
    QString retrieve(const QString& key) const override;
    bool remove(const QString& key) override;

private:
    /// @brief XOR 混淆（临时）
    static QByteArray obfuscate(const QByteArray& data);

    /// @brief XOR 反混淆（临时）
    static QByteArray deobfuscate(const QByteArray& data);

    QString m_companyKey;
    QString m_appKey;
};

#endif // SECRETSTORAGE_H
