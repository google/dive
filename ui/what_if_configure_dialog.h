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

#include "device_dialog.h"
#include "dive/ui/forward.h"

#pragma once

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

class WhatIfConfigureDialog : public DeviceDialog
{
    Q_OBJECT

 public:
    WhatIfConfigureDialog(ApplicationController& controller, QWidget* parent = 0);
    ~WhatIfConfigureDialog();

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
    void HideAllFields();
    void ResetDrawCallFilters();
    void ShowDrawCallFields();
    void ShowImageCreationFields();
    void ShowRenderPassFields();
    void ShowSamplerCreationFields();
    void ShowTimestampFields();

    ApplicationController& m_controller;

    const QString kAdd_Modification = "&Add Modification";
    const QString kDismiss = "&Dismiss";

    // --- Header Section ---
    QHBoxLayout* m_what_if_title_layout;
    QLabel* m_what_if_title_label;

    // --- What-If Type Section ---
    QHBoxLayout* m_what_if_type_layout;
    QComboBox* m_what_if_type_box;
    QStandardItemModel* m_what_if_type_model;

    // Command Selection
    QLabel* m_what_if_command_label;
    QComboBox* m_what_if_command_box;
    QStandardItemModel* m_what_if_command_model;

    // --- Filter Section ---
    QLabel* m_what_if_filter_label;

    // --- Draw Call Filters ---
    QWidget* m_what_if_draw_call_filters_container;
    QLabel* m_what_if_draw_call_index_count_filter_label;
    QSpinBox* m_what_if_draw_call_index_count_filter_box;

    QLabel* m_what_if_draw_call_vertex_count_filter_label;
    QSpinBox* m_what_if_draw_call_vertex_count_filter_box;

    QLabel* m_what_if_draw_call_instance_count_filter_label;
    QSpinBox* m_what_if_draw_call_instance_count_filter_box;

    QLabel* m_what_if_draw_call_draw_count_filter_label;
    QSpinBox* m_what_if_draw_call_draw_count_filter_box;

    QLabel* m_what_if_draw_call_pso_property_filter_label;
    QComboBox* m_what_if_draw_call_pso_property_filter_box;
    QStandardItemModel* m_what_if_draw_call_pso_property_filter_model;

    QLabel* m_what_if_draw_call_render_pass_filter_label;
    QComboBox* m_what_if_draw_call_render_pass_filter_box;
    QStandardItemModel* m_what_if_draw_call_render_pass_filter_model;

    // --- Render Pass Filters ---
    QWidget* m_what_if_render_pass_filters_container;
    QLabel* m_what_if_render_pass_command_buffer_filter_label;
    QComboBox* m_what_if_render_pass_command_buffer_filter_box;
    QStandardItemModel* m_what_if_render_pass_command_buffer_filter_model;
    QLabel* m_what_if_render_pass_render_pass_type_filter_label;
    QComboBox* m_what_if_render_pass_render_pass_type_filter_box;
    QStandardItemModel* m_what_if_render_pass_render_pass_type_filter_model;

    // --- Flag Section ---
    QWidget* m_what_if_flag_container;
    QLabel* m_what_if_flag_label;
    QComboBox* m_what_if_flag_box;
    QStandardItemModel* m_what_if_flag_model;

    // --- Modification Warning ---
    QHBoxLayout* warning_layout;
    QLabel* m_what_if_modification_warning_label;

    // --- Button Section ---
    QHBoxLayout* m_button_layout;
    QPushButton* m_dismiss_button;
    QPushButton* m_add_modification_button;

    QVBoxLayout* m_main_layout;
};
