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
/// @file Type.h
/// @brief 类型系统基础类型头文件

#ifndef BASE_TYPE_H
#define BASE_TYPE_H

#include <string>
#include <map>
#include <set>
#include <vector>

struct TypeData;

/** Type system class
  Many of the classes in the system must have their type
  information registered before any instances are created.
  The use of Type to store this information provides
  lots of various functionality for working with class hierarchies,
  comparing class types, instantiating objects from class names, etc.

  One important note about the use of Type to register class
  information: super classes must be registered before any of their
  derived classes are.
*/

class Type
{
public:
  /// Construction
  Type(const Type& type);
  Type();

  /// Destruction
  virtual ~Type();

  /// @brief 创建此类型的实例
  /// @return 实例指针
  void *createInstance();

  /// @brief 根据类型名称创建实例
  /// @param TypeName 类型名称
  /// @return 实例指针
  static void *createInstanceByName(const char* TypeName);

  typedef void * (*instantiationMethod)();

  /// @brief 根据名称获取类型
  /// @param name 类型名称
  /// @return 类型对象
  static Type fromName(const char *name);

  /// @brief 根据键值获取类型
  /// @param key 键值
  /// @return 类型对象
  static Type fromKey(unsigned int key);

  /// @brief 获取类型名称
  /// @return 类型名称字符串
  const char *getName() const;

  /// @brief 获取父类型
  /// @return 父类型对象
  const Type getParent() const;

  /// @brief 判断是否派生自指定类型
  /// @param type 父类型
  /// @return 是否派生
  bool isDerivedFrom(const Type type) const;

  /// @brief 获取所有派生自指定类型的类型列表
  /// @param type 父类型
  /// @param List 输出类型列表
  /// @return 派生类型数量
  static int getAllDerivedFrom(const Type type, std::vector<Type>& List);

  /// Returns the given named type if is derived from parent type, otherwise return bad type
  /// @brief 获取派生自指定父类型的命名类型
  /// @param name 类型名称
  /// @param parent 父类型
  /// @return 类型对象，如果不是派生则返回bad type
  static Type getTypeIfDerivedFrom(const char* name , const Type parent);

  /// @brief 获取已注册类型总数
  /// @return 类型数量
  static int getNumTypes();

  /// @brief 创建新类型
  /// @param parent 父类型
  /// @param name 类型名称
  /// @param method 实例化方法
  /// @return 新创建的类型
  static const Type createType(const Type parent, const char *name, instantiationMethod method = nullptr);

  /// @brief 获取类型键值
  /// @return 键值
  unsigned int getKey() const;

  /// @brief 判断是否为无效类型
  /// @return 是否无效
  bool isBad() const;

  void operator =  (const Type type);
  bool operator == (const Type type) const;
  bool operator != (const Type type) const;

  bool operator <  (const Type type) const;
  bool operator <= (const Type type) const;
  bool operator >= (const Type type) const;
  bool operator >  (const Type type) const;

  /// @brief 获取无效类型
  /// @return 无效类型对象
  static Type badType();

  /// @brief 初始化类型系统
  static void initialize();

  /// @brief 销毁类型系统
  static void destruct();

private:
  unsigned int index;                               ///< 类型索引
  static std::map<std::string, unsigned int> typemap;     ///< 名称到索引的映射
  static std::vector<TypeData*>     typedata;       ///< 类型数据列表
  static std::set<std::string>  loadModuleSet;      ///< 已加载模块集合
};


inline unsigned int Type::getKey() const
{
  return this->index;
}

inline bool Type::operator != (const Type type) const
{
  return (this->getKey() != type.getKey());
}

inline void Type::operator = (const Type type)
{
  this->index = type.getKey();
}

inline bool Type::operator == (const Type type) const
{
  return (this->getKey() == type.getKey());
}

inline bool Type::operator <  (const Type type) const
{
  return (this->getKey() < type.getKey());
}

inline bool Type::operator <= (const Type type) const
{
  return (this->getKey() <= type.getKey());
}

inline bool Type::operator >= (const Type type) const
{
  return (this->getKey() >= type.getKey());
}

inline bool Type::operator >  (const Type type) const
{
  return (this->getKey() > type.getKey());
}

inline bool Type::isBad() const
{
  return (this->index == 0);
}

#endif // BASE_TYPE_H
