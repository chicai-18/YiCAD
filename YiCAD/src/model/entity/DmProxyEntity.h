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

/// @file DmProxyEntity.h
/// @brief 代理实体：定义它的扩展不在时，自定义实体读成它（RENDER_PLAN.md 第 4.8.3 节）

#ifndef DMPROXYENTITY_H
#define DMPROXYENTITY_H

#include <string>

#include "DmCustomEntity.h"
#include "GiStream.h"

/// @brief 代理实体（对应 AutoCAD 的 AcDbProxyEntity、ODA 的 OdDbProxyEntity）
/// @details 保管原实体的类名、数据版本、数据字节、代理权限与代理图形，按代理图形显示；再存盘时这些原样写回，
///          扩展回来后读出来就是原实体。图层、颜色、线型、线宽、线型比例这些公共属性由宿主保管，照常显示。
///
///          能做哪些操作由代理权限决定（DmProxyFlags），命令经 allows() 查。允许变换时只记下累计的变换（同 AutoCAD 在
///          扩展字典 ACAD_PROXY_DATA 里记矩阵），数据字节不动，代理图形按累计变换画；扩展回来、读成原实体时，
///          宿主按这个变换改动原实体（DmCustomEntity::transformBy）。代理没有夹点
class DmProxyEntity final : public DmCustomEntity
{
    TYPESYSTEM_HEADER();

public:
    DmProxyEntity() = default;
    /// @param originalClassName 原实体的类名
    /// @param proxyFlags 原实体的类登记的代理权限
    DmProxyEntity(const QString& originalClassName, DmProxyFlags proxyFlags);

    DmEntity* clone() const override;

    /// @brief 原实体的类名
    QString className() const override { return m_className; }
    /// @brief 原实体写出时的数据版本
    std::uint32_t classVersion() const override { return m_version; }
    DmProxyFlags proxyFlags() const override { return m_proxyFlags; }
    bool isProxy() const override { return true; }

    /// @brief 代理图形，不含累计的变换
    GiStream proxyGraphics() const override { return m_graphics; }
    /// @brief 设置代理图形（读盘时）
    void setProxyGraphics(const GiStream& graphics) { m_graphics = graphics; }

    /// @brief 累计的变换
    const GiTransform& transform() const { return m_transform; }

    /// @brief 原实体的数据字节（不含版本）
    std::string dataBytes() const override { return m_data; }

    /// @brief 公共属性照常画；图元按累计变换重放代理图形
    void worldDraw(IGiWorldDraw& wd) const override;

    /// @brief 允许变换时记下累计的变换，否则什么也不做（命令应先经 allows() 查）
    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    /// @brief 数据字节读不了：代理只是原样保管，restoreDataBytes 已接管
    void saveData(OutputStream& out) const override;
    bool restoreData(InputStream& in, std::uint32_t version) override;

    /// @brief entity 是否允许 operation：不是代理的实体总是允许，代理按它的代理权限
    static bool allows(const DmEntity* entity, DmProxyFlags operation);

    /// @brief 从 entities 里去掉不允许 operation 的代理
    /// @param skipped 非空时输出去掉的个数
    static std::vector<DmEntity*> filterAllowed(const std::vector<DmEntity*>& entities, DmProxyFlags operation,
                                                int* skipped = nullptr);

protected:
    void saveDataBytes(OutputStream& out) const override;
    bool restoreDataBytes(const std::string& bytes, std::uint32_t version) override;
    void applyStoredTransform(const GiTransform& transform) override;
    GiTransform storedTransform() const override { return m_transform; }

    /// @brief 改归目标文档：代理图形里的图层、线型、块换成目标文档的
    void transferReferences(DmDocumentTransfer& transfer) override;

private:
    /// @brief 允许变换时把 step 合成进累计的变换
    void accumulate(const GiTransform& step);

    QString m_className;                                ///< 原实体的类名
    std::uint32_t m_version = 0;                        ///< 原实体写出时的数据版本
    std::string m_data;                                 ///< 原实体的数据字节
    DmProxyFlags m_proxyFlags = DmProxyFlags::None;     ///< 代理权限
    GiStream m_graphics;                                ///< 代理图形
    GiTransform m_transform;                            ///< 累计的变换
};

#endif // DMPROXYENTITY_H
