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

/// @file HighlightSet.h
/// @brief 一个视图的高亮集：命令进行中给用户看的拾取反馈，不是图纸数据
///
/// 每个视图一个，由 UIView 持有（doc/HIGHLIGHT_SET_PLAN.md 3.3 节，D1）：它把本对象交给画布，
/// 高亮改变时重建缓存并重绘，命令结束时清空（D2）。命令经 ICommandHost::highlight()，放置工具经
/// BasePlaceTool::command() 取得；画布经 IHighlightSource 读取，不认识本类。
///
/// 按实体的 DmId 保存，不存指针，理由同选择集：实体表释放实体时（EntityTable::remove_direct）
/// 不会留下悬空的指针。集合很小，不像选择集那样监听文档：已删除、不可见的实体在查询时过滤，
/// 命令结束时整体清空。

#ifndef HIGHLIGHTSET_H
#define HIGHLIGHTSET_H

#include <unordered_set>
#include <vector>

#include <QObject>

#include "DmId.h"
#include "IHighlightSource.h"

class DmDocument;
class DmEntity;

/// @brief 一个视图的高亮集，见文件说明
class HighlightSet : public QObject, public IHighlightSource
{
    Q_OBJECT

public:
    /// @param document 视图的文档，用它的当前实体表解析 id；必须比本对象活得久
    explicit HighlightSet(DmDocument& document);

    HighlightSet(const HighlightSet&) = delete;
    HighlightSet& operator=(const HighlightSet&) = delete;

    // ---- 修改：内容真正改变时发一次 changed() ----

    /// @brief 高亮实体；空指针、已删除的、不在当前实体表里的实体（预览里的克隆、拾取到的子实体）不加入
    void add(DmEntity* entity);
    /// @brief 取消高亮实体；空指针、不在集合里的实体什么也不做
    void remove(DmEntity* entity);
    /// @brief 取消全部高亮，包括当前不可见的
    void clear();

    // ---- 查询 ----

    /// @brief 实体是否高亮：记录在集合里，是当前实体表里的那个实体，并且可见、未删除
    bool contains(const DmEntity* entity) const;
    /// @brief 高亮的实体，按当前实体表的顺序
    std::vector<DmEntity*> entities() const;

    /// @brief 同 entities()，供画布组高亮组
    std::vector<DmEntity*> highlightedEntities() const override;

signals:
    /// @brief 高亮改变：加入、取消或清空改变了集合时发，内容不变的调用不发
    void changed();

private:
    DmDocument&              m_document;  ///< 视图的文档
    std::unordered_set<DmId> m_ids;       ///< 高亮实体的 id；可能含已删除、已释放实体的 id，查询时过滤
};

#endif  // HIGHLIGHTSET_H
