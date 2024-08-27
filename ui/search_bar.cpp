/*
 Copyright 2024 Google LLC
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

#include "search_bar.h"
#include "shortcuts.h"
#include "QHBoxLayout"
#include "QLabel"
#include "QLineEdit"
#include "QPushButton"
#include "QShortcut"
#include "QVBoxLayout"
#include <qwidget.h>
#include <QDebug>
#include <iostream>

//--------------------------------------------------------------------------------------------------
SearchBar::SearchBar(QWidget* parent) : QWidget(parent)
{
    QLabel *searchlabel = new QLabel(this);
    QPixmap pixmap(":/images/search.png");
    searchlabel->setPixmap(pixmap);

    m_input = new QLineEdit(this);
    QPalette palette = m_input->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black); // Set text color to black
    m_input->setPalette(palette);

    m_prev = new QPushButton(this);
    m_prev->setIcon(QIcon(":/images/arrow_up.png"));

    m_next = new QPushButton(this);
    m_next->setIcon(QIcon(":/images/arrow_down.png"));

    m_search_results = new QLabel(this);
    m_search_results->hide();

    QHBoxLayout* search_buttons_layout = new QHBoxLayout;
    search_buttons_layout->addWidget(searchlabel);
    search_buttons_layout->addWidget(m_input);
    search_buttons_layout->addWidget(m_prev);
    search_buttons_layout->addWidget(m_next);

    QHBoxLayout* search_results_layout = new QHBoxLayout;
    search_results_layout->addWidget(m_search_results);

    QVBoxLayout* searchBar_layout = new QVBoxLayout(this);
    searchBar_layout->addLayout(search_buttons_layout);
    searchBar_layout->addLayout(search_results_layout);


    connect(m_input, SIGNAL(textEdited(const QString&)), this, SLOT(resetSearchResults()));
    connect(m_input, SIGNAL(returnPressed()), this, SLOT(newSearchResults()));
    connect(m_input, SIGNAL(editingFinished()), this, SLOT(searchBarFocusOut()));

    connect(m_prev, SIGNAL(clicked()), this, SLOT(prevSearchedItem()));
    connect(m_next, SIGNAL(clicked()), this, SLOT(nextSearchedItem()));

    QShortcut *previousSearchItemShortcut = new QShortcut(QKeySequence(SHORTCUT_PREVIOUS_SEARCH_RESULT), this);
    connect(previousSearchItemShortcut, &QShortcut::activated, this, &SearchBar::prevSearchedItem);

    QShortcut *nextSearchShortcut = new QShortcut(QKeySequence(SHORTCUT_NEXT_SEARCH_RESULT), this);
    connect(nextSearchShortcut, &QShortcut::activated, this, &SearchBar::nextSearchedItem);
}

//--------------------------------------------------------------------------------------------------
void SearchBar::searchBarFocusOut()
{
    if (!m_input->hasFocus()) {
        if (!m_prev->hasFocus() && !m_next->hasFocus())
        {
            this->hide();
            emit hide_search_bar(this->isHidden());
        }
    }
}

//--------------------------------------------------------------------------------------------------
void SearchBar::positionCurser()
{
    m_input->setCursorPosition(0);
    m_input->setFocus();
}
//--------------------------------------------------------------------------------------------------
void SearchBar::clearSearch()
{
    m_input->setText(QString());
    m_search_results->setText(QString());
}

//--------------------------------------------------------------------------------------------------
void SearchBar::newSearchResults()
{
    if (!m_input->text().isEmpty())
    {
        QString searchString = m_input->text();
        emit    new_search(searchString);
        searched = true;
    }
}

//--------------------------------------------------------------------------------------------------
void SearchBar::prevSearchedItem()
{
    if (!m_input->text().isEmpty() && searched)
    {
        emit prev_search();
    }
}

//--------------------------------------------------------------------------------------------------
void SearchBar::nextSearchedItem()
{
    if (!m_input->text().isEmpty() && searched)
    {
        emit next_search();
    }
}

//--------------------------------------------------------------------------------------------------
void SearchBar::resetSearchResults()
{
    searched = false;
    m_search_results->setText(QString());
    m_search_results->hide();
}

//--------------------------------------------------------------------------------------------------
void SearchBar::updateSearchResults(uint64_t curr_item_pos, uint64_t total_num_of_items)
{
    if (total_num_of_items == 0)
        m_search_results->setText("No results found");
    else
    {
        curr_item_pos++;
        m_search_results->setText(QString::number(curr_item_pos) + " of " +
                                  QString::number(total_num_of_items) + " matching results");
    }
    m_search_results->show();
}
