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

/// @file UIRibbonRegistry.h
/// @brief Ribbon 注册表：类目、面板、按钮与自定义控件的声明式注册。
///
/// 参考 DS（DimX/Source/Ribbon/RibbonRegistry.h）：内置类目与扩展走同一套
/// 注册入口，按钮的外观、位置、可用条件与点击响应都是注册数据，由
/// UIRibbonManager 统一装配到 SARibbonBar 并在状态变化时重算可用状态。
/// 与 DS 的差别：
/// - 按钮可以只给命令 ID（commandId），点击时经宿主按 ID 启动
///   CommandRegistry 里的命令——YiCAD 的命令还要能从命令行启动，所以命令
///   本身不放在 Ribbon 数据里；
/// - 注册冲突用返回值 false 报告而不是 Q_ASSERT，与 CommandRegistry 一致，
///   便于测试；注册表不是单例，由宿主持有；
/// - 暂不支持单选组、下拉菜单与勾选状态（YiCAD 目前用不到）。
///
/// 生命周期：全部 add* 在 finalize() 之前完成（内置类目注册 → 扩展
/// OnRegister → finalize → 装配），finalize 之后数据只读，再调用 add* 返回 false。

#ifndef UIRIBBONREGISTRY_H
#define UIRIBBONREGISTRY_H

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <QString>

class DmDocument;
class QWidget;

/// @brief 重算 Ribbon 可用状态时的上下文快照，由宿主在状态变化时构造。
struct UIRibbonContext
{
    DmDocument* document = nullptr;  ///< 当前活动文档；没有打开文档时为 nullptr
};

/// @brief 可用条件的命名位标志，按 (满足的位 & 要求的位) == 要求的位 判定。
enum class UIRibbonRequires : std::uint32_t
{
    None = 0,
    DocumentOpen = 1u << 0,  ///< 有打开的文档
};

constexpr UIRibbonRequires operator|(UIRibbonRequires a, UIRibbonRequires b) noexcept
{
    using U = std::underlying_type_t<UIRibbonRequires>;
    return static_cast<UIRibbonRequires>(static_cast<U>(a) | static_cast<U>(b));
}

constexpr UIRibbonRequires operator&(UIRibbonRequires a, UIRibbonRequires b) noexcept
{
    using U = std::underlying_type_t<UIRibbonRequires>;
    return static_cast<UIRibbonRequires>(static_cast<U>(a) & static_cast<U>(b));
}

/// @brief 给定上下文，计算当前满足的全部条件位。
UIRibbonRequires deriveSatisfied(const UIRibbonContext& ctx);

/// @brief 可用条件；为空表示始终可用。
using UIRibbonEnableFn = std::function<bool(const UIRibbonContext&)>;

namespace UIRibbonCondition
{
/// @brief 要求的条件全部满足才可用。
UIRibbonEnableFn requireAll(UIRibbonRequires flags);
}  // namespace UIRibbonCondition

/// @brief 类目（Ribbon 标签页）。
struct UIRibbonCategoryDef
{
    QString id;               ///< 全局唯一，如 "category.draw2d"
    QString title;            ///< 已翻译的标题
    QString objectName;       ///< 可选，SARibbonCategory 的 objectName
    UIRibbonEnableFn enableFn;
};

/// @brief 面板。面板里的按钮排进一个 SARibbonButtonGroupWidget。
struct UIRibbonPanelDef
{
    QString id;               ///< 全局唯一，如 "draw2d.line"
    QString categoryId;       ///< 所属类目，finalize 时校验存在
    QString title;
    int rows = 2;             ///< 按钮组的行数
    bool iconOnly = false;    ///< true：按钮只显示图标（文件、设置类目的独立按钮样式）
    UIRibbonEnableFn enableFn;
};

/// @brief 按钮。commandId 与 trigger 二选一：给 commandId 时点击经宿主按 ID
/// 启动 CommandRegistry 里的命令，给 trigger 时直接调用。
struct UIRibbonActionDef
{
    QString id;               ///< 全局唯一；为空时取 commandId
    QString panelId;          ///< 所属面板，或 UIRibbonIds::kPanelRightButtons
    QString text;             ///< 已翻译的按钮文字
    QString iconPath;
    QString toolTip;          ///< 为空时取 text
    QString commandId;
    std::function<void()> trigger;
    UIRibbonEnableFn enableFn;
    QString objectName;       ///< 可选，QAction 的 objectName；为空时取 text
};

/// @brief 自定义控件（如图层面板里的图层下拉框）。
struct UIRibbonWidgetDef
{
    QString id;               ///< 全局唯一
    QString panelId;          ///< 所属面板
    /// @brief 构造控件；parent 是所属的 SARibbonPannel，控件由装配器以大控件方式放入面板。
    std::function<QWidget*(QWidget* parent)> factory;
    UIRibbonEnableFn enableFn;
};

/// @brief 面板里的一项，按注册顺序排列。
using UIRibbonEntry = std::variant<UIRibbonActionDef, UIRibbonWidgetDef>;

/// @brief 内置类目与面板的稳定 ID。扩展把按钮挂进内置面板时引用这里的常量。
namespace UIRibbonIds
{
inline constexpr const char* kCategoryFile = "category.file";
inline constexpr const char* kCategoryDraw2d = "category.draw2d";
inline constexpr const char* kCategoryOptions = "category.options";

inline constexpr const char* kPanelFileFile = "file.file";
inline constexpr const char* kPanelFileExport = "file.export";

inline constexpr const char* kPanelDraw2dLine = "draw2d.line";
inline constexpr const char* kPanelDraw2dCurve = "draw2d.curve";
inline constexpr const char* kPanelDraw2dPolyline = "draw2d.polyline";
inline constexpr const char* kPanelDraw2dCircle = "draw2d.circle";
inline constexpr const char* kPanelDraw2dEllipse = "draw2d.ellipse";
/// @brief 标注面板：宿主只占位，按钮由标注扩展（ext.dim）注册；没有按钮的
/// 面板不装配，移除扩展后这个面板随之消失。
inline constexpr const char* kPanelDraw2dDimension = "draw2d.dimension";
inline constexpr const char* kPanelDraw2dText = "draw2d.text";
inline constexpr const char* kPanelDraw2dOther = "draw2d.other";
inline constexpr const char* kPanelDraw2dModify = "draw2d.modify";
inline constexpr const char* kPanelDraw2dMeasure = "draw2d.measure";
inline constexpr const char* kPanelDraw2dLayer = "draw2d.layer";
inline constexpr const char* kPanelDraw2dBlock = "draw2d.block";

inline constexpr const char* kPanelOptionsSettings = "options.settings";

/// @brief Ribbon 栏右侧常驻按钮组（不属于任何类目），无需注册即可使用。
inline constexpr const char* kPanelRightButtons = "ribbon.right_buttons";
}  // namespace UIRibbonIds

/// @brief Ribbon 注册入口。内置类目直接用 UIRibbonRegistry；扩展拿到的是
/// UIRibbonScopedRegistrar，只接受该扩展命名空间下的 ID。
class UIRibbonRegistrar
{
public:
    virtual ~UIRibbonRegistrar() = default;

    /// @return 成功返回 true；ID 为空或重复、已 finalize 时返回 false。
    virtual bool addCategory(UIRibbonCategoryDef def) = 0;
    /// @return 成功返回 true；ID 为空或重复、已 finalize 时返回 false。
    virtual bool addPanel(UIRibbonPanelDef def) = 0;
    /// @return 成功返回 true；ID 重复、commandId 与 trigger 不是恰好给了一个、
    /// 已 finalize 时返回 false。
    virtual bool addAction(UIRibbonActionDef def) = 0;
    /// @return 成功返回 true；ID 为空或重复、factory 为空、已 finalize 时返回 false。
    virtual bool addWidget(UIRibbonWidgetDef def) = 0;
};

/// @brief Ribbon 注册表本体。
/// @note 仅限 UI 主线程访问。
class UIRibbonRegistry final : public UIRibbonRegistrar
{
public:
    bool addCategory(UIRibbonCategoryDef def) override;
    bool addPanel(UIRibbonPanelDef def) override;
    bool addAction(UIRibbonActionDef def) override;
    bool addWidget(UIRibbonWidgetDef def) override;

    /// @brief 校验并冻结：剔除类目不存在的面板、面板不存在的条目、命令未
    /// 注册的按钮（逐条 qWarning）。只能调用一次。
    void finalize();

    bool isFinalized() const { return m_finalized; }

    /// @brief 全部类目，按注册顺序。
    const std::vector<UIRibbonCategoryDef>& categories() const { return m_categories; }

    /// @brief 属于 categoryId 的面板，按注册顺序。
    std::vector<const UIRibbonPanelDef*> panelsOf(const QString& categoryId) const;

    /// @brief 属于 panelId 的条目，按注册顺序。
    std::vector<const UIRibbonEntry*> entriesOf(const QString& panelId) const;

private:
    bool acceptAdd(const char* kind, const QString& id) const;
    bool hasEntry(const QString& id) const;

    std::vector<UIRibbonCategoryDef> m_categories;
    std::vector<UIRibbonPanelDef> m_panels;
    std::vector<UIRibbonEntry> m_entries;
    bool m_finalized = false;
};

/// @brief 给单个扩展用的注册入口：该扩展自己新建的类目、面板、按钮与控件，
/// ID 必须以 "<扩展 ID>." 开头；引用的父级（categoryId/panelId）与命令 ID
/// 不受此限制，扩展可以把按钮挂进内置面板、也可以给内置命令加按钮。
class UIRibbonScopedRegistrar final : public UIRibbonRegistrar
{
public:
    UIRibbonScopedRegistrar(UIRibbonRegistrar& target, std::string_view extensionId);

    bool addCategory(UIRibbonCategoryDef def) override;
    bool addPanel(UIRibbonPanelDef def) override;
    bool addAction(UIRibbonActionDef def) override;
    bool addWidget(UIRibbonWidgetDef def) override;

private:
    bool owns(const char* kind, const QString& id) const;

    UIRibbonRegistrar& m_target;
    std::string m_extensionId;
};

#endif  // UIRIBBONREGISTRY_H
