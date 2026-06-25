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

/// @file MetaLayers.h
/// @brief 图层实体序列化容器头文件

#ifndef PERSISTENCE_META_LAYERS_H
#define PERSISTENCE_META_LAYERS_H

#include "DmEntity.h"
#include "Persistence.h"

class DmDocument;

/// 图层序列化容器
class MetaLayersContainer : Persistence
{
public:
	/// @brief 构造函数
	/// @param pDoc 文档指针
	MetaLayersContainer(DmDocument* pDoc);

	/// @brief 析构函数
	virtual ~MetaLayersContainer()
	{
	}

	/// @brief 获取内存大小
	/// @return 内存大小（字节）
	unsigned int getMemSize() const override;

	/// @brief 保存为XML格式
	/// @param wrt Writer对象引用
	void saveXML(Writer& wrt) const override;

	/// @brief 从XML格式恢复
	/// @param reader XMLReader对象引用
	void restoreXML(XMLReader& reader) override;

private:
	DmDocument*     m_pDocument = nullptr;  ///< 文档指针
	unsigned int    m_iCount = 0;           ///< 图层数量
};

#endif //PERSISTENCE_META_LAYERS_H
