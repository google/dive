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
#include "problems_view.h"
#include <QLabel>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>
#include "command_model.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/data_core.h"
#include "dive_core/dive_strings.h"
#include "dive_core/log.h"
#include "dive_core/shader_disassembly.h"
#include "hover_help_model.h"

const uint32_t kDescTextColumn = 3;

// =================================================================================================
// ProblemWidgetItem
// =================================================================================================
class ProblemWidgetItem : public QTreeWidgetItem
{
public:
    ProblemWidgetItem(Dive::CrossRef ref,
                      std::string    short_desc,
                      std::string    long_desc,
                      QTreeWidget   *view) :
        QTreeWidgetItem(view),
        m_ref(ref),
        m_short_desc(short_desc),
        m_long_desc(long_desc)
    {
    }
    Dive::CrossRef       GetRef() const { return m_ref; }
    Dive::LogAssociation GetAssociation() const { return m_ref.Type(); }
    uint64_t             GetId() const { return m_ref.Id(); }
    std::string          GetShortDesc() const { return m_short_desc; }
    std::string          GetLongDesc() const { return m_long_desc; }

private:
    Dive::CrossRef m_ref;
    std::string    m_short_desc;
    std::string    m_long_desc;
};

// =================================================================================================
// ProblemsViewDelegate
// =================================================================================================
QSize ProblemsViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex          &index) const
{
    const int kMargin = 5;
    QSize     size_hint = QStyledItemDelegate::sizeHint(option, index);
    return QSize(size_hint.width(), size_hint.height() + kMargin);
}

// =================================================================================================
// ProblemsView
// =================================================================================================
ProblemsView::ProblemsView(const Dive::CommandHierarchy &command_hierarchy) :
    m_command_hierarchy(command_hierarchy)
{
    m_log_list = new QTreeWidget();
    m_log_list->setColumnCount(4);
    m_log_list->setHeaderLabels(QStringList() << ""
                                              << "Type"
                                              << "Event"
                                              << "Description");
    m_log_list->setSortingEnabled(true);
    m_log_list->setAlternatingRowColors(true);
    m_log_list->setAutoScroll(false);
    m_log_list->setItemDelegate(new ProblemsViewDelegate());
    m_log_list->setMouseTracking(true);
    m_log_list->viewport()->setAttribute(Qt::WA_Hover);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_log_list);
    setLayout(layout);

    QObject::connect(m_log_list,
                     SIGNAL(itemSelectionChanged()),
                     this,
                     SLOT(OnProblemSelectionChanged()));
    QObject::connect(m_log_list,
                     SIGNAL(itemEntered(QTreeWidgetItem *, int)),
                     this,
                     SLOT(OnProblemItemHover(QTreeWidgetItem *, int)));
}

//--------------------------------------------------------------------------------------------------
void ProblemsView::Update(const Dive::LogRecord *log_ptr)
{
    m_log_list->clear();
    for (uint32_t i = 0; i < log_ptr->GetNumEntries(); ++i)
    {
        const Dive::LogRecord::LogEntry &entry = log_ptr->GetEntry(i);

        ProblemWidgetItem *item = new ProblemWidgetItem(entry.m_ref,
                                                        entry.m_short_desc,
                                                        entry.m_long_desc,
                                                        m_log_list);
        // Column 0
        switch (entry.m_type)
        {
        case Dive::LogType::kInfo: break;
        case Dive::LogType::kWarning:
            item->setIcon(0, m_log_list->style()->standardIcon(QStyle::SP_MessageBoxWarning));
            break;
        case Dive::LogType::kError:
            item->setIcon(0, m_log_list->style()->standardIcon(QStyle::SP_MessageBoxCritical));
            break;
        };

        // Column 1
        // "Performance Warning"/"Performance Error" are both labeled as "Performance"
        if (entry.m_cat == Dive::LogCategory::kPerformance)
            item->setText(1, "Performance");
        else if (entry.m_type == Dive::LogType::kInfo)
            item->setText(1, "Info");
        else if (entry.m_type == Dive::LogType::kWarning)
            item->setText(1, "Warning");
        else if (entry.m_type == Dive::LogType::kError)
            item->setText(1, "Error");

        // Column 2
        item->setTextAlignment(2, Qt::AlignmentFlag::AlignCenter);
        if (entry.m_ref.Type() == Dive::LogAssociation::kEvent)
        {
        }
        else if (entry.m_ref.Type() == Dive::LogAssociation::kBarrier)
        {
        }

        // Column 3
        item->setText(kDescTextColumn, tr(entry.m_short_desc.c_str()));
    }

    // Resize columns to fit
    uint32_t column_count = (uint32_t)m_log_list->columnCount();
    for (uint32_t column = 0; column < column_count; ++column)
        m_log_list->resizeColumnToContents(column);
}

//--------------------------------------------------------------------------------------------------
void ProblemsView::OnProblemSelectionChanged()
{
    const ProblemWidgetItem *item_ptr = (const ProblemWidgetItem *)m_log_list->currentItem();
    if (item_ptr->GetAssociation() == Dive::LogAssociation::kEvent)
    {
    }
    else if (item_ptr->GetAssociation() == Dive::LogAssociation::kBarrier)
    {
    }
    else
    {
        emit crossReferece(item_ptr->GetRef());
    }
}

//--------------------------------------------------------------------------------------------------
void ProblemsView::OnProblemItemHover(QTreeWidgetItem *item_ptr, int column)
{
    ProblemWidgetItem *problem_item_ptr = (ProblemWidgetItem *)item_ptr;
    HoverHelp         *hover_help_ptr = HoverHelp::Get();
    std::string desc = problem_item_ptr->GetShortDesc() + "<br>" + problem_item_ptr->GetLongDesc();
    hover_help_ptr->SetCurItem(HoverHelp::Item::kNone, 0, 0, 0, desc.c_str());
}

//--------------------------------------------------------------------------------------------------
void ProblemsView::leaveEvent(QEvent *event)
{
    HoverHelp *hover_help_ptr = HoverHelp::Get();
    hover_help_ptr->SetCurItem(HoverHelp::Item::kNone);
}