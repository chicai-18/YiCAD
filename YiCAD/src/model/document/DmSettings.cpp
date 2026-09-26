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


/// @file DmSettings.cpp
/// @brief DmSettings 设置管理类的实现，包括QSettings读写和缓存机制

#include <QSettings>
#include "DmSettings.h"

DmSettings* DmSettings::m_pUniqueInstance = nullptr;
bool DmSettings::save_is_allowed = true;

DmSettings::DmSettings()
    : initialized(false)
{
}

DmSettings* DmSettings::instance()
{
    if (!m_pUniqueInstance)
    {
		m_pUniqueInstance = new DmSettings();
	}
	return m_pUniqueInstance;
}

/// Initialisation.
/// @param companyKey String that identifies the company. Must start with a "/". E.g. "/RibbonSoft"
/// @param appKey String that identifies the application. Must start  with a "/". E.g. "/YiCAD"
void DmSettings::init(const QString& companyKey, const QString& appKey)
{
    group = "";
	
    this->companyKey = companyKey;
    this->appKey = appKey;
    initialized = true;
}

DmSettings::~DmSettings()
{
}

void DmSettings::deleteDmStettings()
{
    if (m_pUniqueInstance)
    {
        delete m_pUniqueInstance;
        m_pUniqueInstance = nullptr;
    }
}

void DmSettings::beginGroup(const QString& group)
{
    this->group = group;
}

void DmSettings::endGroup()
{
    this->group = "";
}

bool DmSettings::writeEntry(const QString& key, int value)
{
    return writeEntry(key, QVariant(value));
}

bool DmSettings::writeEntry(const QString& key,const QString& value)
{
    return writeEntry(key, QVariant(value));
}

bool DmSettings::writeEntry(const QString& key, double value)
{
    return writeEntry(key, QVariant(value));
}

bool DmSettings::writeEntry(const QString& key, const QVariant& value)
{
    QVariant ret = readEntryCache(key);
    if (ret.isValid() && ret == value)
    {
        return true;
    }

    QSettings s(companyKey, appKey);

    s.setValue(QString("%1%2").arg(group).arg(key), value);
	cache[key]=value;

    return true;
}

QString DmSettings::readEntry(const QString& key, const QString& def,bool* ok)
{
    QVariant ret = readEntryCache(key);
    if (!ret.isValid())
    {
        //读取注册表，如果注册表已有数据，正常读取，否则返回true
        QSettings s(companyKey, appKey);
        QString groupKey = QString("%1%2").arg(group).arg(key);
        QStringList allKey = s.allKeys();
        bool containKey = s.contains(groupKey);
        ret = s.value(groupKey, QVariant(def));
        if(containKey)
        {
            cache[key] = ret;
        }
        if (ok) {
            *ok = containKey;
        }
    }
    else
    {
        if (ok)
            *ok = true;
    }

    return ret.toString();
}

QByteArray DmSettings::readByteArrayEntry(const QString& key,const QString& def,bool* ok)
{
    QVariant ret = readEntryCache(key);
    if (!ret.isValid())
    {
        QSettings s(companyKey, appKey);
        bool flag = s.contains(QString("%1%2").arg(group).arg(key));
        ret = s.value(QString("%1%2").arg(group).arg(key), QVariant(def));
        if (flag)
        {
            cache[key] = ret;
        }
        if (ok)
        {
            *ok = flag;
        }
    }
    else
    {
        if (ok)
            *ok = true;
    }

    return ret.toByteArray();

}

int DmSettings::readNumEntry(const QString& key, int def)
{
	QVariant value = readEntryCache(key);
	if (!value.isValid())
	{
		QSettings s(companyKey, appKey);
        QString str = QString("%1%2").arg(group).arg(key);
		value = s.value(str, QVariant(def));
        cache[key] = value;
	}
	return value.toInt();
}

QVariant DmSettings::readEntryCache(const QString& key)
{
    if (!cache.count(key))
    {
        return QVariant();
    }
	return cache[key];
}

void DmSettings::addToCache(const QString& key, const QVariant& value)
{
    cache[key]=value;
}

void DmSettings::clear_all()
{
    QSettings s(companyKey, appKey);
    s.clear();
    save_is_allowed = false;
}

void DmSettings::clear_geometry()
{
    QSettings s(companyKey, appKey);
    s.remove("/Geometry");
    save_is_allowed = false;
}
