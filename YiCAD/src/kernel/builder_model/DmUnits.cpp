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


/// @file DmUnits.cpp
/// @brief DmUnits 单位转换工具类的实现

#include<iostream>
#include<cmath>
#include <QObject>
#include <QStringList>
#include "DmUnits.h"
#include "Math2d.h"
#include "DmVector.h"
#include "Debug.h"

// Converts a DXF integer () to a Unit enum.
DM::Unit DmUnits::dxfint2unit(int dxfint)
{
	return (DM::Unit)dxfint;
}

/// @return a short string representing the given unit (e.g. "mm")
QString DmUnits::unitToSign(DM::Unit u)
{
	QString ret = "";

	switch (u)
	{
	case DM::None:
		ret = "";
		break;
	case DM::Inch:
		ret = "\"";
		break;
	case DM::Foot:
		ret = "'";
		break;
	case DM::Mile:
		ret = "mi";
		break;
	case DM::Millimeter:
		ret = "mm";
		break;
	case DM::Centimeter:
		ret = "cm";
		break;
	case DM::Meter:
		ret = "m";
		break;
	case DM::Kilometer:
		ret = "km";
		break;
	case DM::Microinch:
		ret = "µ\"";
		break;
	case DM::Mil:
		ret = "mil";
		break;
	case DM::Yard:
		ret = "yd";
		break;
	case DM::Angstrom:
		ret = "A";
		break;
	case DM::Nanometer:
		ret = "nm";
		break;
	case DM::Micron:
		ret = "µm";
		break;
	case DM::Decimeter:
		ret = "dm";
		break;
	case DM::Decameter:
		ret = "dam";
		break;
	case DM::Hectometer:
		ret = "hm";
		break;
	case DM::Gigameter:
		ret = "Gm";
		break;
	case DM::Astro:
		ret = "astro";
		break;
	case DM::Lightyear:
		ret = "ly";
		break;
	case DM::Parsec:
		ret = "pc";
		break;
	default:
		ret = "";
		break;
	}

	return ret;
}

/// @return a string representing the given unit (e.g. "Millimeter").
/// translated if @a t is true (the default).
QString DmUnits::unitToString(DM::Unit u, bool t)
{
	switch (u)
	{
	case DM::None:
		return t ? QObject::tr("None", "unknown length unit") : "None";
	case DM::Inch:
		return t ? QObject::tr("Inch") : "Inch";
	case DM::Foot:
		return t ? QObject::tr("Foot") : "Foot";
	case DM::Mile:
		return t ? QObject::tr("Mile") : "Mile";
	case DM::Millimeter:
		return t ? QObject::tr("Millimeter") : "Millimeter";
	case DM::Centimeter:
		return t ? QObject::tr("Centimeter") : "Centimeter";
	case DM::Meter:
		return t ? QObject::tr("Meter") : "Meter";
	case DM::Kilometer:
		return t ? QObject::tr("Kilometer") : "Kilometer";
	case DM::Microinch:
		return t ? QObject::tr("Microinch") : "Microinch";
	case DM::Mil:
		return t ? QObject::tr("Mil") : "Mil";
	case DM::Yard:
		return t ? QObject::tr("Yard") : "Yard";
	case DM::Angstrom:
		return t ? QObject::tr("Angstrom") : "Angstrom";
	case DM::Nanometer:
		return t ? QObject::tr("Nanometer") : "Nanometer";
	case DM::Micron:
		return t ? QObject::tr("Micron") : "Micron";
	case DM::Decimeter:
		return t ? QObject::tr("Decimeter") : "Decimeter";
	case DM::Decameter:
		return t ? QObject::tr("Decameter") : "Decameter";
	case DM::Hectometer:
		return t ? QObject::tr("Hectometer") : "Hectometer";
	case DM::Gigameter:
		return t ? QObject::tr("Gigameter") : "Gigameter";
	case DM::Astro:
		return t ? QObject::tr("Astro") : "Astro";
	case DM::Lightyear:
		return t ? QObject::tr("Lightyear") : "Lightyear";
	case DM::Parsec:
		return t ? QObject::tr("Parsec") : "Parsec";
	default:
		return "";
	}
}

// Converts a string into a unit enum.
DM::Unit DmUnits::stringToUnit(const QString& u)
{
	DM::Unit ret = DM::None;

	if (u == "None")
	{
		ret = DM::None;
	}
	else if (u == QObject::tr("Inch"))
	{
		ret = DM::Inch;
	}
	else if (u == QObject::tr("Foot"))
	{
		ret = DM::Foot;
	}
	else if (u == QObject::tr("Mile"))
	{
		ret = DM::Mile;
	}
	else if (u == QObject::tr("Millimeter"))
	{
		ret = DM::Millimeter;
	}
	else if (u == QObject::tr("Centimeter"))
	{
		ret = DM::Centimeter;
	}
	else if (u == QObject::tr("Meter"))
	{
		ret = DM::Meter;
	}
	else if (u == QObject::tr("Kilometer"))
	{
		ret = DM::Kilometer;
	}
	else if (u == QObject::tr("Microinch"))
	{
		ret = DM::Microinch;
	}
	else if (u == QObject::tr("Mil"))
	{
		ret = DM::Mil;
	}
	else if (u == QObject::tr("Yard"))
	{
		ret = DM::Yard;
	}
	else if (u == QObject::tr("Angstrom"))
	{
		ret = DM::Angstrom;
	}
	else if (u == QObject::tr("Nanometer"))
	{
		ret = DM::Nanometer;
	}
	else if (u == QObject::tr("Micron"))
	{
		ret = DM::Micron;
	}
	else if (u == QObject::tr("Decimeter"))
	{
		ret = DM::Decimeter;
	}
	else if (u == QObject::tr("Decameter"))
	{
		ret = DM::Decameter;
	}
	else if (u == QObject::tr("Hectometer"))
	{
		ret = DM::Hectometer;
	}
	else if (u == QObject::tr("Gigameter"))
	{
		ret = DM::Gigameter;
	}
	else if (u == QObject::tr("Astro"))
	{
		ret = DM::Astro;
	}
	else if (u == QObject::tr("Lightyear"))
	{
		ret = DM::Lightyear;
	}
	else if (u == QObject::tr("Parsec"))
	{
		ret = DM::Parsec;
	}

	return ret;
}

/// @return true: the unit is metric, false: the unit is imperial.
bool DmUnits::isMetric(DM::Unit u)
{
	switch (u)
	{
	case DM::Millimeter:
	case DM::Centimeter:
	case DM::Meter:
	case DM::Kilometer:
	case DM::Angstrom:
	case DM::Nanometer:
	case DM::Micron:
	case DM::Decimeter:
	case DM::Decameter:
	case DM::Hectometer:
	case DM::Gigameter:
	case DM::Astro:
	case DM::Lightyear:
	case DM::Parsec:
		return true;
	default:
		return false;
	}
}

/// @return factor to convert the given unit to Millimeters.
double DmUnits::getFactorToMM(DM::Unit u)
{
	switch (u)
	{
	default:
	case DM::None:
	case DM::Millimeter:
		return 1.0;
	case DM::Inch:
		return 25.4;
	case DM::Foot:
		return 304.8;
	case DM::Mile:
		return 1.609344e6; // international mile
	case DM::Centimeter:
		return 10;
	case DM::Meter:
		return 1e3;
	case DM::Kilometer:
		return 1e6;
	case DM::Microinch:
		return 2.54e-5;
	case DM::Mil:
		return 0.0254;
	case DM::Yard:
		return 914.4;
	case DM::Angstrom:
		return 1e-7;
	case DM::Nanometer:
		return 1e-6;
	case DM::Micron:
		return 1e-3;
	case DM::Decimeter:
		return 100.0;
	case DM::Decameter:
		return 1e4;
	case DM::Hectometer:
		return 1e5;
	case DM::Gigameter:
		return 1e9;
	case DM::Astro:
		return 1.495978707e14;
	case DM::Lightyear:
		return 9.4607304725808e18;
	case DM::Parsec:
		return 3.0856776e19;
	}
}

// Converts the given value 'val' from unit 'src' to unit 'dest'.
double DmUnits::convert(double val, DM::Unit src, DM::Unit dest)
{
	if (getFactorToMM(dest) > 0.0)
	{
		return (val * getFactorToMM(src)) / getFactorToMM(dest);
	}
	else
	{
		return val;
	}
}

// Converts the given vector 'val' from unit 'src' to unit 'dest'.
DmVector DmUnits::convert(const DmVector& val, DM::Unit src, DM::Unit dest)
{
	return DmVector(convert(val.x, src, dest), convert(val.y, src, dest), convert(val.z, src, dest));
}

///	@brief Formats the given length in the given format.
///	@param length The length in the current unit of the drawing.
/// @param format Format of the string.
/// @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
/// @param showUnit Append unit to the value.
QString DmUnits::formatLinear(double length, DM::Unit unit, DM::LinearFormat format, int prec, bool showUnit)
{
	QString ret;

	switch (format)
	{
	case DM::Scientific:
		ret = formatScientific(length, unit, prec, showUnit);
		break;

	case DM::Decimal:
		ret = formatDecimal(length, unit, prec, showUnit);
		break;

	case DM::Engineering:
		ret = formatEngineering(length, unit, prec, showUnit);
		break;

	case DM::Architectural:
		ret = formatArchitectural(length, unit, prec, showUnit);
		break;

	case DM::Fractional:
		ret = formatFractional(length, unit, prec, showUnit);
		break;

	case DM::ArchitecturalMetric:
		ret = formatArchitecturalMetric(length, unit, prec, showUnit);
		break;

	default:
		ret = "";
		break;
	}

	return ret;
}

/// @brief Formats the given length in scientific format (e.g. 2.5E7).
///	@param length The length in the current unit of the drawing.
/// @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
/// @param showUnit Append unit to the value.
QString DmUnits::formatScientific(double length, DM::Unit unit, int prec, bool showUnit)
{
	QString const ret = QString("%1").arg(length, 0, 'E', prec);
	if (showUnit)
	{
		return ret + unitToSign(unit);
	}
	return ret;
}

/// @brief Formats the given length in decimal (normal) format (e.g. 2.5).
///	@param length The length in the current unit of the drawing.
/// @param prec Precision of the value (e.g. 0.001)
/// @param showUnit Append unit to the value.
QString DmUnits::formatDecimal(double length, DM::Unit unit, int prec, bool showUnit)
{
	QString const ret = Math2d::doubleToString(length, prec);

	if (showUnit)
	{
		return ret + unitToSign(unit);
	}
	return ret;
}

/// @brief Formats the given length in engineering format (e.g. 5' 4.5").
///	@param length The length in the current unit of the drawing.
/// @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
/// @param showUnit Append unit to the value.
QString DmUnits::formatEngineering(double length, DM::Unit /*unit*/, int prec, bool /*showUnit*/)
{
	QString ret;

	bool sign = (length < 0.0);
	int feet = (int)floor(fabs(length) / 12);
	double inches = fabs(length) - feet * 12;

	QString sInches = Math2d::doubleToString(inches, prec);

	if (sInches == "12")
	{
		feet++;
		sInches = "0";
	}

	if (feet)
	{
		ret = QString("%1'-%2\"").arg(feet).arg(sInches);
	}
	else
	{
		ret = QString("%1\"").arg(sInches);
	}

	if (sign)
	{
		ret = "-" + ret;
	}

	return ret;
}

/// @brief Formats the given length in architectural format (e.g. 5' 4 1/2").
///	@param length The length in the current unit of the drawing.
/// @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
/// @param showUnit Append unit to the value.
QString DmUnits::formatArchitectural(double length, DM::Unit /*unit*/, int prec, bool showUnit)
{
	QString ret;
	bool neg = (length < 0.0);

	int feet = (int)floor(fabs(length) / 12);
	double inches = fabs(length) - feet * 12;

	QString sInches = formatFractional(inches, DM::Inch, prec, showUnit);

	if (sInches == "12")
	{
		feet++;
		sInches = "0";
	}

	if (neg)
	{
		ret = QString("-%1'-%2\"").arg(feet).arg(sInches);
	}
	else
	{
		ret = QString("%1'-%2\"").arg(feet).arg(sInches);
	}

	return ret;
}

QString DmUnits::formatArchitecturalMetric(double length, DM::Unit unit, int prec, bool showUnit)
{
	QString ret;
	bool neg = (length < 0.0);
	QString zero = "0";

	if (neg)
	{
		length = length * -1.0;
	}

	ret = Math2d::doubleToString(length, prec + 1);
	int iLast = QString(ret.right(1)).toInt();

	// round on 0.005 and use superscript 5
	if ((iLast > 2) && (iLast < 8))
	{
		ret = ret.replace(ret.length() - 1, 1, "\u2075");
	}
	else
	{
		ret = Math2d::doubleToString(length, prec);
	}
	if (ret.startsWith(zero))
	{
		ret = ret.split(".")[1];
		if (ret.startsWith(zero))
		{
			ret = ret.remove(0, 1);
		}
	}
	if (showUnit)
	{
		ret = QString("%1 %2").arg(ret).arg(unitToSign(unit));
	}
	if (neg)
	{
		ret = QString("-%1").arg(ret);
	}
	return ret;
}

/// @brief Formats the given length in fractional (barbarian) format (e.g. 5' 3 1/64").
/// @param length The length in the current unit of the drawing.
/// @param unit Should be inches.
/// @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
/// @param showUnit Append unit to the value.
QString DmUnits::formatFractional(double length, DM::Unit /*unit*/, int prec, bool /*showUnit*/)
{
	QString ret;

	unsigned num;            // number of complete inches (num' 7/128")
	unsigned nominator;      // number of fractions (nominator/128)
	unsigned denominator;    // (7/denominator)

	// sign:
	QString neg = "";
	if (length < 0)
	{
		neg = "-";
		length = fabs(length);
	}

	num = (unsigned)floor(length);

	denominator = 2 << prec;
	nominator = (unsigned)Math2d::round((length - num) * denominator);

	if (nominator == denominator)
	{
		nominator = 0;
		denominator = 0;
		++num;
	}

	// Simplify the fraction
	if (nominator && denominator)
	{
		unsigned gcd = Math2d::findGCD(nominator, denominator);
		if (gcd)
		{
			nominator = nominator / gcd;
			denominator = denominator / gcd;
		}
		else
		{
			nominator = 0;
			denominator = 0;
		}
	}

	if (num && nominator)
	{
		ret = QString("%1%2 %3/%4").arg(neg).arg(num).arg(nominator).arg(denominator);
	}
	else if (nominator)
	{
		ret = QString("%1%2/%3").arg(neg).arg(nominator).arg(denominator);
	}
	else if (num)
	{
		ret = QString("%1%2").arg(neg).arg(num);
	}
	else
	{
		ret = "0";
	}

	return ret;
}

QString DmUnits::formatAngle(double angle, DM::AngleFormat format, int prec)
{
	QString ret;
	double value;

	switch (format)
	{
	case DM::Surveyors:
	case DM::DegreesDecimal:
	case DM::DegreesMinutesSeconds:
		value = Math2d::rad2deg(angle);
		break;
	case DM::Radians:
		value = angle;
		break;
	case DM::Gradians:
		value = Math2d::rad2gra(angle);
		break;
	default:
		return "";
		break;
	}

	switch (format)
	{
	case DM::DegreesDecimal:
	case DM::Radians:
	case DM::Gradians:
		ret = Math2d::doubleToString(value, prec);
		if (format == DM::DegreesDecimal)
		{
			ret += QChar(0xB0);
		}
		if (format == DM::Radians)
		{
			ret += "r";
		}
		if (format == DM::Gradians)
		{
			ret += "g";
		}
		break;

	case DM::DegreesMinutesSeconds:
	{
		int vDegrees, vMinutes;
		double vSeconds;
		QString degrees, minutes, seconds;

		vDegrees = (int)floor(value);
		vMinutes = (int)floor((value - vDegrees) * 60.0);
		vSeconds = (value - vDegrees - (vMinutes / 60.0)) * 3600.0;

		seconds = Math2d::doubleToString(vSeconds, (prec > 1 ? prec - 2 : 0));

		if (seconds == "60")
		{
			seconds = "0";
			++vMinutes;
			if (vMinutes == 60)
			{
				vMinutes = 0;
				++vDegrees;
			}
		}

		if (prec == 0 && vMinutes >= 30.0)
		{
			vDegrees++;
		}
		else if (prec == 1 && vSeconds >= 30.0)
		{
			vMinutes++;
		}

		degrees.setNum(vDegrees);
		minutes.setNum(vMinutes);

		switch (prec)
		{
		case 0:
			ret = degrees + QChar(0xB0);
			break;
		case 1:
			ret = degrees + QChar(0xB0) + " " + minutes + "'";
			break;
		default:
			ret = degrees + QChar(0xB0) + " " + minutes + "' "
				+ seconds + "\"";
			break;
		}
	}
	break;
	case DM::Surveyors:
	{
		QString prefix, suffix;
		int quadrant;
		quadrant = ((int)floor(value) / 90);
		switch (quadrant)
		{
		case 0:
			prefix = "N";
			suffix = "E";
			break;
		case 1:
			prefix = "S";
			suffix = "E";
			value = 180. - value;
			break;
		case 2:
			prefix = "S";
			suffix = "W";
			value = value - 180.;
			break;
		case 3:
			prefix = "N";
			suffix = "W";
			value = 360. - value;
			break;
		}
		ret = prefix + formatAngle(Math2d::deg2rad(value), DM::DegreesMinutesSeconds, prec) + suffix;
		ret.replace(QChar(0xB0), "d");
		ret.replace(" ", "");
	}
	break;
	default:
		break;
	}
	return ret;
}

DM::AngleFormat DmUnits::numberToAngleFormat(int num)
{
	DM::AngleFormat af;

	switch (num)
	{
	default:
	case 0:
		af = DM::DegreesDecimal;
		break;
	case 1:
		af = DM::DegreesMinutesSeconds;
		break;
	case 2:
		af = DM::Gradians;
		break;
	case 3:
		af = DM::Radians;
		break;
	case 4:
		af = DM::Surveyors;
		break;
	}

	return af;
}

/// @return Size of the given paper format.
DmVector DmUnits::paperFormatToSize(DM::PaperFormat p)
{
	switch (p)
	{
	case DM::Custom:
		return DmVector(0.0, 0.0);

	case DM::A0:
		return DmVector(841.0, 1189.0);
	case DM::A1:
		return DmVector(594.0, 841.0);
	case DM::A2:
		return DmVector(420.0, 594.0);
	case DM::A3:
		return DmVector(297.0, 420.0);
	case DM::A4:
		return DmVector(210.0, 297.0);

	case DM::Letter:	// 8.5 x 11.0 in.  Sizes shown are used for 'hard' conversion to metric
		return DmVector(215.9, 279.4);
	case DM::Legal:	// 8.5 x 14.0 in
		return DmVector(215.9, 355.6);
	case DM::Tabloid:  // 11.0 x 17.0
		return DmVector(279.4, 431.8);

	case DM::Ansi_C:	// 17.0 x 22.0 in
		return DmVector(431.8, 558.8);
	case DM::Ansi_D:	// 22.0 x 34.0 in
		return DmVector(558.8, 863.6);
	case DM::Ansi_E:	// 34.0 x 44.0 in
		return DmVector(863.6, 1117.6);

	case DM::Arch_A:	// 9.0 x 12.0 in
		return DmVector(228.6, 304.8);
	case DM::Arch_B:	// 12.0 x 18.0 in
		return DmVector(304.8, 457.2);
	case DM::Arch_C:	// 18.0 x 24.0 in
		return DmVector(457.2, 609.6);
	case DM::Arch_D:	// 24.0 x 36.0 in
		return DmVector(609.6, 914.4);
	case DM::Arch_E:	// 36.0 x 48.0 in
		return DmVector(914.4, 1219.2);

	default:
		break;
	}

	return DmVector(false);
}

// Gets the paper format which matches the given size. If no format matches, DM::Custom is returned.
DM::PaperFormat DmUnits::paperSizeToFormat(const DmVector& s)
{
	DmVector ts1;
	DmVector ts2;

	for (DM::PaperFormat i = DM::FirstPaperFormat; DM::NPageFormat > i; i = static_cast<DM::PaperFormat>(i + 1))
	{
		ts1 = DmUnits::paperFormatToSize(i);
		ts2 = DmVector(ts1.y, ts1.x);

		if (ts1.distanceTo(s) < 1.0e-4 || ts2.distanceTo(s) < 1.0e-4)
		{
			return i;
		}
	}

	return DM::Custom;
}

// Converts a paper format to a string (e.g. for a combobox).
QString DmUnits::paperFormatToString(DM::PaperFormat p)
{
	switch (p)
	{
	case DM::Custom: return QObject::tr("Custom", "Paper format");

	case DM::A0: return QObject::tr("A0", "Paper format");
	case DM::A1: return QObject::tr("A1", "Paper format");
	case DM::A2: return QObject::tr("A2", "Paper format");
	case DM::A3: return QObject::tr("A3", "Paper format");
	case DM::A4: return QObject::tr("A4", "Paper format");

	case DM::Letter: return QObject::tr("Letter/ANSI A", "Paper format");
	case DM::Legal:  return QObject::tr("Legal", "Paper format");
	case DM::Tabloid: return QObject::tr("Tabloid/ANSI B", "Paper format");

	case DM::Ansi_C: return QObject::tr("ANSI C", "Paper format");
	case DM::Ansi_D: return QObject::tr("ANSI D", "Paper format");
	case DM::Ansi_E: return QObject::tr("ANSI E", "Paper format");

	case DM::Arch_A: return QObject::tr("Arch A", "Paper format");
	case DM::Arch_B: return QObject::tr("Arch B", "Paper format");
	case DM::Arch_C: return QObject::tr("Arch C", "Paper format");
	case DM::Arch_D: return QObject::tr("Arch D", "Paper format");
	case DM::Arch_E: return QObject::tr("Arch E", "Paper format");

	default:
		break;
	}

	return QStringLiteral("");
}

// Converts a string to a paper format.
DM::PaperFormat DmUnits::stringToPaperFormat(const QString& p)
{
	QString ls{ p.toLower() };

	if (ls == QStringLiteral("custom") || ls == QObject::tr("custom", "Paper format").toLower())
		return DM::Custom;

	if (ls == QStringLiteral("a0") || ls == QObject::tr("a0", "Paper format").toLower())
		return DM::A0;
	if (ls == QStringLiteral("a1") || ls == QObject::tr("a1", "Paper format").toLower())
		return DM::A1;
	if (ls == QStringLiteral("a2") || ls == QObject::tr("a2", "Paper format").toLower())
		return DM::A2;
	if (ls == QStringLiteral("a3") || ls == QObject::tr("a3", "Paper format").toLower())
		return DM::A3;
	if (ls == QStringLiteral("a4") || ls == QObject::tr("a4", "Paper format").toLower())
		return DM::A4;

	if (ls == QStringLiteral("letter") || ls == QObject::tr("letter", "Paper format").toLower())
		return DM::Letter;
	if (ls == QStringLiteral("legal") || ls == QObject::tr("legal", "Paper format").toLower())
		return DM::Legal;
	if (ls == QStringLiteral("tabloid") || ls == QObject::tr("tabloid", "Paper format").toLower())
		return DM::Tabloid;

	if (ls == QStringLiteral("ansi c") || ls == QObject::tr("ansi c", "Paper format").toLower())
		return DM::Ansi_C;
	if (ls == QStringLiteral("ansi d") || ls == QObject::tr("ansi d", "Paper format").toLower())
		return DM::Ansi_D;
	if (ls == QStringLiteral("ansi e") || ls == QObject::tr("ansi e", "Paper format").toLower())
		return DM::Ansi_E;

	if (ls == QStringLiteral("arch a") || ls == QObject::tr("arch a", "Paper format").toLower())
		return DM::Arch_A;
	if (ls == QStringLiteral("arch b") || ls == QObject::tr("arch b", "Paper format").toLower())
		return DM::Arch_B;
	if (ls == QStringLiteral("arch c") || ls == QObject::tr("arch c", "Paper format").toLower())
		return DM::Arch_C;
	if (ls == QStringLiteral("arch d") || ls == QObject::tr("arch d", "Paper format").toLower())
		return DM::Arch_D;
	if (ls == QStringLiteral("arch e") || ls == QObject::tr("arch e", "Paper format").toLower())
		return DM::Arch_E;

	return DM::Custom;
}

// Calculates a scaling factor from given dpi and units.
double DmUnits::dpiToScale(double dpi, DM::Unit unit)
{
	double scale = DmUnits::convert(1.0, DM::Inch, unit) / dpi;
	return scale;
}

// Calculates a dpi value from given scaling factor and units.
double DmUnits::scaleToDpi(double scale, DM::Unit unit)
{
	double dpi = DmUnits::convert(1.0, DM::Inch, unit) / scale;
	return dpi;
}
