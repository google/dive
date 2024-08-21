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

#include <QDialog>

#pragma once

// Forward declarations
class QLabel;
class QHBoxLayout;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

class ShortcutsDialog : public QDialog
{
    Q_OBJECT

public:
    ShortcutsDialog(QWidget *parent = 0);

private:
    QLabel*        m_shortcut_information;     // The shortcut info.
    QPushButton *m_close_button;          // A button to close the dialog.
    QHBoxLayout *m_shortcuts_layout;  // The shortcuts layout is the shortcut info arranged horizontally
    QHBoxLayout *m_button_layout;   // The button layout is stretch + Close Button
    QVBoxLayout *m_main_layout;     // The main layout Shortcuts + Close button arranged vertically
};
