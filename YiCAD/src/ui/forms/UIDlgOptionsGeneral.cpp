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

/// @file UIDlgOptionsGeneral.cpp
/// @brief 系统设置窗体实现

#include "UIDlgOptionsGeneral.h"

#include <QPushButton>
#include <QMessageBox>
#include <QColorDialog>
#include <QLineEdit>
#include "DmSystem.h"
#include "DmSettings.h"
#include "DmUnits.h"
#include "Commands.h"
#include "UIFileDialog.h"
#include "UICommandWidget.h"
#include "Debug.h"
#include "ApplicationWindow.h"
#include "UIDlgCmdsSetting.h"
#include "DmDocument.h"

int UIDlgOptionsGeneral::current_tab = 0;

UIDlgOptionsGeneral::UIDlgOptionsGeneral(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    gbTheme->hide(); // 屏蔽主题设置

    tabWidget->setCurrentIndex(current_tab);

    this->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    this->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    init();
}

UIDlgOptionsGeneral::~UIDlgOptionsGeneral()
{
    destroy();
}

void UIDlgOptionsGeneral::languageChange()
{
    retranslateUi(this);
}

void UIDlgOptionsGeneral::init()
{
    // 颜色设置
    DMSETTINGS->beginGroup("Colors");
    initComboBox(cbBackgroundColor, DMSETTINGS->readEntry("/background", Colors::BACKGROUND));
    initComboBox(cbGridColor, DMSETTINGS->readEntry("/grid", Colors::GRID));
    initComboBox(cbMetaGridColor, DMSETTINGS->readEntry("/meta_grid", Colors::META_GRID));
    initComboBox(cbSelectedColor, DMSETTINGS->readEntry("/select", Colors::SELECT));
    initComboBox(cbHighlightedColor, DMSETTINGS->readEntry("/highlight", Colors::HIGHLIGHT));
    DMSETTINGS->endGroup();

    // 默认路径设置
    // 路径暂时都屏蔽，后续shx、pat等格式支持之后再启用
    DMSETTINGS->beginGroup("/Paths");
    //lePathFonts->setText(DMSETTINGS->readEntry("/Fonts", ""));
    //lePathLibrary->setText(DMSETTINGS->readEntry("/Library", "").trimmed());
    //leTemplate->setText(DMSETTINGS->readEntry("/Template", "").trimmed());
    //variablefile_field->setText(DMSETTINGS->readEntry("/VariableFile", "").trimmed());
    DMSETTINGS->endGroup();

    // units:
    for (int i = DM::None; i < DM::LastUnit; i++)
    {
        if (i != static_cast<int>(DM::None))
        {
            cbUnit->addItem(DmUnits::unitToString(static_cast<DM::Unit>(i)));
        }
    }
    cbUnit->insertItem(0, DmUnits::unitToString(DM::None));

    const QString DEF_UNIT = "Millimeter";

    DMSETTINGS->beginGroup("/Defaults");
    cbUnit->setCurrentIndex(cbUnit->findText(QObject::tr(DMSETTINGS->readEntry("/Unit", DEF_UNIT).toUtf8().data())));

    // Auto save timer
    const int DEFAULT_AUTO_SAVE_TIME = 5;
    cbAutoSaveTime->setValue(DMSETTINGS->readNumEntry("/AutoSaveTime", DEFAULT_AUTO_SAVE_TIME));
    cbAutoBackup->setChecked(DMSETTINGS->readNumEntry("/AutoBackupDocument", 1) == 1);
    cbInvertZoomDirection->setChecked(DMSETTINGS->readNumEntry("/InvertZoomDirection", 0));

    choTheme->setCurrentIndex(0);

    // 命令设置
    btnModifyKeyboard->setIcon(QIcon(":/ribbon/forms_dlg/modify.svg"));
    btnModifyKeyboard->setFixedWidth(16);
    btnModifyKeyboard->setFixedHeight(16);
    btnModifyKeyboard->setToolTip(tr("Customize command"));
    connect(btnModifyKeyboard, SIGNAL(clicked(bool)), this, SLOT(slotKeyboardModifyClicked()));

    QString defKeyboard = DMSETTINGS->readEntry("/DefaultKeyboard", "");
    QStringList groups = COMMANDS->getGroups();
    if (groups.size() > 0)
    {
        cbKeyboard->addItems(groups);
        int idx = groups.indexOf(defKeyboard);
        if (idx >= 0)
        {
            cbKeyboard->setCurrentIndex(idx);
        }
        else
        {
            cbKeyboard->setCurrentIndex(0);
        }
    }

    DMSETTINGS->endGroup();

    DMSETTINGS->beginGroup("/Startup");
    cbSplash->setChecked(DMSETTINGS->readNumEntry("/ShowSplash", 1) == 1);
    maximize_checkbox->setChecked(DMSETTINGS->readNumEntry("/Maximize", 0) == 1);
    DMSETTINGS->endGroup();

    restartNeeded = false;
}

void UIDlgOptionsGeneral::initComboBox(QComboBox* cb, const QString& text)
{
    int idx = cb->findText(text);
    if (idx < 0)
    {
        idx = 0;
        cb->insertItem(idx, text);
    }
    cb->setCurrentIndex(idx);
}

void UIDlgOptionsGeneral::setRestartNeeded()
{
    restartNeeded = true;
}

void UIDlgOptionsGeneral::ok()
{
    const int theme_index = 0;

    if (DmSettings::save_is_allowed)
    {
        DMSETTINGS->beginGroup("Colors");
        DMSETTINGS->writeEntry("/background", cbBackgroundColor->currentText());
        DMSETTINGS->writeEntry("/grid", cbGridColor->currentText());
        DMSETTINGS->writeEntry("/meta_grid", cbMetaGridColor->currentText());
        DMSETTINGS->writeEntry("/select", cbSelectedColor->currentText());
        DMSETTINGS->writeEntry("/highlight", cbHighlightedColor->currentText());
        DMSETTINGS->endGroup();

        // 路径暂时都屏蔽，后续shx、pat等格式支持之后再启用
        DMSETTINGS->beginGroup("/Paths");
        //DMSETTINGS->writeEntry("/Fonts", lePathFonts->text());
        //DMSETTINGS->writeEntry("/Library", lePathLibrary->text());
        //DMSETTINGS->writeEntry("/Template", leTemplate->text());
        //DMSETTINGS->writeEntry("/VariableFile", variablefile_field->text());
        DMSETTINGS->endGroup();

        // 默认设置
        DMSETTINGS->beginGroup("/Defaults");
        DMSETTINGS->writeEntry("/Unit", DmUnits::unitToString(DmUnits::stringToUnit(cbUnit->currentText()), false/*untr.*/));
        DMSETTINGS->writeEntry("/Theme", theme_index);

        int saveMin = cbAutoSaveTime->value();
        bool isAutoSave = cbAutoBackup->isChecked();
        DMSETTINGS->writeEntry("/AutoSaveTime", saveMin);
        DMSETTINGS->writeEntry("/AutoBackupDocument", isAutoSave ? 1 : 0);
        DMSETTINGS->writeEntry("/InvertZoomDirection", cbInvertZoomDirection->isChecked() ? 1 : 0);
        DMSETTINGS->endGroup();

        // 设置自动保存文件
        for (DmDocument* doc : ApplicationWindow::getAppWindow()->getDocuments())
        {
            doc->enableAutoSave(isAutoSave, saveMin);
        }

        // 设置快捷键
        QString currentKey = cbKeyboard->currentText();
        DMSETTINGS->writeEntry("/DefaultKeyboard", currentKey);
        COMMANDS->load();

        UICommandWidget* cmdWidget = ApplicationWindow::getAppWindow()->getCmdWidget();
        QStringList strs;
        for (auto kv : COMMANDS->getActionCommands())
        {
            strs.append(kv.first);
        }
        cmdWidget->setCompleterStrings(strs);

        DMSETTINGS->beginGroup("/Startup");
        DMSETTINGS->writeEntry("/ShowSplash", cbSplash->isChecked() ? 1 : 0);
        DMSETTINGS->writeEntry("/Maximize", maximize_checkbox->isChecked() ? 1 : 0);
        DMSETTINGS->endGroup();
    }
    else
    {
        // TODO: DmSettings::save_is_allowed 为 false 时是否需要提示用户？
    }

    if (restartNeeded)
    {
        QMessageBox::warning(this, tr("Preferences"), tr("Please restart the application to apply all changes."),
                             QMessageBox::Ok, Qt::NoButton);
    }

    accept();
}

void UIDlgOptionsGeneral::on_tabWidget_currentChanged(int index)
{
    current_tab = index;
}

void UIDlgOptionsGeneral::set_color(QComboBox* combo, QColor custom)
{
    QColor current;
    current.setNamedColor(combo->lineEdit()->text());

    QColorDialog dlg;
    dlg.setCustomColor(0, custom.rgb());

    QColor color = dlg.getColor(current, this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
        combo->lineEdit()->setText(color.name());
    }
    else
    {
        // 用户取消颜色选择，不做任何操作
    }
}

void UIDlgOptionsGeneral::on_pb_background_clicked()
{
    set_color(cbBackgroundColor, QColor(Colors::BACKGROUND));
}

void UIDlgOptionsGeneral::on_pb_grid_clicked()
{
    set_color(cbGridColor, QColor(Colors::GRID));
}

void UIDlgOptionsGeneral::on_pb_meta_clicked()
{
    set_color(cbMetaGridColor, QColor(Colors::META_GRID));
}

void UIDlgOptionsGeneral::on_pb_selected_clicked()
{
    set_color(cbSelectedColor, QColor(Colors::SELECT));
}

void UIDlgOptionsGeneral::on_pb_highlighted_clicked()
{
    set_color(cbHighlightedColor, QColor(Colors::HIGHLIGHT));
}

void UIDlgOptionsGeneral::slotKeyboardModifyClicked()
{
    UIDlgCmdsSetting dlg(this);
    QString currentKey = cbKeyboard->currentText();
    dlg.setGroup(currentKey);
    dlg.setWindowTitle(tr("Modify command table:%1").arg(currentKey));
    dlg.exec();
}

void UIDlgOptionsGeneral::on_pb_clear_all_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Clear settings"),
                                  tr("This will also include custom menus. Continue?"),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        DMSETTINGS->clear_all();
        QMessageBox::information(this, "info", "You must restart yunmoshuCAD to see the changes.");
    }
    else
    {
        // 用户取消清除操作
    }
}

void UIDlgOptionsGeneral::on_pb_clear_geometry_clicked()
{
    DMSETTINGS->clear_geometry();
    QMessageBox::information(this, "info", "You must restart yunmoshuCAD to see the changes.");
}
