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

/// @file DocumentFileService.h
/// @brief 文档文件服务：一份文档的存盘策略
///
/// "什么时候存、存到哪、存之前要不要备份、失败了提示什么"由这里决定；文档本身只会按格式读写
/// 文件（DmDocument::readFile、readNativeFile、writeFile），见 doc/LAYER_RESTRUCTURE_PLAN.md
/// 8.4 节。代码从 DmDocument 的 save/saveAs/open 整段搬来，提示经 GuiDialogFactoryInterface
/// 输出，文字与时机不变：
///   - 自动保存：按设置启动定时器，只写原生格式，写到临时目录的副本（<基名>_<路径 MD5 前 8 位>.ycd，
///     未命名文档用宿主给的名字）；每份文档只自动保存一次；
///   - 手动保存：后缀与保存格式不符时拒绝；磁盘上的文件被别的程序改过时拒绝；先写 <文件名>.tmp，
///     成功后把旧文件改名为 <基名>.bak，再把 .tmp 改名为目标文件，删掉自动保存的副本；
///   - 打开：失败时（文件损坏等）询问是否打开 .bak 或自动保存的副本，按原生格式读，读到的副本复制成
///     带时间戳的新文件；都不行时警告。
///
/// 每份文档一个实例，调用方持有它，并保证它先于文档析构：自动保存的定时器是它的成员，随它析构停止。
/// 实例在构造时登记、析构时注销，find() 按文档找到它（选项对话框改自动保存设置时用）。

#ifndef DOCUMENT_FILE_SERVICE_H
#define DOCUMENT_FILE_SERVICE_H

#include <QDateTime>
#include <QString>
#include <QTimer>

class DmDocument;

/// @brief 文档文件服务，见文件说明
class DocumentFileService
{
public:
    /// @brief 接管一份文档的存盘策略，按设置启动自动保存
    /// @param document 文档；它必须比本对象活得久
    explicit DocumentFileService(DmDocument& document);
    ~DocumentFileService();

    DocumentFileService(const DocumentFileService&) = delete;
    DocumentFileService& operator=(const DocumentFileService&) = delete;

    /// @brief 找管理某份文档的服务
    /// @param document 文档
    /// @return 服务；没有时返回空
    static DocumentFileService* find(const DmDocument* document);

    /// @brief 所管理的文档
    DmDocument& document() const { return m_document; }

    /// @brief 保存文件
    /// @param isAutoSave 是否为自动保存
    /// @param force 未修改时也写盘
    /// @return 保存是否成功；未修改且不强制时直接返回 true
    bool save(bool isAutoSave = false, bool force = false);

    /// @brief 另存为；失败时文档的文件名与保存格式不变
    /// @param filename 目标文件名
    /// @param formatType 文件格式类型
    /// @param force 是否强制保存
    /// @return 保存是否成功
    bool saveAs(const QString& filename, const QString& formatType, bool force = false);

    /// @brief 打开文件，读进文档
    /// @param filename 文件名
    /// @return 打开是否成功
    bool open(const QString& filename);

    /// @brief 通过计时器自动保存
    void autoSave();

    /// @brief 是否已自动保存
    /// @return 如果已自动保存则返回true
    bool hasAutoSaved() const;

    /// @brief 启动或关闭自动保存
    /// @param enableAutoSave 是否启用自动保存
    /// @param saveMinute 自动保存间隔（分钟）
    void enableAutoSave(bool enableAutoSave, int saveMinute);

private:
    /// @brief 写文件；没有接得住该格式的过滤器时弹框（原 FileIO 的提示）
    bool writeFile(const QString& file, const QString& formatType);

    /// @brief 按文件找过滤器读入
    bool readFile(const QString& file);

    /// @brief 按原生格式读入备份
    bool readNativeFile(const QString& file);

    DmDocument&     m_document;                ///< 所管理的文档
    QTimer          m_timer;                   ///< 用于自动保存文件的定时器
    bool            m_bHasAutoSaved = false;   ///< 是否已自动保存
    QDateTime       m_modifiedTime;            ///< 上次保存或打开时文件在磁盘上的修改时间
    QString         m_strCurrentFileName;      ///< 保存文件名副本，用于检测外部修改
};

#endif
