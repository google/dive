/*
Copyright 2023 Google Inc.

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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "android_application.h"
#include "command_utils.h"
#include "constants.h"

#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Dive
{

struct DeviceInfo
{
    std::string m_serial;
    std::string m_manufacturer;
    std::string m_model;
    bool        m_is_adreno_gpu = false;

    std::string GetDisplayName() const;
};

inline bool operator==(const DeviceInfo &lhs, const DeviceInfo &rhs)
{
    return lhs.m_serial == rhs.m_serial && lhs.m_manufacturer == rhs.m_manufacturer &&
           lhs.m_model == rhs.m_model && lhs.m_is_adreno_gpu == rhs.m_is_adreno_gpu;
}

inline bool operator!=(const DeviceInfo &lhs, const DeviceInfo &rhs)
{
    return !(lhs == rhs);
}

struct DeviceState
{
    std::string m_enforce;
    bool        m_root_access_requested = false;
    bool        m_is_root_shell = false;
};

enum class GfxrReplayOptions
{
    kNormal,
    kPm4Dump,       // PM4 data will be captured, producing .rd trace
    kPerfCounters,  // Perf Counter data will be collected using a plugin, producing .csv artifact
    kGpuTiming,     // GPU timing data will be collected, producing .csv artifact
    kRenderDoc,     // RenderDoc capture will be collected, producing .rdc file
};

struct GfxrReplaySettings
{
    std::string       remote_capture_path = "";
    std::string       local_download_dir = "";
    GfxrReplayOptions run_type = GfxrReplayOptions::kNormal;

    // ----------------------------------------------------------------------
    // NOTE: If conflicting flags/settings are provided, early termination occurs.

    // Flags intended to be passed down to GFXR replay binary
    // Flags must be provided with a space (not '=') between flag and value
    std::string replay_flags_str = "";

    // ----------------------------------------------------------------------
    // Additional runtype-specific settings

    // Metrics are collected only with kPerfCounters runs
    std::vector<std::string> metrics = {};
    // Loop settings for GFXR replay binary are hardcoded except for kNormal and kGpuTiming runs.
    // Can also be set by providing --loop-single-frame-count in replay_flags_str and calling
    // ValidateGfxrReplaySettings.
    std::optional<int> loop_single_frame_count = std::nullopt;
};

// Ensures that replay_flags_str is consistent with the other provided settings, and validates
// the entire configuration
absl::StatusOr<GfxrReplaySettings> ValidateGfxrReplaySettings(const GfxrReplaySettings &settings,
                                                              bool is_adreno_gpu);

class AndroidDevice
{
public:
    explicit AndroidDevice(const std::string &serial);
    ~AndroidDevice();

    AndroidDevice &operator=(const AndroidDevice &) = delete;
    AndroidDevice(const AndroidDevice &) = delete;

    absl::Status Init();
    absl::Status ForwardFirstAvailablePort();

    // Only legacy PM4 capture should request root access. Anything related to GFXR doesn't require
    // root access thus should not request it. Will remove the requirement of need root access once
    // we fully transit to using GFXR replay.
    absl::Status RequestRootAccess();
    absl::Status SetupDevice();
    absl::Status CleanupDevice();
    absl::Status CleanupPackage(const std::string &package);
    void         EnableGfxr(bool enable_gfxr);
    bool         IsProcessRunning(absl::string_view process_name) const;
    bool         FileExists(const std::string &file_path);

    enum class PackageListOptions
    {
        kAll,
        kDebuggableOnly,
        kNonDebuggableOnly,
    };

    absl::StatusOr<std::vector<std::string>> ListPackage(
    PackageListOptions option = PackageListOptions::kAll) const;
    std::string  GetDeviceDisplayName() const;
    absl::Status SetupApp(const std::string    &package,
                          const ApplicationType type,
                          const std::string    &command_args,
                          const std::string    &device_architecture,
                          const std::string    &gfxr_capture_directory);
    absl::Status SetupApp(const std::string    &binary,
                          const std::string    &args,
                          const ApplicationType type);

    absl::Status      CleanupApp();
    absl::Status      StartApp();
    absl::Status      StopApp();
    const AdbSession &Adb() const { return m_adb; }
    AdbSession       &Adb() { return m_adb; }
    int               Port() const { return m_port; }
    bool              IsAdrenoGpu() const { return m_dev_info.m_is_adreno_gpu; }

    AndroidApplication *GetCurrentApplication() { return m_app.get(); }

    // Fetches file at remote_file_path to local_save_dir and deletes the remote file after
    // If new_file_name specified, local file is renamed
    // If delete_after_retrieve is false, remote file is not deleted
    absl::Status RetrieveFile(const std::string &remote_file_path,
                              const std::string &local_save_dir,
                              bool               delete_after_retrieve = true,
                              const std::string &new_file_name = "");

    // Pins GPU clock to freq_mhz [MHz]
    absl::Status             PinGpuClock(uint32_t freq_mhz) const;
    absl::Status             UnpinGpuClock() const;
    absl::StatusOr<uint32_t> GetGpuFrequency() const;
    // Checks if the current GPU clock frequency is expected_freq_mhz [MHz]
    absl::Status IsGpuClockPinned(uint32_t expected_freq_mhz) const;

    // Triggers a screenshot and saves it to the specified path.
    absl::Status TriggerScreenCapture(const std::filesystem::path &on_device_screenshot_dir);

private:
    const std::string                   m_serial;
    DeviceInfo                          m_dev_info;
    AdbSession                          m_adb;
    DeviceState                         m_original_state;
    std::unique_ptr<AndroidApplication> m_app;
    bool                                m_gfxr_enabled = false;
    int                                 m_port = kFirstPort;
};

class DeviceManager
{
public:
    DeviceManager() = default;
    DeviceManager &operator=(const DeviceManager &) = delete;
    DeviceManager(const DeviceManager &) = delete;

    std::vector<DeviceInfo>         ListDevice() const;
    absl::StatusOr<AndroidDevice *> SelectDevice(const std::string &serial);
    void                            RemoveDevice() { m_device = nullptr; }
    AndroidDevice                  *GetDevice() const { return m_device.get(); }
    absl::Status                    Cleanup(const std::string &serial, const std::string &package);

    absl::Status DeployReplayApk(const std::string &serial);
    absl::Status RunReplayApk(const GfxrReplaySettings &settings) const;

private:
    // Initiates GFXR replay through the GFXR-provided python script, blocking call
    absl::Status RunReplayGfxrScript(const GfxrReplaySettings &settings) const;
    // Initiates GFXR replay through the profiling plugin, blocking call
    absl::Status RunReplayProfilingBinary(const GfxrReplaySettings &settings) const;

    std::unique_ptr<AndroidDevice> m_device{ nullptr };
};

std::filesystem::path ResolveAndroidLibPath(const std::string &name,
                                            const std::string &device_architecture);

DeviceManager &GetDeviceManager();
}  // namespace Dive