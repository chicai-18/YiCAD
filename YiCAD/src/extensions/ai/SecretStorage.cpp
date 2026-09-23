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

/// @file SecretStorage.cpp
/// @brief SecretStorage 接口及平台实现

#include "SecretStorage.h"

#include <QByteArray>
#include <QSettings>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
#endif

// ---------------------------------------------------------------------------
// 工厂函数
// ---------------------------------------------------------------------------

SecretStorage* createSecretStorage(const QString& companyKey,
                                   const QString& appKey)
{
#ifdef Q_OS_WIN
    return new DpapiSecretStorage(companyKey, appKey);
#else
    return new XorSecretStorage(companyKey, appKey);
#endif
}

// ---------------------------------------------------------------------------
// DpapiSecretStorage（Windows DPAPI 实现）
// ---------------------------------------------------------------------------

#ifdef Q_OS_WIN

namespace {

/// QSettings 存储时使用的组名
constexpr char kDpapiSettingsGroup[] = "AI/Secrets";

/// @brief 使用 DPAPI 加密数据
/// @return Base64 编码的密文，失败返回空字符串
QString dpapiEncrypt(const QByteArray& plainBytes)
{
    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(plainBytes.data()));
    dataIn.cbData = static_cast<DWORD>(plainBytes.size());

    DATA_BLOB dataOut = {};
    BOOL result = CryptProtectData(
        &dataIn,
        L"YiCAD API Key",   // 描述字符串
        nullptr,             // 可选熵
        nullptr,             // 保留
        nullptr,             // 提示结构
        0,                   // 标志
        &dataOut
    );

    if (!result)
    {
        return QString();
    }

    QByteArray cipher(reinterpret_cast<const char*>(dataOut.pbData), static_cast<int>(dataOut.cbData));
    LocalFree(dataOut.pbData);

    return QString::fromLatin1(cipher.toBase64());
}

/// @brief 使用 DPAPI 解密数据
/// @return 明文字节，失败返回空
QByteArray dpapiDecrypt(const QString& encoded)
{
    QByteArray cipher = QByteArray::fromBase64(encoded.toLatin1());

    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(cipher.data());
    dataIn.cbData = static_cast<DWORD>(cipher.size());

    DATA_BLOB dataOut = {};
    LPWSTR description = nullptr;
    BOOL result = CryptUnprotectData(
        &dataIn,
        &description,       // 输出描述字符串
        nullptr,             // 可选熵
        nullptr,             // 保留
        nullptr,             // 提示结构
        0,                   // 标志
        &dataOut
    );

    if (description)
    {
        LocalFree(description);
    }

    if (!result)
    {
        return QByteArray();
    }

    QByteArray plain(reinterpret_cast<const char*>(dataOut.pbData), static_cast<int>(dataOut.cbData));
    LocalFree(dataOut.pbData);

    return plain;
}

}  // namespace

DpapiSecretStorage::DpapiSecretStorage(const QString& companyKey,
                                       const QString& appKey)
    : m_companyKey(companyKey)
    , m_appKey(appKey)
{
}

bool DpapiSecretStorage::isAvailable() const
{
    // 测试 DPAPI 是否可用：加密再解密一段测试数据
    QByteArray test = "dpapi-test";
    QString encrypted = dpapiEncrypt(test);
    if (encrypted.isEmpty())
    {
        return false;
    }
    QByteArray decrypted = dpapiDecrypt(encrypted);
    return decrypted == test;
}

bool DpapiSecretStorage::store(const QString& key, const QString& secret)
{
    if (key.isEmpty())
    {
        return false;
    }

    const QByteArray plainBytes = secret.toUtf8();
    const QString encoded = dpapiEncrypt(plainBytes);

    if (encoded.isEmpty())
    {
        return false;
    }

    QSettings s(m_companyKey, m_appKey);
    s.beginGroup(QLatin1String(kDpapiSettingsGroup));
    s.setValue(key, encoded);
    s.endGroup();

    return true;
}

QString DpapiSecretStorage::retrieve(const QString& key) const
{
    QSettings s(m_companyKey, m_appKey);
    s.beginGroup(QLatin1String(kDpapiSettingsGroup));

    if (!s.contains(key))
    {
        s.endGroup();
        return QString();
    }

    const QString encoded = s.value(key).toString();
    const QByteArray plainBytes = dpapiDecrypt(encoded);

    s.endGroup();
    return QString::fromUtf8(plainBytes);
}

bool DpapiSecretStorage::remove(const QString& key)
{
    QSettings s(m_companyKey, m_appKey);
    s.beginGroup(QLatin1String(kDpapiSettingsGroup));
    s.remove(key);
    s.endGroup();
    return true;
}

#endif // Q_OS_WIN

// ---------------------------------------------------------------------------
// XorSecretStorage（非 Windows 平台回退）
// ---------------------------------------------------------------------------

namespace {

/// 固定的 XOR 盐值（仅用于临时实现，不做安全保证）
/// 长度选择 32 字节以增加轻微混淆度
constexpr char kXorSalt[] = "YiCAD-AI-Secret-Salt-2026-v0.5";
constexpr int  kSaltLen   = sizeof(kXorSalt) - 1;  // 不含 '\0'

/// QSettings 存储时使用的组名
constexpr char kSettingsGroup[] = "AI/Secrets";

}  // namespace

XorSecretStorage::XorSecretStorage(const QString& companyKey,
                                   const QString& appKey)
    : m_companyKey(companyKey)
    , m_appKey(appKey)
{
}

bool XorSecretStorage::isAvailable() const
{
    // 临时实现始终可用
    return true;
}

QByteArray XorSecretStorage::obfuscate(const QByteArray& data)
{
    QByteArray result = data;
    for (int i = 0; i < result.size(); ++i)
    {
        result[i] = result[i] ^ kXorSalt[i % kSaltLen];
    }
    return result;
}

QByteArray XorSecretStorage::deobfuscate(const QByteArray& data)
{
    // XOR 是对称操作，反混淆即再次混淆
    return obfuscate(data);
}

bool XorSecretStorage::store(const QString& key, const QString& secret)
{
    if (key.isEmpty())
    {
        return false;
    }

    const QByteArray plainBytes = secret.toUtf8();
    const QByteArray obfuscated = obfuscate(plainBytes);
    const QString    encoded    = QString::fromLatin1(obfuscated.toBase64());

    // 存入 QSettings（经过 Base64 编码的混淆数据）
    QSettings s(m_companyKey, m_appKey);
    s.beginGroup(QLatin1String(kSettingsGroup));
    s.setValue(key, encoded);
    s.endGroup();

    return true;
}

QString XorSecretStorage::retrieve(const QString& key) const
{
    QSettings s(m_companyKey, m_appKey);
    s.beginGroup(QLatin1String(kSettingsGroup));

    if (!s.contains(key))
    {
        s.endGroup();
        return QString();
    }

    const QString    encoded    = s.value(key).toString();
    const QByteArray obfuscated = QByteArray::fromBase64(encoded.toLatin1());
    const QByteArray plainBytes = deobfuscate(obfuscated);

    s.endGroup();
    return QString::fromUtf8(plainBytes);
}

bool XorSecretStorage::remove(const QString& key)
{
    QSettings s(m_companyKey, m_appKey);
    s.beginGroup(QLatin1String(kSettingsGroup));
    s.remove(key);
    s.endGroup();
    return true;
}
