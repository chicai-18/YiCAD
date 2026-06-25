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

/// @file UIDlgLineType.cpp
/// @brief 线型管理对话框

#include "UIDlgLineType.h"
#include "Transaction.h"

// 连续线型的前三个索引（ByLayer, ByBlock, Continuous）不可删除
constexpr int BY_LAYER_INDEX = 0;
constexpr int BY_BLOCK_INDEX = 1;
constexpr int CONTINUOUS_INDEX = 2;

// Table Widget 尺寸常量
constexpr int TABLE_WIDTH_EXPANDED = 411;
constexpr int TABLE_HEIGHT_EXPANDED = 321;
constexpr int TABLE_HEIGHT_COLLAPSED = 190;

UIDlgLineType::UIDlgLineType(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl),
    m_pLineType(nullptr),
    m_pLineTypeTable(nullptr),
    m_pDocument(nullptr),
    ui(new Ui::UIDlgLineType)
{
    ui->setupUi(this);
    setModal(modal);
    m_hideBox = true;
}

UIDlgLineType::~UIDlgLineType()
{
}

void UIDlgLineType::init()
{
    // 设置Table Widget
    ui->LineTypeData->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->LineTypeData->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->LineTypeData->setSelectionBehavior(QTableWidget::SelectRows);
    ui->LineTypeData->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->LineTypeData->setEditTriggers(QAbstractItemView::NoEditTriggers);
    UpdateTableWidget();

    // 设置msg
    ui->nameEdit->setEnabled(false);
    ui->nameEdit->setText(m_pLineTypeTable->getActive()->getLineTypeName());
    ui->descriptEdit->setEnabled(false);
    ui->descriptEdit->setText(m_pLineTypeTable->getActive()->getLineTypeDesp());

    ui->globalScaleEdit->setText("1.000");
    ui->currentScaleEdit->setText("1.000");
}

void UIDlgLineType::UpdateTableWidget()
{
    ui->LineTypeData->setRowCount(m_pLineTypeTable->count());
    int i = 0;
    for (auto lineType : *m_pLineTypeTable)
    {
        ui->LineTypeData->setItem(i, 0, new QTableWidgetItem(lineType->getLineTypeName()));
        ui->LineTypeData->setItem(i, 1, new QTableWidgetItem(lineType->getLineTypeOutWard()));
        ui->LineTypeData->setItem(i, 2, new QTableWidgetItem(lineType->getLineTypeDesp()));
        i++;
    }
}

int UIDlgLineType::getCurTableWidgetItem()
{
    QList<QTableWidgetItem*> m_items = ui->LineTypeData->selectedItems();
    if (m_items.isEmpty())
    {
        QMessageBox::information(NULL, QObject::tr("Warning"), QObject::tr("Please select a linetype!"),
            QMessageBox::Yes, QMessageBox::Yes);
        return -1;
    }
    return ui->LineTypeData->currentRow();
}

void UIDlgLineType::setLineTypeTable(DmLineTypeTable* lineTypeTable, DmDocument* document)
{
    m_pLineTypeTable = lineTypeTable;
    m_pDocument = document;
    init();
}

DmLineTypeTable* UIDlgLineType::getLineTypeTable()
{
    return m_pLineTypeTable;
}

DmLineType* UIDlgLineType::lineTypeAt(int i)
{
    int idx = 0;
    DmLineType* l = nullptr;
    for (auto lineType : *m_pLineTypeTable)
    {
        if (i == idx)
        {
            l = lineType;
            break;
        }
        idx++;
    }
    return l;
}

void UIDlgLineType::on_LoadLine_clicked()
{
    // 加载
    UIDlgLoadLineType dlg(this);
    connect(&dlg, SIGNAL(lineTypeSelected(DmLineType*)), this, SLOT(slotAddLineType(DmLineType*)));
    dlg.setDocument(m_pDocument);
    dlg.init();
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }
}

void UIDlgLineType::slotAddLineType(DmLineType* data)
{
    if (data != nullptr)
    {
        // 判断重复
        bool isDuplicate = false;
        for (auto lineType : *m_pLineTypeTable)
        {
            if (lineType->getLineTypeName() == data->getLineTypeName())
            {
                QMessageBox::information(NULL, QObject::tr("Add Linetype"), QObject::tr("This linetype has already been inserted!"),
                                         QMessageBox::Yes, QMessageBox::Yes);
                isDuplicate = true;
                break;
            }
        }

        if (!isDuplicate)
        {
            Transaction t(tr("Add line type").toStdString(), m_pDocument);
            t.start();
            m_pLineTypeTable->add(data);
            t.commit();
            UpdateTableWidget();
        }
    }
}

void UIDlgLineType::on_LineTypeData_clicked()
{
    // 单击响应事件
    // 设置为当前
    int row = getCurTableWidgetItem();
    if (row == -1)
    {
        return;
    }
    ui->nameEdit->setText(lineTypeAt(row)->getLineTypeName());
    ui->descriptEdit->setText(lineTypeAt(row)->getLineTypeDesp());
}

void UIDlgLineType::on_DeleteLine_clicked()
{
    // 删除
    // ByLayer, ByBlock, continuous can not be deleted
    QList<QTableWidgetItem*> m_items = ui->LineTypeData->selectedItems();
    if (m_items.isEmpty())
    {
        QMessageBox::information(NULL, QObject::tr("Delete LineType"), QObject::tr("Please select a linetype other than ByLayer,ByBlock,Continuous!"),
            QMessageBox::Yes, QMessageBox::Yes);
        return;
    }
    int row = ui->LineTypeData->currentRow();
    if (row == BY_LAYER_INDEX || row == BY_BLOCK_INDEX || row == CONTINUOUS_INDEX)
    {
        QMessageBox::warning(NULL, QObject::tr("Delete LineType"), QObject::tr("Can't delete ByLayer,ByBlock,continus!"), QMessageBox::Yes, QMessageBox::Yes);
        return;
    }

    auto lt = lineTypeAt(row);
    if (lt == nullptr)
    {
        return;
    }
    Transaction t(tr("Delete line type").toStdString(), m_pDocument);
    t.start();
    m_pLineTypeTable->remove(lt);
    t.commit();
    UpdateTableWidget();
}

void UIDlgLineType::on_SetCurLine_clicked()
{
    // 设置为当前
    int row = getCurTableWidgetItem();
    auto lt = lineTypeAt(row);
    ui->nameEdit->setText(lt->getLineTypeName());
    ui->descriptEdit->setText(lt->getLineTypeDesp());

    Transaction t(tr("Set current line type").toStdString(), m_pDocument);
    t.start();
    m_pLineTypeTable->activate(lt);
    t.commit();
}

void UIDlgLineType::on_HideLine_clicked()
{
    // 设置Groupbox
    if (m_hideBox)
    {
        m_hideBox = false;
        ui->groupBox->setVisible(m_hideBox);

        ui->LineTypeData->resize(TABLE_WIDTH_EXPANDED, TABLE_HEIGHT_EXPANDED);
    }
    else
    {
        m_hideBox = true;
        ui->groupBox->setVisible(m_hideBox);
        ui->LineTypeData->resize(TABLE_WIDTH_EXPANDED, TABLE_HEIGHT_COLLAPSED);
    }
}

void UIDlgLineType::on_Ok_clicked()
{
    // 更改画布 DmPen的线型
    int row = getCurTableWidgetItem();
    if (row == -1)
    {
        return;
    }

    auto lt = lineTypeAt(row);
    Transaction t(tr("Set current line type").toStdString(), m_pDocument);
    t.start();
    m_pLineTypeTable->activate(lt);
    t.commit();
    accept();
}

void UIDlgLineType::on_Cancel_clicked()
{
    reject();
}
