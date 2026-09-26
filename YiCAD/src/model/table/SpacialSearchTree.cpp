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

/// @file SpacialSearchTree.cpp
/// @brief 空间搜索树类实现

#include "SpacialSearchTree.h"
#include "RTree.h"
#include "DmEntity.h"
#include "DmBlockReference.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace
{
bool isValidBoundingBox(const DmVector& min, const DmVector& max)
{
    return std::isfinite(min.x) && std::isfinite(min.y) &&
        std::isfinite(max.x) && std::isfinite(max.y) &&
        min.x <= max.x && min.y <= max.y;
}
}

SearchTreeBoundingBox::SearchTreeBoundingBox(const DmVector& min, const DmVector& max)
    : min(min)
    , max(max)
{
}

bool SearchTreeBoundingBox::operator==(const SearchTreeBoundingBox& box) const
{
    return (min == box.min) && (max == box.max);
}

/// @brief 在 RTree 上补充最近邻查询
/// @details RTree.h 是第三方实现，只提供矩形范围查询；最近邻要遍历受保护的节点结构，
///          因此用派生类实现，不改第三方文件。
class SearchTree : public RTree<DmEntity*, double, 2>
{
public:
    /// @brief best-first 最近邻查询，语义见 SpacialSearchTree::nearest
    DmEntity* nearest(const double pt[2], const std::function<double(DmEntity*)>& distanceFn, double* dist) const
    {
        // 队列元素：node 非空时是待展开的节点，否则是待求精确距离的实体。
        // boxDist 是包围框到查询点的距离，也是其中任何实体精确距离的下界。
        struct Item
        {
            double boxDist;
            Node* node;
            DmEntity* data;
        };
        auto farther = [](const Item& a, const Item& b) { return a.boxDist > b.boxDist; };
        std::priority_queue<Item, std::vector<Item>, decltype(farther)> queue(farther);

        DmEntity* best = nullptr;
        double bestDist = std::numeric_limits<double>::infinity();
        if (m_root && m_root->m_count > 0)
        {
            queue.push({ 0.0, m_root, nullptr });
        }
        while (!queue.empty() && queue.top().boxDist < bestDist)
        {
            Item item = queue.top();
            queue.pop();
            if (!item.node)
            {
                double d = distanceFn(item.data);
                if (d < bestDist)
                {
                    bestDist = d;
                    best = item.data;
                }
                continue;
            }
            for (int i = 0; i < item.node->m_count; ++i)
            {
                const Branch& branch = item.node->m_branch[i];
                double d = rectDistance(branch.m_rect, pt);
                if (d >= bestDist)
                {
                    continue;
                }
                if (item.node->IsInternalNode())
                {
                    queue.push({ d, branch.m_child, nullptr });
                }
                else
                {
                    queue.push({ d, nullptr, branch.m_data });
                }
            }
        }
        if (best && dist)
        {
            *dist = bestDist;
        }
        return best;
    }

    /// @brief 整棵树的包围框，即根节点各分支矩形的并集；空树返回 false
    bool bounds(double min[2], double max[2]) const
    {
        if (!m_root || m_root->m_count == 0)
        {
            return false;
        }
        for (int d = 0; d < 2; ++d)
        {
            min[d] = m_root->m_branch[0].m_rect.m_min[d];
            max[d] = m_root->m_branch[0].m_rect.m_max[d];
        }
        for (int i = 1; i < m_root->m_count; ++i)
        {
            for (int d = 0; d < 2; ++d)
            {
                min[d] = std::min(min[d], m_root->m_branch[i].m_rect.m_min[d]);
                max[d] = std::max(max[d], m_root->m_branch[i].m_rect.m_max[d]);
            }
        }
        return true;
    }

private:
    /// @brief 点到矩形的欧氏距离，点在矩形内为 0
    static double rectDistance(const Rect& rect, const double pt[2])
    {
        double squared = 0.0;
        for (int i = 0; i < 2; ++i)
        {
            double d = std::max({ rect.m_min[i] - pt[i], 0.0, pt[i] - rect.m_max[i] });
            squared += d * d;
        }
        return std::sqrt(squared);
    }
};

class SpacialSearchTreePrivate
{
public:
    SearchTree searchTree;
};

SpacialSearchTree::SpacialSearchTree()
    : m_pTreePrivate(nullptr)
{
    m_pTreePrivate = new SpacialSearchTreePrivate();
}

SpacialSearchTree::~SpacialSearchTree()
{
    if (m_pTreePrivate)
    {
        delete m_pTreePrivate;
        m_pTreePrivate = nullptr;
    }
}

void SpacialSearchTree::insert(DmEntity* entity)
{
    if (!entity)
    {
        return;
    }

    // 已在树中的实体只更新包围框，保证每个实体在树中只有一份：
    // 重复插入会留下 remove 删不掉的残余条目，范围查询也会返回重复结果。
    if (entity->getId().isValid()
        && m_searchTreeBoundingBoxes.find(entity->getId()) != m_searchTreeBoundingBoxes.end())
    {
        update(entity);
        return;
    }

    DmVector min = entity->getMin();
    DmVector max = entity->getMax();
    if (!isValidBoundingBox(min, max))
    {
        return;
    }

    // 插入实体本身（包括块参照）
    double mind[2] = { min.x, min.y };
    double maxd[2] = { max.x, max.y };
    m_pTreePrivate->searchTree.Insert(mind, maxd, entity);
    if (entity->getId().isValid())
    {
        SearchTreeBoundingBox box(min, max);
        m_searchTreeBoundingBoxes.insert(std::make_pair(entity->getId(), box));
    }
}

void SpacialSearchTree::remove(DmEntity* entity)
{
    if (!entity || !entity->getId().isValid())
    {
        return;
    }

    auto it = m_searchTreeBoundingBoxes.find(entity->getId());
    if (it == m_searchTreeBoundingBoxes.end())
    {
        return;
    }

    const DmVector& min = it->second.min;
    const DmVector& max = it->second.max;
    double mind[2] = { min.x, min.y };
    double maxd[2] = { max.x, max.y };
    m_pTreePrivate->searchTree.Remove(mind, maxd, entity);
    m_searchTreeBoundingBoxes.erase(it);

    // 如果是块参照，也移除子实体
    auto entType = entity->getEntityType();
    if (entType == DM::EntityBlockReference)
    {
        DmBlockReference* blkRef = static_cast<DmBlockReference*>(entity);
        std::vector<DmEntity*> subEnts;
        getEntitiesOfBlockReferenceRecursive(blkRef, subEnts);
        for (auto e : subEnts)
        {
            remove(e);
        }
    }
}

void SpacialSearchTree::update(DmEntity* entity)
{
    if (!entity)
    {
        return;
    }
    auto id = entity->getId();
    if (!id.isValid())
    {
        return;
    }
    auto it = m_searchTreeBoundingBoxes.find(id);
    if (it == m_searchTreeBoundingBoxes.end())
    {
        return;
    }
    DmVector newMin = entity->getMin();
    DmVector newMax = entity->getMax();
    double newMinD[2] = { newMin.x, newMin.y };
    double newMaxD[2] = { newMax.x, newMax.y };
    SearchTreeBoundingBox newBox(newMin, newMax);
    if (!(it->second == newBox))
    {
        DmVector oldMin = it->second.min;
        DmVector oldMax = it->second.max;
        double oldMinD[2] = { oldMin.x, oldMin.y };
        double oldMaxD[2] = { oldMax.x, oldMax.y };
        m_pTreePrivate->searchTree.Remove(oldMinD, oldMaxD, entity);
        m_pTreePrivate->searchTree.Insert(newMinD, newMaxD, entity);
        it->second = newBox;
    }
}

void SpacialSearchTree::search(const DmVector& min, const DmVector& max, std::vector<DmEntity*>& ents)
{
    double mind[2] = { min.x, min.y };
    double maxd[2] = { max.x, max.y };
    auto callback = [&ents](DmEntity* ent)
    {
        ents.emplace_back(ent);
        return true;
    };
    m_pTreePrivate->searchTree.Search(mind, maxd, callback);
}

DmEntity* SpacialSearchTree::nearest(const DmVector& pt, const std::function<double(DmEntity*)>& distanceFn, double* dist) const
{
    double ptd[2] = { pt.x, pt.y };
    return m_pTreePrivate->searchTree.nearest(ptd, distanceFn, dist);
}

bool SpacialSearchTree::getBounds(DmVector& min, DmVector& max) const
{
    double mind[2];
    double maxd[2];
    if (!m_pTreePrivate->searchTree.bounds(mind, maxd))
    {
        return false;
    }
    min = DmVector(mind[0], mind[1]);
    max = DmVector(maxd[0], maxd[1]);
    return true;
}

void SpacialSearchTree::getEntitiesOfBlockReferenceRecursive(DmBlockReference* blkRef, std::vector<DmEntity*>& ents)
{
    for (auto e : blkRef->getEntityList())
    {
        if (e->getEntityType() == DM::EntityBlockReference)
        {
            DmBlockReference* subBlkRef = static_cast<DmBlockReference*>(e);
            getEntitiesOfBlockReferenceRecursive(subBlkRef, ents);
        }
        else
        {
            ents.emplace_back(e);
        }
    }
}

void SpacialSearchTree::clear()
{
    m_pTreePrivate->searchTree.RemoveAll();
    m_searchTreeBoundingBoxes.clear();
}
