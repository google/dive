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

#include <string>
#include "absl/status/status.h"
#include "absl/strings/string_view.h"

namespace Dive
{

enum class ApplicationType
{
    OPENXR_APK,  // OPENXR app in apk format
    VULKAN_APK,  // VUlkan app in apk format
    GLES_APK,    // GLES app in apk format
    VULKAN_CLI,  // Vulkan command line application
    UNKNOWN,
};

class AndroidDevice;

class AndroidApplication
{
public:
    AndroidApplication(AndroidDevice  &dev,
                       std::string     package,
                       ApplicationType type,
                       std::string     command_args);
    virtual ~AndroidApplication() = default;

    virtual absl::Status Setup() = 0;

    // Common cleanup for device properties and settings that may be touched by Dive when running an
    // application
    virtual absl::Status Cleanup();

    virtual absl::Status Start();
    virtual absl::Status Stop();
    const std::string   &GetMainActivity() const { return m_main_activity; };
    bool                 IsDebuggable() const { return m_is_debuggable; }
    bool                 IsStarted() const { return m_started; }
    virtual bool         IsRunning() const;
    void                 SetGfxrEnabled(bool enable);
    void SetArchitecture(const std::string &architecture) { m_device_architecture = architecture; };
    void SetGfxrCaptureFileDirectory(const std::string &capture_file_directory)
    {
        m_gfxr_capture_file_directory = capture_file_directory;
    };
    absl::Status CreateGfxrDirectory(const std::string command_args);
    virtual absl::Status GfxrSetup();
    absl::Status HasInternetPermission();
    absl::Status GrantAllFilesAccess();

protected:
    absl::Status ParsePackage();

    AndroidDevice  &m_dev;
    std::string     m_package;
    ApplicationType m_type;
    std::string     m_main_activity;
    std::string     m_command_args;
    // Available architectures are arm64-v8, armeabi-v7a, x86, and x86_64.
    std::string m_device_architecture;
    std::string m_gfxr_capture_file_directory;
    bool        m_is_debuggable;
    bool        m_started;

    bool m_gfxr_enabled;
};

class VulkanApplication : public AndroidApplication
{
public:
    VulkanApplication(AndroidDevice &dev, std::string package, std::string command_args);
    virtual ~VulkanApplication();
    virtual absl::Status Setup() override;

    // Cleanup for device properties and settings related to a Vulkan APK
    virtual absl::Status Cleanup() override;
};

class OpenXRApplication : public AndroidApplication
{
public:
    OpenXRApplication(AndroidDevice &dev, std::string package, std::string command_args);
    virtual ~OpenXRApplication();
    virtual absl::Status Setup() override;

    // Cleanup for device properties and settings related to an OpenXR APK
    virtual absl::Status Cleanup() override;
};

class VulkanCliApplication : public AndroidApplication
{
public:
    VulkanCliApplication(AndroidDevice &dev, std::string command, std::string command_args);
    virtual ~VulkanCliApplication();
    virtual absl::Status Setup() override;

    // Cleanup for device properties and settings related to a Vulkan CLI application
    virtual absl::Status Cleanup() override;

    virtual absl::Status Start() override;
    virtual absl::Status Stop() override;
    virtual bool         IsRunning() const override;
    absl::Status GfxrSetup() override;

private:
    std::string m_command;
    std::string m_pid;
};

}  // namespace Dive