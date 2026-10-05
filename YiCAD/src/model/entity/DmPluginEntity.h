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

/// @file DmPluginEntity.h
/// @brief 插件实体：插件经 C ABI 定义的实体类在宿主里的表示（RENDER_PLAN.md 第 4.8.3 节，D5 做法 A）

#ifndef DMPLUGINENTITY_H
#define DMPLUGINENTITY_H

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "DmCustomEntity.h"
#include "GiStream.h"

/// @brief 插件的实体类：插件提供的一组纯函数，输入都是实体的数据字节
/// @details Model 不认识插件 ABI：插件运行时（shell/plugin_runtime 的 PluginEntityClass）实现本接口，
///          把调用转成插件的 C 函数，并隔离插件抛出的异常（失败时返回 false）。
///          只在插件还在（alive()）时可以调用；插件只在程序退出时卸载，卸载前宿主收回它建的全部实例缓存
class DmPluginEntityClass
{
public:
    /// @brief 捕捉的种类（getSnapPoints 的 snapMode）
    enum class SnapMode
    {
        Endpoint,   ///< 端点
        Midpoint,   ///< 中点
        Center,     ///< 圆心
        Nearest,    ///< 实体上离拾取点最近的点
    };

    virtual ~DmPluginEntityClass() = default;

    /// @brief 类名，"插件 ID.类名"
    virtual QString name() const = 0;
    /// @brief 数据版本：写盘时随数据存下，读回的版本低于它时调用 upgrade
    virtual std::uint32_t version() const = 0;
    /// @brief 代理权限：插件不在、实体读成代理时允许的操作
    virtual DmProxyFlags proxyFlags() const = 0;
    /// @brief worldDraw 是否线程安全：是则图形系统在工作线程上直接调用；否则只在数据变化时于修改它的线程（UI 线程）调用
    virtual bool threadSafeDraw() const = 0;
    /// @brief 插件是否还在；为假时不得再调用下面的任何函数
    virtual bool alive() const = 0;

    /// @brief 为一份数据建实例缓存（插件解码后的对象）；插件不用缓存时返回空
    virtual void* createCache(const std::string& data) = 0;
    /// @brief 释放 createCache 建的缓存
    virtual void destroyCache(void* cache) = 0;

    /// @brief 画实体
    /// @param entity 被画的实体：插件按名字找线型、块、文字样式用它的文档，块里的随块属性初值取它的画笔；可为空
    virtual bool worldDraw(const std::string& data, void* cache, IGiWorldDraw& wd, const DmEntity* entity) const = 0;
    /// @brief 包围框
    virtual bool extents(const std::string& data, void* cache, DmVector& minCorner, DmVector& maxCorner) const = 0;
    /// @brief 按仿射变换改动，out 为改动后的数据
    virtual bool transform(const std::string& data, void* cache, const GiTransform& transform,
                           std::string& out) const = 0;

    /// @brief 插件是否提供夹点
    virtual bool hasGrips() const = 0;
    /// @brief 夹点位置
    virtual bool grips(const std::string& data, void* cache, std::vector<DmVector>& out) const = 0;
    /// @brief 移动夹点，out 为改动后的数据
    /// @param indices grips() 交出的夹点的序号
    virtual bool moveGrips(const std::string& data, void* cache, std::span<const std::uint32_t> indices,
                           const DmVector& offset, std::string& out) const = 0;

    /// @brief 插件是否提供捕捉点
    virtual bool hasSnapPoints() const = 0;
    /// @brief 拾取点 pick 附近某种捕捉点的候选
    virtual bool snapPoints(const std::string& data, void* cache, SnapMode mode, const DmVector& pick,
                            std::vector<DmVector>& out) const = 0;

    /// @brief 插件是否提供炸开
    virtual bool hasExplode() const = 0;
    /// @brief 炸开成基本实体；新建的实体交给调用方持有
    /// @param entity 被炸开的实体：插件没给属性的部分取它的图层与画笔，建出的实体归它的文档
    virtual bool explode(const std::string& data, void* cache, const DmEntity& entity,
                         std::vector<DmEntity*>& out) const = 0;

    /// @brief 插件是否提供数据升级
    virtual bool hasUpgrade() const = 0;
    /// @brief 把 fromVersion 版的数据升级到当前版本
    virtual bool upgrade(std::uint32_t fromVersion, const std::string& data, std::string& out) const = 0;
};

/// @brief 插件实体（第 4.8.3 节）：宿主用这一个类表示所有插件实体
/// @details 持有类、一段不透明的数据字节（插件自己定义编码）、GI 流（兼作代理图形）与包围框。撤销与重做就是换回
///          另一份字节（saveStream/restoreStream），存盘写类名、数据版本、字节与代理图形，读盘不需要插件在场（第 7 阶段的记录）。
///
///          线程：worldDraw 不声明线程安全的类，在数据变化（update）时于当前线程调一次插件、记成 GI 流，之后的 worldDraw
///          只重放它，图形系统在工作线程上并行重放也不会碰到插件；声明线程安全的类不缓存，worldDraw 直接调插件。
///          插件卸载（只在程序退出时）后实体不再调用插件，只画记下的 GI 流
class DmPluginEntity final : public DmCustomEntity
{
    TYPESYSTEM_HEADER();

public:
    /// @brief 没有类的空实体（类型系统建实例用）；不画任何东西
    DmPluginEntity() = default;
    /// @param entityClass 实体类
    /// @param data 数据字节（当前数据版本）
    explicit DmPluginEntity(std::shared_ptr<DmPluginEntityClass> entityClass, std::string data = {});
    DmPluginEntity(const DmPluginEntity& other);
    DmPluginEntity& operator=(const DmPluginEntity&) = delete;
    ~DmPluginEntity() override;

    DmEntity* clone() const override;

    QString className() const override;
    std::uint32_t classVersion() const override;
    DmProxyFlags proxyFlags() const override;

    /// @brief 实体类
    const std::shared_ptr<DmPluginEntityClass>& entityClass() const { return m_class; }

    /// @brief 数据字节
    const std::string& data() const { return m_data; }
    std::string dataBytes() const override { return m_data; }
    /// @brief 换一份数据（当前数据版本）并 update()；修改文档里的实体时调用方先 startModify
    void setData(std::string data);

    /// @brief 数据变了：换实例缓存，不声明线程安全的类重新记下 GI 流，再递增修订号、重算包围框
    void update() override;

    void worldDraw(IGiWorldDraw& wd) const override;
    /// @brief 代理图形：不声明线程安全的类即记下的 GI 流，否则现记
    GiStream proxyGraphics() const override;

    /// @brief 包围框：插件给的；插件不在或失败时按 GI 流（默认实现）
    void calculateBorders() override;

    /// @brief 变换都交给插件的 transform
    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    /// @brief 夹点：插件给的；插件没提供时没有夹点，只能整体移动
    DmVectorSolutions getRefPoints() const override;
    /// @brief 拖动夹点：按位置找出 ref 是哪几个夹点，交给插件的 moveGrips
    void moveRef(const DmVector& ref, const DmVector& offset) override;

    /// @brief 捕捉：插件提供时用插件给的候选，否则用默认实现（按 GI 流）
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr,
                                     DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;

    /// @brief 炸开：插件提供时用插件的，否则或插件失败时用默认实现（按 GI 流做成基本实体）
    std::vector<DmEntity*> explode() const override;

    /// @brief 数据由宿主按字节保管（saveDataBytes/restoreDataBytes），这两个不用
    void saveData(OutputStream& out) const override;
    bool restoreData(InputStream& in, std::uint32_t version) override;

protected:
    void saveDataBytes(OutputStream& out) const override;
    /// @brief 读回数据：版本比类新时读不了（改建代理）；比类旧时经插件的 upgrade 升级，插件没提供时也读不了
    bool restoreDataBytes(const std::string& bytes, std::uint32_t version) override;
    /// @brief 代理时记下的累计变换：一次交给插件的 transform
    void applyStoredTransform(const GiTransform& transform) override;

private:
    /// @brief 插件在、有数据，可以调用插件（读盘时按类建出、尚未读入数据的实体不调用）
    bool callable() const { return m_class && m_class->alive() && !m_data.empty(); }
    /// @brief 按变换改动数据
    void applyTransform(const GiTransform& transform);
    /// @brief 插件给的某种捕捉点里离 coord 最近的；没有时返回无效点
    DmVector nearestSnap(DmPluginEntityClass::SnapMode mode, const DmVector& coord, double* dist) const;
    /// @brief 换实例缓存
    void resetCache();

    std::shared_ptr<DmPluginEntityClass> m_class;   ///< 实体类
    std::string m_data;                             ///< 数据字节
    void* m_cache = nullptr;                        ///< 插件的实例缓存，随数据作废
    GiStream m_graphics;                            ///< 不声明线程安全的类：数据变化时记下的插件输出
};

#endif // DMPLUGINENTITY_H
