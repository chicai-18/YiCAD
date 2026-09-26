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

/// @file SplineData.cpp
/// @brief Nurbs样条曲线数据类实现

#include "SplineData.h"

SplineData::SplineData()
    : EntityData()
    , m_eType(ESplineType::eControlPoints)
    , m_iDegree(1)
    , m_bClosed(false)
    , m_vecControlPoints(std::vector<DmVector>())
    , m_vecKnots(std::vector<double>())
{
    setEntityType(EEntityType::eSpline);
}

SplineData::SplineData(int degree, bool closed, ESplineType splineType /*= ESplineType::eControlPoints*/)
    : EntityData()
	, m_eType(splineType)
	, m_iDegree(degree)
	, m_bClosed(closed)
	, m_vecControlPoints(std::vector<DmVector>())
	, m_vecKnots(std::vector<double>())
{
	setEntityType(EEntityType::eSpline);
}

ESplineType SplineData::getSplineType() const
{
	return m_eType;
}

void SplineData::setSplineType(const ESplineType& eSplineType)
{
	m_eType = eSplineType;
}

int SplineData::getDegree() const
{
	return m_iDegree;
}

void SplineData::setDegree(const int& iDegree)
{
	m_iDegree = iDegree;
}

bool SplineData::getIsClosed() const
{
	return m_bClosed;
}

void SplineData::setIsClosed(const bool& isClosed)
{
	m_bClosed = isClosed;
}

std::vector<DmVector> SplineData::getControlPoints() const
{
	return m_vecControlPoints;
}

void SplineData::setControlPoints(const std::vector<DmVector>& controlPoints)
{
	m_vecControlPoints = controlPoints;
}

DmVector SplineData::getControlPointAt(int i) const
{
    return m_vecControlPoints.at(i);
}

int SplineData::getControlPointsSize() const
{
    return (int)m_vecControlPoints.size();
}

void SplineData::removeControlPointAt(int i)
{
     m_vecControlPoints.erase(m_vecControlPoints.begin() + i);
}

std::vector<double> SplineData::getKnots() const
{
	return m_vecKnots;
}

double SplineData::getKnotAt(int i) const
{
    return m_vecKnots.at(i);
}

void SplineData::setKnots(const std::vector<double>& knots)
{
	m_vecKnots = knots;
}

int SplineData::getKnotsSize() const
{
    return (int)m_vecKnots.size();
}

void SplineData::removeKnotAt(int i)
{
    m_vecKnots.erase(m_vecKnots.begin() + i);
}

std::vector<DmVector> SplineData::getFitPoints() const
{
    return m_vecFitPoints;
}

void SplineData::setFitPoints(const std::vector<DmVector>& fitPoints)
{
    m_vecFitPoints = fitPoints;
}

DmVector SplineData::getFitPointAt(int i) const
{
    return m_vecFitPoints.at(i);
}

int SplineData::getFitPointsSize() const
{
    return (int)m_vecFitPoints.size();
}

void SplineData::removeFitPointAt(int i)
{
    m_vecFitPoints.erase(m_vecFitPoints.begin() + i);
}