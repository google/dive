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

namespace Dive
{

enum class ApplicationType
{
    OPENXR,
    VULKAN,
    GLES,
};

class AndroidDevice;

class AndroidApplication
{
public:
    AndroidApplication(AndroidDevice &dev, std::string package, ApplicationType type);
    virtual ~AndroidApplication() = default;

    virtual void Setup() = 0;
    virtual void Cleanup() = 0;
    void         Start();
    void         Stop();
    std::string  GetMainActivity();

protected:
    AndroidDevice  &m_dev;
    std::string     m_package;
    ApplicationType m_type;
    std::string     m_main_activity;
    bool            m_started;
};

class VulkanApplication : public AndroidApplication
{
public:
    VulkanApplication(AndroidDevice &dev, std::string package, ApplicationType type) :
        AndroidApplication(dev, package, type)
    {
        Setup();
    };
    virtual ~VulkanApplication();
    virtual void Setup() override;
    virtual void Cleanup() override;
};

class OpenXRApplication : public AndroidApplication
{
public:
    OpenXRApplication(AndroidDevice &dev, std::string package, ApplicationType type) :
        AndroidApplication(dev, package, type)
    {
        Setup();
    };
    virtual ~OpenXRApplication();
    virtual void Setup() override;
    virtual void Cleanup() override;
};

}  // namespace Dive