/*
 Copyright 2019 Google LLC

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
#include "most_expensive_events_view.h"

#include <QAction>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextStream>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "dive_core/conversions.h"
#include "dive_core/data_core.h"
#include "gui_constants.h"
#include "hover_help_model.h"
#include "settings.h"

const uint32_t kDurationColumn = 2;
const uint32_t kOccupancyDurationColumn = 3;
const uint32_t kShaderStageColumn = 4;

// =================================================================================================
// EventWidgetItem
// =================================================================================================
class EventWidgetItem : public QTreeWidgetItem
{
public:
    EventWidgetItem(QTreeWidget *view) : QTreeWidgetItem(view) {}
    void SetStageEnabled(Dive::ShaderStage stage, bool enable)
    {
        m_stage_enabled[(uint32_t)stage] = enable;
    }
    bool GetStageEnabled(Dive::ShaderStage stage) { return m_stage_enabled[(uint32_t)stage]; }

private:
    bool operator<(const QTreeWidgetItem &other) const
    {
        int column = treeWidget()->sortColumn();
        if (column == kDurationColumn || column == kOccupancyDurationColumn)
        {
            // Use numeric comparison. Have to remove locale specific punctuation (eg. the commas in
            // 1,000), the suffix (eg. "cycle"), and compare the resulting double
            QLocale c(QLocale::system());

            bool   ok;
            double left = c.toDouble(text(column).remove(QRegExp("\\s*\\D+$")), &ok);
            DIVE_ASSERT(ok);
            double right = c.toDouble(other.text(column).remove(QRegExp("\\s*\\D+$")), &ok);
            DIVE_ASSERT(ok);
            return left < right;
        }
        return text(column) < other.text(column);
    }
    bool m_stage_enabled[Dive::kShaderStageCount] = { false };
};

// =================================================================================================
// MostExpensiveEventsViewDelegate
// =================================================================================================
void MostExpensiveEventsViewDelegate::paint(QPainter                   *painter,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex          &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    painter->save();
    if (index.column() == kShaderStageColumn)
    {
        // Control the order which it shows up in the UI
        Dive::ShaderStage stage_order[Dive::kShaderStageCount] = {
            Dive::ShaderStage::kShaderStageVs,
            Dive::ShaderStage::kShaderStageGs,
            Dive::ShaderStage::kShaderStageHs,
            Dive::ShaderStage::kShaderStagePs,
            Dive::ShaderStage::kShaderStageCs
        };

        uint32_t         offset = 0;
        EventWidgetItem *item_ptr = (EventWidgetItem *)(index.internalPointer());
        for (uint32_t i = 0; i < Dive::kShaderStageCount; ++i)
        {
            Dive::ShaderStage stage = stage_order[i];

            std::string shader_text;
            if (stage == Dive::ShaderStage::kShaderStageVs)
                shader_text = "VS";
            if (stage == Dive::ShaderStage::kShaderStageGs)
                shader_text = "GS";
            if (stage == Dive::ShaderStage::kShaderStageHs)
                shader_text = "HS";
            if (stage == Dive::ShaderStage::kShaderStagePs)
                shader_text = "PS";
            if (stage == Dive::ShaderStage::kShaderStageCs)
                shader_text = "CS";

            QSize text_size = painter
                              ->boundingRect(QRect(0, 0, 0, 0),
                                             Qt::AlignLeft,
                                             QString::fromStdString(shader_text))
                              .size();
            text_size.setWidth(text_size.width() + 4);

            // Build text rectangle centered in shader rectangle.
            QRect rect = option.rect;
            int   height = option.rect.height() - 4;
            rect.setLeft(rect.left() + offset);
            rect.setTop(option.rect.top() + 2);
            rect.setHeight(height);
            QRect text_rect = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignLeft, text_size, rect);

            QPen pen;
            if (item_ptr->GetStageEnabled(stage))
            {
                painter->fillRect(text_rect, Dive::kShaderStageColor[(uint32_t)stage]);
                pen.setColor(QColor(255, 255, 255, 255));
            }
            else
            {
                painter->fillRect(text_rect, QColor(64, 64, 64, 255));
                pen.setColor(QColor(0, 0, 0, 255));
            }

            painter->setPen(pen);
            painter->drawText(text_rect, Qt::AlignCenter, QString::fromStdString(shader_text));

            offset += text_size.width() + 5;
        }
    }
    painter->restore();
}

//--------------------------------------------------------------------------------------------------
QSize MostExpensiveEventsViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                                const QModelIndex          &index) const
{
    const int kMargin = 5;
    QSize     size_hint = QStyledItemDelegate::sizeHint(option, index);
    return QSize(size_hint.width(), size_hint.height() + kMargin);
}

// =================================================================================================
// MostExpensiveEventsView
// =================================================================================================
MostExpensiveEventsView::MostExpensiveEventsView(const Dive::CaptureMetadata &capture_metadata) :
    m_capture_metadata(capture_metadata)
{
    m_event_list = new QTreeWidget();
    m_event_list->setColumnCount(4);
    m_event_list->setHeaderLabels(QStringList() << "Event"
                                                << "Type"
                                                << "Duration"
                                                << "Occupancy Duration"
                                                << "Shader Stages");
    m_event_list->setSortingEnabled(true);
    m_event_list->setAlternatingRowColors(true);
    m_event_list->setAutoScroll(false);
    m_event_list->setItemDelegate(new MostExpensiveEventsViewDelegate());
    m_event_list->setMouseTracking(true);
    m_event_list->viewport()->setAttribute(Qt::WA_Hover);
    m_event_list->setContextMenuPolicy(Qt::CustomContextMenu);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_event_list);
    setLayout(layout);

    QObject::connect(m_event_list,
                     SIGNAL(customContextMenuRequested(QPoint)),
                     this,
                     SLOT(OnCustomContextMenuRequested(QPoint)));
}

//--------------------------------------------------------------------------------------------------
void MostExpensiveEventsView::Update() { return; }

//--------------------------------------------------------------------------------------------------
void MostExpensiveEventsView::OnCustomContextMenuRequested(QPoint pos)
{
    if (m_event_list->currentColumn() == kDurationColumn ||
        m_event_list->currentColumn() == kOccupancyDurationColumn)
    {
        Settings::DisplayUnit unit = Settings::Get()->ReadEventListDisplayUnit();

        QMenu menu;

        QAction cycle_action("Display in cycles");
        cycle_action.setCheckable(true);
        cycle_action.setChecked(unit == Settings::DisplayUnit::kCycle);
        menu.addAction(&cycle_action);

        QAction ms_action("Display in milliseconds (ms)");
        ms_action.setCheckable(true);
        ms_action.setChecked(unit == Settings::DisplayUnit::kMs);
        menu.addAction(&ms_action);

        QAction us_action("Display in microseconds (us)");
        us_action.setCheckable(true);
        us_action.setChecked(unit == Settings::DisplayUnit::kUs);
        menu.addAction(&us_action);

        QAction ns_action("Display in nanoseconds (ns)");
        ns_action.setCheckable(true);
        ns_action.setChecked(unit == Settings::DisplayUnit::kNs);
        menu.addAction(&ns_action);

        QAction *selected_action_ptr = menu.exec(m_event_list->mapToGlobal(pos));
        if (selected_action_ptr == &cycle_action)
            Settings::Get()->WriteEventListDisplayUnit(Settings::DisplayUnit::kCycle);
        else if (selected_action_ptr == &ms_action)
            Settings::Get()->WriteEventListDisplayUnit(Settings::DisplayUnit::kMs);
        else if (selected_action_ptr == &us_action)
            Settings::Get()->WriteEventListDisplayUnit(Settings::DisplayUnit::kUs);
        else if (selected_action_ptr == &ns_action)
            Settings::Get()->WriteEventListDisplayUnit(Settings::DisplayUnit::kNs);
    }
    Update();
}

//--------------------------------------------------------------------------------------------------
void MostExpensiveEventsView::leaveEvent(QEvent *event)
{
    HoverHelp *hover_help_ptr = HoverHelp::Get();
    hover_help_ptr->SetCurItem(HoverHelp::Item::kNone);
}

//--------------------------------------------------------------------------------------------------
QString MostExpensiveEventsView::GetDurationString(uint64_t cycle) const
{
    Settings::DisplayUnit unit = Settings::Get()->ReadEventListDisplayUnit();

    QString     duration_string;
    QTextStream out(&duration_string);

    out.setRealNumberNotation(QTextStream::FixedNotation);  // Equivalent of %f
    out.setRealNumberPrecision(0);
    out.setLocale(QLocale::system());  // To format #s (eg. English locale: 1000 -> 1,000)

    if (unit == Settings::DisplayUnit::kCycle)
    {
        out << cycle << " cycle";
    }
    else if (unit == Settings::DisplayUnit::kMs)
    {
        out.setRealNumberPrecision(4);  // #.####
        out << ConvertCyclesToMs(cycle) << " ms";
    }
    else if (unit == Settings::DisplayUnit::kUs)
    {
        out.setRealNumberPrecision(2);  // #.##
        out << ConvertCyclesToUs(cycle) << " us";
    }
    else if (unit == Settings::DisplayUnit::kNs)
    {
        out << ConvertCyclesToNs(cycle) << " ns";
    }
    return duration_string;
}
