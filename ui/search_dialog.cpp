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

#include "search_dialog.h"

#include "QHBoxLayout"
#include "QLabel"
#include "QLineEdit"
#include "QPushButton"
#include "QShortcut"
#include "QVBoxLayout"
#include "shortcuts.h"

//--------------------------------------------------------------------------------------------------
SearchDialog::SearchDialog(QWidget* parent, const QString& title) : QDialog(parent)

{
    m_input = new QLineEdit;
    m_search = new QPushButton;
    m_search->setIcon(QIcon(":/images/search.png"));
    m_prev = new QPushButton;
    m_prev->setIcon(QIcon(":/images/arrow_up.png"));
    m_next = new QPushButton;
    m_next->setIcon(QIcon(":/images/arrow_down.png"));
    m_search_results = new QLabel;

    connect(m_input, SIGNAL(textEdited(const QString&)), this, SLOT(resetSearchResults()));
    connect(m_search, SIGNAL(clicked()), this, SLOT(newSearchResults()));
    connect(m_prev, SIGNAL(clicked()), this, SLOT(prevSearchedItem()));
    connect(m_next, SIGNAL(clicked()), this, SLOT(nextSearchedItem()));

    QShortcut* previousSearchItemShortcut = new QShortcut(QKeySequence(
                                                          SHORTCUT_PREVIOUS_SEARCH_RESULT),
                                                          this);
    connect(previousSearchItemShortcut,
            &QShortcut::activated,
            this,
            &SearchDialog::prevSearchedItem);

    QShortcut* nextSearchShortcut = new QShortcut(QKeySequence(SHORTCUT_NEXT_SEARCH_RESULT), this);
    connect(nextSearchShortcut, &QShortcut::activated, this, &SearchDialog::nextSearchedItem);

    QHBoxLayout* search_buttons_layout = new QHBoxLayout;
    search_buttons_layout->addWidget(m_input);
    search_buttons_layout->addWidget(m_search);
    search_buttons_layout->addWidget(m_prev);
    search_buttons_layout->addWidget(m_next);

    QHBoxLayout* search_results_layout = new QHBoxLayout;
    search_results_layout->addWidget(m_search_results);

    QVBoxLayout* dialog_layout = new QVBoxLayout;
    dialog_layout->addLayout(search_buttons_layout);
    dialog_layout->addLayout(search_results_layout);

    setWindowTitle("Search " + title);
    setWindowModality(Qt::NonModal);
    setLayout(dialog_layout);
}

//--------------------------------------------------------------------------------------------------
void SearchDialog::newSearchResults()
{
    if (!m_input->text().isEmpty())
    {
        QString searchString = m_input->text();
        emit    new_search(searchString);
        searched = true;
    }
}

//--------------------------------------------------------------------------------------------------
void SearchDialog::prevSearchedItem()
{
    if (!m_input->text().isEmpty() && searched)
    {
        emit prev_search();
    }
}

//--------------------------------------------------------------------------------------------------
void SearchDialog::nextSearchedItem()
{
    if (!m_input->text().isEmpty() && searched)
    {
        emit next_search();
    }
}

//--------------------------------------------------------------------------------------------------
void SearchDialog::resetSearchResults()
{
    searched = false;
    m_search_results->setText(QString());
}

//--------------------------------------------------------------------------------------------------
void SearchDialog::updateSearchResults(uint64_t curr_item_pos, uint64_t total_num_of_items)
{
    if (total_num_of_items == 0)
        m_search_results->setText("No results found");
    else
    {
        curr_item_pos++;
        m_search_results->setText(QString::number(curr_item_pos) + " of " +
                                  QString::number(total_num_of_items) + " matching results");
    }
}