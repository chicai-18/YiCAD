/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmLayer.h
/// @brief 图层类，管理图层的名称、画笔、冻结和锁定状态

#ifndef DMLAYER_H
#define DMLAYER_H

#ifdef __hpux
#    include <sys/_size_t.h>
#endif

#include <iosfwd>

#include "DmPen.h"
#include "DmObject.h"

class DmLayerData
{
public:
    DmLayerData() = default;

    /// @brief 构造图层数据
    /// @param [in] name 图层名
    /// @param [in] pen 默认画笔
    /// @param [in] frozen 是否冻结（不可见）
    /// @param [in] locked 是否锁定
    DmLayerData(const QString& name, const DmPen& pen,
        bool frozen, bool locked);

    QString name;            ///< Layer name
    DmPen pen;               ///< default pen for this layer
    bool frozen = false;     ///< 是否可见
    bool locked = false;     ///< Locked flag
    bool print = true;       ///< Print flag
};

/// @brief 图层
class DmLayer : public DmObject
{
    TYPESYSTEM_HEADER();
private:
    DmLayer(const DmLayer& l) = default;

public:
    DmLayer();

    /// @brief 以名称构造图层
    explicit DmLayer(const QString& name);

    /// @brief 克隆图层
    DmLayer* clone() const;

    /// @brief 设置图层名称
    void setName(const QString& name);

    /// @brief 获取图层名称
    QString getName() const;

    /// @brief 设置图层画笔
    void setPen(const DmPen& pen);

    /// @brief 获取图层画笔
    DmPen getPen() const;

    /// @brief 是否冻结（不可见）
    bool isFrozen() const;

    /// @brief 是否为第0层
    bool isZeroLayer() const;

    /// @brief 切换冻结状态
    void toggle();

    /// @brief 设置冻结状态
    void freeze(bool freeze);

    /// @brief 设置锁定状态
    void lock(bool l);

    /// @brief 是否锁定
    bool isLocked() const;

    /// @brief 设置打印开关
    bool setPrint(const bool print);

    /// @brief 是否打印
    bool isPrint() const;

    /// @brief 获取图层数据副本
    DmLayerData getData() const;

    /// @brief 设置图层数据
    void setData(const DmLayerData& d);

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader,
        const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

private:
    std::shared_ptr<DmLayerData> m_data; ///< Layer data
};

inline bool DmLayer::isZeroLayer() const
{
    return (m_data->name == "0");
}

#endif // DMLAYER_H
