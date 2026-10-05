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

/// @file DmCustomEntity.h
/// @brief 自定义实体的基类与代理权限（RENDER_PLAN.md 第 4.8 节）

#ifndef DMCUSTOMENTITY_H
#define DMCUSTOMENTITY_H

#include <cstdint>
#include <vector>

#include "DmEntity.h"
#include "DmGiExplode.h"
#include "GiTransform.h"

class GiStream;

/// @brief 代理权限：定义实体类的扩展不在、实体读成代理（DmProxyEntity）时允许的操作
/// @details 位的取值与 ODA 的 OdDbProxyEntity::ProxyFlags、AutoCAD DXF 的 CLASSES 段组码 90 相同；
///          YiCAD 没有打印样式与材质，不用那两位。只约束代理，类在场的实体不受限制（与 AutoCAD 相同）。
///          炸开不看权限：代理总能按代理图形炸开，不允许删除时原对象留着（AutoCAD 2026 实测）
enum class DmProxyFlags : std::uint32_t
{
    None = 0,
    Erase = 0x001,                  ///< 删除
    Transform = 0x002,              ///< 移动、旋转、缩放、镜像
    ColorChange = 0x004,            ///< 改颜色
    LayerChange = 0x008,            ///< 改图层
    LineTypeChange = 0x010,         ///< 改线型
    LineTypeScaleChange = 0x020,    ///< 改线型比例
    VisibilityChange = 0x040,       ///< 改可见性
    Cloning = 0x080,                ///< 复制（复制命令、镜像保留原对象、剪贴板、复制到图层、建块）
    LineWeightChange = 0x100,       ///< 改线宽
    AllButCloning = 0x17F,          ///< 除复制外全部
    All = 0x1FF,                    ///< 全部
};

constexpr DmProxyFlags operator|(DmProxyFlags a, DmProxyFlags b)
{
    return static_cast<DmProxyFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

/// @brief flags 是否含 flag 的全部位
constexpr bool hasFlag(DmProxyFlags flags, DmProxyFlags flag)
{
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(flag)) == static_cast<std::uint32_t>(flag);
}

/// @brief 自定义实体的基类：扩展派生它，经 IExtensionContext::registerEntityClass 注册（第 4.8.2 节）
/// @details 派生类必须实现：worldDraw、move/rotate/scale/mirror、clone、saveData/restoreData，类型用
///          TYPESYSTEM_SOURCE_NAMED 注册（类名以扩展 ID 加点开头，版本号即数据版本）。修改数据后调用 update()。
///          worldDraw 与内置实体一样遵守 IGiDrawable 的约定：const、确定性，可以在图形系统的工作线程上与别的实体并行调用。
///
///          可选实现（没实现的由宿主从 worldDraw 的输出推导，第 4.8.4 节，做法同 ODA 的 explodeGeometry）：
///          包围框（calculateBorders）、到点的距离与拾取、捕捉（端点、中点、圆心、最近点、交点 intersectWith）、
///          交叉选（getSubEntities）、炸开（explode）。夹点默认没有（getRefPoints 为空），只能整体移动。
///
///          存盘：宿主写公共属性（图层、画笔、线型比例），派生类的数据由宿主当作一段带长度的字节保管，
///          另存一份代理图形（worldDraw 的 GI 流）。读盘时类没注册、或 restoreData 读不了，就建代理实体，字节原样保留
class DmCustomEntity : public DmEntity
{
    TYPESYSTEM_HEADER();

public:
    DmCustomEntity(DmEntity* parent = nullptr);
    DmCustomEntity(const DmCustomEntity& other);
    DmCustomEntity& operator=(const DmCustomEntity&) = delete;
    ~DmCustomEntity() override;

    /// @return DM::EntityCustom
    DM::EntityType getEntityType() const final;

    bool isContainer() const override { return false; }

    /// @brief 类名：存盘、按类名登记的属性编辑与双击编辑命令都按它找；默认取 MetaType 的类型名
    virtual QString className() const;

    /// @brief 数据版本：写出时随数据存下，读回时交给 restoreData；默认取注册表里这个类的版本
    virtual std::uint32_t classVersion() const;

    /// @brief 代理权限，见 DmProxyFlags；默认取注册表里这个类登记的
    virtual DmProxyFlags proxyFlags() const;

    /// @brief 是否为代理实体（DmProxyEntity）
    virtual bool isProxy() const { return false; }

    /// @brief 写出派生类自己的数据（公共属性由宿主写）
    virtual void saveData(OutputStream& out) const = 0;

    /// @brief 读回 saveData 写出的数据
    /// @param version 写出时的数据版本
    /// @return false：读不了（例如版本比程序新），宿主改建代理实体、数据原样保留（对应 ODA 的 eMakeMeProxy）
    virtual bool restoreData(InputStream& in, std::uint32_t version) = 0;

    /// @brief 代理图形：存盘时随实体存下，类不在时代理实体按它显示。默认记录 worldDraw 的输出（GiRegenType::ProxyGraphics）
    virtual GiStream proxyGraphics() const;

    /// @brief 修改数据后调用：递增修订号，作废默认实现缓存的基本实体，重算包围框
    void update() override;

    // ---- 默认实现（第 4.8.4 节），都基于 worldDraw 输出的基本实体（DmGiExplode，Purpose::Query）

    /// @brief 包围框：基本实体包围框的并
    void calculateBorders() override;
    /// @brief 到点的距离：到最近的基本实体（含文字的笔画）
    double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr,
                              DM::ResolveLevel level = DM::ResolveNone) const override;
    bool isPointOnEntity(const DmVector& coord, double tolerance = DM_TOLERANCE) const override;
    /// @brief 捕捉：基本实体（不含文字的笔画）里最近的端点、实体上的点、圆心、中点
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr,
                                     DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;
    /// @brief 交叉选、按子实体搜索用：全部基本实体（含文字的笔画），属本实体所有，下次修改后失效
    std::list<DmEntity*> getSubEntities() const override;

    /// @brief 与另一个实体的交点（对应 ObjectARX 的 subIntersectWith）；默认按基本实体（不含文字的笔画）逐个求交
    /// @details Information::getIntersection 遇到自定义实体时交给它
    virtual DmVectorSolutions intersectWith(const DmEntity* other, bool onEntities) const;

    /// @brief 炸开：返回新建的实体，调用方持有；默认按 worldDraw 的图元做成基本实体（DmGiExplode，Purpose::Explode）
    virtual std::vector<DmEntity*> explode() const;

    /// @brief 按仿射变换改动实体：拆成旋转、镜像、缩放、旋转、平移，依次调用 rotate、mirror、scale、move
    /// @details 代理被变换过（允许变换的代理只记下累计的变换），扩展回来、读成原实体时，宿主用它补上这个变换
    void transformBy(const GiTransform& transform);

    // persistent helper：公共属性、数据版本、数据字节、累计变换（代理才有，原实体恒为恒等）
    void saveStream(OutputStream& wrt) const override;
    void restoreStream(InputStream& rdr, const std::vector<PAIR>& revs) override;
    void restoreStreamWithRev(InputStream& rdr, int rev) override;
    void restoreStream(InputStream& rdr) override;

    /// @brief 上一次 restoreStream 是否因 restoreData 返回 false 而没读成
    bool restoreFailed() const { return m_restoreFailed; }

    /// @brief 写一条存盘记录：类名、代理权限、saveStream 的字节、代理图形
    /// @details 原生格式的自定义实体容器与块定义里的自定义实体都用它（MetaCustomEntities、DmBlock）
    static void writeRecord(OutputStream& out, const DmCustomEntity& entity);
    /// @brief 读一条 writeRecord 写的记录：类注册了且读得了就建原实体，否则建代理实体
    /// @param document 实体所属文档；代理图形的引用按它找回
    /// @param revs 文件里 DmObject 到 DmCustomEntity 各层的版本
    static DmCustomEntity* readRecord(InputStream& in, DmDocument* document, const std::vector<PAIR>& revs);

protected:
    /// @brief 默认实现用的基本实体，按修订号缓存
    const std::vector<DmGiExplode::Item>& queryEntities() const;

    /// @brief 数据字节与版本：代理实体原样保管它们，见 DmProxyEntity
    virtual void saveDataBytes(OutputStream& out) const;
    /// @brief 读回数据字节；默认交给 restoreData
    /// @return false：restoreData 读不了
    virtual bool restoreDataBytes(const std::string& bytes, std::uint32_t version);

    /// @brief 读回时文件里记下的累计变换：原实体在读完数据后按它改动自己（transformBy），代理记下它
    virtual void applyStoredTransform(const GiTransform& transform);

    /// @brief 代理的累计变换；原实体恒为恒等
    virtual GiTransform storedTransform() const;

private:
    mutable std::vector<DmGiExplode::Item> m_queryEntities;    ///< 默认实现用的基本实体
    mutable std::uint64_t m_queryRevision = ~std::uint64_t(0); ///< m_queryEntities 生成时的修订号
    mutable bool m_hasQueryEntities = false;                    ///< m_queryEntities 是否已生成
    bool m_restoreFailed = false;                               ///< 见 restoreFailed()
};

#endif // DMCUSTOMENTITY_H
