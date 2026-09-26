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
/// @file MetaBlockTableRecords.h
/// @brief 块表记录元数据容器头文件

#ifndef PERSISTENCE_META_BLOCKTABLERECORDS_H
#define PERSISTENCE_META_BLOCKTABLERECORDS_H

#include <vector>

#include "DmEntity.h"
#include "Persistence.h"

class DmDocument;
class DmBlockTable;
class DmBlock;

class MetaBlockTableRecordsContainer : Persistence
{
public:
	/// @brief 构造函数
	/// @param pDoc 文档指针
	MetaBlockTableRecordsContainer(DmDocument* pDoc);

	/// @brief 析构函数
	virtual ~MetaBlockTableRecordsContainer() {};

	//persistent helper
	/// @brief 获取内存大小
	/// @return 内存大小（字节）
	unsigned int getMemSize() const override;

	/// @brief 保存为XML格式
	/// @param wrt Writer对象引用
	void		 saveXML(Writer& wrt) const override;

	/// @brief 从XML格式恢复
	/// @param reader XMLReader对象引用
	void		 restoreXML(XMLReader& reader) override;

	/// @brief 保存为流格式
	/// @param wrt OutputStream对象引用
	void		 saveStream(OutputStream& wrt) const override;

	/// @brief 从流格式恢复
	/// @param rdr InputStream对象引用
	void		 restoreStream(InputStream& rdr) override;

	/// @brief 设置块列表
	/// @param pBlockList 块指针列表引用
	void		 setBlockList(std::vector<DmBlock*>& pBlockList);

private:
	DmDocument*				m_pDocument = nullptr;  ///< 文档指针
	std::vector<PAIR>		m_revs;                 ///< 版本号列表
	std::vector<DmBlock*>	m_pBlocks;              ///< 块指针列表
	int						m_Count = 0;            ///< 块数量
};

#endif //PERSISTENCE_META_BLOCKTABLERECORDS_H
