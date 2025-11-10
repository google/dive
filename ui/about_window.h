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

#include <QDialog>

#pragma once

// Forward declarations
class QLabel;
class QHBoxLayout;
class QIcon;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    AboutDialog(QWidget *parent = 0);

private:
    void CreateHeaderLayout();
    void CreateVersionLayout();
    void CreateLicenseLayout();
    void CreateButtonLayout();

    QVBoxLayout *m_main_layout;

    // Contains the Dive logo and a high-level summary
    QHBoxLayout *m_header_layout;
    QIcon       *m_icon;
    QLabel      *m_icon_label;
    QLabel      *m_build_information;

    // Contains detailed version info for different components of the Dive tool
    QVBoxLayout    *m_version_layout;
    QLabel         *m_version_label;
    QPlainTextEdit *m_version_details;

    // Contains aggregated license info
    QVBoxLayout    *m_license_layout;
    QLabel         *m_third_party_licenses;
    QPlainTextEdit *m_license_notice;

    // Close button
    QHBoxLayout *m_button_layout;
    QPushButton *m_close_button;
};
