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
    AndroidApplication(AndroidDevice &dev, std::string package, ApplicationType type);
    virtual ~AndroidApplication() = default;

    virtual absl::Status Setup() = 0;
    virtual absl::Status Cleanup() = 0;
    virtual absl::Status Start();
    virtual absl::Status Stop();
    const std::string   &GetMainActivity() const { return m_main_activity; };
    bool                 IsDebuggable() const { return m_is_debuggable; }
    bool                 IsRunning() const { return m_started; }

protected:
    absl::Status ParsePackage();

    AndroidDevice  &m_dev;
    std::string     m_package;
    ApplicationType m_type;
    std::string     m_main_activity;
    bool            m_is_debuggable;
    bool            m_started;
};

class VulkanApplication : public AndroidApplication
{
public:
    VulkanApplication(AndroidDevice &dev, std::string package) :
        AndroidApplication(dev, package, ApplicationType::VULKAN_APK)
    {
        ParsePackage().IgnoreError();
    };
    virtual ~VulkanApplication();
    virtual absl::Status Setup() override;
    virtual absl::Status Cleanup() override;
};

class OpenXRApplication : public AndroidApplication
{
public:
    OpenXRApplication(AndroidDevice &dev, std::string package) :
        AndroidApplication(dev, package, ApplicationType::OPENXR_APK)
    {
        ParsePackage().IgnoreError();
    };
    virtual ~OpenXRApplication();
    virtual absl::Status Setup() override;
    virtual absl::Status Cleanup() override;
};

class VulkanCliApplication : public AndroidApplication
{
public:
    VulkanCliApplication(AndroidDevice &dev, std::string binary, std::string args) :
        AndroidApplication(dev, "", ApplicationType::VULKAN_CLI),
        m_binary(std::move(binary)),
        m_args(std::move(args))
    {
    }
    virtual ~VulkanCliApplication();
    virtual absl::Status Setup() override;
    virtual absl::Status Cleanup() override;
    virtual absl::Status Start() override;
    virtual absl::Status Stop() override;

private:
    std::string m_binary;
    std::string m_args;
    std::string m_pid;
};

}  // namespace Dive