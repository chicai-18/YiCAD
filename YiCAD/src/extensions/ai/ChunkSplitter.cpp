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

/// @file ChunkSplitter.cpp
/// @brief ChunkSplitter 实现

#include "ChunkSplitter.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

// ============================================================================
// 构造
// ============================================================================

ChunkSplitter::ChunkSplitter(const Params& params)
    : m_params(params)
{
}

// ============================================================================
// 公开接口 —— 文件级
// ============================================================================

QVector<Chunk> ChunkSplitter::splitFile(const QString& filePath,
                                        const QString& sourceType)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    const QString content = in.readAll();
    file.close();

    return splitContent(content, filePath, sourceType);
}

QVector<Chunk> ChunkSplitter::splitDirectory(const QString& dirPath)
{
    QVector<Chunk> allChunks;

    QDirIterator it(dirPath, QStringList() << "*.md",
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext())
    {
        it.next();
        const QString filePath = it.filePath();
        const QVector<Chunk> chunks = splitFile(filePath, "doc");
        allChunks.append(chunks);
    }

    return allChunks;
}

// ============================================================================
// 核心逻辑 —— 按标题 + 段落切分
// ============================================================================

QVector<Chunk> ChunkSplitter::splitContent(const QString& content,
                                           const QString& filePath,
                                           const QString& sourceType)
{
    QVector<Chunk> result;

    const QString   title      = extractTitle(content);
    const QString   language   = detectLanguage(content);
    const QFileInfo fileInfo(filePath);
    const QString   docPath    = fileInfo.fileName();  // short name for citation

    // 用 heading 作为分段边界
    // 匹配行首的 #、##、### 等
    static const QRegularExpression headingRe(
        "^(#{1,6})\\s+(.+)$",
        QRegularExpression::MultilineOption);

    // 收集所有 heading 位置和文本
    struct Section
    {
        int     startPos;   // heading 行起始位置
        int     contentStart; // heading 行之后正文起始位置
        QString headingText;
        int     level;      // # 数量
    };

    QVector<Section> sections;

    // 使用全局匹配收集所有 section
    QRegularExpressionMatchIterator it = headingRe.globalMatch(content);
    while (it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        Section sec;
        sec.startPos     = m.capturedStart();
        sec.headingText  = m.captured(2).trimmed();
        sec.level        = m.captured(1).length();
        sec.contentStart = m.capturedEnd() + 1;  // 跳过 \n
        sections.append(sec);
    }

    // 如果没有 heading，整个文档作为一个 section
    if (sections.isEmpty())
    {
        Section sec;
        sec.startPos     = 0;
        sec.contentStart = 0;
        sec.headingText  = title;
        sec.level        = 1;
        sections.append(sec);
    }

    // 为每个 section 取正文范围，按段落切分
    for (int i = 0; i < sections.size(); ++i)
    {
        const Section& sec = sections[i];

        // 正文结束位置：下一个 section 的起始位置，或文件末尾
        int endPos = (i + 1 < sections.size())
                         ? sections[i + 1].startPos
                         : content.length();

        // 提取正文
        QString body = content.mid(sec.contentStart,
                                   endPos - sec.contentStart).trimmed();
        if (body.isEmpty())
        {
            continue;
        }

        // 按空行切分为段落
        // 使用正则：连续两个及以上 \n
        static const QRegularExpression paraSplitRe("\\n\\s*\\n");
        QStringList paragraphs = body.split(paraSplitRe, Qt::SkipEmptyParts);

        // 跟踪 chunk 编号
        int chunkIdx = 0;

        for (const QString& para : paragraphs)
        {
            QString trimmed = para.trimmed();
            if (trimmed.isEmpty())
            {
                continue;
            }

            // 跳过纯代码块行（以 ``` 开头或结尾的）
            if (trimmed.startsWith("```") || trimmed == "```")
            {
                continue;
            }

            // 对过长段落再切分
            QStringList subParas = splitLongParagraph(trimmed);

            for (const QString& sub : subParas)
            {
                QString cleaned = sub.trimmed();
                if (cleaned.isEmpty())
                {
                    continue;
                }

                Chunk c;
                c.docPath    = docPath;
                c.title      = title;
                c.section    = sec.headingText;
                c.chunkId    = QString("%1-%2").arg(docPath).arg(chunkIdx++, 3, 10, QChar('0'));
                c.content    = cleaned;
                c.language   = language;
                c.sourceType = sourceType;
                result.append(c);
            }
        }
    }

    // 后处理：合并过短的 chunk
    return mergeShortChunks(result, m_params.minChunkChars);
}

// ============================================================================
// 辅助 —— 长段落再切分
// ============================================================================

QStringList ChunkSplitter::splitLongParagraph(const QString& text) const
{
    if (text.length() <= m_params.maxChunkChars)
    {
        return {text};
    }

    // 按句号、分号、问号等切分
    static const QRegularExpression sentenceRe(
        "([。！？；\\.\\!\\?;]\\s*)");

    QStringList parts;
    int lastEnd = 0;

    QRegularExpressionMatchIterator it = sentenceRe.globalMatch(text);
    while (it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        int end = m.capturedEnd();
        parts.append(text.mid(lastEnd, end - lastEnd));
        lastEnd = end;
    }
    if (lastEnd < text.length())
    {
        parts.append(text.mid(lastEnd));
    }

    // 合并回 chunk 大小范围
    QStringList result;
    QString     accum;
    for (const QString& part : parts)
    {
        if (accum.length() + part.length() > m_params.maxChunkChars && !accum.isEmpty())
        {
            result.append(accum.trimmed());
            accum = part;
        }
        else
        {
            accum += part;
        }
    }
    if (!accum.trimmed().isEmpty())
    {
        result.append(accum.trimmed());
    }

    return result.isEmpty() ? QStringList{text} : result;
}

// ============================================================================
// 辅助 —— 合并过短 chunk
// ============================================================================

QVector<Chunk> ChunkSplitter::mergeShortChunks(QVector<Chunk> chunks, int minChars)
{
    if (chunks.size() <= 1)
    {
        return chunks;
    }

    QVector<Chunk> merged;
    Chunk          accum = chunks[0];

    for (int i = 1; i < chunks.size(); ++i)
    {
        const Chunk& cur = chunks[i];

        // 同文档 + 同章节才合并
        bool sameSection = (accum.docPath == cur.docPath &&
                            accum.section == cur.section);

        if (sameSection && accum.content.length() < minChars)
        {
            accum.content += "\n" + cur.content;
            // 保留第一个 chunk 的 metadata
        }
        else
        {
            merged.append(accum);
            accum = cur;
        }
    }
    merged.append(accum);

    return merged;
}

// ============================================================================
// 辅助 —— 语言检测
// ============================================================================

QString ChunkSplitter::detectLanguage(const QString& text)
{
    // 简单启发式：统计中文字符占比
    int cjkCount = 0;
    int total    = 0;
    for (const QChar& ch : text)
    {
        if (ch.isLetter())
        {
            ++total;
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF)
            {
                ++cjkCount;
            }
        }
    }
    if (total > 0 && static_cast<double>(cjkCount) / total > 0.3)
    {
        return "zh";
    }
    return "en";
}

// ============================================================================
// 辅助 —— 标题提取
// ============================================================================

QString ChunkSplitter::extractTitle(const QString& content)
{
    static const QRegularExpression titleRe(
        "^#\\s+(.+)$",
        QRegularExpression::MultilineOption);
    QRegularExpressionMatch m = titleRe.match(content);
    if (m.hasMatch())
    {
        return m.captured(1).trimmed();
    }
    return {};
}
