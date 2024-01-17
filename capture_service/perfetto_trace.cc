/*
Copyright 2024 Google Inc.

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

#include "perfetto_trace.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <perfetto.h>
#include <chrono>
#include <thread>

#include "log.h"

PERFETTO_DEFINE_CATEGORIES(
perfetto::Category("gpu_renderstage").SetDescription("Rendering and graphics events"));

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

namespace Dive
{

void PerfettoTraceManager::InitializePerfetto()
{
    if (m_initalized)
        return;
    LOGI("In InitializePerfetto \n");
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kSystemBackend;
    perfetto::Tracing::Initialize(args);
    bool registered = perfetto::TrackEvent::Register();
    LOGI("Registered %d \n", registered);
    m_initalized = true;
}

void PerfettoTraceManager::StartNewSession(const std::string &trace_file_name)
{
    InitializePerfetto();
    LOGI("Begin schedule work\n");
    m_tracing_worker.Schedule([trace_file_name] {
        LOGI("In StartTracing \n");
        perfetto::TraceConfig cfg;
        auto                 *buffers = cfg.add_buffers();
        cfg.set_flush_period_ms(1000);
        buffers->set_size_kb(131072);
        buffers->set_fill_policy(
        perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy_RING_BUFFER);

        auto *ds_cfg_render = cfg.add_data_sources()->mutable_config();
        ds_cfg_render->set_name("gpu.renderstages");
        cfg.set_duration_ms(8000);

        int fd = open(trace_file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd <= 0)
        {
            LOGE("Failed to create perfetto trace file\n");
        }

        auto tracing_session = perfetto::Tracing::NewTrace();
        tracing_session->Setup(cfg, fd);
        LOGI("StartBlocking \n");
        tracing_session->StartBlocking();

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        LOGI("StopBlocking \n");
        tracing_session->StopBlocking();
        close(fd);
        LOGI("session ended \n");
    });

    LOGI("End schedule work\n");
}

PerfettoTraceManager &GetPerfettoMgr()
{
    static PerfettoTraceManager mgr;
    return mgr;
}

}  // namespace Dive