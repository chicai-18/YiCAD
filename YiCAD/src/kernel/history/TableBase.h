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

/// @file TableBase.h
/// @brief 表基类和过滤迭代器模板，提供已删除元素过滤功能

#ifndef TABLEBASE_H
#define TABLEBASE_H

#include <vector>
#include <iterator>
#include <type_traits>
#include <unordered_map>
#include "DmId.h"
#include "DmObject.h"
#include "Cmd.h"

class DmDocument;

/// @brief 表接口基类
class ITable
{
public:
    ITable() : m_pDoc(nullptr)
    {
    }

    virtual void setDocument(DmDocument* pDoc)
    {
        m_pDoc = pDoc;
    }

    DmDocument* getDocument() const
    {
        return m_pDoc;
    }

    virtual void startModify(DmObject* e) = 0;

protected:
    DmDocument* m_pDoc = nullptr; ///< 关联的文档指针
};

/// @brief 元素是否已删除谓词
template<typename T>
struct ObjectIsErasedPredicate
{
    bool operator()(T objPtr) const
    {
        return objPtr->isErased(); // 假设T有 bool isErased() const 成员函数
    }
};

/// @class 过滤迭代器类
/// @brief 根据指定谓词（默认为ObjectIsErasedPredicate）过滤元素
/// @details https://yb.tencent.com/s/lygj9V2cHyJU .提示词:帮我实现一个迭代器，可以过滤标记为已删除的元素，用c++实现，使用的容器是std::vector
template<typename Iterator, typename Predicate = ObjectIsErasedPredicate<typename std::iterator_traits<Iterator>::value_type>>
class FilterIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;

    /// @brief 构造函数 - 接受开始、结束迭代器和谓词
    FilterIterator(Iterator begin, Iterator end, Predicate pred = Predicate{})
        : current_(begin)
        , end_(end)
        , pred_(pred)
    {
        skip_filtered(); // 初始时跳过已删除元素
    }

    /// @brief 构造函数 - 用于创建结束迭代器
    FilterIterator(Iterator end)
        : current_(end)
        , end_(end)
        , pred_(Predicate{})
    {
    }

    // 解引用操作符
    reference operator*() const
    {
        return *current_;
    }

    pointer operator->() const
    {
        return &(*current_);
    }

    // 前缀递增
    FilterIterator& operator++()
    {
        ++current_;
        skip_filtered();
        return *this;
    }

    // 后缀递增
    FilterIterator operator++(int)
    {
        FilterIterator temp = *this;
        ++(*this);
        return temp;
    }

    // 相等比较
    bool operator==(const FilterIterator& other) const
    {
        return current_ == other.current_;
    }

    bool operator!=(const FilterIterator& other) const
    {
        return !(*this == other);
    }

private:
    Iterator current_;   ///< 当前迭代器位置
    Iterator end_;       ///< 结束迭代器位置
    Predicate pred_;     ///< 过滤谓词

    /// @brief 跳过被过滤的元素
    void skip_filtered()
    {
        while (current_ != end_ && pred_(*current_))
        {
            ++current_;
        }
    }
};

#endif //TABLEBASE_H
