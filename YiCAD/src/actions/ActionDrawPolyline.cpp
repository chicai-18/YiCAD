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


/// @file ActionDrawPolyline.cpp
/// @brief 多段线绘制交互动作实现，处理用户的鼠标/键盘事件以逐步构建多段线

#include <cmath>
#include "ActionDrawPolyline.h"

#include <QAction>
#include <QMouseEvent>

#include "Commands.h"
#include "Debug.h"
#include "DmArc.h"
#include "DmLine.h"
#include "DmSolid.h"
#include "DmPolyline.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "GeometryMethods.h"
#include "DmDocument.h"
#include "Transaction.h"

// 将角度转换为凸度的除数因子（360 * 2）
constexpr double ANGLE_TO_BULGE_DIVISOR = 720.0;
// 凸度接近零的阈值，低于此值视为直线
constexpr double SMALL_BULGE_THRESHOLD = 1E-5;

/// @brief 构造函数
/// @param doc 文档对象指针
/// @param docView 文档视图指针
ActionDrawPolyline::ActionDrawPolyline(DmDocument* doc, GuiDocumentView* docView)
	: PreviewActionInterface("Draw polylines", doc, docView)
	, m_startWeight(0.0)
	, m_endWeight(0.0)
	, m_bCCW(true)
	, pPoints(new ADPPoints)
	, m_pDocument(docView->getDocument())
{
	actionType = DM::ActionDrawPolyline;
	reset();
}

/// @brief 析构函数
ActionDrawPolyline::~ActionDrawPolyline() = default;

/// @brief 重置绘制状态，清除所有临时数据
void ActionDrawPolyline::reset()
{
	pPoints->polyline = nullptr;

	pPoints->start = {};
	pPoints->history.clear();
	pPoints->bHistory.clear();
}

/// @brief 初始化动作状态
/// @param status 初始状态值
void ActionDrawPolyline::init(int status)
{
	reset();
	PreviewActionInterface::init(status);
}

/// @brief 触发动作完成，将多段线添加到文档
void ActionDrawPolyline::trigger()
{
	PreviewActionInterface::trigger();

	if (!pPoints->polyline)
	{
		return;
	}

	// 已添加到文档，无需操作
	deleteSnapper();
	docView->moveRelativeZero(pPoints->polyline->getEndpoint());
	drawSnapper();

	pPoints->polyline = nullptr;
}

/// @brief 鼠标移动事件处理，预览当前多段线效果
/// @param e 鼠标事件指针
void ActionDrawPolyline::mouseMoveEvent(QMouseEvent* e)
{
	DmVector mouse = snapPoint(e);
	pPoints->mouse = mouse;
	double bulge = solveBulge(mouse);
	if ((getStatus() == SetNextPoint) && pPoints->point.valid)
	{
		deletePreview();
		// 直线
		if ((fabs(bulge) < DM_TOLERANCE) || (m_Mode == Line))
		{
			if ((m_startWeight == 0.0) && (m_endWeight == 0.0))
			{
				DmLine* line = new DmLine(preview->getEntityContainer(),
				                          pPoints->point, mouse);
				line->setDocument(pDocument);
				preview->addEntity(line);
			}
			else
			{
				constexpr double HALF = 0.5;
				auto normalize =
					GeometryMethods::getPerpendicularNormalizeVector(
						pPoints->point, mouse);
				auto pt1 = pPoints->point
				           + normalize * m_startWeight * HALF;
				auto pt2 = pPoints->point
				           - normalize * m_startWeight * HALF;
				auto pt3 = mouse + normalize * m_endWeight * HALF;
				auto pt4 = mouse - normalize * m_endWeight * HALF;
				std::vector<DmVector> vertexs = {pt1, pt2, pt3, pt4};
				SolidData solidData = SolidData(vertexs);
				DmSolid* entity = new DmSolid(
					preview->getEntityContainer(), solidData);
				entity->setDocument(pDocument);
				preview->addEntity(entity);
			}
		}
		else
		{
			// 圆弧
			DmArc arc(nullptr, pPoints->arc_data);
			std::vector<DmEntity*> ents;
			DmPolyline::getEntitiesByInfo(
				arc.getStartpoint(), arc.getEndpoint(),
				arc.getBulge(), m_startWeight, m_endWeight, ents);
			for (auto ent : ents)
			{
				ent->setParent(nullptr);
				ent->setDocument(pDocument);
				preview->addEntity(ent);
			}
		}
		drawPreview();
	}
}

/// @brief 鼠标释放事件处理，左键添加点，右键回退或完成
/// @param e 鼠标事件指针
void ActionDrawPolyline::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		GuiCoordinateEvent ce(snapPoint(e));
		coordinateEvent(&ce);
	}
	else if (e->button() == Qt::RightButton)
	{
		if (getStatus() == SetNextPoint)
		{
			trigger();
		}
		deletePreview();
		deleteSnapper();
		init(getStatus() - 1);
	}
	else
	{
		// 其他鼠标按钮暂不处理
	}
}

/// @brief 根据鼠标位置和当前分段模式计算凸度值
/// @param mouse 鼠标位置
/// @return 计算得到的凸度值
double ActionDrawPolyline::solveBulge(const DmVector& mouse)
{
	double b = 0.0; // 凸度
	bool suc = false;
	DmArc arc;
	DmLine line;
	double direction = 0.0;

	DmVector arcCenter(true);
	DmVector arcNormal(true);
	double arcRadius = 0.0;
	double arcStartAngle = 0.0;
	double arcEndAngle = 0.0;

	switch (m_Mode)
	{
	case Tangential:
		if (pPoints->polyline)
		{
			direction = getLastPartDirection();
			suc = GeometryMethods::createArcInfoTangentialFree(
				pPoints->point, DmVector(direction), mouse,
				arcCenter, arcNormal, arcRadius,
				arcStartAngle, arcEndAngle);
			if (suc)
			{
				pPoints->arc_data = ArcData(
					arcCenter, arcNormal, arcRadius,
					arcStartAngle, arcEndAngle);
				b = DmArc(nullptr, pPoints->arc_data).getBulge();
			}
			break;
		}
		// 无多段线时回退到相切半径模式
		[[fallthrough]];
	case TanRad:
		if (pPoints->polyline)
		{
			direction = getLastPartDirection();
			suc = GeometryMethods::createArcInfoTangentialLockRadius(
				pPoints->point, DmVector(direction), mouse, m_dRadius,
				arcCenter, arcNormal, arcRadius,
				arcStartAngle, arcEndAngle);
			if (suc)
			{
				pPoints->arc_data = ArcData(
					arcCenter, arcNormal, arcRadius,
					arcStartAngle, arcEndAngle);
				b = DmArc(nullptr, pPoints->arc_data).getBulge();
			}
		}
		break;
	case Ang:
	{
		if (isCCW())
		{
			b = tan(m_dAngle * M_PI / ANGLE_TO_BULGE_DIVISOR);
		}
		else
		{
			b = tan(-m_dAngle * M_PI / ANGLE_TO_BULGE_DIVISOR);
		}
		if (fabs(b) > SMALL_BULGE_THRESHOLD)
		{
			DmVector center(true);
			double radius = 0.0;
			double startAng = 0.0;
			double endAng = 0.0;
			DmVector normal(true);
			GeometryMethods::getArcInfo(
				pPoints->point, mouse, b,
				center, radius, startAng, endAng, normal);
			DmArc arcCalc(nullptr,
				ArcData(center, normal, radius, startAng, endAng));
			pPoints->arc_data = arcCalc.getData();
		}
		else
		{
			b = 0.0;
		}
		break;
	}
	default:
		break;
	}
	return b;
}

/// @brief 添加下一个点到多段线
/// @param mouse 鼠标位置
/// @param bulge 凸度
/// @param startWeight 起始线宽
/// @param endWeight 终止线宽
void ActionDrawPolyline::addNextPoint(const DmVector& mouse,
                                      const double bulge,
                                      const double startWeight,
                                      const double endWeight)
{
	docView->moveRelativeZero(mouse);
	pPoints->point = mouse;
	pPoints->history.append(mouse);
	pPoints->bHistory.append(bulge);
	if (!pPoints->polyline)
	{
		pPoints->polyline = new DmPolyline(nullptr, PolylineData());
		pPoints->polyline->setDocument(pDocument);
		pPoints->polyline->getDataRef().setVertexs({pPoints->start});
	}
	if (pPoints->polyline)
	{
		// TODO ：可优化（不用update）
		pPoints->polyline->insertVertex(
			pPoints->polyline->getDataConstRef().getVertexCount(),
			mouse, bulge, startWeight, endWeight);
		pPoints->polyline->update();
		// 第一次生成子实体时保存到doc
		if (pPoints->polyline->getDataConstRef().getBulgesCount() == 1)
		{
			// TODO: 检查"Add cloud line"是否应为"Add polyline"
			Transaction t(tr("Add cloud line").toStdString(), pDocument);
			t.start();
			pDocument->getEntityTable()->add(pPoints->polyline);
			t.commit();
		}
		else
		{
			pDocument->specifyModifiedEntity(pPoints->polyline);
		}

		deletePreview();
		deleteSnapper();
		docView->redraw();
		drawSnapper();
	}
	updateMouseButtonHints();
}

/// @brief 获得最后一段子实体的切线方向
/// @return 最后一段的切线方向角度
double ActionDrawPolyline::getLastPartDirection() const
{
	double direction = 0.0;
	const auto& dataRef = pPoints->polyline->getDataConstRef();
	double lastBulge = dataRef.getBulgeAt(dataRef.getBulgesCount() - 1);
	DmVector lastVertex = dataRef.getVertexAt(dataRef.getVertexCount() - 1);
	DmVector lastSecVertex =
		dataRef.getVertexAt(dataRef.getVertexCount() - 2);
	if (lastBulge == 0.0)
	{
		direction = lastSecVertex.angleTo(lastVertex);
	}
	else
	{
		// 从最后一段圆弧计算切线方向
		DmVector center(true);
		double radius = 0.0;
		double startAng = 0.0;
		double endAng = 0.0;
		DmVector normal(true);
		GeometryMethods::getArcInfo(
			lastSecVertex, lastVertex, lastBulge,
			center, radius, startAng, endAng, normal);
		DmArc lastArc(nullptr,
			ArcData(center, normal, radius, startAng, endAng));
		direction =
			Math2d::correctAngle(lastArc.getDirection2() + M_PI);
	}
	return direction;
}

/// @brief 坐标输入事件处理，设置起始点或添加下一个点
/// @param e 坐标事件指针
void ActionDrawPolyline::coordinateEvent(GuiCoordinateEvent* e)
{
	if (!e)
	{
		return;
	}

	DmVector mouse = e->getCoordinate();
	double bulge = solveBulge(mouse);
	switch (getStatus())
	{
	case SetStartpoint:
		pPoints->point = mouse;
		pPoints->history.clear();
		pPoints->history.append(mouse);
		pPoints->bHistory.clear();
		pPoints->bHistory.append(0.0);
		pPoints->start = pPoints->point;
		setStatus(SetNextPoint);
		docView->moveRelativeZero(mouse);
		updateMouseButtonHints();
		break;

	case SetNextPoint:
	{
		if (bulge == 0.0)
		{
			addNextPoint(mouse, bulge, m_startWeight, m_endWeight);
		}
		else
		{
			DmArc arc(nullptr, pPoints->arc_data);
			addNextPoint(arc.getEndpoint(), arc.getBulge(),
			             m_startWeight, m_endWeight);
		}
		break;
	}
	default:
		break;
	}
}

/// @brief 设置分段模式
/// @param m 分段模式
void ActionDrawPolyline::setMode(SegmentMode m)
{
	m_Mode = m;
}

/// @brief 获取当前分段模式
/// @return 当前分段模式（以整数返回）
int ActionDrawPolyline::getMode() const
{
	return m_Mode;
}

/// @brief 设置起始线宽
/// @param start 起始线宽
void ActionDrawPolyline::setStartWeight(const double& start)
{
	m_startWeight = start;
}

/// @brief 获取起始线宽
/// @return 起始线宽
double ActionDrawPolyline::getStartWeight() const
{
	return m_startWeight;
}

/// @brief 设置终止线宽
/// @param end 终止线宽
void ActionDrawPolyline::setEndWeight(const double& end)
{
	m_endWeight = end;
}

/// @brief 获取终止线宽
/// @return 终止线宽
double ActionDrawPolyline::getEndWeight() const
{
	return m_endWeight;
}

/// @brief 设置圆弧半径
/// @param r 半径值
void ActionDrawPolyline::setRadius(const double r)
{
	m_dRadius = r;
}

/// @brief 获取圆弧半径
/// @return 半径值
double ActionDrawPolyline::getRadius() const
{
	return m_dRadius;
}

/// @brief 设置圆弧角度
/// @param a 角度值
void ActionDrawPolyline::setAngle(const double a)
{
	m_dAngle = a;
}

/// @brief 获取圆弧角度
/// @return 角度值
double ActionDrawPolyline::getAngle() const
{
	return m_dAngle;
}

/// @brief 设置是否逆时针
/// @param ccw 是否逆时针
void ActionDrawPolyline::setCCW(const bool ccw)
{
	m_bCCW = ccw;
}

/// @brief 获取是否逆时针
/// @return 是否逆时针
bool ActionDrawPolyline::isCCW() const
{
	return m_bCCW;
}

/// @brief 命令输入事件处理，解析用户输入的命令和坐标
/// @param e 命令事件指针
void ActionDrawPolyline::commandEvent(GuiCommandEvent* e)
{
	QString c = e->getCommand().toLower();

	switch (getStatus())
	{
	case SetStartpoint:
		if (checkCommand("help", c))
		{
			GUIDIALOGFACTORY->commandMessage(
				msgAvailableCommands()
				+ getAvailableCommands().join(", "));
			return;
		}
		break;

	case SetNextPoint:
	{
		bool ok = false;
		double length = Math2d::eval(c, &ok);
		if (ok)
		{
			DmVector vec(1.0, 0.0);
			DmVector lastPt = pPoints->point;
			if (pPoints->mouse.valid)
			{
				vec = (pPoints->mouse - lastPt).normalize();
			}
			DmVector endPt = lastPt + vec * length;
			addNextPoint(endPt, 0.0, m_startWeight, m_endWeight);
			e->accept();
		}

		if (checkCommand("close", c))
		{
			close();
			e->accept();
			updateMouseButtonHints();
			return;
		}

		if (checkCommand("undo", c))
		{
			undo();
			e->accept();
			updateMouseButtonHints();
			return;
		}
	}
	break;

	default:
		break;
	}
}

/// @brief 获取当前状态下可用的命令列表
/// @return 可用命令的字符串列表
QStringList ActionDrawPolyline::getAvailableCommands()
{
	QStringList cmd;

	switch (getStatus())
	{
	case SetStartpoint:
		break;
	case SetNextPoint:
		if (pPoints->history.size() >= 2)
		{
			cmd += command("undo");
		}
		if (pPoints->history.size() >= 3)
		{
			cmd += command("close");
		}
		break;
	default:
		break;
	}

	return cmd;
}

/// @brief 更新鼠标按钮提示文本，根据当前状态显示不同的操作提示
void ActionDrawPolyline::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case SetStartpoint:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify first point"), tr("Cancel"));
		break;
	case SetNextPoint:
	{
		QString msg = "";

		if (pPoints->history.size() >= 3)
		{
			msg += COMMANDS->command("close");
			msg += "/";
		}
		if (pPoints->history.size() >= 2)
		{
			msg += COMMANDS->command("undo");
		}

		if (pPoints->history.size() >= 2)
		{
			GUIDIALOGFACTORY->updateMouseWidget(
				tr("Specify next point or [%1]").arg(msg),
				tr("Back"));
		}
		else
		{
			GUIDIALOGFACTORY->updateMouseWidget(
				tr("Specify next point"), tr("Back"));
		}
	}
	break;
	default:
		GUIDIALOGFACTORY->updateMouseWidget();
		break;
	}
}

/// @brief 显示选项对话框
void ActionDrawPolyline::showOptions()
{
	ActionInterface::showOptions();

	GUIDIALOGFACTORY->requestOptions(this, true);
}

/// @brief 隐藏选项对话框
void ActionDrawPolyline::hideOptions()
{
	ActionInterface::hideOptions();

	GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 更新鼠标光标样式为CAD十字光标
void ActionDrawPolyline::updateMouseCursor()
{
	docView->setMouseCursor(DM::CadCursor);
}

/// @brief 闭合多段线，连接首尾端点
void ActionDrawPolyline::close()
{
	if (pPoints->polyline
	    && (pPoints->polyline->getDataConstRef().getVertexCount() >= 2))
	{
		pPoints->polyline->setClosed(true);
		pPoints->polyline->getDataRef().appendBulge(0.0);
		pPoints->polyline->getDataRef().appendLineWeight(
			m_startWeight, m_endWeight);
		pPoints->polyline->update();
		pDocument->specifyModifiedEntity(pPoints->polyline);
		trigger();
		setStatus(SetStartpoint);
		docView->moveRelativeZero(pPoints->start);
	}
	else
	{
		GUIDIALOGFACTORY->commandMessage(
			tr("Cannot close sequence of lines: "
			   "Not enough entities defined yet."));
	}
}

/// @brief 撤销上一个添加的点
void ActionDrawPolyline::undo()
{
	if (pPoints->history.size() > 1)
	{
		pPoints->history.removeLast();
		pPoints->bHistory.removeLast();
		deletePreview();
		pPoints->point = pPoints->history.last();

		if (pPoints->history.size() == 1)
		{
			docView->moveRelativeZero(pPoints->history.front());
			pDocument->getEntityTable()->remove_direct(pPoints->polyline);
			pPoints->polyline = nullptr;
			docView->redraw();
		}
		if (pPoints->polyline)
		{
			// 移除最后一个点
			auto& dataRef = pPoints->polyline->getDataRef();
			std::vector<DmVector> vertexs = dataRef.getVertexs();
			vertexs.erase(vertexs.end() - 1);
			dataRef.setVertexs(vertexs);
			std::vector<double> bulges = dataRef.getBulges();
			bulges.erase(bulges.end() - 1);
			dataRef.setBulges(bulges);
			std::vector<double> weights = dataRef.getLineWeights();
			weights.erase(weights.end() - 2, weights.end() - 1);
			dataRef.setLineWeights(weights);
			pPoints->polyline->update();
			pDocument->specifyModifiedEntity(pPoints->polyline);

			docView->moveRelativeZero(
				pPoints->polyline->getEndpoint());
			docView->redraw();
		}
	}
	else
	{
		GUIDIALOGFACTORY->commandMessage(
			tr("Cannot undo: Not enough entities defined yet."));
	}
}
