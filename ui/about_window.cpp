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

AboutDialog::AboutDialog(QWidget* parent)
{
    m_main_layout = new QVBoxLayout;
    m_main_layout->addLayout(CreateHeaderLayout());
    m_main_layout->addLayout(CreateVersionLayout());
    m_main_layout->addLayout(CreateLicenseLayout());
    m_main_layout->addLayout(CreateButtonLayout());

    // Disable help icon, set size, title, and layout
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(640, 480);
    setWindowTitle("About Dive");
    setLayout(m_main_layout);
}

QHBoxLayout* AboutDialog::CreateHeaderLayout()
{
    auto icon = new QIcon(":/images/dive.ico");
    auto icon_label = new QLabel();
    icon_label->setPixmap(icon->pixmap(64, 64));
    icon_label->setFixedSize(64, 64);

    auto build_information = new QLabel(Dive::GetDiveDescription().c_str());
    build_information->setWordWrap(true);

    QHBoxLayout* header_layout = new QHBoxLayout;
    header_layout->addWidget(icon_label);
    header_layout->addWidget(build_information);

    return header_layout;
}

QVBoxLayout* AboutDialog::CreateVersionLayout()
{
    auto version_label = new QLabel("Version details:");

    auto version_details = new QPlainTextEdit();
    version_details->setPlainText(Dive::GetLongVersionString().c_str());
    version_details->setReadOnly(true);
    version_details->setFixedHeight(100);

    QVBoxLayout* version_layout = new QVBoxLayout;
    version_layout->addWidget(version_label);
    version_layout->addWidget(version_details);

    return version_layout;
}

QVBoxLayout* AboutDialog::CreateLicenseLayout()
{
    auto third_party_licenses = new QLabel("Third Party Licenses:");

    auto  license_notice = new QPlainTextEdit();
    QFile licenseFile{ QDir{ QCoreApplication::applicationDirPath() }.filePath("NOTICE") };
    if (licenseFile.open(QIODevice::ReadOnly))
    {
        license_notice->setPlainText(licenseFile.readAll());
    }
    license_notice->setReadOnly(true);

    QVBoxLayout* license_layout = new QVBoxLayout;
    license_layout->addWidget(third_party_licenses);
    license_layout->addWidget(license_notice);

    return license_layout;
}

QHBoxLayout* AboutDialog::CreateButtonLayout()
{
    auto close_button = new QPushButton;
    close_button->setText("Close");
    connect(close_button, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout* button_layout = new QHBoxLayout;
    button_layout->addStretch();
    button_layout->addWidget(close_button);

    return button_layout;
}
