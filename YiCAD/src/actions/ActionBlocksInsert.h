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


/// @file ActionBlocksInsert.h
/// @brief 块插入动作类头文件

#ifndef ACTIONBLOCKSINSERT_H
#define ACTIONBLOCKSINSERT_H

#include "DmBlockReference.h"
#include "PreviewActionInterface.h"

class DmBlock;
struct DmBlockReferenceData;

/**
 * @brief 处理将块插入到当前图形的用户事件
 */
class ActionBlocksInsert : public PreviewActionInterface
{
    Q_OBJECT
public:
    /**
     * @brief 动作状态枚举
     */
    enum EStatus
    {
        eSetUndefined   = -1, /**< 未定义状态，用于初始化 */
        eSetTargetPoint = 0,  /**< 设置参考点 */
        eSetAngle,            /**< 在命令行设置角度 */
        eSetFactor,           /**< 在命令行设置比例因子 */
        eSetColumns,          /**< 在命令行设置列数 */
        eSetRows,             /**< 在命令行设置行数 */
        eSetColumnSpacing,    /**< 在命令行设置列间距 */
        eSetRowSpacing        /**< 在命令行设置行间距 */
    };

public:
    /**
     * @brief 构造函数
     * @param[in] doc 文档指针
     * @param[in] docView 文档视图指针
     */
    ActionBlocksInsert(DmDocument* doc, GuiDocumentView* docView);

    /**
     * @brief 析构函数
     */
    ~ActionBlocksInsert() override;

    /**
     * @brief 初始化动作
     * @param[in] status 初始状态，默认为0
     */
    void init(int status = 0) override;

    /**
     * @brief 重置动作数据
     */
    void reset();

    /**
     * @brief 触发动作执行
     */
    void trigger() override;

    /**
     * @brief 鼠标移动事件处理
     * @param[in] e 鼠标事件指针
     */
    void mouseMoveEvent(QMouseEvent* e) override;

    /**
     * @brief 鼠标释放事件处理
     * @param[in] e 鼠标事件指针
     */
    void mouseReleaseEvent(QMouseEvent* e) override;

    /**
     * @brief 坐标事件处理
     * @param[in] e 坐标事件指针
     */
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /**
     * @brief 命令事件处理
     * @param[in] e 命令事件指针
     */
    void commandEvent(GuiCommandEvent* e) override;

    /**
     * @brief 获取可用命令列表
     * @return 可用命令字符串列表
     */
    QStringList getAvailableCommands() override;

    /**
     * @brief 判断是否为子动作
     * @return true表示是子动作
     */
    bool isSubAction() override;

    /**
     * @brief 显示选项对话框
     */
    void showOptions() override;

    /**
     * @brief 隐藏选项对话框
     */
    void hideOptions() override;

    /**
     * @brief 更新鼠标按钮提示
     */
    void updateMouseButtonHints() override;

    /**
     * @brief 更新鼠标光标
     */
    void updateMouseCursor() override;

    /**
     * @brief 获取角度值
     * @return 角度值（弧度）
     */
    double getAngle() const;

    /**
     * @brief 设置角度值
     * @param[in] a 角度值（弧度）
     */
    void setAngle(double a);

    /**
     * @brief 获取比例因子
     * @return 比例因子
     */
    double getFactor() const;

    /**
     * @brief 设置比例因子
     * @param[in] f 比例因子
     */
    void setFactor(double f);

    /**
     * @brief 获取列数
     * @return 列数
     */
    int getColumns() const;

    /**
     * @brief 设置列数
     * @param[in] c 列数
     */
    void setColumns(int c);

    /**
     * @brief 获取行数
     * @return 行数
     */
    int getRows() const;

    /**
     * @brief 设置行数
     * @param[in] r 行数
     */
    void setRows(int r);

    /**
     * @brief 获取列间距
     * @return 列间距
     */
    double getColumnSpacing() const;

    /**
     * @brief 设置列间距
     * @param[in] cs 列间距
     */
    void setColumnSpacing(double cs);

    /**
     * @brief 获取行间距
     * @return 行间距
     */
    double getRowSpacing() const;

    /**
     * @brief 设置行间距
     * @param[in] rs 行间距
     */
    void setRowSpacing(double rs);

protected:
    DmBlock* block;                                           /**< 块指针 */
    std::unique_ptr<DmBlockReferenceData> data;               /**< 块参照数据 */

    /** 进入选项前的最后状态 */
    EStatus lastStatus;
};

#endif
