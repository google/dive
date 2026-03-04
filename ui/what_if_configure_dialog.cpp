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

#include "what_if_configure_dialog.h"

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "application_controller.h"
#include "capture_service/device_mgr.h"
#include "dive/common/what_if_modification_types.h"

namespace
{
constexpr int kNumImageCreationFlags = 2;
constexpr int kNumRenderPassTypes = 4;

constexpr std::string_view kFrameTitleStrings[kNumImageCreationFlags] = {
    "VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM", "VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT"};

constexpr std::string_view kRenderPassTypeStrings[kNumRenderPassTypes] = {
    "Shadow Map Pass", "G-Buffer/Deferred Pass", "Lightening Pass", "UI/Overlay Pass"};

constexpr std::string_view kAddModification = "&Add Modification";
constexpr std::string_view kDismiss = "&Dismiss";
}  // namespace

// =================================================================================================
// MultiCheckComboBoxEventFilter
// =================================================================================================
MultiCheckComboBoxEventFilter::MultiCheckComboBoxEventFilter(QComboBox* parent)
    : QObject(parent), combo_box(parent)
{
}

bool MultiCheckComboBoxEventFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (combo_box && watched == combo_box->view()->viewport())
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
            QModelIndex index = combo_box->view()->indexAt(mouse_event->pos());

            if (index.isValid())
            {
                QStandardItemModel* model = qobject_cast<QStandardItemModel*>(combo_box->model());
                if (model)
                {
                    QStandardItem* item = model->itemFromIndex(index);
                    if (item && item->isCheckable() && (item->flags() & Qt::ItemIsEnabled))
                    {
                        // Manually toggle the check state
                        Qt::CheckState current_state = item->checkState();
                        item->setCheckState(current_state == Qt::Checked ? Qt::Unchecked
                                                                         : Qt::Checked);

                        // Consume the mouse press event. This prevents the default
                        // selection behavior and stops the event from propagating
                        // further, which might close the combo box popup.
                        return true;
                    }
                }
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            // Consume the MouseButtonRelease on the checkable item
            // to be absolutely sure the combo box popup doesn't close.
            QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
            QModelIndex index = combo_box->view()->indexAt(mouse_event->pos());
            if (index.isValid())
            {
                const QStandardItemModel* model =
                    qobject_cast<const QStandardItemModel*>(combo_box->model());
                if (model)
                {
                    QStandardItem* item = model->itemFromIndex(index);
                    if (item && item->isCheckable() && (item->flags() & Qt::ItemIsEnabled))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return QObject::eventFilter(watched, event);
}

// =================================================================================================
// WhatIfConfigureDialog
// =================================================================================================
WhatIfConfigureDialog::WhatIfConfigureDialog(QWidget* parent)
{
    qDebug() << "WhatIfConfigureDialog created.";

    setWindowTitle("Dive Runtime What-Ifs");
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    // --- Header Section ---
    main_layout->addLayout(CreateHeaderLayout());
    main_layout->addSpacing(15);

    // --- Settings Section ---
    main_layout->addLayout(CreateSettingsLayout());
    m_specific_what_if_settings_container = CreateSpecificWhatIfSettingsContainer();
    main_layout->addWidget(m_specific_what_if_settings_container);
    main_layout->addStretch();

    // --- Button Section ---
    main_layout->addLayout(CreateButtonLayout());
    setLayout(main_layout);

    m_specific_what_if_settings_container->hide();
    m_what_if_command_box->setEnabled(false);

    SetupConnections();
}

QVBoxLayout* WhatIfConfigureDialog::CreateHeaderLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();
    // --- Font Definition ---
    QFont title_font = this->font();
    title_font.setBold(true);
    title_font.setPointSize(title_font.pointSize() + 2);

    // --- Header Section ---
    QLabel* what_if_title_label = new QLabel(tr("What would happen if..."));
    what_if_title_label->setFont(title_font);
    QStandardItemModel* what_if_type_model = new QStandardItemModel();
    m_what_if_type_box = new QComboBox();

    QStandardItem* what_if_type_placeholder =
        new QStandardItem("Please select a modification type");
    what_if_type_placeholder->setFlags(what_if_type_placeholder->flags() & ~Qt::ItemIsSelectable);
    what_if_type_model->appendRow(what_if_type_placeholder);

    for (const auto& ty : Dive::kWhatIfTypeInfos)
    {
        QStandardItem* item = new QStandardItem(ty.ui_name.data());
        what_if_type_model->appendRow(item);
    }
    m_what_if_type_box->setModel(what_if_type_model);

    layout->addWidget(what_if_title_label);
    layout->addSpacing(15);
    layout->addWidget(m_what_if_type_box);
    return layout;
}

QGridLayout* WhatIfConfigureDialog::CreateSettingsLayout()
{
    // --- Grid Section ---
    QGridLayout* settings_grid = new QGridLayout();
    settings_grid->setColumnStretch(2, 1);
    settings_grid->setColumnStretch(1, 0);

    // --- Command Selector ---
    QLabel* what_if_command_label = new QLabel(tr("Command:"));
    m_what_if_command_box = new QComboBox();
    m_what_if_command_model = new QStandardItemModel();
    m_what_if_command_box->setModel(m_what_if_command_model);
    settings_grid->addWidget(what_if_command_label, 0, 0, Qt::AlignRight);
    settings_grid->addWidget(m_what_if_command_box, 0, 1, 1, 2);

    return settings_grid;
}

QWidget* WhatIfConfigureDialog::CreateSpecificWhatIfSettingsContainer()
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    // --- Filter Section ---
    m_what_if_filter_label = new QLabel(tr("Filter By:"));
    layout->addWidget(m_what_if_filter_label);

    m_what_if_draw_call_filters_container = SetupDrawCallFiltersContainer();
    layout->addWidget(m_what_if_draw_call_filters_container);

    m_what_if_render_pass_filters_container = SetupRenderPassFiltersContainer();
    layout->addWidget(m_what_if_render_pass_filters_container);

    m_what_if_flag_container = SetupFlagContainer();
    layout->addWidget(m_what_if_flag_container);

    // --- Modification Warning ---
    m_what_if_modification_warning_label =
        new QLabel(tr("⚠ When testing this modification, the application will be relaunched."));
    layout->addWidget(m_what_if_modification_warning_label);

    return container;
}

QWidget* WhatIfConfigureDialog::SetupDrawCallFiltersContainer()
{
    QWidget* container = new QWidget();
    QGridLayout* draw_call_filter_layout = new QGridLayout(container);
    draw_call_filter_layout->setContentsMargins(0, 0, 0, 0);
    draw_call_filter_layout->setColumnStretch(2, 1);
    draw_call_filter_layout->setColumnStretch(1, 0);

    m_what_if_draw_call_index_count_filter_label = new QLabel(tr("Index Count:"));
    m_what_if_draw_call_index_count_filter_box = new QSpinBox();
    m_what_if_draw_call_index_count_filter_box->setRange(0, 1000);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_index_count_filter_label, 0, 1,
                                       Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_index_count_filter_box, 0, 2);

    m_what_if_draw_call_vertex_count_filter_label = new QLabel(tr("Vertex Count:"));
    m_what_if_draw_call_vertex_count_filter_box = new QSpinBox();
    m_what_if_draw_call_vertex_count_filter_box->setRange(0, 1000);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_vertex_count_filter_label, 0, 1,
                                       Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_vertex_count_filter_box, 0, 2);

    m_what_if_draw_call_instance_count_filter_label = new QLabel(tr("Instance Count:"));
    m_what_if_draw_call_instance_count_filter_box = new QSpinBox();
    m_what_if_draw_call_instance_count_filter_box->setRange(0, 1000);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_instance_count_filter_label, 1, 1,
                                       Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_instance_count_filter_box, 1, 2);

    m_what_if_draw_call_draw_count_filter_label = new QLabel(tr("Draw Count:"));
    m_what_if_draw_call_draw_count_filter_box = new QSpinBox();
    m_what_if_draw_call_draw_count_filter_box->setRange(0, 1000);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_draw_count_filter_label, 1, 1,
                                       Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_draw_count_filter_box, 1, 2);

    QLabel* what_if_draw_call_pso_property_filter_label =
        new QLabel(tr("Pipeline State Object (PSO) Property:"));
    m_what_if_draw_call_pso_property_filter_box = new QComboBox();
    QStandardItemModel* what_if_draw_call_pso_property_filter_model = new QStandardItemModel();
    QStandardItem* what_if_draw_call_pso_property_filter_placeholder =
        new QStandardItem("Select a PSO property");
    what_if_draw_call_pso_property_filter_placeholder->setFlags(
        what_if_draw_call_pso_property_filter_placeholder->flags() & ~Qt::ItemIsSelectable);
    what_if_draw_call_pso_property_filter_model->appendRow(
        what_if_draw_call_pso_property_filter_placeholder);
    m_what_if_draw_call_pso_property_filter_box->setModel(
        what_if_draw_call_pso_property_filter_model);
    draw_call_filter_layout->addWidget(what_if_draw_call_pso_property_filter_label, 2, 1,
                                       Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_pso_property_filter_box, 2, 2);

    QLabel* what_if_draw_call_render_pass_filter_label = new QLabel(tr("Render Pass:"));
    m_what_if_draw_call_render_pass_filter_box = new QComboBox();
    QStandardItemModel* what_if_draw_call_render_pass_filter_model = new QStandardItemModel();
    QStandardItem* what_if_render_pass_render_pass_filter_placeholder =
        new QStandardItem("Select a render pass");
    what_if_render_pass_render_pass_filter_placeholder->setFlags(
        what_if_render_pass_render_pass_filter_placeholder->flags() & ~Qt::ItemIsSelectable);
    what_if_draw_call_render_pass_filter_model->appendRow(
        what_if_render_pass_render_pass_filter_placeholder);
    m_what_if_draw_call_render_pass_filter_box->setModel(
        what_if_draw_call_render_pass_filter_model);
    draw_call_filter_layout->addWidget(what_if_draw_call_render_pass_filter_label, 3, 1,
                                       Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_what_if_draw_call_render_pass_filter_box, 3, 2);
    return container;
}

QWidget* WhatIfConfigureDialog::SetupRenderPassFiltersContainer()
{
    QWidget* container = new QWidget();
    QGridLayout* render_pass_filter_layout = new QGridLayout(container);
    render_pass_filter_layout->setContentsMargins(0, 0, 0, 0);
    render_pass_filter_layout->setColumnStretch(2, 1);
    render_pass_filter_layout->setColumnStretch(1, 0);

    QLabel* what_if_render_pass_command_buffer_filter_label = new QLabel(tr("Command Buffer:"));
    m_what_if_render_pass_command_buffer_filter_box = new QComboBox();
    QStandardItemModel* what_if_render_pass_command_buffer_filter_model = new QStandardItemModel();
    QStandardItem* what_if_render_pass_command_buffer_filter_placeholder =
        new QStandardItem("Select a command buffer");
    what_if_render_pass_command_buffer_filter_placeholder->setFlags(
        what_if_render_pass_command_buffer_filter_placeholder->flags() & ~Qt::ItemIsSelectable);
    what_if_render_pass_command_buffer_filter_model->appendRow(
        what_if_render_pass_command_buffer_filter_placeholder);
    m_what_if_render_pass_command_buffer_filter_box->setModel(
        what_if_render_pass_command_buffer_filter_model);
    render_pass_filter_layout->addWidget(what_if_render_pass_command_buffer_filter_label, 0, 1,
                                         Qt::AlignRight);
    render_pass_filter_layout->addWidget(m_what_if_render_pass_command_buffer_filter_box, 0, 2);

    QLabel* what_if_render_pass_render_pass_type_filter_label = new QLabel(tr("Type:"));
    m_what_if_render_pass_render_pass_type_filter_box = new QComboBox();
    QStandardItemModel* what_if_render_pass_render_pass_type_filter_model =
        new QStandardItemModel();
    QStandardItem* what_if_render_pass_type_filter_placeholder =
        new QStandardItem("Select render pass type");
    what_if_render_pass_type_filter_placeholder->setFlags(
        what_if_render_pass_type_filter_placeholder->flags() & ~Qt::ItemIsSelectable);
    what_if_render_pass_render_pass_type_filter_model->appendRow(
        what_if_render_pass_type_filter_placeholder);
    for (int i = 0; i < kNumRenderPassTypes; i++)
    {
        QStandardItem* item = new QStandardItem(kRenderPassTypeStrings[i].data());
        what_if_render_pass_render_pass_type_filter_model->appendRow(item);
    }
    m_what_if_render_pass_render_pass_type_filter_box->setModel(
        what_if_render_pass_render_pass_type_filter_model);
    render_pass_filter_layout->addWidget(what_if_render_pass_render_pass_type_filter_label, 1, 1,
                                         Qt::AlignRight);
    render_pass_filter_layout->addWidget(m_what_if_render_pass_render_pass_type_filter_box, 1, 2);
    return container;
}

QWidget* WhatIfConfigureDialog::SetupFlagContainer()
{
    QWidget* container = new QWidget();
    QHBoxLayout* flag_layout = new QHBoxLayout(container);
    flag_layout->setContentsMargins(0, 0, 0, 0);

    QLabel* what_if_flag_label = new QLabel(tr("Flag(s):"));
    m_what_if_flag_box = new QComboBox();
    MultiCheckComboBoxEventFilter* filter = new MultiCheckComboBoxEventFilter(m_what_if_flag_box);
    m_what_if_flag_box->view()->viewport()->installEventFilter(filter);
    m_what_if_flag_model = new QStandardItemModel();

    QStandardItem* what_if_flag_place_holder = new QStandardItem("Select image creation flag(s)");
    what_if_flag_place_holder->setFlags(what_if_flag_place_holder->flags() & ~Qt::ItemIsSelectable);
    m_what_if_flag_model->appendRow(what_if_flag_place_holder);
    for (int i = 0; i < kNumImageCreationFlags; i++)
    {
        QStandardItem* item = new QStandardItem(kFrameTitleStrings[i].data());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        m_what_if_flag_model->appendRow(item);
    }
    m_what_if_flag_box->setModel(m_what_if_flag_model);

    flag_layout->addWidget(what_if_flag_label);
    flag_layout->addWidget(m_what_if_flag_box);

    container->hide();
    return container;
}

QHBoxLayout* WhatIfConfigureDialog::CreateButtonLayout()
{
    QHBoxLayout* button_layout = new QHBoxLayout();
    QPushButton* dismiss_button = new QPushButton(kDismiss.data(), this);
    m_add_modification_button = new QPushButton(kAddModification.data(), this);
    m_add_modification_button->setEnabled(false);
    button_layout->addWidget(dismiss_button);
    button_layout->addWidget(m_add_modification_button);

    QObject::connect(dismiss_button, &QPushButton::clicked, this,
                     &WhatIfConfigureDialog::ResetDialog);
    QObject::connect(dismiss_button, &QPushButton::clicked, this, &QDialog::reject);
    QObject::connect(m_add_modification_button, &QPushButton::clicked, this,
                     &WhatIfConfigureDialog::OnAddModificationClicked);
    return button_layout;
}

void WhatIfConfigureDialog::SetupConnections()
{
    QObject::connect(m_what_if_type_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnWhatIfModificationTypeChanged);

    QObject::connect(m_what_if_command_box, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &WhatIfConfigureDialog::OnWhatIfModificationCommandChanged);

    QObject::connect(m_what_if_type_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_draw_call_index_count_filter_box,
                     QOverload<int>::of(&QSpinBox::valueChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_draw_call_vertex_count_filter_box,
                     QOverload<int>::of(&QSpinBox::valueChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_draw_call_instance_count_filter_box,
                     QOverload<int>::of(&QSpinBox::valueChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_draw_call_draw_count_filter_box,
                     QOverload<int>::of(&QSpinBox::valueChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_draw_call_pso_property_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_draw_call_render_pass_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_render_pass_command_buffer_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    QObject::connect(m_what_if_render_pass_render_pass_type_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnUpdateAddModificationButtonState);

    // Connect flag model dataChanged for check state changes
    QObject::connect(m_what_if_flag_model, &QStandardItemModel::dataChanged, this,
                     &WhatIfConfigureDialog::OnFlagModelChanged);
}

//--------------------------------------------------------------------------------------------------
WhatIfConfigureDialog::~WhatIfConfigureDialog()
{
    qDebug() << "WhatIfConfigureDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::ResetDialog()
{
    m_what_if_type_box->setCurrentIndex(-1);
    m_specific_what_if_settings_container->hide();
    this->adjustSize();
    m_what_if_command_box->clear();
    m_what_if_command_box->setEnabled(false);
    m_what_if_render_pass_command_buffer_filter_box->setCurrentIndex(-1);
    m_what_if_render_pass_render_pass_type_filter_box->setCurrentIndex(-1);

    // Reset flag checkboxes
    for (int i = 1; i < m_what_if_flag_model->rowCount(); ++i)
    {
        QStandardItem* item = m_what_if_flag_model->item(i);
        if (item && item->isCheckable())
        {
            item->setCheckState(Qt::Unchecked);
        }
    }

    m_add_modification_button->setEnabled(false);
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::ShowMessage(const QString& message)
{
    auto message_box = new QMessageBox(this);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setText(message);
    message_box->open();
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnAddModificationClicked()
{
    const int modification_type_index =
        m_what_if_type_box->currentIndex() - 1;  // -1 because of the placeholder

    const auto& modification_type_info = Dive::kWhatIfTypeInfos[modification_type_index];

    emit AddModification(modification_type_info.ui_name_short.data());

    ShowMessage(QString::fromStdString(absl::StrCat("\"", modification_type_info.ui_name_short,
                                                    "\"", " modification added successfully")));
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnFlagModelChanged(const QModelIndex& topLeft,
                                               const QModelIndex& bottomRight,
                                               const QVector<int>& roles)
{
    if (roles.isEmpty() || roles.contains(Qt::CheckStateRole))
    {
        OnUpdateAddModificationButtonState();
    }
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnUpdateAddModificationButtonState()
{
    const int modification_type_index =
        m_what_if_type_box->currentIndex() - 1;  // -1 because of the placeholder
    if (modification_type_index < 0)
    {
        m_add_modification_button->setEnabled(false);
        return;
    }

    // Based on the modification type, check if the required fields are filled. At least one filter
    // or flag should be selected.
    const auto& modification_type_info = Dive::kWhatIfTypeInfos[modification_type_index];
    if (modification_type_info.type == Dive::WhatIfType::kDrawCallDisabled)
    {
        if (m_what_if_draw_call_index_count_filter_box->value() == 0 &&
            m_what_if_draw_call_vertex_count_filter_box->value() == 0 &&
            m_what_if_draw_call_instance_count_filter_box->value() == 0 &&
            m_what_if_draw_call_draw_count_filter_box->value() == 0 &&
            m_what_if_draw_call_pso_property_filter_box->currentIndex() == 0 &&
            m_what_if_draw_call_render_pass_filter_box->currentIndex() == 0)
        {
            m_add_modification_button->setEnabled(false);
            return;
        }
    }
    else if (modification_type_info.type == Dive::WhatIfType::kImageCreationFlagRemoved)
    {
        if (m_what_if_flag_box->isVisible())
        {
            bool any_flag_checked = false;
            // Iterate through the model items, skipping the placeholder at index 0
            for (int i = 1; i < m_what_if_flag_model->rowCount(); ++i)
            {
                QStandardItem* item = m_what_if_flag_model->item(i);
                if (item && item->isCheckable() && item->checkState() == Qt::Checked)
                {
                    any_flag_checked = true;
                    break;
                }
            }
            if (!any_flag_checked)
            {
                m_add_modification_button->setEnabled(false);
                return;
            }
        }
    }
    else if (modification_type_info.type == Dive::WhatIfType::kRenderPassLoadStoreOpOverridden ||
             modification_type_info.type == Dive::WhatIfType::kRenderPassScissorOverridden)
    {
        if (m_what_if_render_pass_command_buffer_filter_box->currentIndex() == 0 &&
            m_what_if_render_pass_render_pass_type_filter_box->currentIndex() == 0)
        {
            m_add_modification_button->setEnabled(false);
            return;
        }
    }

    m_add_modification_button->setEnabled(true);
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnWhatIfModificationCommandChanged(int index)
{
    const int modification_type_index =
        m_what_if_type_box->currentIndex() - 1;  // -1 because of the placeholder
    if (modification_type_index < 0)
    {
        m_add_modification_button->setEnabled(false);
        return;
    }

    // Based on the modification type, check if the required fields are filled. At least one filter
    // or flag should be selected.
    const auto& modification_type_info = Dive::kWhatIfTypeInfos[modification_type_index];
    if (modification_type_info.type == Dive::WhatIfType::kDrawCallDisabled)
    {
        ResetDrawCallFilters();

        QString command_string = m_what_if_command_box->itemText(index);
        if (command_string.contains("Indirect", Qt::CaseInsensitive))
        {
            m_what_if_draw_call_draw_count_filter_label->show();
            m_what_if_draw_call_draw_count_filter_box->show();
        }
        else if (command_string.contains("Indexed", Qt::CaseInsensitive))
        {
            m_what_if_draw_call_index_count_filter_label->show();
            m_what_if_draw_call_index_count_filter_box->show();

            m_what_if_draw_call_instance_count_filter_label->show();
            m_what_if_draw_call_instance_count_filter_box->show();
        }
        else
        {
            m_what_if_draw_call_vertex_count_filter_label->show();
            m_what_if_draw_call_vertex_count_filter_box->show();

            m_what_if_draw_call_instance_count_filter_label->show();
            m_what_if_draw_call_instance_count_filter_box->show();
        }
        this->adjustSize();
    }
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnWhatIfModificationTypeChanged(int index)
{
    if (index == -1)  // Placeholder selected
    {
        m_specific_what_if_settings_container->hide();
        this->adjustSize();
        m_what_if_command_box->clear();
        m_what_if_command_box->setEnabled(false);
        m_add_modification_button->setEnabled(false);
        return;
    }
    const auto& ty_info = Dive::kWhatIfTypeInfos[index - 1];  // -1 because of the placeholder
    m_what_if_command_box->clear();
    m_what_if_command_model->clear();
    HideSpecificWhatIfSettings();
    m_what_if_command_box->setEnabled(true);

    for (const auto& cmd : ty_info.supported_commands)
    {
        m_what_if_command_model->appendRow(new QStandardItem(cmd.data()));
    }
    m_what_if_command_box->setModel(m_what_if_command_model);

    m_specific_what_if_settings_container->show();
    switch (ty_info.type)
    {
        case Dive::WhatIfType::kDrawCallDisabled:
            m_what_if_filter_label->show();
            m_what_if_draw_call_filters_container->show();
            break;
        case Dive::WhatIfType::kRenderPassLoadStoreOpOverridden:
            m_what_if_modification_warning_label->show();
            [[fallthrough]];
        case Dive::WhatIfType::kRenderPassScissorOverridden:
            m_what_if_filter_label->show();
            m_what_if_render_pass_filters_container->show();
            break;
        case Dive::WhatIfType::kImageCreationFlagRemoved:
            m_what_if_flag_container->show();
            m_what_if_modification_warning_label->show();
            break;
        case Dive::WhatIfType::kAnisotropicFilterDisabled:
            m_what_if_modification_warning_label->show();
            break;
        default:
            m_specific_what_if_settings_container->hide();
            qDebug() << absl::StrCat("The case ", ty_info.ui_name, " is unimplemented/unsupported")
                            .c_str();
            break;
    }
    this->adjustSize();
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::HideSpecificWhatIfSettings()
{
    m_specific_what_if_settings_container->hide();

    const auto& children = m_specific_what_if_settings_container->children();
    for (QObject* child : children)
    {
        if (QWidget* widget = qobject_cast<QWidget*>(child))
        {
            widget->hide();
        }
    }
}

void WhatIfConfigureDialog::ResetDrawCallFilters()
{
    m_what_if_draw_call_draw_count_filter_label->hide();
    m_what_if_draw_call_draw_count_filter_box->hide();
    m_what_if_draw_call_draw_count_filter_box->setValue(0);

    m_what_if_draw_call_index_count_filter_label->hide();
    m_what_if_draw_call_index_count_filter_box->hide();
    m_what_if_draw_call_index_count_filter_box->setValue(0);

    m_what_if_draw_call_instance_count_filter_label->hide();
    m_what_if_draw_call_instance_count_filter_box->hide();
    m_what_if_draw_call_instance_count_filter_box->setValue(0);

    m_what_if_draw_call_vertex_count_filter_label->hide();
    m_what_if_draw_call_vertex_count_filter_box->hide();
    m_what_if_draw_call_vertex_count_filter_box->setValue(0);

    m_what_if_draw_call_pso_property_filter_box->setCurrentIndex(0);
    m_what_if_draw_call_render_pass_filter_box->setCurrentIndex(0);
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::closeEvent(QCloseEvent* event)
{
    event->accept();
    return;
}
