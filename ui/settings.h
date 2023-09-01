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

#pragma once
#include <stdint.h>
#include <QString>
#include <QStringList>

//--------------------------------------------------------------------------------------------------
class Settings
{
public:
    enum DisplayUnit : uint32_t
    {
        kCycle,
        kMs,
        kUs,
        kNs
    };

    QStringList ReadRecentFiles();
    void        WriteRecentFiles(QStringList recent_files);

    QString ReadLastFilePath();
    void    WriteLastFilePath(QString last_file_path);

    uint32_t ReadCaptureDelay();
    void     WriteCaptureDelay(uint32_t capture_delay);

    DisplayUnit ReadRulerDisplayUnit();
    void        WriteRulerDisplayUnit(DisplayUnit display_unit);

    DisplayUnit ReadEventListDisplayUnit();
    void        WriteEventListDisplayUnit(DisplayUnit display_unit);

    // Singleton
    static Settings* Get();

private:
    Settings();
};
