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

/// @file UIActionGroupManager.h
/// @brief 操作分组管理器，管理工具栏中各类操作的QActionGroup

#ifndef UIACTIONGROUPMANAGER_H
#define UIACTIONGROUPMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>

class QActionGroup;
class QAction;

class UIActionGroupManager : public QObject
{
    Q_OBJECT

public:
    explicit UIActionGroupManager(QObject* parent);

    QActionGroup* block = nullptr;
    QActionGroup* circle = nullptr;
    QActionGroup* curve = nullptr;
    QActionGroup* edit = nullptr;
    QActionGroup* ellipse = nullptr;
    QActionGroup* file = nullptr;
    QActionGroup* dimension = nullptr;
    QActionGroup* info = nullptr;
    QActionGroup* layer = nullptr;
    QActionGroup* line = nullptr;
    QActionGroup* modify = nullptr;
    QActionGroup* options = nullptr;
    QActionGroup* other = nullptr;
    QActionGroup* polyline = nullptr;
    QActionGroup* restriction = nullptr;
    QActionGroup* select = nullptr;
    QActionGroup* snap = nullptr;
    QActionGroup* snap_extras = nullptr;
    QActionGroup* view = nullptr;
    QActionGroup* widgets = nullptr;

    QList<QActionGroup*> toolGroups();
    QMap<QString, QActionGroup*> allGroups();
    void sortGroupsByName(QList<QActionGroup*>& list);

public slots:
    void toggleExclusiveSnapMode(bool state);
    void toggleTools(bool state);

private:
    QList<bool> snap_memory;

};

#endif // UIACTIONGROUPMANAGER_H
