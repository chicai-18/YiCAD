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
/// 按实体的 DmId 保存，不存指针：实体表释放实体时（EntityTable::remove_direct、clear_direct）
/// 选择集里不会留下悬空的指针。查询只算当前实体表（块编辑时是块的）里可见、未删除的实体，
/// 按实体表的顺序给出。本类登记为文档的监听者：文档通知修改（事务提交、撤销、重做）时剔除
/// 已删除或已不在当前实体表里的实体，所以删除后撤销，实体回来时是未选中的；进出块编辑时清空。

#ifndef SELECTIONSET_H
#define SELECTIONSET_H

#include <list>
#include <unordered_set>
#include <vector>

#include <QObject>
#include <QString>

#include "Datamodel.h"
#include "DmDocumentListener.h"
#include "DmId.h"
#include "DmVector.h"
#include "ISelectionSource.h"

class DmDocument;
class DmEntity;

/// @brief 一份文档的选择集，见文件说明
class SelectionSet : public QObject, public ISelectionSource, private DmDocumentListener
{
    Q_OBJECT

public:
    /// @param document 选择集所属的文档；必须比本对象活得久
    explicit SelectionSet(DmDocument& document);
    ~SelectionSet() override;

    SelectionSet(const SelectionSet&) = delete;
    SelectionSet& operator=(const SelectionSet&) = delete;

    // ---- 修改：每次调用后发一次 changed() ----

    /// @brief 选中实体；锁定图层上的、已删除的、不在当前实体表里的实体不选中
    void add(DmEntity* entity);
    /// @brief 取消选中实体
    void remove(DmEntity* entity);
    /// @brief 切换实体的选中状态；锁定图层上的实体不变，也不发 changed()
    void toggle(DmEntity* entity);
    /// @brief 取消选中全部实体，包括当前不可见的
    void clear();

    /// @brief 按矩形选择（CAD 的窗选、交叉选）
    /// @param corner1 矩形的一个角点（世界坐标）
    /// @param corner2 矩形的对角点（世界坐标）
    /// @param select true 为选中，false 为取消选中
    /// @param cross true 为交叉选（与矩形相交即命中），false 为窗选（完全落在矩形内才命中）
    /// @param types 限定的实体类型，为空时不限
    void selectWindow(const DmVector& corner1, const DmVector& corner2, bool select = true, bool cross = false,
                      const std::list<DM::EntityType>& types = {});
    /// @brief 选中或取消选中一个图层上的全部可见实体；锁定图层上的实体不变
    /// @param layerName 图层名称
    /// @param select true 为选中，false 为取消选中
    void selectLayer(const QString& layerName, bool select = true);
    /// @brief 选中全部可见实体；锁定图层上的实体不选中
    void selectAll();

    // ---- 查询 ----

    /// @brief 实体是否选中：记录在选择集里，并且可见、未删除
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
    /// @brief 选中的实体，顺序不定：按 id 在实体表里查，开销只随选中数（entities() 按实体表的顺序，要遍历全表）；
    ///        供画布重建选中组与夹点
    std::vector<DmEntity*> selectedEntities() const override;
    bool hasMoreThan(std::size_t count) const override;

signals:
    /// @brief 选择改变：每次修改调用后发一次（一次框选算一次），剔除了已删除的实体、进出块编辑清空后也发
    void changed();

private:
    /// @brief 文档已修改：剔除已删除或已不在当前实体表里的实体，剔除了就发 changed()
    void documentModified() override;
    void redrawRequested() override {}
    /// @brief 进出块编辑：清空
    void paintContainerChanged(DmEntityContainer* container) override;

    /// @brief 实体在当前实体表里、未删除，且不在锁定图层上，可以加入
    bool canAdd(const DmEntity* entity) const;
    /// @brief 按 id 在当前实体表里找可见、未删除的实体；没有时返回空
    DmEntity* findVisible(const DmId& id) const;

    DmDocument&              m_document;  ///< 所属的文档
    std::unordered_set<DmId> m_ids;       ///< 选中实体的 id；可能含已删除实体的 id，查询时过滤、文档通知修改时剔除
};

#endif  // SELECTIONSET_H
