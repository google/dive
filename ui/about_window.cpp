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

#include "utils/version_info.h"

// =================================================================================================
// AboutDialog
// =================================================================================================

AboutDialog::AboutDialog(QWidget *parent)
{
    m_main_layout = new QVBoxLayout;
    CreateHeaderLayout();
    CreateVersionLayout();
    CreateLicenseLayout();
    CreateButtonLayout();

    // Disable help icon, set size, title, and layout
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(640, 480);
    setWindowTitle("About Dive");
    setLayout(m_main_layout);
}

void AboutDialog::CreateHeaderLayout()
{
    m_icon = new QIcon(":/images/dive.ico");
    m_icon_label = new QLabel();
    m_icon_label->setPixmap(m_icon->pixmap(64, 64));
    m_icon_label->setFixedSize(64, 64);

    m_build_information = new QLabel(Dive::GetDiveDescription().c_str());
    m_build_information->setWordWrap(true);

    m_header_layout = new QHBoxLayout;
    m_header_layout->addWidget(m_icon_label);
    m_header_layout->addWidget(m_build_information);

    m_main_layout->addLayout(m_header_layout);
}

void AboutDialog::CreateVersionLayout()
{
    m_version_label = new QLabel("Version details:");

    m_version_details = new QPlainTextEdit();
    m_version_details->setPlainText(Dive::GetLongVersionString().c_str());
    m_version_details->setReadOnly(true);
    m_version_details->setFixedHeight(100);

    m_version_layout = new QVBoxLayout;
    m_version_layout->addWidget(m_version_label);
    m_version_layout->addWidget(m_version_details);

    m_main_layout->addLayout(m_version_layout);
}

void AboutDialog::CreateLicenseLayout()
{
    m_third_party_licenses = new QLabel("Third Party Licenses:");

    m_license_notice = new QPlainTextEdit();
    QFile licenseFile{ QDir{ QCoreApplication::applicationDirPath() }.filePath("NOTICE") };
    if (licenseFile.open(QIODevice::ReadOnly))
    {
        m_license_notice->setPlainText(licenseFile.readAll());
    }
    m_license_notice->setReadOnly(true);

    m_license_layout = new QVBoxLayout;
    m_license_layout->addWidget(m_third_party_licenses);
    m_license_layout->addWidget(m_license_notice);

    m_main_layout->addLayout(m_license_layout);
}

void AboutDialog::CreateButtonLayout()
{
    m_close_button = new QPushButton;
    m_close_button->setText("Close");
    connect(m_close_button, SIGNAL(clicked()), this, SLOT(close()));

    m_button_layout = new QHBoxLayout;
    m_button_layout->addStretch();
    m_button_layout->addWidget(m_close_button);

    m_main_layout->addLayout(m_button_layout);
}
