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

/// @file Transaction.cpp
/// @brief 事务(Transaction)和事务组(TransactionGroup)类实现

#include "DmDocument.h"
#include "Transaction.h"

Transaction::Transaction(const std::string& name, DmDocument* doc)
    : m_name(name)
    , m_pDocument(doc)
    , m_curState(TransactionState::Invalid)
{
}

void Transaction::start()
{
    if (m_pDocument == nullptr)
    {
        throw std::exception("Transaction start with document null.");
    }

    if (m_curState == TransactionState::Started)
    {
        return;
    }

    m_pDocument->getCmdManager()->start(m_name);
    m_curState = TransactionState::Started;
}

void Transaction::commit()
{
    if (m_curState != TransactionState::Started)
    {
        throw std::exception("Transaction has not started.");
    }

    m_pDocument->getCmdManager()->commit();
    m_pDocument->regenerate();
    m_curState = TransactionState::Committed;
}

Transaction::~Transaction()
{
    m_curState = TransactionState::Invalid;
}

void Transaction::rollback()
{
    if (m_curState != TransactionState::Started)
    {
        throw std::exception("Transaction has not started.");
    }

    m_pDocument->getCmdManager()->rollback();
    m_curState = TransactionState::Rollbacked;
}

TransactionGroup::TransactionGroup(const std::string& name, DmDocument* doc)
    : m_name(name)
    , m_pDocument(doc)
    , m_curState(TransactionState::Invalid)
{
}

TransactionGroup::~TransactionGroup()
{
    m_curState = TransactionState::Invalid;
}

void TransactionGroup::start()
{
    if (m_pDocument == nullptr)
    {
        throw std::exception("TransactionGroup start with document null.");
    }

    if (m_curState == TransactionState::Started)
    {
        return;
    }

    m_pDocument->getCmdManager()->startGroup(m_name);
    m_curState = TransactionState::Started;
}

void TransactionGroup::commit()
{
    if (m_curState != TransactionState::Started)
    {
        throw std::exception("TransactionGroup has not started.");
    }

    m_pDocument->getCmdManager()->commitGroup();
    m_pDocument->regenerate();
    m_curState = TransactionState::Committed;
}

void TransactionGroup::rollback()
{
    if (m_curState != TransactionState::Started)
    {
        throw std::exception("TransactionGroup has not started.");
    }

    m_pDocument->getCmdManager()->rollbackGroup();
    m_curState = TransactionState::Rollbacked;
}
