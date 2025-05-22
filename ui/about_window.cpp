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
#include "about_window.h"

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
#include <sstream>

#include "version.h"

// =================================================================================================
// AboutDialog
// =================================================================================================

AboutDialog::AboutDialog(QWidget *parent)
{
    // Build version string
    std::ostringstream os;
    os << VERSION_PRODUCTNAME << std::endl;
    os << VERSION_DESCRIPTION << std::endl;
    os << VERSION_COPYRIGHT << std::endl;
    os << std::endl;
    os << "Version ";
    os << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION << "." << VERSION_BUILD;
    m_build_information = new QLabel(os.str().c_str());

    // Load third party license information
    m_license_notice = new QPlainTextEdit();
    QFile licenseFile{ QDir{ QCoreApplication::applicationDirPath() }.filePath("NOTICE") };
    if (licenseFile.open(QIODevice::ReadOnly))
    {
        m_license_notice->setPlainText(licenseFile.readAll());
    }
    m_license_notice->setReadOnly(true);

    // Load icon
    m_icon = new QIcon(":/images/dive.ico");
    m_icon_label = new QLabel();
    m_icon_label->setPixmap(m_icon->pixmap(64, 64));
    m_icon_label->setFixedSize(64, 64);

    // Version layout is Icon + Build Info
    m_version_layout = new QHBoxLayout;
    m_version_layout->addWidget(m_icon_label);
    m_version_layout->addWidget(m_build_information);

    // Close button
    m_close_button = new QPushButton;
    m_close_button->setText("Close");
    connect(m_close_button, SIGNAL(clicked()), this, SLOT(close()));

    m_button_layout = new QHBoxLayout;
    m_button_layout->addStretch();
    m_button_layout->addWidget(m_close_button);

    // Main layout is Version + Label + NOTICE
    m_main_layout = new QVBoxLayout;
    m_main_layout->addLayout(m_version_layout);
    m_third_party_licenses = new QLabel("Third Party Licenses:");
    m_main_layout->addWidget(m_third_party_licenses);
    m_main_layout->addWidget(m_license_notice);
    m_main_layout->addLayout(m_button_layout);

    // Disable help icon, set size, title, and layout
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(640, 480);
    setWindowTitle("About Dive");
    setLayout(m_main_layout);
}
