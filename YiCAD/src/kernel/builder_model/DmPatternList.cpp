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


/// @file DmPatternList.cpp
/// @brief 填充图案表实现：单例、文件加载、图案查询

#include "DmPatternList.h"

#include <iostream>
#include <QString>
#include "Math2d.h"

#include "DmSystem.h"
#include "DmPattern.h"
#include "Debug.h"

/// @brief 获取全局唯一图案列表实例
/// @return 单例指针
DmPatternList* DmPatternList::instance()
{
    static DmPatternList instance;
    return &instance;
}

DmPatternList::~DmPatternList() = default;

/// @brief 初始化图案列表，从.pat文件解析并加载图案
void DmPatternList::init()
{
    /// <summary>
    /// 找到.pat文件并存入List
    /// </summary>

    QStringList list = DMSYSTEM->getPatternList();
    if (list.size() != 2)
    {
        return;
    }

    m_patterns.clear();

    QFile file(list[1]);
    if (!file.exists())
    {
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    else
    {
        while (file.atEnd() == false)
        {
            QString fileline(file.readLine());
            if (fileline.startsWith(";", Qt::CaseSensitive))
            {
                continue;
            }
            else if (fileline.startsWith("*", Qt::CaseSensitive))
            {
                // 获取pattern name
                QStringList strList = fileline.split(",");
                QString pat_name = strList[0];
                QString name = pat_name.remove(0, 1);
                DmPattern* m_pat = new DmPattern(name.toStdWString());

                // 获取pattern data
                /// 循环读取名字之后每一行，遇*和;以及空格 退出循环
                while (file.atEnd() == false)
                {
                    qint64 cursor = file.pos();
                    QString s(file.readLine());
                    if (s.startsWith(";", Qt::CaseSensitive)
                        || s.startsWith("*", Qt::CaseSensitive)
                        || s.startsWith(" ", Qt::CaseSensitive))
                    {
                        file.seek(cursor);
                        // 回退游标到上一行
                        break;
                    }

                    s.remove(QRegExp("\\s"));
                    QStringList slist = s.split(",");

                    std::vector<double> pat;
                    pat.reserve(slist.count());

                    for (int i = 0; i < slist.count(); i++)
                    {
                        if (i == 0)
                        {
                            pat.emplace_back(
                                Math2d::deg2rad(slist[i].toDouble()));
                        }
                        else
                        {
                            pat.emplace_back(slist[i].toDouble());
                        }
                    }

                    m_pat->addPattern(pat);
                }

                // 初始化pattern
                m_patterns[QString::fromStdWString(
                    m_pat->getPatternName())] =
                    std::unique_ptr<DmPattern>{ m_pat };
            }
        } // while
    }

    file.close();
}

/// @brief 获取图案数量
/// @return 图案总数
int DmPatternList::countPatterns() const
{
    return m_patterns.size();
}

std::map<QString, std::unique_ptr<DmPattern>>::iterator DmPatternList::begin()
{
    return m_patterns.begin();
}

std::map<QString, std::unique_ptr<DmPattern>>::const_iterator
DmPatternList::begin() const
{
    return m_patterns.begin();
}

std::map<QString, std::unique_ptr<DmPattern>>::iterator DmPatternList::end()
{
    return m_patterns.end();
}

std::map<QString, std::unique_ptr<DmPattern>>::const_iterator
DmPatternList::end() const
{
    return m_patterns.end();
}

/// @brief 按名称请求图案，若未加载则自动加载
/// @param name 图案名称
/// @return 图案指针，未找到返回 nullptr
DmPattern* DmPatternList::requestPattern(const QString& name)
{
    QString name2 = name;
    if (m_patterns.count(name2))
    {
        if (!m_patterns[name2])
        {
            DmPattern* p = new DmPattern(name2.toStdWString());
            m_patterns[name2].reset(p);
        }

        return m_patterns[name2].get();
    }

    return nullptr;
}

/// @brief 检查是否包含指定名称的图案
/// @param name 图案名称
/// @return 包含返回 true
bool DmPatternList::contains(const QString& name) const
{
    return m_patterns.count(name);
}
