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

/// @file LLMSettingsPage.h
/// @brief LLM 配置对话框（最小闭环）
///
/// 提供简单的 QDialog 表单用于编辑 LLM 配置项。
/// 所有读写走 LLMSettingsService。
/// 本对话框不依赖任何 AI 调用逻辑，纯配置层入口。

#ifndef LLMSETTINGSPAGE_H
#define LLMSETTINGSPAGE_H

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;

class LLMSettingsPage : public QDialog
{
    Q_OBJECT

public:
    explicit LLMSettingsPage(QWidget* parent = nullptr);
    ~LLMSettingsPage() override = default;

private slots:
    void onSave();
    void slotResetKey();

private:
    void loadSettings();
    void setupUi();

    QLineEdit*      m_providerEdit;
    QLineEdit*      m_baseUrlEdit;
    QLineEdit*      m_modelEdit;
    QSpinBox*       m_timeoutSpin;
    QDoubleSpinBox* m_temperatureSpin;
    QLineEdit*      m_apiKeyEdit;
};

#endif // LLMSETTINGSPAGE_H
