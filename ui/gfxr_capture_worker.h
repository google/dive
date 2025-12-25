/*
 Copyright 2025 Google LLC

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

#include <qobject.h>

#include <filesystem>

#include "capture_service/device_mgr.h"
#include "capture_worker.h"

class GfxrCaptureWorker : public CaptureWorker
{
    Q_OBJECT

 public:
    GfxrCaptureWorker(QObject* parent = nullptr) : CaptureWorker(parent) {}

    void run() override;
    void SetGfxrSourceCaptureDir(const std::string& source_capture_dir);
    bool AreTimestampsCurrent(Dive::AndroidDevice* device,
                              const std::map<std::string, std::string>& previous_timestamps);
    absl::StatusOr<qlonglong> getGfxrCaptureDirectorySize(Dive::AndroidDevice* device);

 private:
    std::string m_source_capture_dir;  // On Android, better to keep as std::string since the
                                       // host platform delimiter may be inconsistent
    std::vector<std::string> m_file_list;
};
