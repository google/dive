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
    AboutDialog(QWidget* parent = 0);

private:
    // Contains the Dive logo and a high-level summary
    QHBoxLayout* CreateHeaderLayout();
    // Contains detailed version info for different components of the Dive tool
    QVBoxLayout* CreateVersionLayout();
    // Contains aggregated license info
    QVBoxLayout* CreateLicenseLayout();
    // Close button
    QHBoxLayout* CreateButtonLayout();

    QVBoxLayout* m_main_layout;
};
