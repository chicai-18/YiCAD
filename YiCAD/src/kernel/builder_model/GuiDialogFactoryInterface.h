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
/// @brief 对话框工厂接口类，定义创建和显示对话框的纯虚接口

#ifndef GUIDIALOGFACTORYINTERFACE_H
#define GUIDIALOGFACTORYINTERFACE_H

#include <QString>

#include "Datamodel.h"

class ActionInterface;
class AttributesData;
class BevelData;
class DmBlock;
struct DmBlockData;
class DmBlockTable;
class DmAttributeDefinition;
class DmAttribute;
struct DmCircleData;
struct DmDimLinearData;
struct DmDimensionData;
class DmEntity;
class GuiEventHandler;
class DmDocument;
class GuiDocumentView;
class GuiGrid;
class DmHatch;
class DmBlockReference;
class DmLayer;
class DmLayerTable;
class MirrorData;
class CopyData;
class DmMText;
class GuiPainter;
class Rotate2Data;
class RotateData;
class RoundData;
class ScaleData;
class DmSolid;
class DmText;
class DmVector;
class UICommandWidget;
class UIBottomWindow;
class DmTextStyleTable;
class DmLineTypeTable;
class DmViewport;
class DmViewportTable;
class DmTableStyle;
class DmTableStyleTable;
class DmTable;

/// @brief 对话框工厂接口
/// @details 定义了创建和显示各类 CAD 对话框的纯虚接口
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

    /// @brief 请求新建图层对话框
    /// @param layerTable 图层表
    /// @return 新创建的图层或 nullptr
    virtual DmLayer* requestNewLayerDialog(DmLayerTable* layerTable = NULL) = 0;

    /// @brief 请求编辑图层属性对话框
    /// @details 该方法不应实际编辑图层，由调用者负责编辑操作
    /// @param layerTable 图层表
    /// @return 修改后的图层指针，用户取消则返回 nullptr
    virtual DmLayer* requestEditLayerDialog(DmLayerTable* layerTable = NULL) = 0;

    /// @brief 请求新建块对话框
    /// @details 该方法应创建新块但不将其添加到块列表中，由调用者负责添加
    /// @param blockTable 块表
    /// @return 新创建的块数据，用户取消则返回空数据
    virtual DmBlockData requestNewBlockDialog(DmBlockTable* blockTable) = 0;

    /// @brief 对于"属性块"，创建块时提示设置属性定义
    /// @param blkName 块名称
    /// @param attrDefs 属性定义列表
    /// @param[out] attrs 属性列表
    /// @return true 表示用户确认，false 表示取消
    virtual bool requestBlockEditAttributeDialog(const QString& blkName, const std::list<DmAttributeDefinition*>& attrDefs, std::list< DmAttribute*>& attrs) = 0;

    /// @brief 属性定义对话框
    /// @param attrDef 属性定义
    /// @return true 表示用户确认，false 表示取消
    virtual bool requestDefineAttributesDialog(DmAttributeDefinition* attrDef) = 0;

    /// @brief 请求打开图片文件对话框
    /// @details 该方法不应实际打开文件，由调用者负责打开操作
    /// @return 文件名，用户取消则返回空字符串
    virtual QString requestImageOpenDialog() = 0;

    /// @brief 显示操作的选项控件
    /// @param action 需要选项的操作指针
    /// @param on true 打开控件，false 关闭控件
    /// @param update true 从操作获取数据，false 从配置文件获取数据
    virtual void requestOptions(ActionInterface* action, bool on, bool update = false) = 0;

    /// @brief 显示带距离选项的捕捉点控件
    /// @param[out] dist 距离值，控件可直接修改
    /// @param on true 打开控件，false 关闭控件
    virtual void requestSnapDistOptions(double& dist, bool on) = 0;

    /// @brief 显示捕捉中点选项控件
    /// @param[out] middlePoints 中点数量
    /// @param on true 打开控件，false 关闭控件
    virtual void requestSnapMiddleOptions(int& middlePoints, bool on) = 0;

    /// @brief 偏移单个实体选项
    /// @param[out] dist 偏移距离
    /// @param on true 打开控件，false 关闭控件
    /// @param update true 从操作获取数据，false 从配置文件获取数据
    virtual void requestModifySingleOffsetOptions(double& dist, bool on, bool update = false) = 0;

    /// @brief 显示编辑实体属性对话框
    /// @param entity 要编辑的实体
    /// @return true 表示用户确认，false 表示取消
    virtual bool requestModifyEntityDialog(DmEntity* entity) = 0;

    /// @brief 显示文字实体属性编辑对话框
    /// @param text 文字实体
    /// @return true 表示用户确认，false 表示取消
    virtual bool requestTextDialog(DmText* text) = 0;

    /// @brief 显示文字样式窗口
    /// @param textStyleTable 文档原有的文字样式表
    /// @param document 当前文档
    /// @return 用户确定返回 true，取消返回 false
    virtual bool requestTextStyleDialog(DmTextStyleTable* textStyleTable, DmDocument* document) = 0;

    /// @brief 显示填充图案选择对话框
    /// @param hatch 填充实体
    /// @return true 表示用户确认，false 表示取消
    virtual bool requestHatchDialog(DmHatch* hatch) = 0;

    /// @brief 显示应用程序通用选项对话框
    virtual void requestOptionsGeneralDialog() = 0;

    /// @brief 显示绘图选项对话框
    /// @param document 当前文档
    virtual void requestOptionsDrawingDialog(DmDocument& document) = 0;


    /// @brief 显示文件另存为对话框
    /// @param caption 对话框标题
    /// @param dir 默认目录
    /// @param filter 文件过滤器
    /// @param[out] selectedFilter 选中的过滤器
    /// @return 用户选择的文件路径
    virtual QString requestFileSaveAsDialog(const QString& caption = QString(), const QString& dir = QString(), const QString& filter = QString(), QString* selectedFilter = 0) = 0;

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

    /// @brief 更新圆弧切线选项控件
    /// @param radius 半径
    /// @param lockRadius 是否锁定半径
    /// @param angle 角度
    /// @param lockAngle 是否锁定角度
    virtual void updateArcTangentialOptions(const double& radius, const bool& lockRadius, const double& angle, const bool& lockAngle) = 0;

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

    /// @brief 显示线型管理对话框
    /// @param lineTypeTable 线型表
    /// @param document 当前文档
    /// @return true 表示用户确认，false 表示取消
    virtual bool requestLineTypeDialog(DmLineTypeTable* lineTypeTable, DmDocument* document) = 0;
};

#endif
