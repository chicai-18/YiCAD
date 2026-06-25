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

/// @file UIDlgLoadLineType.cpp
/// @brief 从线型文件加载线型的对话框

#include "UIDlgLoadLineType.h"

#include "Debug.h"
#include "DmDocument.h"

UIDlgLoadLineType::UIDlgLoadLineType(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);
}

UIDlgLoadLineType::~UIDlgLoadLineType()
{
    clearLineTypes();
}

void UIDlgLoadLineType::init()
{
    if (!m_doc)
    {
        return;
    }

    // 获得单位
    int measure = m_doc->getVariableInt("MEASUREMENT", 0);
    QString fileName;
    if (measure)
    {
        fileName = LineType::LineTypeIsoFile;
    }
    else
    {
        fileName = LineType::LineTypeFile;
    }

    // 根据单位获得lin文件
    QStringList filePaths = DMSYSTEM->getLineTypesList();
    QString foundPath;
    for (auto file : filePaths)
    {
        if (file.contains(fileName))
        {
            foundPath = file;
            break;
        }
    }
    if (foundPath.isEmpty())
    {
        return;
    }

    // 读取lin文件
    leFileName->setText(fileName);
    readFile(foundPath);
    tblLineTypeData->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    tblLineTypeData->setSelectionMode(QAbstractItemView::SingleSelection);
    tblLineTypeData->setSelectionBehavior(QTableWidget::SelectRows);
    tblLineTypeData->setRowCount((int)m_lineTypes.size());

    for (int i = 0; i < (int)m_lineTypes.size(); i++)
    {
        int col = 0;
        tblLineTypeData->setItem(i, col++, new QTableWidgetItem(m_lineTypes.at(i)->getLineTypeName()));
        tblLineTypeData->setItem(i, col++, new QTableWidgetItem(m_lineTypes.at(i)->getLineTypeDesp()));
    }
}

DmLineType* UIDlgLoadLineType::selectTableWidget()
{
    auto items = tblLineTypeData->selectedItems();
    if (items.isEmpty())
    {
        return nullptr;
    }
    int row = tblLineTypeData->currentRow();
    DmLineType* m_lineType = new DmLineType();
    m_lineType->setLineTypeName(m_lineTypes.at(row)->getLineTypeName());
    m_lineType->setLineTypeDesp(m_lineTypes.at(row)->getLineTypeDesp());
    m_lineType->setLineTypeOutWard(m_lineTypes.at(row)->getLineTypeOutWard());
    m_lineType->setLineTypeData(m_lineTypes.at(row)->getLineTypeData());

    return m_lineType;
}

void UIDlgLoadLineType::readFile(const QString& path)
{
    constexpr int LINETYPE_NAME_OFFSET = 1; // 去掉行首的 '*' 字符
    constexpr int LINETYPE_DATA_OFFSET = 2; // 删除前两位 "A,"

    QFile file(path);
    if (!file.exists())
    {
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    clearLineTypes();
    while (file.atEnd() == false)
    {
        QString fileline(file.readLine());
        if (fileline.startsWith("*", Qt::CaseSensitive))
        {
            DmLineType* lineType = new DmLineType();
            // LINETYPE
            QStringList strList = fileline.split(",");
            QString m_Lname = strList[0];
            lineType->setLineTypeName(m_Lname.remove(0, LINETYPE_NAME_OFFSET));
            QString m_LDescription = strList[1];
            lineType->setLineTypeDesp(m_LDescription.trimmed());
            QString m_LOutward = m_LDescription.replace(QRegExp("[a-zA-Z0-9()]"), "");
            lineType->setLineTypeOutWard(m_LOutward.trimmed());

            // LINETYPEDATA
            QString filelineData(file.readLine());
            if (filelineData.startsWith("A", Qt::CaseSensitive))
            {
                filelineData.remove(0, LINETYPE_DATA_OFFSET); // 删除前两位

                lineType->setLineTypeData(filelineData);
            }
            else
            {
                delete lineType;
                continue;
            }
            m_lineTypes.emplace_back(lineType);
        }
    }
    file.close();
}

void UIDlgLoadLineType::clearLineTypes()
{
    for (auto l : m_lineTypes)
    {
        delete l;
    }
    m_lineTypes.clear();
}

void UIDlgLoadLineType::on_file_clicked()
{
    // 添加打开文件dialog
    // 路径问题
    QStringList filePath = DMSYSTEM->getLineTypesList();
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("open file"),
                                                    filePath[0],
                                                    tr("LineType(*.lin);;All files(*.*)"));
    if (!filename.isEmpty())
    {
        // 设置文件名称
        leFileName->setText(filename);

        // 设置内容
        readFile(filename);
        tblLineTypeData->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        tblLineTypeData->setRowCount((int)m_lineTypes.size());
        for (int i = 0; i < (int)m_lineTypes.size(); i++)
        {
            int col = 0;
            tblLineTypeData->setItem(i, col++, new QTableWidgetItem(m_lineTypes.at(i)->getLineTypeName()));
            tblLineTypeData->setItem(i, col++, new QTableWidgetItem(m_lineTypes.at(i)->getLineTypeDesp()));
        }
    }
}

void UIDlgLoadLineType::on_Ok_clicked()
{
    DmLineType* tmp = selectTableWidget();
    emit lineTypeSelected(tmp);
    accept();
}

void UIDlgLoadLineType::on_Cancel_clicked()
{
    reject();
}
