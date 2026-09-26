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

/// @file Cmd.h
/// @brief 命令基类及模板化的实体变更命令，提供 Undo/Redo 框架基础

#ifndef CMD_H
#define CMD_H

/// @brief 命令类型枚举
enum class CmdType
{
    None,
    ActivePenModify,

    EntityTableAddCmd,
    EntityTableRemoveCmd,
    EntityTableModifyCmd,

    LayerTableAddCmd,
    LayerTableModifyCmd,
    LayerTableRemoveCmd,
    LayerTableActivateCmd,

    LineTypeTableAddCmd,
    LineTypeTableModifyCmd,
    LineTypeTableRemoveCmd,
    LineTypeTableActivateCmd,

    TextStyleTableAddCmd,
    TextStyleTableModifyCmd,
    TextStyleTableRemoveCmd,
    TextStyleTableActivateCmd,

    DimensionStyleTableAddCmd,
    DimensionStyleTableModifyCmd,
    DimensionStyleTableRemoveCmd,
    DimensionStyleTableActivateCmd,
    BlockTableAddCmd,
    BlockTableRemoveCmd,
    BlockTableModifyCmd,

    BlockEditEnterCmd,
    BlockEditExitCmd,

};

class DmObject;

/// @brief 命令基类
/// @details 所有 Undo/Redo 命令的抽象基类
class ICmd
{
public:
    virtual ~ICmd() = default;

    /// @brief 检查命令是否已执行
    bool isExecuted() const{return m_isExecuted;}

    /// @brief 第一次执行命令
    virtual void execute() {m_isExecuted = true;}
    /// @brief 撤销命令
    virtual void undo() {m_isExecuted = false;}
    /// @brief 重做命令
    virtual void redo() {m_isExecuted = true;}
    /// @brief 回滚命令
    virtual void rollback();
    /// @brief 清空 redo 列表和释放所有命令时调用
    virtual void clear() = 0;
    /// @brief 获得命令修改的实体
    /// @return 被修改的实体对象
    virtual DmObject* getObject() {return nullptr;}
    /// @brief 获取命令类型
    virtual CmdType getCmdType() const {return CmdType::None;}

protected:
    bool m_isExecuted = false; ///< 是否已执行
};

/// @brief 实体单属性变更命令（模板类）
/// @details 通过成员指针实现泛型属性修改
/// 在 deepseek 中问："c++如何把一个成员变量名传入模板类，在模板类中通过这个变量名访问变量"
/// 回答：通过成员指针（Pointer-to-member）实现类似功能
template<typename EntityClass, typename DataType, DataType EntityClass::*memberName>
class EntityChangeCmd:public ICmd
{
public:
    /// @brief 构造实体变更命令
    /// @param ent 目标实体
    /// @param newData 新数据
    /// @param originData 原始数据
    EntityChangeCmd(EntityClass* ent, const DataType& newData, const DataType& originData)
    :m_ent(ent), m_newData(newData), m_originData(originData){}
    void execute() override
    {
        if (m_isExecuted)
        {
            return;
        }
        (*m_ent).*memberName = m_newData;
        ICmd::execute();
    }
    void undo() override
    {
        if (!m_isExecuted)
        {
            return;
        }
        (*m_ent).*memberName = m_originData;
        ICmd::undo();
    }
    void redo() override
    {
        if (m_isExecuted)
        {
            return;
        }
        (*m_ent).*memberName = m_newData;
        ICmd::redo();
    }

    void clear() override{}
protected:
    DataType m_newData;         ///< 新数据值
    DataType m_originData;      ///< 原始数据值
    EntityClass* m_ent;         ///< 目标实体指针
};

/// @brief 实体参数设置命令（模板类）
/// @details 通过两个成员函数指针实现泛型参数设置
template<typename EntityClass, typename BaseClass, void (BaseClass::*setFunc)(unsigned int), void (BaseClass::*unsetFunc)(unsigned int)>
class EntitySetParaCmd : public ICmd
{
public:
    /// @brief 构造实体参数设置命令
    /// @param ent 目标实体
    /// @param setPara 要设置的参数值
    /// @param unsetPara 要取消的参数值
    EntitySetParaCmd(EntityClass* ent, const unsigned int& setPara, const unsigned int& unsetPara)
            :m_ent(ent), m_setPara(setPara), m_unsetPara(unsetPara){}

    void execute() override
    {
        if (m_isExecuted)
        {
            return;
        }
        ((*m_ent).*setFunc)(m_setPara);
        ICmd::execute();
    }
    void undo() override
    {
        if (!m_isExecuted)
        {
            return;
        }
        ((*m_ent).*unsetFunc)(m_unsetPara);
        ICmd::undo();
    }
    void redo() override
    {
        if (m_isExecuted)
        {
            return;
        }
        ((*m_ent).*setFunc)(m_setPara);
        ICmd::redo();
    }
    void clear() override{}
protected:
    unsigned int m_setPara;         ///< 要设置的参数
    unsigned int m_unsetPara;       ///< 要取消的参数
    EntityClass* m_ent;             ///< 目标实体
};

/// @brief 实体参数设置命令2（模板类）
/// @details 通过成员函数指针实现泛型参数设置，支持可变参数
template<typename EntityClass, typename DataType, typename... Args>
class EntitySetParaCmd2:public ICmd
{
public:
    using FunctionPtr = void (EntityClass::*)(Args...);

    /// @brief 构造命令，保存成员函数指针
    /// @param setParaFuncPtr 设置参数的成员函数指针
    /// @param ent 目标实体
    /// @param newParaValue 新参数值
    /// @param originParaValue 原始参数值
    explicit EntitySetParaCmd2(FunctionPtr setParaFuncPtr, EntityClass* ent,const DataType& newParaValue, const DataType& originParaValue)
    : m_setParaFuncPtr(setParaFuncPtr), m_ent(ent), m_newParaValue(newParaValue), m_originParaValue(originParaValue) {}

    void execute() override
    {
        if (m_isExecuted)
        {
            return;
        }
        ((*m_ent).*m_setParaFuncPtr)(m_newParaValue);
        ICmd::execute();
    }
    void undo() override
    {
        if (!m_isExecuted)
        {
            return;
        }
        ((*m_ent).*m_setParaFuncPtr)(m_originParaValue);
        ICmd::undo();
    }
    void redo() override
    {
        if (m_isExecuted)
        {
            return;
        }
        ((*m_ent).*m_setParaFuncPtr)(m_newParaValue);
        ICmd::redo();
    }
    void clear() override{}
protected:
    DataType m_newParaValue;            ///< 新参数值
    DataType m_originParaValue;         ///< 原始参数值
    EntityClass* m_ent;                 ///< 目标实体
    FunctionPtr m_setParaFuncPtr;       ///< 设置参数的成员函数指针
};

#endif //CMD_H
