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
#include "shortcuts_window.h"
#include <qfont.h>
#include <qlabel.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <iostream>
#include <sstream>
#include "shortcuts.h"

// =================================================================================================
// ShortcutsDialog
// =================================================================================================

ShortcutsDialog::ShortcutsDialog(QWidget *parent)
{
    // Build shortcuts string
    std::ostringstream os;
    os << "Begin an Events Search: ";
    os << SHORTCUT_EVENTS_SEARCH << std::endl;
    os << std::endl;
    os << "Begin a Tab View Search: ";
    os << SHORTCUT_TAB_VIEW_SEARCH << std::endl;
    os << std::endl;
    os << "View the next search result: ";
    os << SHORTCUT_NEXT_SEARCH_RESULT << std::endl;
    os << std::endl;
    os << "View the previous search result: ";
    os << SHORTCUT_PREVIOUS_SEARCH_RESULT << std::endl;
    os << std::endl;
    os << "View the Overview tab: ";
    os << SHORTCUT_OVERVIEW_TAB << std::endl;
    os << std::endl;
    os << "View the Commands tab: ";
    os << SHORTCUT_COMMANDS_TAB << std::endl;
    os << std::endl;
    os << "View the Shaders tab: ";
    os << SHORTCUT_SHADERS_TAB << std::endl;
    os << std::endl;
    os << "View the Event State tab: ";
    os << SHORTCUT_EVENT_STATE_TAB << std::endl;
    os << std::endl;
    m_shortcut_information = new QLabel(os.str().c_str());

    // Set the font for the shortcuts string
    QFont font = m_shortcut_information->font();
    font.setBold(true);

    // Shortcuts layout is shortcut Info
    m_shortcuts_layout = new QHBoxLayout;
    m_shortcuts_layout->addWidget(m_shortcut_information);

    // Close button
    m_close_button = new QPushButton;
    m_close_button->setText("Close");
    connect(m_close_button, SIGNAL(clicked()), this, SLOT(close()));

    m_button_layout = new QHBoxLayout;
    m_button_layout->addStretch();
    m_button_layout->addWidget(m_close_button);

    // Main layout is Shortcuts + Close Button
    m_main_layout = new QVBoxLayout;
    m_main_layout->addLayout(m_shortcuts_layout);
    m_main_layout->setAlignment(m_shortcuts_layout, Qt::AlignCenter);
    m_main_layout->addLayout(m_button_layout);

    // Disable help icon, set size, title, and layout
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(640, 480);
    setWindowTitle("Dive Keyboard Shortcuts");
    setLayout(m_main_layout);
}
