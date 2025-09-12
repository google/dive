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

#include <qevent.h>
#include <QDialog>
#include <unordered_set>

#pragma once

// Forward declarations
class QButtonGroup;
class QRadioButton;
class QLabel;
class QPushButton;

class PackageFilter : public QWidget
{
    Q_OBJECT

public:
    PackageFilter(QWidget *parent = nullptr);

public slots:
    void ApplyFilters();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void FiltersApplied(const QString &applied_filter_text);

private:
    const std::size_t kTotalFilterCount = 3;
    QButtonGroup     *m_filter_button_group = nullptr;
    QRadioButton     *m_all_filter = nullptr;
    QRadioButton     *m_non_debuggable_filter = nullptr;
    QRadioButton     *m_debuggable_filter = nullptr;
    QPushButton      *m_apply = nullptr;
    QString           m_active_filter_text = "";
};
