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

/// @file DrawArcTangentialCommand.h
/// @brief 相切圆弧命令 ext.draw.arc_tangential，取代原 ActionDrawArcTangential：
///        选直线、圆弧或多段线的一端，画与它相切的圆弧，可锁定半径或包角

#ifndef DRAWARCTANGENTIALCOMMAND_H
#define DRAWARCTANGENTIALCOMMAND_H

#include <functional>
#include <memory>

#include <QCoreApplication>

#include "PlaceCommand.h"

class ArcData;
class DmArc;
class DrawArcTangentialTool;

/// @brief 相切圆弧命令；交互由 DrawArcTangentialTool 驱动，锁定参数在本类
class DrawArcTangentialCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawArcTangentialCommand)

public:
    DrawArcTangentialCommand();
    ~DrawArcTangentialCommand() override;

    // ---- 选项条（UIArcTangentialOptions）----

    /// @brief 最近一次求出的圆弧的半径
    double getRadius() const;
    /// @brief 最近一次求出的圆弧的包角
    double getAngle() const;
    bool isLockRadius() const { return m_lockRadius; }
    void setIsLockRadius(bool lock) { m_lockRadius = lock; }
    double lockRadius() const { return m_lockRadiusValue; }
    void setLockRadius(double radius) { m_lockRadiusValue = radius; }
    bool isLockAngle() const { return m_lockAngle; }
    void setIsLockAngle(bool lock) { m_lockAngle = lock; }
    double lockAngle() const { return m_lockAngleValue; }
    void setLockAngle(double angle) { m_lockAngleValue = angle; }
    /// @brief 按当前参数重算并重画预览
    void updatePreview();

    /// @brief 选项条的回写：工具每求出一个圆弧就把半径与包角（度）交给它
    /// @details 由选项条在 setCommand() 时设置。原先经对话框工厂的 updateArcTangentialOptions()，
    ///          选项条随绘图扩展搬走后，宿主的对话框工厂不再认识它的类型（9.4 节）
    using OptionsUpdater = std::function<void(double radius, bool lockRadius, double angle, bool lockAngle)>;
    void setOptionsUpdater(OptionsUpdater updater) { m_optionsUpdater = std::move(updater); }
    /// @brief 把新求出的圆弧参数交给选项条；没有选项条时什么也不做
    void updateOptions(double radius, bool lockRadius, double angle, bool lockAngle) const;

    // ---- 预览与提交（工具调用）----

    /// @brief 预览圆弧
    void previewArc(const ArcData& data);
    /// @brief 提交圆弧，相对零点移到圆心
    void commitArc(const ArcData& data);
    /// @brief 最近一次求出的圆弧（工具与选项条共用）
    DmArc& arc() { return *m_arc; }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    DrawArcTangentialTool* tool() const;

    std::unique_ptr<DmArc> m_arc;    ///< 最近一次求出的圆弧
    double m_lockRadiusValue = 100.0; ///< 锁定半径值
    bool m_lockRadius = false;       ///< 是否锁定半径
    double m_lockAngleValue = 0.0;   ///< 锁定包角值
    bool m_lockAngle = false;        ///< 是否锁定包角
    OptionsUpdater m_optionsUpdater; ///< 选项条的回写，见 setOptionsUpdater()
};

#endif // DRAWARCTANGENTIALCOMMAND_H
