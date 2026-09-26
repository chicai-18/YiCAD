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
/// @file MigratorBase.cpp
/// @brief 数据迁移器基类实现文件

#include <TSingleton.hpp>
#include <assert.h>
#include "MigratorBase.h"

/// @brief 执行所有迁移器的后恢复操作
/// @return 全部成功返回true，否则返回false
bool DmMigrateContext::postRestore()
{
	bool bResult = true;
	for (auto img : m_vecMigrators)
	{
		if (img != nullptr)
		{
			bResult = bResult && img->ExecuteMigration();
			if (!bResult)
			{
				assert(bResult);
				m_vecErrorMsgs.push_back(img->Error());
			}
		}
	}

	return bResult;
}
