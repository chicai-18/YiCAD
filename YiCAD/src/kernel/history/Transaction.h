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

/// @file Transaction.h
/// @brief 事务(Transaction)和事务组(TransactionGroup)类，封装 Undo/Redo 的开始/提交/回滚操作

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "MacroCmd.h"
#include <string>

/// @brief 事务状态枚举
enum class TransactionState
{
    Invalid,    ///< 无效状态
    Started,    ///< 已开始
    Committed,  ///< 已提交
    Rollbacked  ///< 已回滚
};

class DmDocument;

/// @brief 事务类，用于生成一个整体Undo/Redo操作
class Transaction
{
public:
    /// @brief 构造函数
    /// @param [in] name 事务名称
    /// @param [in] doc 文档指针
    Transaction(const std::string& name, DmDocument* doc);

    Transaction() = delete;
    Transaction(const Transaction&) = delete;

    ~Transaction();

    /// @brief 开始事务
    void start();

    /// @brief 提交事务
    void commit();

    /// @brief 回滚事务
    void rollback();

private:
    std::string m_name;                ///< 事务名称
    DmDocument* m_pDocument = nullptr; ///< 关联的文档指针
    TransactionState m_curState = TransactionState::Invalid; ///< 当前事务状态
};

/// @brief 事务组类，用于生成一组整体Undo/Redo操作
class TransactionGroup
{
public:
    /// @brief 构造函数
    /// @param [in] name 事务组名称
    /// @param [in] doc 文档指针
    TransactionGroup(const std::string& name, DmDocument* doc);

    ~TransactionGroup();

    /// @brief 开始事务组
    void start();

    /// @brief 提交事务组
    void commit();

    /// @brief 回滚事务组
    void rollback();

private:
    std::string m_name;                ///< 事务组名称
    DmDocument* m_pDocument = nullptr; ///< 关联的文档指针
    TransactionState m_curState = TransactionState::Invalid; ///< 当前事务状态
};


#endif //TRANSACTION_H
