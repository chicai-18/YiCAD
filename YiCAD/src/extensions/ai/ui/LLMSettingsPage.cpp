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

/// @file LLMSettingsPage.cpp
/// @brief LLM 配置对话框实现

#include "LLMSettingsPage.h"
#include "LLMSettingsService.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>

LLMSettingsPage::LLMSettingsPage(QWidget* parent)
    : QDialog(parent)
    , m_providerEdit(nullptr)
    , m_baseUrlEdit(nullptr)
    , m_modelEdit(nullptr)
    , m_timeoutSpin(nullptr)
    , m_temperatureSpin(nullptr)
    , m_apiKeyEdit(nullptr)
{
    setWindowTitle(tr("LLM Settings"));
    setMinimumWidth(480);

    setupUi();
    loadSettings();

    // 连接保存按钮
    QDialogButtonBox* btnBox = findChild<QDialogButtonBox*>();
    if (btnBox)
    {
        connect(btnBox, &QDialogButtonBox::accepted, this, &LLMSettingsPage::onSave);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
}

void LLMSettingsPage::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);

    // ---- 顶部说明 ----
    auto* headerLabel = new QLabel(tr("Configure AI language model connection parameters.\n"
                                      "AI features are unavailable without an API Key."));
    headerLabel->setWordWrap(true);
    mainLayout->addWidget(headerLabel);

    // ---- 表单区 ----
    auto* formLayout = new QFormLayout();

    m_providerEdit = new QLineEdit(this);
    m_providerEdit->setPlaceholderText(tr("deepseek"));
    formLayout->addRow(tr("Provider:"), m_providerEdit);

    m_baseUrlEdit = new QLineEdit(this);
    m_baseUrlEdit->setPlaceholderText("https://api.deepseek.com/v1");
    formLayout->addRow(tr("Base URL:"), m_baseUrlEdit);

    m_modelEdit = new QLineEdit(this);
    m_modelEdit->setPlaceholderText("deepseek-chat");
    formLayout->addRow(tr("Model:"), m_modelEdit);

    m_timeoutSpin = new QSpinBox(this);
    m_timeoutSpin->setRange(5, 600);
    m_timeoutSpin->setSuffix(tr(" sec"));
    m_timeoutSpin->setToolTip(tr("Request timeout in seconds"));
    formLayout->addRow(tr("Timeout:"), m_timeoutSpin);

    m_temperatureSpin = new QDoubleSpinBox(this);
    m_temperatureSpin->setRange(0.0, 2.0);
    m_temperatureSpin->setSingleStep(0.1);
    m_temperatureSpin->setDecimals(2);
    m_temperatureSpin->setToolTip(tr("Generation randomness: 0=deterministic, 2=maximum randomness"));
    formLayout->addRow(tr("Temperature:"), m_temperatureSpin);

    // API Key 行独立于 QFormLayout，以便右侧放置重设按钮
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("sk-..."));
    m_apiKeyEdit->setToolTip(tr("API Key is stored encrypted and will not be saved in plain text"));

    mainLayout->addLayout(formLayout);

    // ---- API Key 行（含重设按钮）----
    auto* apiKeyRow = new QHBoxLayout();
    apiKeyRow->setContentsMargins(0, 0, 0, 0);
    apiKeyRow->setSpacing(4);

    auto* apiKeyLabel = new QLabel(tr("API Key:"), this);
    // 对齐 QFormLayout 的 label 宽度（近似）
    apiKeyLabel->setMinimumWidth(100);
    apiKeyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    apiKeyRow->addWidget(apiKeyLabel);
    apiKeyRow->addWidget(m_apiKeyEdit, 1);

    auto* resetBtn = new QPushButton(this);
    resetBtn->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    resetBtn->setToolTip(tr("Clear saved API Key"));
    resetBtn->setFixedSize(28, 28);
    resetBtn->setFlat(true);
    connect(resetBtn, &QPushButton::clicked, this, &LLMSettingsPage::slotResetKey);
    apiKeyRow->addWidget(resetBtn);

    mainLayout->addLayout(apiKeyRow);

    // ---- 按钮 ----
    auto* btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btnBox->button(QDialogButtonBox::Ok)->setText(tr("Save"));
    btnBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    mainLayout->addWidget(btnBox);
}

void LLMSettingsPage::loadSettings()
{
    LLMSettingsService* svc = LLMSettingsService::instance();
    if (!svc || !svc->isInitialized())
    {
        return;
    }

    m_providerEdit->setText(svc->provider());
    m_baseUrlEdit->setText(svc->baseUrl());
    m_modelEdit->setText(svc->model());
    m_timeoutSpin->setValue(svc->timeoutSecs());
    m_temperatureSpin->setValue(svc->temperature());

    // API Key 不预填（出于安全，用户需手动输入查看）
    // 如果已有 Key，用占位提示
    if (svc->hasApiKey())
    {
        m_apiKeyEdit->setPlaceholderText(tr("(Set; enter new key to overwrite)"));
    }
}

void LLMSettingsPage::onSave()
{
    LLMSettingsService* svc = LLMSettingsService::instance();
    if (!svc || !svc->isInitialized())
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("LLM settings service is not initialized."));
        return;
    }

    svc->setProvider(m_providerEdit->text());
    svc->setBaseUrl(m_baseUrlEdit->text());
    svc->setModel(m_modelEdit->text());
    svc->setTimeoutSecs(m_timeoutSpin->value());
    svc->setTemperature(m_temperatureSpin->value());

    // 仅当用户输入了新 Key 才更新（避免空输入覆盖已有 Key）
    const QString newKey = m_apiKeyEdit->text().trimmed();
    if (!newKey.isEmpty())
    {
        svc->setApiKey(newKey);
    }

    accept();
}

void LLMSettingsPage::slotResetKey()
{
    LLMSettingsService* svc = LLMSettingsService::instance();
    if (svc && svc->isInitialized())
    {
        svc->setApiKey(QString());  // 空字符串 = 删除密钥
    }
    m_apiKeyEdit->clear();
    m_apiKeyEdit->setPlaceholderText(tr("sk-..."));
}
