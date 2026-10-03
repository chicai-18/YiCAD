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

/// @file HiddenSet.h
/// @brief 一个视图的临时隐藏集：命令为了显示预览暂时不画的文档实体，不是图纸数据
///
/// 渲染方案第 4.3.9 节（P18）：修剪、延伸、打断时光标下的实体要换成修剪后的预览，多行文字在位编辑时原文字要让给
/// 编辑器。原先命令直接改实体的可见性（持久属性），再要求画布整图重建；现在交给本集合，与高亮集同样的设计：
/// 每个视图一个，由 UIView 持有，交给画布（经 IHiddenSource 读取），命令结束时清空。命令经 ICommandHost::hidden()
/// 取得。按实体的 DmId 保存，理由同高亮集（HighlightSet.h）。

#ifndef HIDDENSET_H
#define HIDDENSET_H

#include <unordered_set>
#include <vector>

#include <QObject>

#include "DmId.h"
#include "IHiddenSource.h"

class DmDocument;
class DmEntity;

/// @brief 一个视图的临时隐藏集，见文件说明
class HiddenSet : public QObject, public IHiddenSource
{
    Q_OBJECT

public:
    /// @param document 视图的文档，用它的当前实体表解析 id；必须比本对象活得久
    explicit HiddenSet(DmDocument& document);

    HiddenSet(const HiddenSet&) = delete;
    HiddenSet& operator=(const HiddenSet&) = delete;

    // ---- 修改：内容真正改变时发一次 changed() ----

    /// @brief 临时隐藏实体；空指针、已删除的、不在当前实体表里的实体不加入
    void add(DmEntity* entity);
    /// @brief 不再隐藏实体；空指针、不在集合里的实体什么也不做
    void remove(DmEntity* entity);
    /// @brief 全部恢复显示
    void clear();

    // ---- IHiddenSource ----

    /// @brief 隐藏的实体（仍在当前实体表里、未删除的）
    std::vector<DmEntity*> hiddenEntities() const override;
    /// @brief 实体是否隐藏
    bool isHidden(const DmEntity& entity) const override;

signals:
    /// @brief 内容改变时发
    void changed();

private:
    DmDocument&              m_document;  ///< 视图的文档
    std::unordered_set<DmId> m_ids;       ///< 隐藏实体的 id；可能含已删除实体的 id，查询时过滤
};

#endif  // HIDDENSET_H
