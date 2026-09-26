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

/// @file MetaDimensionStyles.h
/// @brief 标注样式实体序列化容器头文件

#ifndef PERSISTENCE_META_DIMENSIONSTYLES_H
#define PERSISTENCE_META_DIMENSIONSTYLES_H

#include <vector>

#include "DmEntity.h"
#include "Persistence.h"

class DmDimensionStyle;
class DmDocument;

/// 标注样式序列化容器
class MetaDimensionStylesContainer : Persistence
{
public:
	/// @brief 构造函数
	/// @param pDoc 文档指针
	MetaDimensionStylesContainer(DmDocument* pDoc);

	/// @brief 析构函数
	virtual ~MetaDimensionStylesContainer()
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

	/// @brief 保存为二进制流
	/// @param wrt OutputStream对象引用
	void saveStream(OutputStream& wrt) const override;

	/// @brief 从二进制流恢复
	/// @param rdr InputStream对象引用
	void restoreStream(InputStream& rdr) override;

	/// @brief 设置标注样式列表
	/// @param styles 标注样式向量引用
	void setDimStyles(std::vector<DmDimensionStyle*>& styles);

private:
	DmDocument*                     m_pDocument = nullptr;  ///< 文档指针
	std::vector<PAIR>              m_revs;                 ///< 版本列表
	std::vector<DmDimensionStyle*> m_dimStyleList;         ///< 标注样式列表
	QString                         m_strActiveStyle;       ///< 当前激活的样式名称
};

#endif //PERSISTENCE_META_DIMENSIONSTYLES_H
