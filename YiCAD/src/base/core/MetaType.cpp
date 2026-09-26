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
/// @file MetaType.cpp
/// @brief 类型系统元类型实现文件

# include <cassert>
#include "MetaType.h"

Type MetaType::classTypeId = Type::badType();
int  MetaType::revId = 0;

//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
MetaType::MetaType()
{
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
MetaType::~MetaType()
{
}


//**************************************************************************
// separator for other implementation aspects

/// @brief 初始化MetaType类型系统
void MetaType::initialize()
{
	assert(MetaType::classTypeId == Type::badType() && "don't init() twice!");
	/* Set up entry in the type system. */
	MetaType::classTypeId =
		Type::createType(Type::badType(),
			"MetaType",
			MetaType::create);
}

/// @brief 获取类类型ID
/// @return 类型对象
Type MetaType::getClassTypeId()
{
	return MetaType::classTypeId;
}

/// @brief 获取实例类型ID
/// @return 类型对象
Type MetaType::getTypeId() const
{
	return MetaType::classTypeId;
}

/// @brief 获取版本ID列表
/// @param vecRev 版本对列表
void MetaType::getRevId(std::vector<PAIR>& vecRev)
{
	vecRev.push_back(std::make_pair(getClassTypeId().getName(), MetaType::revId));
}


/// @brief 初始化子类
/// @param toInit 待初始化的类型
/// @param ClassName 类名
/// @param ParentName 父类名
/// @param method 实例化方法
void MetaType::initSubclass(Type& toInit, const char* ClassName, const char* ParentName,
	Type::instantiationMethod method)
{
	// don't init twice!
	assert(toInit == Type::badType());
	// get the parent class
	Type parentType(Type::fromName(ParentName));
	// forgot init parent!
	assert(parentType != Type::badType());

	// create the new type
	toInit = Type::createType(parentType, ClassName, method);
}
