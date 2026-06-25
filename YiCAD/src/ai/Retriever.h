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

/// @file Retriever.h
/// @brief 检索器 —— IRetriever 抽象接口 + KeywordRetriever 首版实现
///
/// 设计意图：
///   - IRetriever 为后续升级到向量检索（Embedding + VectorStore）预留接口
///   - KeywordRetriever 是首版简化实现，基于关键词 token 重叠度评分
///   - 外部代码只依赖 IRetriever，方便后续替换

#ifndef RETRIEVER_H
#define RETRIEVER_H

#include "RAGTypes.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

// ---------------------------------------------------------------------------
// IRetriever —— 检索器抽象接口
// ---------------------------------------------------------------------------

class IRetriever
{
public:
    virtual ~IRetriever() = default;

    /// @brief 建立索引（批量导入所有 chunk）
    virtual void index(const QVector<Chunk>& chunks) = 0;

    /// @brief 检索与 query 最相关的 topK 个 chunk
    virtual QVector<Chunk> retrieve(const QString& query, int topK = 5) const = 0;

    /// @brief 是否已完成索引（有数据可检索）
    virtual bool isReady() const = 0;

    /// @brief 当前索引中的 chunk 总数
    virtual int chunkCount() const = 0;
};

// ---------------------------------------------------------------------------
// KeywordRetriever —— 关键词检索器（首版简化实现）
// ---------------------------------------------------------------------------
//
// 检索策略：
//   1. 对 query 和每个 chunk.content 分别做分词
//      - 中文：提取连续 CJK 字符的 bigram
//      - 英文/数字：按空白和标点切分，转小写
//   2. 计算 Jaccard 相似度：|交集| / |并集|
//   3. 按相似度降序排序，返回 topK
//
// 分词示例：
//   "修剪命令怎么用" → ["修剪", "剪命", "命令", "令怎", "怎么", "么用"]
//   "how to trim"    → ["how", "to", "trim"]

class KeywordRetriever : public IRetriever
{
public:
    void index(const QVector<Chunk>& chunks) override;
    QVector<Chunk> retrieve(const QString& query, int topK = 5) const override;
    bool isReady() const override;
    int chunkCount() const override;

private:
    /// @brief 对文本分词
    static QStringList tokenize(const QString& text);

    /// @brief 计算两个 token 集合的 Jaccard 相似度
    static double similarity(const QStringList& tokensA,
                             const QStringList& tokensB);

    QVector<Chunk> m_chunks;
};

#endif // RETRIEVER_H
