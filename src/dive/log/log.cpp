/*
Copyright 2026 Google Inc.

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

#include "dive/log/log.h"

#include <string>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"
#include "absl/status/status.h"
#include "dive/log/android_log_sink.h"

namespace Dive
{

void AbslLogger::Init(std::string_view android_tag)
{
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
    absl::InitializeLog();

    if (!android_tag.empty())
    {
        m_android_log_sink.SetAndroidTag(android_tag);
    }

    absl::AddLogSink(&m_android_log_sink);

    if (android_tag.empty())
    {
        LOG(INFO) << "AbslLogger initialized with no android tag, using default:"
                  << m_android_log_sink.GetAndroidTag();
    }
    else
    {
        LOG(INFO) << "AbslLogger initialized with android tag: "
                  << m_android_log_sink.GetAndroidTag();
    }
}

}  // namespace Dive
