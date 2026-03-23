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
#include "absl/status/statusor.h"
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

absl::Status CustomEventFilter(QComboBox* combo_box, QObject* watched, QEvent* event)
{
    if (!combo_box || (watched != combo_box->view()->viewport()))
    {
        return absl::FailedPreconditionError("Not the watched viewport");
    }

    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        {
            QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
            QModelIndex index = combo_box->view()->indexAt(mouse_event->pos());
            if (!index.isValid())
            {
                return absl::NotFoundError("Invalid index");
            }

            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(combo_box->model());
            if (!model)
            {
                return absl::InternalError("Model is not QStandardItemModel");
            }

            QStandardItem* item = model->itemFromIndex(index);
            if (!item || !item->isCheckable() || !(item->flags() & Qt::ItemIsEnabled))
            {
                return absl::FailedPreconditionError("Item not checkable or enabled");
            }

            if (event->type() == QEvent::MouseButtonPress)
            {
                // Manually toggle the check state
                Qt::CheckState current_state = item->checkState();
                item->setCheckState(current_state == Qt::Checked ? Qt::Unchecked : Qt::Checked);
            }

            // Consume the mouse event. This prevents the default
            // selection behavior and stops the event from propagating
            // further, which might close the combo box popup.
            return absl::OkStatus();
        }
        default:
            return absl::UnimplementedError(
                absl::StrCat("Event type ", static_cast<int>(event->type()), " not handled"));
    }
}
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
    absl::Status result = CustomEventFilter(combo_box, watched, event);
    if (result.ok())
    {
        return true;
    }

    if (result.code() != absl::StatusCode::kUnimplemented)
    {
        qDebug() << "CustomEventFilter failed: " << result.ToString().c_str();
    }

    return QObject::eventFilter(watched, event);
}

// =================================================================================================
// WhatIfConfigureDialog
// =================================================================================================
WhatIfConfigureDialog::WhatIfConfigureDialog(QWidget* parent) : DeviceDialog(parent)
{
    qDebug() << "WhatIfConfigureDialog created.";

    setWindowTitle("Dive Runtime What-Ifs");
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    // --- Header Section ---
    main_layout->addLayout(CreateHeaderLayout());
    main_layout->addSpacing(15);

    // --- Settings Section ---
    main_layout->addLayout(CreateSettingsLayout());
    m_specific_settings_container = CreateSpecificSettingsContainer();
    main_layout->addWidget(m_specific_settings_container);
    main_layout->addStretch();

    // --- Button Section ---
    main_layout->addLayout(CreateButtonLayout());
    setLayout(main_layout);

    main_layout->setSizeConstraint(QLayout::SetFixedSize);

    m_command_box->setEnabled(false);

    SetupConnections();
    OnConfigValuesChanged();
}

QVBoxLayout* WhatIfConfigureDialog::CreateHeaderLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();
    // --- Font Definition ---
    QFont title_font = this->font();
    title_font.setBold(true);
    title_font.setPointSize(title_font.pointSize() + 2);

    // --- Header Section ---
    QLabel* title_label = new QLabel(tr("What would happen if..."));
    title_label->setFont(title_font);
    QStandardItemModel* type_model = new QStandardItemModel();
    m_type_box = new QComboBox();

    QStandardItem* type_placeholder = new QStandardItem("Please select a modification type");
    type_placeholder->setFlags(type_placeholder->flags() & ~Qt::ItemIsSelectable);
    type_model->appendRow(type_placeholder);

    for (const auto& ty : Dive::kWhatIfTypeInfos)
    {
        QStandardItem* item = new QStandardItem(ty.ui_name.data());
        type_model->appendRow(item);
    }
    m_type_box->setModel(type_model);

    layout->addWidget(title_label);
    layout->addSpacing(15);
    layout->addWidget(m_type_box);
    return layout;
}

QGridLayout* WhatIfConfigureDialog::CreateSettingsLayout()
{
    // --- Grid Section ---
    QGridLayout* settings_grid = new QGridLayout();
    settings_grid->setColumnStretch(2, 1);
    settings_grid->setColumnStretch(1, 0);

    // --- Command Selector ---
    QLabel* command_label = new QLabel(tr("Command:"));
    m_command_box = new QComboBox();
    m_command_model = new QStandardItemModel();
    m_command_box->setModel(m_command_model);
    settings_grid->addWidget(command_label, 0, 0, Qt::AlignRight);
    settings_grid->addWidget(m_command_box, 0, 1, 1, 2);

    return settings_grid;
}

QWidget* WhatIfConfigureDialog::CreateSpecificSettingsContainer()
{
    QWidget* container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    // --- Filter Section ---
    m_filter_label = new QLabel(tr("Filter By:"));
    layout->addWidget(m_filter_label);

    m_draw_call_filters_container = SetupDrawCallFiltersContainer();
    layout->addWidget(m_draw_call_filters_container);

    m_render_pass_filters_container = SetupRenderPassFiltersContainer();
    layout->addWidget(m_render_pass_filters_container);

    m_flag_container = SetupFlagContainer();
    layout->addWidget(m_flag_container);

    // --- Modification Warning ---
    m_modification_warning_label =
        new QLabel(tr("⚠ When testing this modification, the application will be relaunched."));
    layout->addWidget(m_modification_warning_label);

    return container;
}

QWidget* WhatIfConfigureDialog::SetupDrawCallFiltersContainer()
{
    QWidget* container = new QWidget();
    QGridLayout* draw_call_filter_layout = new QGridLayout(container);
    draw_call_filter_layout->setContentsMargins(0, 0, 0, 0);
    draw_call_filter_layout->setColumnStretch(2, 1);
    draw_call_filter_layout->setColumnStretch(1, 0);

    CreateDrawCallFilterSpinner(m_index_count_filter, tr("Index Count:"), 0, 1000);
    draw_call_filter_layout->addWidget(m_index_count_filter.label, 0, 1, Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_index_count_filter.spin_box, 0, 2);

    CreateDrawCallFilterSpinner(m_vertex_count_filter, tr("Vertex Count:"), 0, 1000);
    draw_call_filter_layout->addWidget(m_vertex_count_filter.label, 0, 1, Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_vertex_count_filter.spin_box, 0, 2);

    CreateDrawCallFilterSpinner(m_instance_count_filter, tr("Instance Count:"), 0, 1000);
    draw_call_filter_layout->addWidget(m_instance_count_filter.label, 1, 1, Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_instance_count_filter.spin_box, 1, 2);

    CreateDrawCallFilterSpinner(m_draw_count_filter, tr("Draw Count:"), 0, 1000);
    draw_call_filter_layout->addWidget(m_draw_count_filter.label, 1, 1, Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_draw_count_filter.spin_box, 1, 2);

    QLabel* draw_call_pso_property_filter_label =
        new QLabel(tr("Pipeline State Object (PSO) Property:"));
    m_draw_call_pso_property_filter_box = new QComboBox();
    QStandardItemModel* draw_call_pso_property_filter_model = new QStandardItemModel();
    QStandardItem* draw_call_pso_property_filter_placeholder =
        new QStandardItem("Select a PSO property");
    draw_call_pso_property_filter_placeholder->setFlags(
        draw_call_pso_property_filter_placeholder->flags() & ~Qt::ItemIsSelectable);
    draw_call_pso_property_filter_model->appendRow(draw_call_pso_property_filter_placeholder);
    m_draw_call_pso_property_filter_box->setModel(draw_call_pso_property_filter_model);
    draw_call_filter_layout->addWidget(draw_call_pso_property_filter_label, 2, 1, Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_draw_call_pso_property_filter_box, 2, 2);

    QLabel* draw_call_render_pass_filter_label = new QLabel(tr("Render Pass:"));
    m_draw_call_render_pass_filter_box = new QComboBox();
    QStandardItemModel* draw_call_render_pass_filter_model = new QStandardItemModel();
    QStandardItem* render_pass_render_pass_filter_placeholder =
        new QStandardItem("Select a render pass");
    render_pass_render_pass_filter_placeholder->setFlags(
        render_pass_render_pass_filter_placeholder->flags() & ~Qt::ItemIsSelectable);
    draw_call_render_pass_filter_model->appendRow(render_pass_render_pass_filter_placeholder);
    m_draw_call_render_pass_filter_box->setModel(draw_call_render_pass_filter_model);
    draw_call_filter_layout->addWidget(draw_call_render_pass_filter_label, 3, 1, Qt::AlignRight);
    draw_call_filter_layout->addWidget(m_draw_call_render_pass_filter_box, 3, 2);
    return container;
}

QWidget* WhatIfConfigureDialog::SetupRenderPassFiltersContainer()
{
    QWidget* container = new QWidget();
    QGridLayout* render_pass_filter_layout = new QGridLayout(container);
    render_pass_filter_layout->setContentsMargins(0, 0, 0, 0);
    render_pass_filter_layout->setColumnStretch(2, 1);
    render_pass_filter_layout->setColumnStretch(1, 0);

    QLabel* render_pass_command_buffer_filter_label = new QLabel(tr("Command Buffer:"));
    m_render_pass_command_buffer_filter_box = new QComboBox();
    QStandardItemModel* render_pass_command_buffer_filter_model = new QStandardItemModel();
    QStandardItem* render_pass_command_buffer_filter_placeholder =
        new QStandardItem("Select a command buffer");
    render_pass_command_buffer_filter_placeholder->setFlags(
        render_pass_command_buffer_filter_placeholder->flags() & ~Qt::ItemIsSelectable);
    render_pass_command_buffer_filter_model->appendRow(
        render_pass_command_buffer_filter_placeholder);
    m_render_pass_command_buffer_filter_box->setModel(render_pass_command_buffer_filter_model);
    render_pass_filter_layout->addWidget(render_pass_command_buffer_filter_label, 0, 1,
                                         Qt::AlignRight);
    render_pass_filter_layout->addWidget(m_render_pass_command_buffer_filter_box, 0, 2);

    QLabel* render_pass_render_pass_type_filter_label = new QLabel(tr("Type:"));
    m_render_pass_render_pass_type_filter_box = new QComboBox();
    QStandardItemModel* render_pass_render_pass_type_filter_model = new QStandardItemModel();
    QStandardItem* render_pass_type_filter_placeholder =
        new QStandardItem("Select render pass type");
    render_pass_type_filter_placeholder->setFlags(render_pass_type_filter_placeholder->flags() &
                                                  ~Qt::ItemIsSelectable);
    render_pass_render_pass_type_filter_model->appendRow(render_pass_type_filter_placeholder);
    for (int i = 0; i < kNumRenderPassTypes; i++)
    {
        QStandardItem* item = new QStandardItem(kRenderPassTypeStrings[i].data());
        render_pass_render_pass_type_filter_model->appendRow(item);
    }
    m_render_pass_render_pass_type_filter_box->setModel(render_pass_render_pass_type_filter_model);
    render_pass_filter_layout->addWidget(render_pass_render_pass_type_filter_label, 1, 1,
                                         Qt::AlignRight);
    render_pass_filter_layout->addWidget(m_render_pass_render_pass_type_filter_box, 1, 2);
    return container;
}

QWidget* WhatIfConfigureDialog::SetupFlagContainer()
{
    QWidget* container = new QWidget();
    QHBoxLayout* flag_layout = new QHBoxLayout(container);
    flag_layout->setContentsMargins(0, 0, 0, 0);

    QLabel* flag_label = new QLabel(tr("Flag(s):"));
    m_flag_box = new QComboBox();
    MultiCheckComboBoxEventFilter* filter = new MultiCheckComboBoxEventFilter(m_flag_box);
    m_flag_box->view()->viewport()->installEventFilter(filter);
    m_flag_model = new QStandardItemModel();

    QStandardItem* flag_place_holder = new QStandardItem("Select image creation flag(s)");
    flag_place_holder->setFlags(flag_place_holder->flags() & ~Qt::ItemIsSelectable);
    m_flag_model->appendRow(flag_place_holder);
    for (int i = 0; i < kNumImageCreationFlags; i++)
    {
        QStandardItem* item = new QStandardItem(kFrameTitleStrings[i].data());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        m_flag_model->appendRow(item);
    }
    m_flag_box->setModel(m_flag_model);

    flag_layout->addWidget(flag_label);
    flag_layout->addWidget(m_flag_box);

    return container;
}

QHBoxLayout* WhatIfConfigureDialog::CreateButtonLayout()
{
    QHBoxLayout* button_layout = new QHBoxLayout();
    m_dismiss_button = new QPushButton(kDismiss.data(), this);
    m_add_modification_button = new QPushButton(kAddModification.data(), this);
    m_add_modification_button->setEnabled(false);
    button_layout->addWidget(m_dismiss_button);
    button_layout->addWidget(m_add_modification_button);

    return button_layout;
}

void WhatIfConfigureDialog::SetupConnections()
{
    QObject::connect(m_type_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnWhatIfModificationTypeChanged);

    QObject::connect(m_command_box, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::OnConfigValuesChanged);

    QObject::connect(m_index_count_filter.spin_box, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_vertex_count_filter.spin_box, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_instance_count_filter.spin_box, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_draw_count_filter.spin_box, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_draw_call_pso_property_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_draw_call_render_pass_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_render_pass_command_buffer_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_render_pass_render_pass_type_filter_box,
                     QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &WhatIfConfigureDialog::UpdateAddModificationButtonState);

    QObject::connect(m_add_modification_button, &QPushButton::clicked, this,
                     &WhatIfConfigureDialog::OnAddModificationClicked);

    QObject::connect(m_dismiss_button, &QPushButton::clicked, this,
                     &WhatIfConfigureDialog::ResetDialog);
    QObject::connect(m_dismiss_button, &QPushButton::clicked, this, &QDialog::reject);

    // Connect flag model dataChanged for check state changes
    QObject::connect(m_flag_model, &QStandardItemModel::dataChanged, this,
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
    m_type_box->setCurrentIndex(-1);
    m_render_pass_command_buffer_filter_box->setCurrentIndex(-1);
    m_render_pass_render_pass_type_filter_box->setCurrentIndex(-1);
    ResetDrawCallFilters();

    // Reset flag checkboxes
    for (int i = 1; i < m_flag_model->rowCount(); ++i)
    {
        QStandardItem* item = m_flag_model->item(i);
        if (item && item->isCheckable())
        {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::ShowMessage(const QString& message)
{
    QMessageBox* message_box = new QMessageBox(this);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setText(message);
    message_box->open();
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnAddModificationClicked()
{
    const int modification_type_index =
        m_type_box->currentIndex() - 1;  // -1 because of the placeholder

    const auto& modification_type_info = Dive::kWhatIfTypeInfos[modification_type_index];

    QStringList filter_strings;
    Dive::WhatIfModification modification;
    modification.type = modification_type_info.type;
    modification.command = m_command_box->currentText();

    switch (modification_type_info.type)
    {
        case Dive::WhatIfType::kDrawCallDisabled:
        {
            Dive::DrawCallFilters draw_call_filters;
            if (m_index_count_filter.spin_box->value() > 0)
            {
                draw_call_filters.index_count = m_index_count_filter.spin_box->value();
                filter_strings << QString("Index Count: %1").arg(draw_call_filters.index_count);
            }
            if (m_vertex_count_filter.spin_box->value() > 0)
            {
                draw_call_filters.vertex_count = m_vertex_count_filter.spin_box->value();
                filter_strings << QString("Vertex Count: %1").arg(draw_call_filters.vertex_count);
            }
            if (m_instance_count_filter.spin_box->value() > 0)
            {
                draw_call_filters.instance_count = m_instance_count_filter.spin_box->value();
                filter_strings
                    << QString("Instance Count: %1").arg(draw_call_filters.instance_count);
            }
            if (m_draw_count_filter.spin_box->value() > 0)
            {
                draw_call_filters.draw_count = m_draw_count_filter.spin_box->value();
                filter_strings << QString("Draw Count: %1").arg(draw_call_filters.draw_count);
            }
            if (m_draw_call_pso_property_filter_box->currentIndex() > 0)
            {
                draw_call_filters.pso_property = m_draw_call_pso_property_filter_box->currentText();
                filter_strings << QString("PSO Property: %1").arg(draw_call_filters.pso_property);
            }
            if (m_draw_call_render_pass_filter_box->currentIndex() > 0)
            {
                draw_call_filters.render_pass = m_draw_call_render_pass_filter_box->currentText();
                filter_strings << QString("Render Pass: %1").arg(draw_call_filters.render_pass);
            }
            modification.draw_call_filters = draw_call_filters;
            break;
        }
        case Dive::WhatIfType::kImageCreationFlagRemoved:
        {
            QStringList flags;
            for (int i = 1; i < m_flag_model->rowCount(); ++i)
            {
                QStandardItem* item = m_flag_model->item(i);
                if (item && item->isCheckable() && item->checkState() == Qt::Checked)
                {
                    flags << item->text();
                }
            }
            if (!flags.isEmpty())
            {
                modification.flags = flags;
                filter_strings << QString("Flags: %1").arg(flags.join(", "));
            }
            break;
        }
        case Dive::WhatIfType::kRenderPassLoadStoreOpOverridden:
        case Dive::WhatIfType::kRenderPassScissorOverridden:
        {
            Dive::RenderPassFilters render_pass_filters;
            if (m_render_pass_command_buffer_filter_box->currentIndex() > 0)
            {
                render_pass_filters.command_buffer =
                    m_render_pass_command_buffer_filter_box->currentText();
                filter_strings
                    << QString("Command Buffer: %1").arg(render_pass_filters.command_buffer);
            }
            if (m_render_pass_render_pass_type_filter_box->currentIndex() > 0)
            {
                render_pass_filters.render_pass_type =
                    m_render_pass_render_pass_type_filter_box->currentText();
                filter_strings << QString("Type: %1").arg(render_pass_filters.render_pass_type);
            }
            modification.render_pass_filters = render_pass_filters;
            break;
        }
        case Dive::WhatIfType::kAnisotropicFilterDisabled:
            break;
        default:
            break;
    }

    QString modification_string =
        QString::fromUtf8(modification_type_info.ui_name_short.data(),
                          static_cast<int>(modification_type_info.ui_name_short.size()));
    if (!filter_strings.isEmpty())
    {
        modification_string += " (" + filter_strings.join(", ") + ")";
    }

    modification.ui_text = modification_string;

    emit AddModification(modification);

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
        UpdateAddModificationButtonState();
    }
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::UpdateAddModificationButtonState()
{
    const int modification_type_index =
        m_type_box->currentIndex() - 1;  // -1 because of the placeholder
    if (modification_type_index < 0)
    {
        m_add_modification_button->setEnabled(false);
        return;
    }

    // Based on the modification type, check if the required fields are filled. At least one filter
    // or flag should be selected.
    const auto& modification_type_info = Dive::kWhatIfTypeInfos[modification_type_index];
    switch (modification_type_info.type)
    {
        case Dive::WhatIfType::kDrawCallDisabled:
        {
            if (m_index_count_filter.spin_box->value() == 0 &&
                m_vertex_count_filter.spin_box->value() == 0 &&
                m_instance_count_filter.spin_box->value() == 0 &&
                m_draw_count_filter.spin_box->value() == 0 &&
                m_draw_call_pso_property_filter_box->currentIndex() == 0 &&
                m_draw_call_render_pass_filter_box->currentIndex() == 0)
            {
                m_add_modification_button->setEnabled(false);
                return;
            }
            break;
        }
        case Dive::WhatIfType::kImageCreationFlagRemoved:
        {
            // Iterate through the model items, skipping the placeholder at index 0
            for (int i = 1; i < m_flag_model->rowCount(); ++i)
            {
                QStandardItem* item = m_flag_model->item(i);
                if (item && item->isCheckable() && item->checkState() == Qt::Checked)
                {
                    m_add_modification_button->setEnabled(true);
                    return;
                }
            }
            m_add_modification_button->setEnabled(false);
            return;
        }
        case Dive::WhatIfType::kRenderPassLoadStoreOpOverridden:
        case Dive::WhatIfType::kRenderPassScissorOverridden:
        {
            if (m_render_pass_command_buffer_filter_box->currentIndex() == 0 &&
                m_render_pass_render_pass_type_filter_box->currentIndex() == 0)
            {
                m_add_modification_button->setEnabled(false);
                return;
            }
            break;
        }
        case Dive::WhatIfType::kAnisotropicFilterDisabled:
            break;
        default:
            qDebug() << absl::StrCat("The case ", modification_type_info.ui_name,
                                     " is unimplemented/unsupported")
                            .c_str();
            break;
    }

    m_add_modification_button->setEnabled(true);
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnConfigValuesChanged()
{
    UpdateVisibility();
    UpdateAddModificationButtonState();
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::UpdateVisibility()
{
    // Based on the modification type, check if the required fields are filled. At least one filter
    // or flag should be selected.
    int type_index = m_type_box->currentIndex();
    HideSpecificSettings();
    if (type_index <= 0)
    {
        this->adjustSize();
        return;
    }

    const auto& modification_type_info = Dive::kWhatIfTypeInfos[type_index - 1];

    switch (modification_type_info.type)
    {
        case Dive::WhatIfType::kDrawCallDisabled:
        {
            m_filter_label->show();
            m_draw_call_filters_container->show();
            HideDrawCallFilters();

            QString command_string = m_command_box->currentText();
            if (command_string.contains("Indirect", Qt::CaseInsensitive))
            {
                SetDrawCallFilterSpinnerVisibility(m_draw_count_filter, /*is_visible=*/true);
            }
            else if (command_string.contains("Indexed", Qt::CaseInsensitive))
            {
                SetDrawCallFilterSpinnerVisibility(m_index_count_filter, /*is_visible=*/true);
                SetDrawCallFilterSpinnerVisibility(m_instance_count_filter, /*is_visible=*/true);
            }
            else
            {
                SetDrawCallFilterSpinnerVisibility(m_vertex_count_filter, /*is_visible=*/true);
                SetDrawCallFilterSpinnerVisibility(m_instance_count_filter, /*is_visible=*/true);
            }
            this->adjustSize();
            break;
        }
        case Dive::WhatIfType::kImageCreationFlagRemoved:
            m_flag_container->show();
            m_modification_warning_label->show();
            break;
        case Dive::WhatIfType::kRenderPassLoadStoreOpOverridden:
            m_modification_warning_label->show();
            [[fallthrough]];
        case Dive::WhatIfType::kRenderPassScissorOverridden:
            m_filter_label->show();
            m_render_pass_filters_container->show();
            break;
        case Dive::WhatIfType::kAnisotropicFilterDisabled:
            m_modification_warning_label->show();
            break;
        default:
            qDebug() << absl::StrCat("The case ", modification_type_info.ui_name,
                                     " is unimplemented/unsupported")
                            .c_str();
            break;
    }
    this->adjustSize();
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::OnWhatIfModificationTypeChanged(int index)
{
    // Update Command Model
    m_command_box->blockSignals(true);
    m_command_box->clear();
    m_command_model->clear();

    if (index > 0)
    {
        const Dive::WhatIfTypeInfo& ty_info =
            Dive::kWhatIfTypeInfos[index - 1];  // -1 because of the placeholder
        for (const auto& cmd : ty_info.supported_commands)
        {
            m_command_model->appendRow(new QStandardItem(cmd.data()));
        }
        m_command_box->setModel(m_command_model);
        m_command_box->setEnabled(true);
        if (m_command_model->rowCount() > 0)
        {
            m_command_box->setCurrentIndex(0);
        }
    }
    else
    {
        m_command_box->setEnabled(false);
    }
    m_command_box->blockSignals(false);

    OnConfigValuesChanged();
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::HideSpecificSettings()
{
    const auto& children = m_specific_settings_container->children();
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
    m_draw_count_filter.spin_box->setValue(0);
    m_index_count_filter.spin_box->setValue(0);
    m_instance_count_filter.spin_box->setValue(0);
    m_vertex_count_filter.spin_box->setValue(0);

    m_draw_call_pso_property_filter_box->setCurrentIndex(0);
    m_draw_call_render_pass_filter_box->setCurrentIndex(0);
}

void WhatIfConfigureDialog::HideDrawCallFilters()
{
    SetDrawCallFilterSpinnerVisibility(m_draw_count_filter, /*is_visible=*/false);
    SetDrawCallFilterSpinnerVisibility(m_index_count_filter, /*is_visible=*/false);
    SetDrawCallFilterSpinnerVisibility(m_instance_count_filter, /*is_visible=*/false);
    SetDrawCallFilterSpinnerVisibility(m_vertex_count_filter, /*is_visible=*/false);
}

void WhatIfConfigureDialog::CreateDrawCallFilterSpinner(DrawCallFilterSpinner& filter,
                                                        const QString& label_text, int min, int max)
{
    filter.label = new QLabel(label_text);
    filter.spin_box = new QSpinBox();
    filter.spin_box->setRange(min, max);
}

void WhatIfConfigureDialog::SetDrawCallFilterSpinnerVisibility(DrawCallFilterSpinner& filter,
                                                               bool is_visible)
{
    if (filter.label) filter.label->setVisible(is_visible);
    if (filter.spin_box)
    {
        filter.spin_box->setVisible(is_visible);
    }
}

//--------------------------------------------------------------------------------------------------
void WhatIfConfigureDialog::closeEvent(QCloseEvent* event)
{
    ResetDialog();
    event->accept();
}
