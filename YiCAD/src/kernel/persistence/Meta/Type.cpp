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
/// @file Type.cpp
/// @brief 类型系统基础类型实现文件

# include <cassert>
#include "Type.h"

using namespace std;

struct TypeData
{
  TypeData(const char *theName,
           const Type type = Type::badType(),
           const Type theParent = Type::badType(),
           Type::instantiationMethod method = nullptr
          ):name(theName), parent(theParent), type(type), instMethod(method) { }

  std::string name;
  Type parent;
  Type type;
  Type::instantiationMethod instMethod;
};

map<string, unsigned int> Type::typemap;
vector<TypeData*>        Type::typedata;
set<string>              Type::loadModuleSet;

//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Type::Type()
: index(0)
{
}


Type::Type(const Type& type)
:index(type.index)
{
}


/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Type::~Type()
{
}

/// @brief 创建此类型的实例
/// @return 实例指针
void *Type::createInstance()
{
  return (typedata[index]->instMethod)();
}


/// @brief 根据类型名称创建实例
/// @param TypeName 类型名称
/// @return 实例指针
void *Type::createInstanceByName(const char* TypeName)
{
  // now the type should be in the type map
  Type t = fromName(TypeName);
  if (t == badType())
  {
    return nullptr;
  }

  return t.createInstance();
}

/// @brief 获取无效类型
/// @return 无效类型对象
Type Type::badType()
{
  Type bad;
  bad.index = 0;
  return bad;
}


/// @brief 创建新类型
/// @param parent 父类型
/// @param name 类型名称
/// @param method 实例化方法
/// @return 新创建的类型
const Type Type::createType(const Type parent, const char *name, instantiationMethod method)
{
  Type newType;
  newType.index = static_cast<unsigned int>(Type::typedata.size());
  TypeData * typeData = new TypeData(name, newType, parent, method);
  Type::typedata.push_back(typeData);

  // add to dictionary for fast lookup
  Type::typemap[name] = newType.getKey();

  return newType;
}


/// @brief 初始化类型系统
void Type::initialize()
{
  assert(Type::typedata.size() == 0);

  Type::typedata.push_back(new TypeData("BadType"));
  Type::typemap["BadType"] = 0;
}

/// @brief 销毁类型系统
void Type::destruct()
{
  for (std::vector<TypeData*>::const_iterator it = typedata.begin(); it != typedata.end(); ++it)
  {
    delete *it;
  }
  typedata.clear();
  typemap.clear();
  loadModuleSet.clear();
}

/// @brief 根据名称获取类型
/// @param name 类型名称
/// @return 类型对象
Type Type::fromName(const char *name)
{
  std::map<std::string, unsigned int>::const_iterator pos;

  pos = typemap.find(name);
  if (pos != typemap.end())
  {
    return typedata[pos->second]->type;
  }
  else
  {
    return Type::badType();
  }
}

/// @brief 根据键值获取类型
/// @param key 键值
/// @return 类型对象
Type Type::fromKey(unsigned int key)
{
  if (key < typedata.size())
  {
    return typedata[key]->type;
  }
  else
  {
    return Type::badType();
  }
}

/// @brief 获取类型名称
/// @return 类型名称字符串
const char *Type::getName() const
{
  return typedata[index]->name.c_str();
}

/// @brief 获取父类型
/// @return 父类型对象
const Type Type::getParent() const
{
  return typedata[index]->parent;
}

/// @brief 判断是否派生自指定类型
/// @param type 父类型
/// @return 是否派生
bool Type::isDerivedFrom(const Type type) const
{
  Type temp(*this);
  do {
    if (temp == type)
    {
      return true;
    }
    temp = temp.getParent();
  } while (temp != badType());

  return false;
}

/// @brief 获取所有派生自指定类型的类型列表
/// @param type 父类型
/// @param List 输出类型列表
/// @return 派生类型数量
int Type::getAllDerivedFrom(const Type type, std::vector<Type> & List)
{
  int cnt = 0;

  for (std::vector<TypeData*>::const_iterator it = typedata.begin(); it != typedata.end(); ++it)
  {
    if ((*it)->type.isDerivedFrom(type))
    {
      List.push_back((*it)->type);
      cnt++;
    }
  }
  return cnt;
}

/// @brief 获取已注册类型总数
/// @return 类型数量
int Type::getNumTypes()
{
  return static_cast<int>(typedata.size());
}

/// @brief 获取派生自指定父类型的命名类型
/// @param name 类型名称
/// @param parent 父类型
/// @return 类型对象，如果不是派生则返回bad type
Type Type::getTypeIfDerivedFrom(const char* name , const Type parent)
{
  Type type = fromName(name);

  if (type.isDerivedFrom(parent))
  {
    return type;
  }
  else
  {
    return Type::badType();
  }
}
