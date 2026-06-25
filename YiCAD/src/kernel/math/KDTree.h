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

/// @file KDTree.h
/// @brief KD树模板实现，用于多维空间点的高效范围查询
/// @details 参考：https://github.com/benjones/kdTree

#ifndef _BJ_KD_TREE_H
#define _BJ_KD_TREE_H

#include <vector>
#include <memory>
#include <numeric>
#include <limits>

/// @brief KD树点结构，包含三维坐标和维度信息
struct KDTreePoint
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    constexpr static int DIMENSION = 3;

    /// @brief 获取指定维度的坐标值
    /// @param [in] dim 维度索引 (0=x, 1=y, 2=z)
    /// @return 坐标值，无效维度返回NaN
    double getDimension(int dim) const
    {
        switch (dim)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            return std::numeric_limits<double>::quiet_NaN();
        }
    }
};

/// @brief KD树节点模板类
/// @tparam PointType 点类型
/// @tparam SplitDimension 当前分割维度
template <typename PointType, int SplitDimension>
class KDNode
{
public:
    typedef KDNode<PointType, (SplitDimension + 1) % PointType::DIMENSION> ChildType;

    KDNode(size_t ind)
        : treeIndex(ind)
    {}

    size_t treeIndex = 0;                       ///< 节点在树点列表中的索引
    std::unique_ptr<ChildType> leftChild;       ///< 左子节点
    std::unique_ptr<ChildType> rightChild;      ///< 右子节点
};

/// @brief KD树模板类，支持构建、查询、插入、删除操作
/// @tparam PointType 点类型
/// @tparam PointArray 点数组容器类型，默认std::vector<PointType>
template <typename PointType, typename PointArray = std::vector<PointType> >
class KDTree
{
public:
    KDTree() {}

    KDTree(const PointArray& pointsIn)
    {
        buildTree(pointsIn);
    }

    /// @brief 根据点数组构建KD树
    /// @param [in] pointsIn 输入点数组
    void buildTree(const PointArray& pointsIn);

    /// @brief 中序遍历输出树结构
    void dumpTreeInorder();

    /// @brief 中序遍历，对每个点执行指定函数
    /// @tparam F 可调用对象类型
    /// @param [in] func 对每个点执行的操作
    template <typename F>
    void inorderTraversal(F func);

    /// @brief 查询以testPoint为中心、radius为半径的立方体内的点
    /// @param [in] testPoint 查询中心点
    /// @param [in] radius 查询半径
    /// @return 在范围内的点索引列表
    std::vector<size_t> getPointsWithinCube(PointType testPoint, double radius);

    /// @brief 查找指定维度上的最小值索引
    /// @param [in] dimension 维度
    /// @return 最小值对应的点索引
    size_t findMin(int dimension);

    /// @brief 输出指定节点信息
    /// @param [in] i 节点索引
    void dumpNode(size_t i)
    {
        std::cout << points[i] << std::endl;
    }

    /// @brief 删除指定索引的点
    /// @param [in] nodeIndex 点索引
    void deletePoint(size_t nodeIndex);

    /// @brief 获取指定索引的点
    /// @param [in] nodeIndex 点索引
    /// @return 点数据
    PointType getPoint(size_t nodeIndex)
    {
        return points[nodeIndex];
    }

    /// @brief 获取所有点
    /// @return 点数组
    PointArray getPoints() const
    {
        return points;
    }

    /// @brief 插入点
    /// @param [in] p 待插入的点
    void insertPoint(const PointType& p);

    // END PUBLIC API
private:
    std::unique_ptr<KDNode<PointType, 0> > root;   ///< 根节点
    PointArray points;                               ///< 点数组
    std::vector<size_t> pointIndeces;                ///< 点索引列表

    /// @brief 递归构建子树
    /// @tparam SplitDimension 当前分割维度
    /// @param [in] begin 索引范围起始迭代器
    /// @param [in] end 索引范围结束迭代器
    /// @return 构建的子节点
    template <int SplitDimension>
    std::unique_ptr<KDNode<PointType, SplitDimension> > buildSubtree(
        std::vector<size_t>::iterator begin,
        std::vector<size_t>::iterator end);

    /// @brief 递归输出子树结构
    /// @tparam SplitDimension 当前分割维度
    /// @param [in] node 当前节点
    template<int SplitDimension>
    void dumpSubtree(std::unique_ptr<KDNode<PointType, SplitDimension> >& node);

    /// @brief 递归查询立方体内的点
    /// @tparam SplitDimension 当前分割维度
    /// @param [in] testPoint 查询中心点
    /// @param [in] queryRange 查询范围数组
    /// @param [in] node 当前节点
    /// @param [out] ret 结果点索引列表
    template<int SplitDimension>
    void getPointsWithinCubeSubtree(PointType testPoint,
        double queryRange[2 * PointType::DIMENSION],
        std::unique_ptr<KDNode<PointType, SplitDimension> >& node,
        std::vector<size_t>& ret);

    /// @brief 判断点是否在查询范围内
    /// @param [in] testPoint 测试点
    /// @param [in] queryRange 查询范围数组
    /// @return 在范围内返回true
    static bool pointInRange(PointType testPoint, double queryRange[2 * PointType::DIMENSION])
    {
        for (int i = 0; i < PointType::DIMENSION; ++i)
        {
            if (testPoint.getDimension(i) < queryRange[2 * i]
                || testPoint.getDimension(i) > queryRange[2 * i + 1])
            {
                return false;
            }
        }
        return true;
    }

    /// @brief 递归查找指定维度的最小值
    /// @tparam SplitDimension 当前分割维度
    /// @param [in] dimension 目标维度
    /// @param [in] node 当前节点
    /// @return 最小值对应的点索引
    template<int SplitDimension>
    size_t findMinSubtree(int dimension,
        std::unique_ptr<KDNode<PointType, SplitDimension> >& node);

    /// @brief 递归删除子树中的节点
    /// @tparam SplitDimension 当前分割维度
    /// @param [in] nodeIndex 待删除点索引
    /// @param [in] node 当前节点
    /// @return 更新后的节点
    template<int SplitDimension>
    std::unique_ptr<KDNode<PointType, SplitDimension> >
    deleteFromSubtree(size_t nodeIndex,
        std::unique_ptr<KDNode<PointType, SplitDimension> >& node);

    /// @brief 递归中序遍历子树
    /// @tparam SplitDimension 当前分割维度
    /// @tparam F 可调用对象类型
    /// @param [in] func 对每个点执行的操作
    /// @param [in] node 当前节点
    template<int SplitDimension, typename F>
    void inorderTraversalSubtree(F func,
        std::unique_ptr<KDNode<PointType, SplitDimension> >& node);

    /// @brief 递归插入点到子树
    /// @tparam SplitDimension 当前分割维度
    /// @param [in] node 当前节点
    /// @param [in] pointIndex 待插入点索引
    /// @return 更新后的节点
    template<int SplitDimension>
    std::unique_ptr<KDNode<PointType, SplitDimension> >
    insertPointSubtree(std::unique_ptr<KDNode<PointType, SplitDimension> >& node,
        size_t pointIndex);
};

template<typename PointType, typename PointArray>
void KDTree<PointType, PointArray>::buildTree(const PointArray& pointsIn)
{
    points = pointsIn;
    pointIndeces.resize(points.size());
    std::iota(begin(pointIndeces), end(pointIndeces), 0);
    root = buildSubtree<0>(begin(pointIndeces), end(pointIndeces));
}

template<typename PointType, typename PointArray>
template<int SplitDimension>
std::unique_ptr<KDNode<PointType, SplitDimension> >
KDTree<PointType, PointArray>::buildSubtree(std::vector<size_t>::iterator begin,
    std::vector<size_t>::iterator end)
{
    auto rangeSize = std::distance(begin, end);

    if (rangeSize == 0)
    {
        return std::unique_ptr<KDNode<PointType, SplitDimension> >(nullptr);
    }
    else
    {
        std::sort(begin, end,
            [this](size_t a, size_t b)
            {
                return points[a].getDimension(SplitDimension) < points[b].getDimension(SplitDimension);
            });
        auto median = begin + rangeSize / 2;
        while (median != begin
            && points[*(median)].getDimension(SplitDimension) ==
            points[*(median - 1)].getDimension(SplitDimension))
        {
            --median;
            // put all the nodes with equal coord value in the right subtree
        }
        auto ret = std::unique_ptr<KDNode<PointType, SplitDimension> >
            (new KDNode<PointType, SplitDimension>(*median));

        ret->leftChild = buildSubtree<(SplitDimension + 1) % PointType::DIMENSION>(begin, median);
        ret->rightChild = buildSubtree<(SplitDimension + 1) % PointType::DIMENSION>(median + 1, end);

        return ret;
    }
}

template<typename PointType, typename PointArray>
void KDTree<PointType, PointArray>::dumpTreeInorder()
{
    dumpSubtree<0>(root);
}

template<typename PointType, typename PointArray>
template<int SplitDimension>
void KDTree<PointType, PointArray>::dumpSubtree(
    std::unique_ptr<KDNode<PointType, SplitDimension> >& node)
{
    if (node->leftChild)
    {
        dumpSubtree<(SplitDimension + 1) % PointType::DIMENSION>(node->leftChild);
    }
    //std::cout << node->treeIndex << ": " << points[node->treeIndex] << std::endl;
    if (node->rightChild)
    {
        dumpSubtree<(SplitDimension + 1) % PointType::DIMENSION>(node->rightChild);
    }
}

template<typename PointType, typename PointArray>
std::vector<size_t> KDTree<PointType, PointArray>::getPointsWithinCube(
    PointType testPoint, double radius)
{
    double queryRange[2 * PointType::DIMENSION];
    for (int i = 0; i < PointType::DIMENSION; ++i)
    {
        queryRange[2 * i] = testPoint.getDimension(i) - radius;
        queryRange[2 * i + 1] = testPoint.getDimension(i) + radius;
    }

    std::vector<size_t> ret;
    getPointsWithinCubeSubtree<0>(testPoint, queryRange, root, ret);

    return ret;
}

template<typename PointType, typename PointArray>
template<int SplitDimension>
void KDTree<PointType, PointArray>::getPointsWithinCubeSubtree(
    PointType testPoint,
    double queryRange[2 * PointType::DIMENSION],
    std::unique_ptr<KDNode<PointType, SplitDimension> >& node,
    std::vector<size_t>& ret)
{
    if (node == nullptr)
    {
        return;
    }

    auto nodePoint = points[node->treeIndex];
    if (pointInRange(nodePoint, queryRange))
    {
        ret.push_back(node->treeIndex);
    }
    if (nodePoint.getDimension(SplitDimension) >= queryRange[2 * SplitDimension])
    {
        // query range goes into the left subtree
        getPointsWithinCubeSubtree<(SplitDimension + 1) % PointType::DIMENSION>(testPoint,
            queryRange,
            node->leftChild,
            ret);
    }
    if (nodePoint.getDimension(SplitDimension) <= queryRange[2 * SplitDimension + 1])
    {
        // query range goes into the right subtree
        getPointsWithinCubeSubtree<(SplitDimension + 1) % PointType::DIMENSION>(testPoint,
            queryRange,
            node->rightChild,
            ret);
    }
}

template<typename PointType, typename PointArray>
size_t KDTree<PointType, PointArray>::findMin(int dimension)
{
    return findMinSubtree<0>(dimension, root);
}

template<typename PointType, typename PointArray>
template<int SplitDimension>
size_t KDTree<PointType, PointArray>::findMinSubtree(int dimension,
    std::unique_ptr<KDNode<PointType, SplitDimension> >& node)
{
    constexpr size_t INVALID_INDEX = 123456; // 无效索引标记，用于最小值查找的初始值

    if (SplitDimension == dimension)
    {
        if (node->leftChild == nullptr)
        {
            return node->treeIndex;
        }
        else
        {
            return findMinSubtree<(SplitDimension + 1) % PointType::DIMENSION>(dimension,
                node->leftChild);
        }
    }
    else
    {
        size_t leftMin = INVALID_INDEX;
        size_t rightMin = INVALID_INDEX;
        if (node->leftChild)
        {
            leftMin = findMinSubtree<(SplitDimension + 1) % PointType::DIMENSION>(dimension,
                node->leftChild);
        }
        if (node->rightChild)
        {
            rightMin = findMinSubtree<(SplitDimension + 1) % PointType::DIMENSION>(dimension,
                node->rightChild);
        }

        auto nodeValue = points[node->treeIndex].getDimension(dimension);
        if (node->leftChild
            && points[leftMin].getDimension(dimension) < nodeValue)
        {
            if (node->rightChild)
            {
                return (points[leftMin].getDimension(dimension) <
                    points[rightMin].getDimension(dimension)) ? leftMin : rightMin;
            }
            else
            {
                return leftMin;
            }
        }
        else if (node->rightChild
            && points[rightMin].getDimension(dimension) < nodeValue)
        {
            return rightMin;
        }
        else
        {
            return node->treeIndex;
        }
    }
}

template<typename PointType, typename PointArray>
void KDTree<PointType, PointArray>::deletePoint(size_t nodeIndex)
{
    root = deleteFromSubtree<0>(nodeIndex, root);
}

template<typename PointType, typename PointArray>
template<int SplitDimension>
std::unique_ptr<KDNode<PointType, SplitDimension> >
KDTree<PointType, PointArray>::deleteFromSubtree(size_t nodeIndex,
    std::unique_ptr<KDNode<PointType, SplitDimension> >& node)
{
    constexpr size_t nextDimension = (SplitDimension + 1) % PointType::DIMENSION;

    if (node->treeIndex == nodeIndex)
    {
        if (node->rightChild)
        {
            auto rightMin = findMinSubtree<nextDimension>(SplitDimension, node->rightChild);
            node->treeIndex = rightMin;
            node->rightChild = deleteFromSubtree<nextDimension>(rightMin,
                node->rightChild);
        }
        else if (node->leftChild)
        {
            auto leftMin = findMinSubtree<nextDimension>(SplitDimension, node->leftChild);
            node->treeIndex = leftMin;
            node->rightChild = deleteFromSubtree<nextDimension>(leftMin,
                node->leftChild);
            node->leftChild = nullptr;
        }
        else
        {
            return nullptr;
        }
    }
    else if (points[nodeIndex].getDimension(SplitDimension) <
        points[node->treeIndex].getDimension(SplitDimension))
    {
        node->leftChild = deleteFromSubtree<nextDimension>(nodeIndex,
            node->leftChild);
    }
    else
    {
        node->rightChild = deleteFromSubtree<nextDimension>(nodeIndex,
            node->rightChild);
    }
    return std::move(node);
}

template<typename PointType, typename PointArray>
template <typename F>
void KDTree<PointType, PointArray>::inorderTraversal(F func)
{
    inorderTraversalSubtree<0, F>(func, root);
}

template<typename PointType, typename PointArray>
template<int SplitDimension, typename F>
void KDTree<PointType, PointArray>::inorderTraversalSubtree(F func,
    std::unique_ptr<KDNode<PointType, SplitDimension> >& node)
{
    auto constexpr nextDimension = (SplitDimension + 1) % PointType::DIMENSION;
    if (node->leftChild)
    {
        inorderTraversalSubtree<nextDimension, F>(func, node->leftChild);
    }
    func(points[node->treeIndex]);
    if (node->rightChild)
    {
        inorderTraversalSubtree<nextDimension, F>(func, node->rightChild);
    }
}

template<typename PointType, typename PointArray>
void KDTree<PointType, PointArray>::insertPoint(const PointType& point)
{
    points.push_back(point);
    root = insertPointSubtree<0>(root, points.size() - 1);
}

template<typename PointType, typename PointArray>
template<int SplitDimension>
std::unique_ptr<KDNode<PointType, SplitDimension> >
KDTree<PointType, PointArray>::insertPointSubtree(
    std::unique_ptr<KDNode<PointType, SplitDimension> >& node,
    size_t pointIndex)
{
    auto constexpr nextDimension = (SplitDimension + 1) % PointType::DIMENSION;

    if (node == nullptr)
    {
        return std::unique_ptr<KDNode<PointType, SplitDimension> >(
            new KDNode<PointType, SplitDimension>(pointIndex));
    }
    else if (points[pointIndex].getDimension(SplitDimension) <
        points[node->treeIndex].getDimension(SplitDimension))
    {
        node->leftChild = insertPointSubtree<nextDimension>(node->leftChild, pointIndex);
    }
    else
    {
        node->rightChild = insertPointSubtree<nextDimension>(node->rightChild, pointIndex);
    }
    return std::move(node);
}

#endif //_BJ_KD_TREE_H
