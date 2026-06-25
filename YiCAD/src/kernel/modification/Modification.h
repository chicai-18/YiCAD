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

/// @file Modification.h
/// @brief 实体修改操作类，提供复制、移动、偏移、裁剪、倒角、倒圆、打断等功能

#ifndef MODIFICATION_H
#define MODIFICATION_H

#include "DmVector.h"
#include "DmPen.h"
#include <QHash>
#include <vector>

class DmAtomicEntity;
class DmEntity;
class DmEntityContainer;
class DmMText;
class DmText;
class DmPolyline;
class DmDocument;
class GuiDocumentView;
class DmLine;
class DmCircle;
class DmArc;
class DmSpline;

/// @brief 复制信息
class CopyData
{
public:
	int number;         ///< 复制的个数
	DmVector offset;    ///< 偏移量
};

/// @brief Holds the data needed for offset modifications.
class OffsetData
{
public:
	int number;               ///< 偏移数量
	bool useCurrentAttributes; ///< 是否使用当前属性
	bool useCurrentLayer;      ///< 是否使用当前图层
	DmVector coord;            ///< 偏移参考坐标
	double distance;           ///< 偏移距离
};

/// @brief 旋转信息
class RotateData
{
public:
	DmVector center;  ///< 旋转中心
	double angle;     ///< 旋转角度
};

/// @brief 缩放信息
class ScaleData
{
public:
	DmVector referencePoint; ///< 缩放引用点
	double factor;           ///< 缩放比例，仅支持均匀缩放
};

/// @brief 镜像时的数据
class MirrorData
{
public:
	bool copy;              ///< 复制。为false表示删除原始，为true时不删除原始
	DmVector axisPoint1;    ///< 镜像轴点1
	DmVector axisPoint2;    ///< 镜像轴点2
};

/// @brief Holds the data needed for beveling modifications.
class BevelData
{
public:
	double length1;  ///< 倒角长度1
	double length2;  ///< 倒角长度2
	bool trim;       ///< 是否裁剪
};

/// @brief Holds the data needed for rounding modifications.
class RoundData
{
public:
	double radius; ///< 圆角半径
	bool trim;     ///< 是否裁剪
};

/// @brief Holds the data needed for moving reference points.
class MoveRefData
{
public:
	DmVector ref;    ///< 参考点
	DmVector offset; ///< 偏移量
};

/// @brief Holds the data needed for pasting.
class PasteData
{
public:
	/// @brief 构造粘贴数据
	/// @param insertionPoint 插入点
	/// @param factor 缩放比例
	/// @param angle 旋转角度
	/// @param asInsert 是否作为块插入
	/// @param blockName 块名称
	PasteData(DmVector insertionPoint, double factor, double angle, bool asInsert, const QString& blockName);

	DmVector insertionPoint; ///< Insertion point.
	double factor;           ///< Scale factor.
	double angle;            ///< Rotation angle.
	bool asInsert;           ///< Paste as an insert rather than individual entities.
	QString blockName;       ///< Name of the block to create or an empty string to assign a new auto name.
};

/// @brief 实体修改操作类
///
/// 提供实体的复制、移动、偏移、裁剪、倒角、倒圆、打断等修改功能。
class Modification
{
public:
	Modification() = delete;

	/// @brief 构造函数
	/// @param docView 文档视图指针
	Modification(GuiDocumentView* docView);

	/// @brief 删除选中的实体
	void remove();

	/// @brief 复制或剪切选中的实体到剪贴板
	/// @param ref 参考点，实体将平移-ref
	/// @param cut true表示剪切，false表示复制
	void copy(const DmVector& ref, const bool cut);

private:
	/// @brief 复制单个实体到剪贴板
	/// @param e 实体指针
	/// @param ref 参考点
	/// @param cut true表示剪切
	void copyEntity(DmEntity* e, const DmVector& ref);

	/// @brief 复制实体关联的图层到剪贴板
	/// @param e 实体指针
	void copyLayers(DmEntity* e);

	/// @brief 复制实体关联的块到剪贴板
	/// @param e 实体指针
	void copyBlocks(DmEntity* e);

	// TODO: Modification 废弃，块相关代码已注释
	//bool pasteLayers(DmDocument* source);

	/// @brief 在目标容器中粘贴包含块引用的实体
	/// @param entity 源实体
	/// @param container 目标容器
	/// @param blocksDict 块重命名映射
	/// @param insertionPoint 插入点
	bool pasteContainer(DmEntity* entity, DmEntityContainer* container, QHash<QString, QString> blocksDict, DmVector insertionPoint);

	/// @brief 在目标容器中粘贴实体
	/// @param entity 源实体
	/// @param container 目标容器
	bool pasteEntity(DmEntity* entity, DmEntityContainer* container);

public:
	//void paste(const PasteData& data, DmDocument* source = NULL);

	/// @brief 移动选中的实体
	/// @param offset 偏移量
	void move(const DmVector& offset);

	/// @brief 裁剪实体，支持直线，圆弧，圆，椭圆、椭圆弧、多段线、样条线
	/// @param ents 剪切实体列表，被剪实体可在此列表内，剪切成功后从此列表移除
	/// @param entBeenCut 被裁剪的实体
	/// @param mousePt 鼠标点位置
	/// @return 裁剪成功返回true
	bool trim(std::vector<DmEntity*>& ents, DmEntity* entBeenCut, const DmVector& mousePt);

	/// @brief 尝试剪切实体，获得剩余部分及删除部分
	/// @param ents 剪切实体列表
	/// @param entBeenCut 被裁剪的实体
	/// @param mousePt 鼠标点位置
	/// @param remainResults 输出剩余实体
	/// @param deleteEnt 输出待删除实体
	/// @return 操作是否成功
	static bool tryTrim(const std::vector<DmEntity*>& ents, DmEntity* entBeenCut, const DmVector& mousePt, std::vector<DmEntity*>& remainResults, DmEntity*& deleteEnt);

	/// @brief 偏移实体
	/// @param data 偏移参数
	/// @return 操作是否成功
	bool offset(const OffsetData& data);

	/// @brief 实体是否可被打断
	/// @param ent 实体指针
	/// @return 可打断返回true
	static bool isCutableEntity(DmEntity* ent);

	/// @brief 单点打断实体
	/// @param cutCoord 打断点坐标
	/// @param cutEntity 待打断实体
	/// @return 操作是否成功
	bool cut(const DmVector& cutCoord, DmEntity* cutEntity);

	/// @brief 两点打断实体
	/// @param firstCoord 第一个打断点
	/// @param secondCoord 第二个打断点
	/// @param cutEntity 待打断实体
	/// @return 操作是否成功
	bool cut2P(const DmVector& firstCoord, const DmVector& secondCoord, DmEntity* cutEntity);

	/// @brief 尝试两点打断实体
	/// @param firstCoord 第一个打断点
	/// @param secondCoord 第二个打断点
	/// @param cutEntity 待打断实体
	/// @param remainResults 输出剩余实体
	/// @param deleteEnt 输出待删除实体
	/// @return 操作是否成功
	static bool tryCut2P(const DmVector& firstCoord, const DmVector& secondCoord, DmEntity* cutEntity, std::vector<DmEntity*>& remainResults, DmEntity*& deleteEnt);

	//bool bevel(const DmVector& coord1, DmAtomicEntity* entity1, const DmVector& coord2, DmAtomicEntity* entity2, BevelData& data);

	/// @brief 根据引用点移动实体
	/// @param data 移动参考数据
	/// @return 操作是否成功
	bool moveRef(MoveRefData& data);

	/// @brief 由另一实体更新实体信息。支持直线、圆弧、椭圆、多段线、样条线
	/// @param ent 目标实体
	/// @param entDataCopyFrom 数据来源实体
	static void updateEntityData(DmEntity* ent, DmEntity* entDataCopyFrom);

private:
	/// @brief 取消选中所有实体，并可选择删除
	/// @param remove true表示同时删除实体
	void deselectOriginals(bool remove);

	/// @brief 单点打断实体（核心实现）
	/// @param cutCoord 打断点坐标
	/// @param ent 待打断实体
	/// @param cut1 输出第一段实体
	/// @param cut2 输出第二段实体
	/// @return 操作是否成功
	static bool cutEntity1P(const DmVector& cutCoord, DmEntity* ent, DmEntity*& cut1, DmEntity*& cut2);

	/// @brief 两点打断实体（核心实现）
	/// @param firstCoord 第一个打断点
	/// @param secondCoord 第二个打断点
	/// @param cutEntity 待打断实体
	/// @param cut1 输出第一段实体
	/// @param cut2 输出第二段实体
	/// @param cut3 输出第三段实体
	/// @return 操作是否成功
	static bool cutEntity2P(const DmVector& firstCoord, const DmVector& secondCoord, DmEntity* cutEntity, DmEntity*& cut1, DmEntity*& cut2, DmEntity*& cut3);

	/// @brief 对多段线打断
	/// @param cutCoord 打断点坐标
	/// @param poly 多段线实体
	/// @param cut1 输出第一段
	/// @param cut2 输出第二段
	/// @return 操作是否成功
	static bool cutForPolyline(const DmVector& cutCoord, DmPolyline* poly, DmEntity*& cut1, DmEntity*& cut2);

	/// @brief 获取容器与实体的交点
	/// @param container 实体容器
	/// @param ent 实体
	/// @param sol 输出交点集合
	static void getIntersectionOfContainer(const DmEntityContainer* container, const DmEntity* ent, DmVectorSolutions& sol);

protected:
	DmDocument* document = nullptr;       ///< 关联的文档
	GuiDocumentView* docView = nullptr;   ///< 关联的文档视图
};

#endif
