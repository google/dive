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

#include "gfxr_capture_worker.h"

#include <QDebug>
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "utils/component_files.h"

static constexpr int kStallTimeoutSeconds = 10;
static constexpr int kFileStatusPollingIntervalMs = 50;

void GfxrCaptureWorker::SetGfxrSourceCaptureDir(const std::string &source_capture_dir)
{
    m_source_capture_dir = source_capture_dir;
}

bool GfxrCaptureWorker::AreTimestampsCurrent(
Dive::AndroidDevice                      *device,
const std::map<std::string, std::string> &previous_timestamps)
{
    for (const auto &[file_name, timestamp] : previous_timestamps)
    {
        std::string get_timestamp_command = absl::StrCat("shell stat -c %Y ",
                                                         m_source_capture_dir,
                                                         "/",
                                                         file_name.data());

        absl::StatusOr<std::string> current_timestamp = device->Adb().RunAndGetResult(
        get_timestamp_command);
        if (!current_timestamp.ok())
        {
            qDebug() << "Failed to get timestamp for " << file_name.data() << ": "
                     << current_timestamp.status().message().data();
            return false;
        }

        if (current_timestamp->data() != timestamp)
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
absl::StatusOr<qlonglong> GfxrCaptureWorker::getGfxrCaptureDirectorySize(
Dive::AndroidDevice *device)
{
    // Retrieve the names of the files in the capture directory on the device.
    std::string                 ls_command = "shell ls " + m_source_capture_dir;
    absl::StatusOr<std::string> ls_output = device->Adb().RunAndGetResult(ls_command);

    if (!ls_output.ok())
    {
        std::cout << "Error getting capture files: " << ls_output.status().message() << std::endl;
        return ls_output.status();
    }

    m_file_list = absl::StrSplit(std::string(ls_output->data()), '\n', absl::SkipEmpty());
    for (std::string &file_with_trailing : m_file_list)
    {
        // Windows-style line endings use \r\n. When absl::StrSplit splits by \n, the \r remains
        // at the end of each line if the input string originated from a Windows-style line
        // ending.
        if (!file_with_trailing.empty() && file_with_trailing.back() == '\r')
        {
            file_with_trailing.pop_back();
        }
    }

    auto last_progress_time = std::chrono::steady_clock::now();

    while (true)
    {
        auto elapsed_since_progress = std::chrono::duration_cast<std::chrono::seconds>(
                                      std::chrono::steady_clock::now() - last_progress_time)
                                      .count();

        if (elapsed_since_progress > kStallTimeoutSeconds)
        {
            std::string err_msg = absl::
            StrFormat("GFXR capture stalled: No change in file status observed for %d seconds.",
                      kStallTimeoutSeconds);
            qDebug() << err_msg.c_str();
            return absl::DeadlineExceededError(err_msg);
        }

        // Ensure that the .gfxa, .gfxr, and .png file sizes are set and neither is being written
        // to.
        qlonglong                          total_size = 0;
        std::map<std::string, std::string> current_timestamps;
        bool                               found_zero_size = false;
        // Get the size of the file and timestamp for the last time the file was updated.
        std::string                 combined_cmd = absl::StrCat("shell 'stat -c \"%n|%s|%Y\" ",
                                                m_source_capture_dir,
                                                "/*'");
        absl::StatusOr<std::string> stat_output = device->Adb().RunAndGetResult(combined_cmd);
        if (!stat_output.ok())
        {
            std::cout << "Error getting capture file stats: " << stat_output.status().message()
                      << std::endl;
            return stat_output.status();
        }

        std::vector<std::string> stat_lines = absl::StrSplit(*stat_output, '\n', absl::SkipEmpty());

        for (const std::string &line : stat_lines)
        {
            std::vector<std::string> parts = absl::StrSplit(line, '|');

            // Expected format: [file_path, size, timestamp]
            if (parts.size() < 3)
            {
                qDebug() << "Warning: Could not parse stat output line: " << line.c_str();
                continue;
            }

            // Extract filename from the full path
            std::filesystem::path full_path = parts[0];
            std::string           file_name = full_path.filename().string();

            // Check if this file is one we expect from m_file_list
            if (std::none_of(m_file_list.begin(),
                             m_file_list.end(),
                             [&file_name](const auto &item) { return item == file_name; }))
            {
                continue;
            }

            qlonglong file_size = 0;

            // Check if the size string can be parsed into an integer
            if (!absl::SimpleAtoi(parts[1], &file_size))
            {
                qDebug() << "Failed to parse size for file: " << file_name.c_str();
                continue;
            }

            std::string file_timestamp = parts[2];

            // If a file size is 0, then the file has not finished being written to yet.
            if (file_size == 0)
            {
                found_zero_size = true;
                break;
            }

            last_progress_time = std::chrono::steady_clock::now();

            // Add the timestamp and update the total size.
            current_timestamps[file_name] = file_timestamp;
            total_size += file_size;
        }

        // If a file size is 0, then the file has not finished being written to yet. Sleep and
        // restart the size calculation.
        if (found_zero_size)
        {
            QThread::msleep(kFileStatusPollingIntervalMs);
            continue;
        }

        // If the size is greater than zero and the timestamps have been recorded check if the
        // timestamps are current.
        if (total_size > 0 && !current_timestamps.empty())
        {
            // If the timestamps are current, return the size of the directory.
            if (AreTimestampsCurrent(device, current_timestamps))
            {
                return total_size;
            }

            // If timestamps are not current, wait and restart the loop.
            QThread::msleep(kFileStatusPollingIntervalMs);
            continue;
        }

        // If total_size == 0 or current_timestamps is empty, wait/restart.
        QThread::msleep(kFileStatusPollingIntervalMs);
    }
}

//--------------------------------------------------------------------------------------------------
void GfxrCaptureWorker::run()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        qDebug() << "Failed to connect to device";
        return;
    }

    qlonglong capture_directory_size = 0;
    {
        absl::StatusOr<qlonglong> ret = getGfxrCaptureDirectorySize(device);
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to get size of gfxr capture directory",
                                               " error: ",
                                               ret.status().message());
            qDebug() << err_msg.c_str();
            emit ErrorMessage(QString::fromStdString(err_msg));
            return;
        }
        capture_directory_size = *ret;
    }

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    qDebug() << "Begin to download the gfxr capture file to "
             << m_target_capture_dir.generic_string().c_str();

    qlonglong   size = 0;
    std::string gfxr_stem;
    std::string original_screenshot_path;

    std::string gfxr_capture_file_path;
    // Retrieve each file in the capture directory (capture file and asset file).
    for (std::string file : m_file_list)
    {
        std::filesystem::path filename = file.data();
        std::filesystem::path target_path = m_target_capture_dir;
        target_path /= filename;

        // Source path is intended for Android, cannot use std::filesystem here
        std::string source_file = absl::StrCat(m_source_capture_dir, "/", filename.string());

        auto retrieve_file = device->RetrieveFile(source_file, m_target_capture_dir.string());

        if (!retrieve_file.ok())
        {
            std::cout << "Failed to retrieve gfxr capture: " << retrieve_file.message()
                      << std::endl;
            qDebug() << retrieve_file.message().data();
            emit ErrorMessage(QString::fromStdString(retrieve_file.message().data()));
            return;
        }

        if (Dive::IsGfxrFile(filename))
        {
            if (gfxr_stem.empty())
            {
                gfxr_stem = filename.stem().string();
            }
            gfxr_capture_file_path = target_path.string();
        }
        else if (Dive::IsPngFile(filename))
        {
            original_screenshot_path = target_path.string();
        }

        size += static_cast<qlonglong>(std::filesystem::file_size(target_path.string()));
        emit DownloadedSize(size, capture_directory_size);
    }

    if (!original_screenshot_path.empty() && !gfxr_stem.empty())
    {
        Dive::ComponentFilePaths component_files = {};
        {
            absl::StatusOr<Dive::ComponentFilePaths>
            ret = Dive::GetComponentFilesHostPaths(m_target_capture_dir, gfxr_stem);
            if (!ret.ok())
            {
                std::string err_msg = absl::StrFormat("Failed to get component files: %s",
                                                      ret.status().message());
                qDebug() << err_msg.c_str();
                return;
            }
            component_files = *ret;
        }

        std::error_code error_code;
        std::filesystem::rename(original_screenshot_path,
                                component_files.screenshot_png,
                                error_code);

        if (error_code)
        {
            qDebug() << "Failed to rename screenshot file from " << original_screenshot_path.c_str()
                     << " to " << component_files.screenshot_png.c_str() << ": "
                     << error_code.message().c_str();
        }
    }

    int64_t time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();
    qDebug() << "Time used to download the gfxr capture directory is "
             << (time_used_to_load_ms / 1000.0) << " seconds.";

    QString capture_saved_path(gfxr_capture_file_path.c_str());

    emit CaptureAvailable(capture_saved_path);
}
