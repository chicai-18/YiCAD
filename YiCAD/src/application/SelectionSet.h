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

/// @file SelectionSet.h
/// @brief 一份文档的选择集：选中哪些实体是编辑会话的状态，不是图纸数据
///
/// 每份打开的图纸一个，由 AppDocument 持有（doc/SELECTION_SET_PLAN.md 3.3 节）。命令经
/// ICommandHost::selection()，即时命令经 CommandContext::selection，控件与扩展经
/// IDocumentManager::selection() 取得；画布经 ISelectionSource 读取，不认识本类。
///
/// 选中状态暂时仍存在实体的 FlagSelected 位上，修改后仍经文档通知监听者（标记修改并重绘）；
/// 方案第 4 步改为按 DmId 保存、经自己的信号通知。所以查询的语义与实体的 isSelected() 一致：
/// 只算当前实体表（块编辑时是块的）里可见、未删除的实体。

#ifndef SELECTIONSET_H
#define SELECTIONSET_H

#include <list>
#include <vector>

#include <QString>

#include "Datamodel.h"
#include "DmVector.h"
#include "ISelectionSource.h"

class DmDocument;
class DmEntity;

/// @brief 一份文档的选择集，见文件说明
class SelectionSet : public ISelectionSource
{
public:
    /// @param document 选择集所属的文档；必须比本对象活得久
    explicit SelectionSet(DmDocument& document);

    SelectionSet(const SelectionSet&) = delete;
    SelectionSet& operator=(const SelectionSet&) = delete;

    // ---- 修改：每次调用后通知一次 ----

    /// @brief 选中实体；锁定图层上的实体不选中
    void add(DmEntity* entity);
    /// @brief 取消选中实体
    void remove(DmEntity* entity);
    /// @brief 切换实体的选中状态；锁定图层上的实体不变，也不通知
    void toggle(DmEntity* entity);
    /// @brief 取消选中全部实体
    void clear();

    /// @brief 按矩形选择（CAD 的窗选、交叉选）
    /// @param corner1 矩形的一个角点（世界坐标）
    /// @param corner2 矩形的对角点（世界坐标）
    /// @param select true 为选中，false 为取消选中
    /// @param cross true 为交叉选（与矩形相交即命中），false 为窗选（完全落在矩形内才命中）
    /// @param types 限定的实体类型，为空时不限
    void selectWindow(const DmVector& corner1, const DmVector& corner2, bool select = true, bool cross = false,
                      const std::list<DM::EntityType>& types = {});
    /// @brief 选中或取消选中一个图层上的全部实体；锁定图层上的实体不变
    /// @param layerName 图层名称
    /// @param select true 为选中，false 为取消选中
    void selectLayer(const QString& layerName, bool select = true);
    /// @brief 选中全部可见实体；锁定图层上的实体不选中
    void selectAll();

    // ---- 查询 ----

    /// @brief 实体是否选中
    bool contains(const DmEntity* entity) const;
    /// @brief 选中的实体数
    int count() const;
    /// @brief 没有选中的实体
    bool isEmpty() const;
    /// @brief 选中的实体，按当前实体表的顺序
    std::vector<DmEntity*> entities() const;
    /// @brief 选中实体的夹点里离 coord 最近的一个
    /// @param coord 世界坐标
    /// @param dist 不为空时写入最近距离；没有夹点时不写
    /// @return 最近的夹点；没有时返回无效向量
    DmVector nearestRef(const DmVector& coord, double* dist = nullptr) const;

    /// @brief 同 contains()，供画布判断实体是否按选中绘制
    bool isSelected(const DmEntity& entity) const override;

private:
    /// @brief 通知文档的监听者：标记修改并重绘
    void notifyChanged();

    DmDocument& m_document;  ///< 所属的文档
};

#endif  // SELECTIONSET_H
