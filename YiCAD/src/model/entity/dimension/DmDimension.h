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


/// @file DmDimension.h
/// @brief 标注基类

#ifndef DMDIMENSION_H
#define DMDIMENSION_H

#include "DmEntity.h"
#include "DmMText.h"
#include "DmDimensionStyle.h"
#include "DimVars.h"

/// @brief 标注的一般数据
struct DmDimensionData
{
	DmDimensionData() = default;

	/// @param definitionPoint Definition point.
	/// @param middleOfText Middle point of dimension text.
	/// @param valign Vertical alignment.
	/// @param halign Horizontal alignment.
	/// @param lineSpacingStyle Line spacing style.
	/// @param lineSpacingFactor Line spacing factor.
	/// @param text Text string entered explicitly by user or null or "<>" for the actual measurement or " " (one blank space).for suppressing the text.
	/// @param style Dimension style name.
	/// @param angle Rotation angle of dimension text away from default orientation.
	DmDimensionData(const DmVector& definitionPoint, const DmVector& middleOfText, EMTextVertMode valign, EMTextHorzMode halign,
		double lineSpacingFactor, QString text, double angle, DmDimensionStyle* pStyle);

	/// @brief 定义点。对于对齐标注，为标注界线2起点到标注线的投影
	DmVector definitionPoint{false};
	EMTextVertMode valign = EMTextVertMode::kTextBottom;
	EMTextHorzMode halign = EMTextHorzMode::kTextLeft;
	double lineSpacingFactor = 0.0;
	QString text{""};
	//角度（弧度）。对于对齐标注，表示旋转角度
	double angle = 0.0;
	DmVector textCenter{false};
	DmDimensionStyle* pDimStyle = nullptr;

	///////以下为替代属性
public:
	DIM_FUNCS_DECLARE()
private:
	DIM_MEMERS_DECLARE()
};

/// @brief 标注的基类
class DmDimension : public DmEntity
{
	TYPESYSTEM_HEADER();
public:
	DmDimension();
	DmDimension(DmEntity* parent, const DmDimensionData& d);
	~DmDimension();
	DmEntity* clone() const override;

	DmVector getNearestRef(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestSelectedRef(const DmVector& coord, double* dist = nullptr) const override;

	DmDimensionData getData() const;
	DmDimensionData& getDataRef();

	/// @brief 获得标注的替代文字
	virtual QString getLabel() const;
	void setLabel(const QString& l);

	void update() override;
	virtual void updateDim(bool autoText = false);
	void addEntity(DmEntity* e);
	void clear();
	bool removeEntity(DmEntity* entity);

	DmVector getDefinitionPoint() const;

	DmVector getMiddleOfText() const;

	EMTextVertMode getVAlign() const;

	EMTextHorzMode getHAlign() const;

	double getLineSpacingFactor();
	double getLineSpacingFactor() const;

	DmDimensionStyle* getStyle();
	DmDimensionStyle* getStyle() const;

	double getAngle();

	bool isContainer() const override;
	void calculateBorders() override;
	double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr, DM::ResolveLevel level = DM::ResolveNone) const override;
	DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
	DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
	DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
	void setVisible(bool v) override;
	bool setSelected(bool select = true) override;
	void setHighlighted(bool highlight = true) override;

	void move(const DmVector& offset) override;
	void rotate(const DmVector& center, const DmVector& angleVector) override;
	void scale(const DmVector& center, const DmVector& factor) override;
	void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

	std::list<DmEntity*> getSubEntities() const override;

	DM::ArrowType stringToArrow(const std::string& arrowName);

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
	/// @brief 用文字裁剪直线
	/// @retrun 成功裁剪返回true，否则false
	bool clipLine(const DmLine* line, const DmVector& clipTextBasePt, const DmVector& textDir, const double clipTextWidth);

	/// @brief 用文字裁剪圆弧
	/// @retrun 成功裁剪返回true，否则false
	bool clipArc(const DmArc* arc, const DmVector& clipTextBasePt, const DmVector& textDir, const double clipTextWidth);

	/// @brief 判断角度是否在两者之间（包括边界）
	/// @param angle[in] 待判断角度（0-2pi）
	/// /// @param startAngle[in] 起始角度（0-2pi）
	/// /// @param endAngle[in] 终止角度（0-2pi）
	bool isAngleBetween(const double angle, const double startAngle, const double endAngle) const;

	double angleBetween(const double startAngle, const double endAngle) const;

	/// @brief 获得这个标注的文字实体（父实体为this）。如果有替代文字，返回替代文字的实体，传入的textValue无效。
	DmEntityContainer* getTextEntity(const double& textValue);

protected:
	DmDimensionData data;
	DmEntityContainer* container = nullptr;
};

#endif
