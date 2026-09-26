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


/// @file DmUnits.h
/// @brief 单位转换工具类

#ifndef DMUNITS_H
#define DMUNITS_H

#include "Datamodel.h"

class DmVector;
class QString;

/// @brief 单位转换类
class DmUnits
{
public:
    static DM::Unit dxfint2unit(int dxfint);

    static QString unitToString(DM::Unit u, bool t = true);
    static DM::Unit stringToUnit(const QString& u);

    static bool isMetric(DM::Unit u);
    static double getFactorToMM(DM::Unit u);
    static double convert(double val, DM::Unit src, DM::Unit dest);
    static DmVector convert(const DmVector& val, DM::Unit src, DM::Unit dest);

    static QString unitToSign(DM::Unit u);

    static QString formatLinear(double length, DM::Unit unit, DM::LinearFormat format, int prec, bool showUnit = false);
    static QString formatScientific(double length, DM::Unit unit, int prec, bool showUnit = false);
    static QString formatDecimal(double length, DM::Unit unit, int prec, bool showUnit = false);
    static QString formatEngineering(double length, DM::Unit unit, int prec, bool showUnit = false);
    static QString formatArchitectural(double length, DM::Unit unit, int prec, bool showUnit = false);
    static QString formatFractional(double length, DM::Unit unit, int prec, bool showUnit = false);
    static QString formatArchitecturalMetric(double length, DM::Unit unit, int prec, bool showUnit = false);

    static QString formatAngle(double angle, DM::AngleFormat format, int prec);
    static DM::AngleFormat numberToAngleFormat(int num);

    static DmVector paperFormatToSize(DM::PaperFormat p);
    static DM::PaperFormat paperSizeToFormat(const DmVector& s);

    static QString paperFormatToString(DM::PaperFormat p);
    static DM::PaperFormat stringToPaperFormat(const QString& p);

    static double dpiToScale(double dpi, DM::Unit unit);
    static double scaleToDpi(double scale, DM::Unit unit);
};

#endif
