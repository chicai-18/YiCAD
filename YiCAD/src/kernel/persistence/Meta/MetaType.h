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
/// @file MetaType.h
/// @brief 类型系统元类型头文件

#ifndef META_TYPE_HEADER
#define META_TYPE_HEADER

#include "Type.h"

typedef std::pair<std::string, int> PAIR;

/// define for subclass Type
#define TYPESYSTEM_HEADER() \
public: \
  static Type getClassTypeId(void); \
  virtual Type getTypeId(void) const override; \
  static  void getRevId(std::vector<PAIR>& vecRev);\
  static void initialize(void); \
  static void* create(void); \
private: \
  static Type classTypeId;\
  static int  revId;\

/// define to implement a subclass of Type
#define TYPESYSTEM_SOURCE(_class_, _parentclass_, _rev_) \
Type _class_::getClassTypeId(void) { return _class_::classTypeId; } \
Type _class_::getTypeId(void) const { return _class_::classTypeId; } \
Type _class_::classTypeId = Type::badType();  \
void * _class_::create(void){\
   return new _class_ ();}\
void _class_::initialize(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); }\
int _class_::revId = _rev_;\
void _class_::getRevId(std::vector<PAIR>& vecRev) { _parentclass_::getRevId(vecRev); vecRev.push_back(std::make_pair( getClassTypeId().getName(), _class_::revId ));}\

/// define to implement a subclass of Type,who is a abstract class, means
/// not able to create object from this definition
#define TYPESYSTEM_SOURCE_ABSTRACT(_class_, _parentclass_, _rev_) \
Type _class_::getClassTypeId(void) { return _class_::classTypeId; } \
Type _class_::getTypeId(void) const { return _class_::classTypeId; } \
Type _class_::classTypeId = Type::badType();  \
void * _class_::create(void){return 0;}\
void _class_::initialize(void){\
  initSubclass(_class_::classTypeId, #_class_ , #_parentclass_, &(_class_::create) ); }\
int _class_::revId = _rev_;\
void _class_::getRevId(std::vector<PAIR>& vecRev) { _parentclass_::getRevId(vecRev); vecRev.push_back(std::make_pair( getClassTypeId().getName(), _class_::revId ));}\

/// MetaType class and root of the type system
class MetaType
{
public:
	/// @brief 获取类类型ID
	/// @return 类型对象
	static Type getClassTypeId();

	/// @brief 获取实例类型ID
	/// @return 类型对象
	virtual Type getTypeId() const;

	/// @brief 获取版本ID列表
	/// @param vecRev 版本对列表
	static void  getRevId(std::vector<PAIR>& vecRev);

	/// @brief 判断是否派生自指定类型
	/// @param type 父类型
	/// @return 是否派生
	bool isDerivedFrom(const Type type) const
	{
		return getTypeId().isDerivedFrom(type);
	}

	/// @brief 初始化类型系统
	static void initialize();

	/// @brief 创建实例
	/// @return 实例指针
	static void* create()
	{
		return nullptr;
	}

private:
	static Type classTypeId;  ///< 类类型ID
	static int  revId;        ///< 版本号

protected:
	/// @brief 初始化子类
	/// @param toInit 待初始化的类型
	/// @param ClassName 类名
	/// @param ParentName 父类名
	/// @param method 实例化方法
	static void initSubclass(Type& toInit, const char* ClassName, const char* ParentName, Type::instantiationMethod method = nullptr);

public:
	/// Construction
	MetaType();
	MetaType(const MetaType&) = default;
	MetaType& operator=(const MetaType&) = default;
	/// Destruction
	virtual ~MetaType();

};

/**
  * Template that works just like dynamic_cast, but expects the argument to
  * inherit from MetaType.
  *
  */
template<typename T> T* yicad_dynamic_cast(MetaType* t)
{
	if (t && t->isDerivedFrom(T::getClassTypeId()))
	{
		return static_cast<T*>(t);
	}
	else
	{
		return nullptr;
	}
}

/**
 * Template that works just like dynamic_cast, but expects the argument to
 * inherit from a const MetaTypes.
 *
 */
template<typename T> const T* yicad_dynamic_cast(const MetaType* t)
{
	if (t && t->isDerivedFrom(T::getClassTypeId()))
	{
		return static_cast<const T*>(t);
	}
	else
	{
		return nullptr;
	}
}

#endif // META_TYPE_HEADER
