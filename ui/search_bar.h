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

#include <QWidget>
#include "dive_tree_view.h"

#pragma once

// Forward declarations
class QLabel;
class QLineEdit;
class QPushButton;

class SearchBar : public QWidget
{
    Q_OBJECT

public:
    SearchBar(QWidget* parent = nullptr);

public slots:
    void searchBarFocusOut();
    void positionCurser();
    void clearSearch();
    void newSearchResults();
    void prevSearchedItem();
    void nextSearchedItem();
    void resetSearchResults();
    void updateSearchResults(uint64_t curr_item_pos, uint64_t total_num_of_items);
    void cancelSearch();
    void setView(QAbstractItemView* view);

signals:
    void new_search(const QString& search_string);
    void prev_search();
    void next_search();
    void hide_search_bar(bool isHidden);

private:
    bool               m_searched = false;
    QLineEdit*         m_input = nullptr;
    QPushButton*       m_prev = nullptr;
    QPushButton*       m_next = nullptr;
    QPushButton*       m_cancel = nullptr;
    QLabel*            m_search_results = nullptr;
    QAbstractItemView* m_view = nullptr;
};
