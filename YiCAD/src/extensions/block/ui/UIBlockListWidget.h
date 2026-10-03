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

/// @file UIBlockListWidget.h
/// @brief 块列表控件

#ifndef UIBLOCKLISTWIDGET_H
#define UIBLOCKLISTWIDGET_H

#include <functional>
#include <memory>

#include <QWidget>
#include <QIcon>

class DmBlock;
class DmBlockTable;
class GsModel;

/// @class ModelWidget
/// @brief 块预览子控件，响应点击事件
class ModelWidget : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    ModelWidget(QWidget* parent = nullptr);

    /// @brief 析构函数
    ~ModelWidget() = default;

protected:
    /// @brief 鼠标释放事件
    /// @param [in] ev 鼠标事件
    void mouseReleaseEvent(QMouseEvent* ev);

signals:
    /// @brief 点击信号
    void clicked();
};

/// @class UIBlockListWidget
/// @brief 块列表控件，以网格形式展示块预览
class UIBlockListWidget : public QWidget
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] onChosen 用户点了列表里的一个块（已激活它）之后调用，如插入块命令进入放置
    /// @param [in] parent 父窗口指针
    /// @param [in] name 对象名称
    /// @param [in] fl 窗口标志
    UIBlockListWidget(std::function<void(DmBlock*)> onChosen, QWidget* parent, const char* name = 0, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIBlockListWidget();

    /// @brief 设置块表
    /// @param [in] blockTable 块表指针
    /// @param [in] graphics 文档的图形模型，预览直接画其中的块几何；为空时每个预览自建模型
    void setBlockList(DmBlockTable* blockTable, std::shared_ptr<GsModel> graphics = nullptr);

    /// @brief 获取块表
    /// @return 块表指针
    DmBlockTable* getBlockTable();

    /// @brief 刷新界面
    void update();

private:
    DmBlockTable*       m_pBlockList;                                           ///< 块表指针
    std::shared_ptr<GsModel> m_graphics;                                        ///< 文档的图形模型
    std::function<void(DmBlock*)> m_onChosen;                                   ///< 点了一个块之后的回调
    QWidget*            m_pBackWidget;                                          ///< 背景控件
};

#endif
