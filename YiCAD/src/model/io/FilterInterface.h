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

/// @file FilterInterface.h
/// @brief 文件转换器接口类头文件

#ifndef FILTERINTERFACE_H
#define FILTERINTERFACE_H

#include "DmDocument.h"

#include <QObject>

/// @brief 转换器接口类
class FilterInterface
{
public:
	FilterInterface() = default;
	virtual ~FilterInterface() = default;

	/// @brief 判断类型是否支持导入
	virtual bool canImport(const QString& type) const = 0;

	/// @brief 判断类型是否支持导出
	virtual bool canExport(const QString& type) const = 0;

	/// @brief 导入文件并存储实体到当前实体容器中
	virtual bool fileImport(DmDocument& document, const QString& file) = 0;

	/// @brief 导出指定格式的文件
	virtual bool fileExport(DmDocument& document, const QString& file, const QString& formatType) = 0;

	/// @brief Request the error message for the last import/export action, based on member variable \p errorCode.
	/// The default implementation is for existing filters, inherited without error handling methods.
	/// It is strongly recommend for new implementations to overwrite this method with some useful error messages.
	virtual QString lastError() const
	{
		return QObject::tr("undefined error", "FilterInterface");
	}

	/// @brief Request the error code for the last import/export action.
	/// The default value 0 means no error.
	virtual int lastErrorCode() const
	{
		return errorCode;
	}

	static FilterInterface* createFilter()
	{
		return NULL;
	}

protected:
	/// 上次导入/导出操作的错误代码
	int errorCode = 0;
};

#endif
