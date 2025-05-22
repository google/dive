/*
 Copyright 2021 Google LLC

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
#include "settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include "dive_core/common.h"

//--------------------------------------------------------------------------------------------------
Settings* Settings::Get()
{
    static Settings* singleton_ptr = nullptr;
    if (singleton_ptr == nullptr)
    {
        singleton_ptr = new Settings();
    }
    return singleton_ptr;
}

//--------------------------------------------------------------------------------------------------
Settings::Settings()
{
    QCoreApplication::setOrganizationName("Google");
    QCoreApplication::setApplicationName("Dive");
}

//--------------------------------------------------------------------------------------------------
QStringList Settings::ReadRecentFiles()
{
    QSettings settings;
    return settings.value("recentFiles").toStringList();
}

//--------------------------------------------------------------------------------------------------
void Settings::WriteRecentFiles(QStringList recent_files)
{
    QMutableStringListIterator i(recent_files);
    while (i.hasNext())
    {
        if (!QFile::exists(i.next()))
            i.remove();
    }

    QSettings settings;
    settings.setValue("recentFiles", recent_files);
}

//--------------------------------------------------------------------------------------------------
QString Settings::ReadLastFilePath()
{
    QSettings settings;
    return settings.value("lastFilePath", QDir::homePath()).toString();
}
//--------------------------------------------------------------------------------------------------
void Settings::WriteLastFilePath(QString last_file_path)
{
    QSettings settings;
    settings.setValue("lastFilePath", last_file_path);
}

//--------------------------------------------------------------------------------------------------
uint32_t Settings::ReadCaptureDelay()
{
    QSettings settings;
    return settings.value("captureDelay", 0).toInt();
}
//--------------------------------------------------------------------------------------------------
void Settings::WriteCaptureDelay(uint32_t capture_delay)
{
    QSettings settings;
    settings.setValue("captureDelay", capture_delay);
}

//--------------------------------------------------------------------------------------------------
Settings::DisplayUnit Settings::ReadRulerDisplayUnit()
{
    QSettings   settings;
    DisplayUnit display_unit = (DisplayUnit)settings.value("rulerDisplayUnit", 0).toInt();
    DIVE_ASSERT(display_unit == kCycle || display_unit == kMs || display_unit == kUs ||
                display_unit == kNs);
    return display_unit;
}
//--------------------------------------------------------------------------------------------------
void Settings::WriteRulerDisplayUnit(DisplayUnit display_unit)
{
    QSettings settings;
    settings.setValue("rulerDisplayUnit", display_unit);
}

//--------------------------------------------------------------------------------------------------
Settings::DisplayUnit Settings::ReadEventListDisplayUnit()
{
    QSettings   settings;
    DisplayUnit display_unit = (DisplayUnit)settings.value("eventListDisplayUnit", 0).toInt();
    DIVE_ASSERT(display_unit == kCycle || display_unit == kMs || display_unit == kUs ||
                display_unit == kNs);
    return display_unit;
}
//--------------------------------------------------------------------------------------------------
void Settings::WriteEventListDisplayUnit(DisplayUnit display_unit)
{
    QSettings settings;
    settings.setValue("eventListDisplayUnit", display_unit);
}