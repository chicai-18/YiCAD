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

/// @file GuiDialogFactoryInterface.h
/// @brief 界面服务接口：内核、命令机制与扩展经它使用宿主界面
///
/// 名字沿用"对话框工厂"，内容已不是业务对话框：业务对话框由扩展直接构造
/// （doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）。留下的是
///   - 通用提示：警告、确认、是/否/取消（测试夹具重写它们预设回答）；
///   - 内核反向要的：当前活动文档、未命名文档的名字、文件读写；
///   - 选项条的摆放：命令的（CommandInfo::commandOptionsFactory）、编辑模式的、捕捉中点的；
///   - 状态栏与命令行的反馈：坐标、按键提示、选中数量、命令消息。
/// 由主窗口经 GuiDialogFactory::setFactoryObject 装入实现（UIDialogFactory），没有装入时用
/// 什么也不做的 GuiDialogFactoryAdapter。

#ifndef GUIDIALOGFACTORYINTERFACE_H
#define GUIDIALOGFACTORYINTERFACE_H

#include <functional>

#include <QString>

#include "Datamodel.h"

class DmDocument;
class DmVector;
class IExclusiveCommand;
class QWidget;
class UIBottomWindow;
class UICommandWidget;

/// @brief 是/否/取消对话框的回答
enum class DialogAnswer
{
    Yes,    ///< 是
    No,     ///< 否
    Cancel  ///< 取消
};

/// @brief 界面服务接口，见文件说明
class GuiDialogFactoryInterface
{
public:
    virtual ~GuiDialogFactoryInterface() = default;

    /// @brief 显示警告消息对话框
    /// @param warning 警告消息文本
    virtual void requestWarningDialog(const QString& warning) = 0;

    /// @brief 显示确认对话框（确定/取消）
    /// @param title 标题
    /// @param message 提示文本
    /// @return 用户选择确定返回 true，取消返回 false
    virtual bool requestConfirmDialog(const QString& title, const QString& message) = 0;

    /// @brief 显示是/否/取消的提问对话框
    /// @param title 标题
    /// @param message 提问文本
    /// @return 用户的回答；关闭对话框视为取消
    virtual DialogAnswer requestYesNoCancelDialog(const QString& title, const QString& message) = 0;

    /// @brief 获取当前活动文档
    /// @details Model 层部分实体（如标注）在缺少自身文档上下文时，需要落回
    /// 应用当前活动文档；该概念由 App/UI 层维护，此处仅做接口注入。
    /// @return 当前活动文档指针，无则返回 nullptr
    virtual DmDocument* requestActiveDocument() = 0;

    /// @brief 为从未保存过的文档请求一个用于自动保存的默认名称
    /// @details 默认实现里这个名称通常来自文档所在的界面标签页标题
    /// @param document 待命名的文档
    /// @return 建议的名称，无法获取时返回空字符串
    virtual QString requestUntitledDocumentName(DmDocument* document) = 0;

    /// @brief 请求执行文件导出（写盘）
    /// @param document 待导出的文档
    /// @param file 目标文件路径
    /// @param formatType 导出格式
    /// @return 导出是否成功
    virtual bool requestFileExport(DmDocument& document, const QString& file, const QString& formatType) = 0;

    /// @brief 请求执行文件导入（读盘）
    /// @param document 承接导入内容的文档
    /// @param file 源文件路径
    /// @return 导入是否成功
    virtual bool requestFileImport(DmDocument& document, const QString& file) = 0;

    /// @brief 显示交互命令的选项条（CommandInfo::commandOptionsFactory 注册的控件）
    /// @param command 需要选项的命令
    /// @param on true 打开控件，false 关闭控件
    /// @param update true 从命令获取数据，false 从配置文件获取数据
    virtual void requestCommandOptions(IExclusiveCommand* command, bool on, bool update = false) = 0;

    /// @brief 在选项条区域显示或收起编辑模式的选项条（如块编辑模式的块名与"完成"按钮）
    /// @details 编辑模式不是命令，选项条不随命令注册；控件由调用方构造，宿主只负责摆放。
    ///          与命令的选项条各占一个位置，互不删除
    /// @param build 在给定的选项条容器里构造控件；on 为 false 时不调用
    /// @param on true 打开，false 收起
    virtual void requestEditModeOptions(const std::function<QWidget*(QWidget*)>& build, bool on) = 0;

    /// @brief 显示捕捉中点选项控件
    /// @param[out] middlePoints 中点数量
    /// @param on true 打开控件，false 关闭控件
    virtual void requestSnapMiddleOptions(int& middlePoints, bool on) = 0;

    /// @brief 更新坐标显示控件
    /// @details 每次鼠标位置变化时调用
    /// @param abs 鼠标光标的绝对坐标或捕捉点坐标
    /// @param rel 相对坐标
    /// @param updateFormat 是否更新格式
    virtual void updateCoordinateWidget(const DmVector& abs, const DmVector& rel, bool updateFormat = false) = 0;

    /// @brief 更新鼠标按钮提示控件
    /// @details 通常由操作调用，告知用户当前鼠标按钮的功能
    /// @param left 左键帮助文本
    /// @param right 右键帮助文本
    virtual void updateMouseWidget(const QString & = QString(), const QString & = QString()) = 0;

    /// @brief 更新选中实体数量显示
    /// @details 每次选择变化时调用
    /// @param num 选中实体的数量
    virtual void updateSelectionWidget(int num) = 0;

    /// @brief 显示命令消息
    /// @details 通常由操作调用，向用户显示当前事件和错误信息
    /// @param message 要显示的消息
    virtual void commandMessage(const QString& message) = 0;

    /// @brief 设置命令控件
    /// @param widget 命令控件指针
    virtual void setCommandWidget(UICommandWidget* widget) = 0;

    /// @brief 设置底部窗口控件
    /// @param widget 底部窗口控件指针
    virtual void setBottomWidget(UIBottomWindow* widget) = 0;
};

#endif
