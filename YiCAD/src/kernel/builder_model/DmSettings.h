/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmSettings.h
/// @brief 应用程序设置管理类，支持读写配置和缓存

#ifndef DMSETTINGS_H
#define DMSETTINGS_H

#include <QString>
#include <map>

class QVariant;

// ---------------------------------------------------------------------------
// Default Settings
// ---------------------------------------------------------------------------

namespace Colors
{
    const QString SNAP_INDICATOR = "#FFC200";
    const QString BACKGROUND     = "#1E1E1E";
    const QString GRID           = "#323748";
    const QString META_GRID      = "#494F69";
    const QString SELECT         = "#4D80FF";
    const QString HIGHLIGHT      = "#FFFFFF";
    const QString START_HANDLE   = "Cyan";
    const QString HANDLE         = "Blue";
    const QString END_HANDLE     = "Blue";
}  // namespace Colors

// ---------------------------------------------------------------------------

// TODO: 考虑改为 constexpr 函数
#define DMSETTINGS DmSettings::instance()

/// This class can store and reload settings from a
/// configuration file or the windoze registry.
/// Please note that the Qt default implementation doesn't
/// work as one would expect. That's why this class overwrites
/// most of the default behaviour.
class DmSettings
{
public:
    ~DmSettings();

    void deleteDmStettings();

    /// @return Instance to the unique settings object.
    static DmSettings* instance();

    /// @brief Initialize the system.
    /// @param companyKey Company Key
    /// @param appKey Application key
    void init(const QString& companyKey, const QString& appKey);

    void beginGroup(const QString& group);
    void endGroup();

    bool writeEntry(const QString& key, int value);
    bool writeEntry(const QString& key, double value);
    bool writeEntry(const QString& key, const QVariant& value);
    bool writeEntry(const QString& key, const QString& value);
    QString readEntry(const QString& key, const QString& def = QString(), bool* ok = 0);
    QByteArray readByteArrayEntry(const QString& key, const QString& def = QString(), bool* ok = 0);
    int readNumEntry(const QString& key, int def = 0);
    void clear_all();
    void clear_geometry();
    static bool save_is_allowed;

private:
    DmSettings();
    DmSettings(DmSettings const&) = delete;
    DmSettings& operator=(DmSettings const&) = delete;
    QVariant readEntryCache(const QString& key);
    void addToCache(const QString& key, const QVariant& value);

private:
    static DmSettings*          m_pUniqueInstance;

    std::map<QString, QVariant> cache;
    QString                     companyKey;
    QString                     appKey;
    QString                     group;
    bool                        initialized;
};

#endif
