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

/// @file CmdManager.cpp
/// @brief 命令管理器实现

#include "CmdManager.h"

#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "DmText.h"
#include "DmMText.h"
#include "DmDimAligned.h"
#include "DmLeader.h"

CmdManager::CmdManager()
    :m_currentCmd(nullptr)
    ,m_pDocument(nullptr)
    ,m_currentGroupCmd(nullptr)
{
}

CmdManager::~CmdManager()
{
    clearRedo();
    clearUndo();
}

/// @brief 撤销
void CmdManager::undo()
{
    if (m_undoCmds.size() == 0)
    {
        return;
    }
    CmdTypeObjectVector cmdTypes;
    static_cast<MacroCmd*>(m_undoCmds.back())->getCmdTypes(cmdTypes);
    m_undoCmds.back()->undo();
    updateWhenStyleChanged(cmdTypes);
    m_redoCmds.emplace_back(m_undoCmds.back());
    m_undoCmds.pop_back();

    registerChanges(cmdTypes);
    emitSignals(cmdTypes);
}

/// @brief 重做
void CmdManager::redo()
{
    if (m_redoCmds.size() == 0)
    {
        return;
    }
    CmdTypeObjectVector cmdTypes;
    static_cast<MacroCmd*>(m_redoCmds.back())->getCmdTypes(cmdTypes);
    m_redoCmds.back()->redo();
    updateWhenStyleChanged(cmdTypes);
    m_undoCmds.emplace_back(m_redoCmds.back());
    m_redoCmds.pop_back();

    registerChanges(cmdTypes);
    emitSignals(cmdTypes);
}

/// @brief 添加命令到当前事务
void CmdManager::addToCurrentCmd(ICmd* cmd)
{
    m_currentCmd->addCmd(cmd);
}

/// @brief 添加并立即执行命令
void CmdManager::addAndExecuteCmd(ICmd* cmd)
{
    cmd->execute();
    m_currentCmd->addCmd(cmd);
}

/// @brief 提交事务
void CmdManager::commit()
{
    // 在 group 中
    if (m_currentGroupCmd)
    {
        CmdTypeObjectVector cmdTypes;
        m_currentCmd->getCmdTypes(cmdTypes);
        m_currentCmd->execute();
        updateWhenStyleChanged(cmdTypes);
        m_currentGroupCmd->addCmd(m_currentCmd);
        m_currentCmd = nullptr;
        registerChanges(cmdTypes);
        // emitSignals(cmdTypes); // commitGroup()发送信号
    }
    // 没有 group
    else
    {
        CmdTypeObjectVector cmdTypes;
        m_currentCmd->getCmdTypes(cmdTypes);
        m_currentCmd->execute();
        updateWhenStyleChanged(cmdTypes);
        m_undoCmds.emplace_back(m_currentCmd);
        m_currentCmd = nullptr;
        // 清空 redo 列表
        clearRedo();

        registerChanges(cmdTypes);
        emitSignals(cmdTypes);
    }
}

/// @brief 获取最后一个可撤销命令
MacroCmd* CmdManager::getLastUndoCmd() const
{
    if (!m_undoCmds.empty())
    {
        return static_cast<MacroCmd*>(m_undoCmds.back());
    }
    return nullptr;
}

/// @brief 查找命令在撤销栈中的索引
int CmdManager::indexOfCmd(ICmd* cmd) const
{
    for (int i = 0; i < m_undoCmds.size(); i++)
    {
        if (m_undoCmds[i] == cmd)
        {
            return i;
        }
    }
    return -1;
}

/// @brief 回滚并删除指定索引（包含）之后的所有命令
void CmdManager::rollbackAndRemoveAfter(size_t index)
{
    CmdTypeObjectVector cmdTypes;
    for (size_t i = m_undoCmds.size(); i > index; )
    {
        --i;
        static_cast<MacroCmd*>(m_undoCmds[i])->getCmdTypes(cmdTypes);
        if (m_undoCmds[i]->isExecuted())
        {
            m_undoCmds[i]->undo();
        }
        delete m_undoCmds[i];
    }
    m_undoCmds.resize(index);
    registerChanges(cmdTypes);
}

/// @brief 开始事务
void CmdManager::start(const std::string &name)
{
    if (!m_currentCmd)
    {
        m_currentCmd = new MacroCmd(name);
    }
}

/// @brief 清空重做列表
void CmdManager::clearRedo()
{
    // 最后的是最近的 undo 的命令，所以从前往后清理
    for (auto cmd : m_redoCmds)
    {
        cmd->clear();
        delete cmd;
    }
    m_redoCmds.clear();
}

/// @brief 清空撤销列表
void CmdManager::clearUndo()
{
    // 最后的是最近提交的命令，所以从后往前清理
    for (auto it = m_undoCmds.end(); it != m_undoCmds.begin();)
    {
        --it; // 先递减迭代器
        (*it)->clear();
        delete *it;
    }
    m_undoCmds.clear();
}

/// @brief 回滚事务
void CmdManager::rollback()
{
    if (m_currentCmd) // 命令还没 commit，commit 之后的命令不能 rollback
    {
        CmdTypeObjectVector cmdTypes;
        m_currentCmd->getCmdTypes(cmdTypes);
        m_currentCmd->rollback();
        registerChanges(cmdTypes);
        m_currentCmd->clear();
        delete m_currentCmd;
        m_currentCmd = nullptr;
    }
}

/// @brief 开始事务组
void CmdManager::startGroup(const std::string& name)
{
    if (!m_currentGroupCmd)
    {
        m_currentGroupCmd = new MacroCmd(name);
    }
}

/// @brief 提交事务组
void CmdManager::commitGroup()
{
    CmdTypeObjectVector cmdTypes;
    m_currentGroupCmd->getCmdTypes(cmdTypes);
    m_undoCmds.emplace_back(m_currentGroupCmd);
    m_currentGroupCmd = nullptr;

    // 清空 redo 列表
    clearRedo();

    registerChanges(cmdTypes);
    emitSignals(cmdTypes);
}

/// @brief 回滚事务组
void CmdManager::rollbackGroup()
{
    if (m_currentGroupCmd)
    {
        CmdTypeObjectVector cmdTypes;
        m_currentGroupCmd->getCmdTypes(cmdTypes);
        m_currentGroupCmd->rollback();
        registerChanges(cmdTypes);
        m_currentGroupCmd->clear();
        delete m_currentGroupCmd;
        m_currentGroupCmd = nullptr;
    }
}

/// @brief 获取命令状态信息
void CmdManager::getCmdData(bool &undoable, std::string &undoName, bool &redoable, std::string &redoName)
{
    redoable = m_redoCmds.size() != 0;
    if (redoable)
    {
        redoName = dynamic_cast<MacroCmd*>(m_redoCmds.back())->getName();
    }
    undoable = m_undoCmds.size() != 0;
    if (undoable)
    {
        undoName = dynamic_cast<MacroCmd*>(m_undoCmds.back())->getName();
    }
}

/// @brief 收集命令类型并发出相应信号
void CmdManager::emitSignals(const CmdTypeObjectVector& cmdTypes)
{
    bool entityModify = false;
    bool lineTypeModify = false;
    bool layerModify = false;
    bool textStyleModify = false;
    bool dimStyleModify = false;
    bool activePenModify = false;
    bool blockModify = false;

    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::EntityTableAddCmd || type == CmdType::EntityTableRemoveCmd || type == CmdType::EntityTableModifyCmd)
        {
            entityModify = true;
            break;
        }
    }
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::LineTypeTableAddCmd || type == CmdType::LineTypeTableRemoveCmd || type == CmdType::LineTypeTableModifyCmd || type == CmdType::LineTypeTableActivateCmd)
        {
            lineTypeModify = true;
            break;
        }
    }
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::LayerTableAddCmd || type == CmdType::LayerTableRemoveCmd || type == CmdType::LayerTableModifyCmd || type == CmdType::LayerTableActivateCmd)
        {
            layerModify = true;
            break;
        }
    }
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::TextStyleTableAddCmd || type == CmdType::TextStyleTableRemoveCmd || type == CmdType::TextStyleTableModifyCmd || type == CmdType::TextStyleTableActivateCmd)
        {
            textStyleModify = true;
            break;
        }
    }
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::DimensionStyleTableAddCmd || type == CmdType::DimensionStyleTableRemoveCmd || type == CmdType::DimensionStyleTableModifyCmd || type == CmdType::DimensionStyleTableActivateCmd)
        {
            dimStyleModify = true;
            break;
        }
    }
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::ActivePenModify)
        {
            activePenModify = true;
            break;
        }
    }
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::BlockTableAddCmd || type == CmdType::BlockTableRemoveCmd
           || type == CmdType::BlockTableModifyCmd)
        {
            blockModify = true;
            break;
        }
    }

    if (entityModify)
    {
        emit entityModified();
    }
    if (layerModify)
    {
        emit layerModified();
    }
    if (lineTypeModify)
    {
        emit lineTypeModified();
    }
    if (textStyleModify)
    {
        emit textStyleModified();
    }
    if (dimStyleModify)
    {
        emit dimStyleModified();
    }
    if (activePenModify)
    {
        emit activePenModified();
    }
    if (blockModify)
    {
        emit blockModified();
    }
    emit cmdChanged();
}

/// @brief 样式修改后刷新相关实体
void CmdManager::updateWhenStyleChanged(const CmdTypeObjectVector& cmdTypes)
{
    updateWhenTextStyleChanged(cmdTypes);
    updateWhenDimStyleChanged(cmdTypes);
    updateWhenBlockChanged(cmdTypes);
}

/// @brief 文字样式发生改变，更新样式匹配的文字、标注
void CmdManager::updateWhenTextStyleChanged(const CmdTypeObjectVector &cmdTypes)
{
    auto entTable = m_pDocument->getEntityTable();
    CmdTypeObjectVector textStyleModifyVec;
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::TextStyleTableModifyCmd)
        {
            textStyleModifyVec.emplace_back(cmdType);
        }
    }
    if (!textStyleModifyVec.empty())
    {
        // 更新样式匹配的文字、标注
        for (auto cmdType : textStyleModifyVec)
        {
            DmObject* obj = std::get<1>(cmdType);
            DmTextStyle* textStyle = dynamic_cast<DmTextStyle*>(obj);
            if (textStyle)
            {
                for (auto e : *entTable)
                {
                    if (e->getEntityType() == DM::EntityText)
                    {
                        auto text = static_cast<DmText*>(e);
                        if (text->getStyle() == textStyle)
                        {
                            text->update();
                            entTable->touch(text);
                        }
                    }
                    else if (e->getEntityType() == DM::EntityMText)
                    {
                        auto mtext = static_cast<DmMText*>(e);
                        if (mtext->getStyle() == textStyle)
                        {
                            mtext->update();
                            entTable->touch(mtext);
                        }
                    }
                    else if (e->getEntityType() == DM::EntityDimAligned || e->getEntityType() == DM::EntityDimLinear ||
                            e->getEntityType() == DM::EntityDimRadial || e->getEntityType() == DM::EntityDimDiametric
                            || e->getEntityType() == DM::EntityDimAngular)
                    {
                        DmDimension* dim = static_cast<DmDimension*>(e);
                        if (dim->getStyle()->getDataConstRef().textStyle() == textStyle)
                        {
                            dim->update();
                            entTable->touch(dim);
                        }
                    }
                    else if (e->getEntityType() == DM::EntityDimLeader)
                    {
                        DmLeader* l = static_cast<DmLeader*>(e);
                        if (l->getDataRef().textStyle() == textStyle)
                        {
                            l->update();
                            entTable->touch(l);
                        }
                    }
                }
            }
        }
    }
}

/// @brief 标注样式发生改变，更新样式匹配的标注
void CmdManager::updateWhenDimStyleChanged(const CmdTypeObjectVector &cmdTypes)
{
    auto entTable = m_pDocument->getEntityTable();
    CmdTypeObjectVector dimStyleModifyVec;
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::DimensionStyleTableModifyCmd)
        {
            dimStyleModifyVec.emplace_back(cmdType);
        }
    }
    if (!dimStyleModifyVec.empty())
    {
        // 更新样式匹配的标注
        for (auto cmdType : dimStyleModifyVec)
        {
            DmObject* obj = std::get<1>(cmdType);
            DmDimensionStyle* dimStyle = dynamic_cast<DmDimensionStyle*>(obj);
            if (dimStyle)
            {
                for (auto e : *entTable)
                {
                    if (e->getEntityType() == DM::EntityDimAligned || e->getEntityType() == DM::EntityDimLinear ||
                            e->getEntityType() == DM::EntityDimRadial || e->getEntityType() == DM::EntityDimDiametric
                            || e->getEntityType() == DM::EntityDimAngular)
                    {
                        DmDimension* dim = static_cast<DmDimension*>(e);
                        if (dim->getStyle() == dimStyle)
                        {
                            dim->update();
                            entTable->touch(dim);
                        }
                    }
                    else if (e->getEntityType() == DM::EntityDimLeader)
                    {
                        DmLeader* l = static_cast<DmLeader*>(e);
                        if (l->getDataRef().pStyle == dimStyle)
                        {
                            l->update();
                            entTable->touch(l);
                        }
                    }
                }
            }
        }
    }
}

/// @brief 块发生改变，更新块参照
void CmdManager::updateWhenBlockChanged(const CmdTypeObjectVector& cmdTypes)
{
    auto entTable = m_pDocument->getEntityTable();
    CmdTypeObjectVector blockModifyVec;
    for (auto cmdType : cmdTypes)
    {
        CmdType type = std::get<0>(cmdType);
        if (type == CmdType::BlockTableModifyCmd)
        {
            blockModifyVec.emplace_back(cmdType);
        }
    }
    if (!blockModifyVec.empty())
    {
        // 更新引用了被修改块的块参照
        for (auto cmdType : blockModifyVec)
        {
            DmObject* obj = std::get<1>(cmdType);
            DmBlock* block = dynamic_cast<DmBlock*>(obj);
            if (block)
            {
                for (auto e : *entTable)
                {
                    if (e->getEntityType() == DM::EntityBlockReference)
                    {
                        DmBlockReference* blockRef = static_cast<DmBlockReference*>(e);
                        if (blockRef->getName() == block->getName())
                        {
                            blockRef->update();
                            entTable->touch(blockRef);
                        }
                    }
                }
            }
        }
    }
}

/// @brief 把命令涉及的符号表与块定义登记进文档的变更跟踪器，再交给监听者
/// @details 实体表的命令在执行、撤销、重做时自己登记实体；符号表、块表的命令撤销、重做时直接改删除标记或
///          从数据流恢复，不经表的 add_direct 等，在这里按命令类型补登（RENDER_PLAN.md 第 4.3.6 节）
void CmdManager::registerChanges(const CmdTypeObjectVector& cmdTypes)
{
    if (!m_pDocument)
    {
        return;
    }
    DmChangeTracker& tracker = m_pDocument->changeTracker();
    for (const auto& [type, object] : cmdTypes)
    {
        switch (type)
        {
        case CmdType::LayerTableAddCmd:
        case CmdType::LayerTableModifyCmd:
        case CmdType::LayerTableRemoveCmd:
            tracker.touchTable(DmSymbolTableKind::Layer);
            break;
        case CmdType::LineTypeTableAddCmd:
        case CmdType::LineTypeTableModifyCmd:
        case CmdType::LineTypeTableRemoveCmd:
            tracker.touchTable(DmSymbolTableKind::LineType);
            break;
        case CmdType::TextStyleTableAddCmd:
        case CmdType::TextStyleTableModifyCmd:
        case CmdType::TextStyleTableRemoveCmd:
            tracker.touchTable(DmSymbolTableKind::TextStyle);
            break;
        case CmdType::DimensionStyleTableAddCmd:
        case CmdType::DimensionStyleTableModifyCmd:
        case CmdType::DimensionStyleTableRemoveCmd:
            tracker.touchTable(DmSymbolTableKind::DimensionStyle);
            break;
        case CmdType::BlockTableAddCmd:
        case CmdType::BlockTableModifyCmd:
        case CmdType::BlockTableRemoveCmd:
            tracker.touchBlock(dynamic_cast<const DmBlock*>(object));
            break;
        default:
            break;
        }
    }
    m_pDocument->flushChanges();
}
