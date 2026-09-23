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

/// @file ChunkSplitter.h
/// @brief 文档分块器 —— 将 Markdown 文档切分为适合检索的 Chunk
///
/// 分块策略：
///   - 按标题（#、##、###）切分，保留层级信息
///   - 标题之间按段落（空行分隔）二次切分
///   - 每个 chunk 目标长度 200~500 字符
///   - 每个 chunk 附带标题、章节、路径等元数据

#ifndef CHUNKSPLITTER_H
#define CHUNKSPLITTER_H

#include "RAGTypes.h"

#include <QString>
#include <QStringList>
#include <QVector>

class ChunkSplitter
{
public:
    /// @brief 分块参数
    struct Params
    {
        int minChunkChars = 100;   ///< chunk 最小字符数，低于此值会与相邻 chunk 合并
        int maxChunkChars = 500;   ///< chunk 建议最大字符数，超长段落会按句子再切
    };

    /// @brief 构造函数
    explicit ChunkSplitter(const Params& params = Params());

    /// @brief 从单个 Markdown 文件加载并分块
    /// @param filePath   文件绝对/相对路径
    /// @param sourceType 来源类型，如 "doc" 或 "readme"
    /// @return 切分后的 chunk 列表
    QVector<Chunk> splitFile(const QString& filePath, const QString& sourceType);

    /// @brief 从目录递归加载所有 .md 文件并分块
    /// @param dirPath 目录路径
    /// @return 所有文件的切分结果
    QVector<Chunk> splitDirectory(const QString& dirPath);

private:
    /// @brief 从 Markdown 源文本切分
    /// @param content    文件原始文本
    /// @param filePath   文件路径（用于元数据）
    /// @param sourceType 来源类型
    QVector<Chunk> splitContent(const QString& content,
                                const QString& filePath,
                                const QString& sourceType);

    /// @brief 检测文本主要语言
    static QString detectLanguage(const QString& text);

    /// @brief 从文件内容提取标题（第一个 # heading）
    static QString extractTitle(const QString& content);

    /// @brief 将段落文本按 maxChunkChars 再切分
    QStringList splitLongParagraph(const QString& text) const;

    /// @brief 合并过短的相邻 chunk
    static QVector<Chunk> mergeShortChunks(QVector<Chunk> chunks, int minChars);

    Params m_params;
};

#endif // CHUNKSPLITTER_H
