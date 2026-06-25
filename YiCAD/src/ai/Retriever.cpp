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

/// @file Retriever.cpp
/// @brief KeywordRetriever 实现 —— 基于关键词 token 重叠度的检索

#include "Retriever.h"

#include <QSet>
#include <algorithm>
#include <cmath>
#include <utility>

// ============================================================================
// KeywordRetriever —— 公开接口
// ============================================================================

void KeywordRetriever::index(const QVector<Chunk>& chunks)
{
    m_chunks = chunks;
}

QVector<Chunk> KeywordRetriever::retrieve(const QString& query, int topK) const
{
    if (m_chunks.isEmpty() || query.trimmed().isEmpty())
    {
        return {};
    }

    // 对 query 分词
    const QStringList queryTokens = tokenize(query);
    if (queryTokens.isEmpty())
    {
        return {};
    }

    // 计算每个 chunk 的相似度
    struct ScoredChunk
    {
        int    index;
        double score;
    };

    QVector<ScoredChunk> scored;
    scored.reserve(m_chunks.size());

    for (int i = 0; i < m_chunks.size(); ++i)
    {
        const QStringList chunkTokens = tokenize(m_chunks[i].content);
        double s = similarity(queryTokens, chunkTokens);
        if (s > 0.0)
        {
            scored.append({i, s});
        }
    }

    // 按相似度降序排序
    std::sort(scored.begin(), scored.end(),
              [](const ScoredChunk& a, const ScoredChunk& b) {
                  return a.score > b.score;
              });

    // 取 topK
    const int k = std::min(topK, static_cast<int>(scored.size()));
    QVector<Chunk> result;
    result.reserve(k);
    for (int i = 0; i < k; ++i)
    {
        result.append(m_chunks[scored[i].index]);
    }

    return result;
}

bool KeywordRetriever::isReady() const
{
    return !m_chunks.isEmpty();
}

int KeywordRetriever::chunkCount() const
{
    return m_chunks.size();
}

// ============================================================================
// KeywordRetriever —— 分词
// ============================================================================

QStringList KeywordRetriever::tokenize(const QString& text)
{
    QStringList tokens;
    if (text.isEmpty())
    {
        return tokens;
    }

    // ---- Pass 1: 提取连续 CJK 字符序列，生成 bigram ----
    // 先用正则提取所有 CJK 连续段
    int i = 0;
    while (i < text.length())
    {
        const QChar ch = text[i];

        // 判断是否为 CJK 字符（含中文、日文汉字等）
        const ushort uc = ch.unicode();
        bool isCJK = (uc >= 0x4E00 && uc <= 0x9FFF)   // CJK Unified
                  || (uc >= 0x3400 && uc <= 0x4DBF)   // CJK Ext-A
                  || (uc >= 0xF900 && uc <= 0xFAFF)   // CJK Compat
                  || (uc >= 0x3040 && uc <= 0x309F)   // Hiragana
                  || (uc >= 0x30A0 && uc <= 0x30FF)   // Katakana
                  || (uc >= 0xAC00 && uc <= 0xD7AF);  // Hangul

        if (isCJK)
        {
            // 收集连续 CJK 段
            int start = i;
            while (i < text.length())
            {
                const ushort u = text[i].unicode();
                if (!((u >= 0x4E00 && u <= 0x9FFF)
                   || (u >= 0x3400 && u <= 0x4DBF)
                   || (u >= 0xF900 && u <= 0xFAFF)
                   || (u >= 0x3040 && u <= 0x309F)
                   || (u >= 0x30A0 && u <= 0x30FF)
                   || (u >= 0xAC00 && u <= 0xD7AF)))
                {
                    break;
                }
                ++i;
            }
            // 从这一段生成 bigram
            const int len = i - start;
            if (len == 1)
            {
                // 单个 CJK 字符作为一个 token
                tokens.append(text.mid(start, 1));
            }
            else
            {
                for (int j = 0; j < len - 1; ++j)
                {
                    tokens.append(text.mid(start + j, 2));
                }
            }
        }
        else
        {
            ++i;
        }
    }

    // ---- Pass 2: 提取英文/数字 token ----
    // 按非字母数字字符切分
    QString currentWord;
    for (int j = 0; j < text.length(); ++j)
    {
        const QChar ch = text[j];
        if (ch.isLetterOrNumber())
        {
            currentWord += ch.toLower();
        }
        else
        {
            if (currentWord.length() >= 2)
            {
                tokens.append(currentWord);
            }
            currentWord.clear();
        }
    }
    if (currentWord.length() >= 2)
    {
        tokens.append(currentWord);
    }

    // 去重（保留首次出现顺序）
    QSet<QString> seen;
    QStringList unique;
    for (const QString& t : tokens)
    {
        if (!seen.contains(t))
        {
            seen.insert(t);
            unique.append(t);
        }
    }

    return unique;
}

// ============================================================================
// KeywordRetriever —— 相似度计算
// ============================================================================

double KeywordRetriever::similarity(const QStringList& tokensA,
                                    const QStringList& tokensB)
{
    if (tokensA.isEmpty() || tokensB.isEmpty())
    {
        return 0.0;
    }

    // 转为 set 以便快速求交/并
    const QSet<QString> setA(tokensA.begin(), tokensA.end());
    const QSet<QString> setB(tokensB.begin(), tokensB.end());

    // 交集大小
    int intersectCount = 0;
    for (const QString& t : setA)
    {
        if (setB.contains(t))
        {
            ++intersectCount;
        }
    }

    // 并集大小
    QSet<QString> unionSet = setA;
    unionSet.unite(setB);

    if (unionSet.isEmpty())
    {
        return 0.0;
    }

    return static_cast<double>(intersectCount) / unionSet.size();
}
