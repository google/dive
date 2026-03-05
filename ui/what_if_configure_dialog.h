/*
 Copyright 2026 Google LLC
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#pragma once

#include "device_dialog.h"
#include "dive/ui/forward.h"

// Event filter to keep a QComboBox popup open when checking/unchecking items.
class MultiCheckComboBoxEventFilter : public QObject
{
    Q_OBJECT

 public:
    explicit MultiCheckComboBoxEventFilter(QComboBox* parent = nullptr);

    MultiCheckComboBoxEventFilter(const MultiCheckComboBoxEventFilter&) = delete;
    MultiCheckComboBoxEventFilter& operator=(const MultiCheckComboBoxEventFilter&) = delete;

 protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

 private:
    QComboBox* combo_box;
};

struct DrawCallFilterSpinner
{
    QLabel* label = nullptr;
    QSpinBox* spin_box = nullptr;
};

class WhatIfConfigureDialog : public DeviceDialog
{
    Q_OBJECT

 public:
    WhatIfConfigureDialog(QWidget* parent = nullptr);
    ~WhatIfConfigureDialog();

    // -- Layout Creation --
    QVBoxLayout* CreateHeaderLayout();
    QGridLayout* CreateSettingsLayout();
    QWidget* CreateSpecificSettingsContainer();
    QWidget* SetupDrawCallFiltersContainer();
    QWidget* SetupRenderPassFiltersContainer();
    QWidget* SetupFlagContainer();
    QHBoxLayout* CreateButtonLayout();
    void SetupConnections();

 protected:
    void closeEvent(QCloseEvent* event) override;

 private slots:
    void ResetDialog();
    void ShowMessage(const QString& message) override;
    void OnAddModificationClicked();
    void OnFlagModelChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                            const QVector<int>& roles);
    void OnWhatIfModificationCommandChanged(int index);
    void OnWhatIfModificationTypeChanged(int index);
    void OnUpdateAddModificationButtonState();

 signals:
    void AddModification(const QString& modification_type_short_name);

 private:
    void HideSpecificSettings();
    void ResetDrawCallFilters();
    void CreateDrawCallFilterSpinner(DrawCallFilterSpinner& filter, const QString& label_text,
                                     int min, int max);
    void HideDrawCallFilterSpinner(DrawCallFilterSpinner& filter);

    // --- What-If Type Section ---
    QComboBox* m_type_box = nullptr;

    // --- Command Selection ---
    QComboBox* m_command_box = nullptr;
    QStandardItemModel* m_command_model = nullptr;

    // --- Specific Settings Container ---
    QWidget* m_specific_settings_container = nullptr;

    // --- Filter Section ---
    QLabel* m_filter_label = nullptr;

    // --- Draw Call Filters ---
    QWidget* m_draw_call_filters_container = nullptr;
    DrawCallFilterSpinner m_index_count_filter;
    DrawCallFilterSpinner m_vertex_count_filter;
    DrawCallFilterSpinner m_instance_count_filter;
    DrawCallFilterSpinner m_draw_count_filter;

    QComboBox* m_draw_call_pso_property_filter_box = nullptr;

    QComboBox* m_draw_call_render_pass_filter_box = nullptr;

    // --- Render Pass Filters ---
    QWidget* m_render_pass_filters_container = nullptr;
    QComboBox* m_render_pass_command_buffer_filter_box = nullptr;
    QComboBox* m_render_pass_render_pass_type_filter_box = nullptr;

    // --- Flag Section ---
    QWidget* m_flag_container = nullptr;
    QComboBox* m_flag_box = nullptr;
    QStandardItemModel* m_flag_model = nullptr;

    // --- Modification Warning ---
    QLabel* m_modification_warning_label = nullptr;

    // --- Button Section ---
    QPushButton* m_add_modification_button = nullptr;
};
