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
#include <cstdint>
#include <string>
#include <thread>

#include "log.h"

PERFETTO_DEFINE_CATEGORIES(
perfetto::Category("dive").SetDescription("Dive layer events"),
perfetto::Category("gpu_renderstage").SetDescription("Rendering and graphics events"));

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

namespace Dive
{

namespace
{
// TODO(renfeng): determine how long should be the trace, and pass in the value here.
static constexpr int32_t kTraceDurationMs = 5000;
static constexpr int32_t kFlushPeriodMs = 1000;
static constexpr int32_t kBufferSize = 131072;  // 128k
}  // namespace

// Example code from perfetto sdk:
// https://github.com/google/perfetto/blob/master/examples/sdk/example_system_wide.cc#L67
class PerfettoSessionObserver : public perfetto::TrackEventSessionObserver
{
public:
    PerfettoSessionObserver() { perfetto::TrackEvent::AddSessionObserver(this); }
    ~PerfettoSessionObserver() override { perfetto::TrackEvent::RemoveSessionObserver(this); }

    void OnStart(const perfetto::DataSourceBase::StartArgs &) override
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.notify_one();
    }

    void WaitForTracingStart()
    {
        PERFETTO_LOG("Waiting for tracing to start...");
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [] { return perfetto::TrackEvent::IsEnabled(); });
        PERFETTO_LOG("Tracing started");
    }

    std::mutex              mutex;
    std::condition_variable cv;
};

PerfettoTraceManager::PerfettoTraceManager()
{
    InitializePerfetto();
}

void PerfettoTraceManager::InitializePerfetto()
{
    // TODO(renfeng): setprop debug.graphics.gpu.profiler.perfetto 1
    LOGI("In InitializePerfetto \n");
    perfetto::TracingInitArgs args;
    args.backends = perfetto::kSystemBackend;
    args.backends |= perfetto::kInProcessBackend;

    perfetto::Tracing::Initialize(args);

    bool registered = perfetto::TrackEvent::Register();
    LOGI("TrackEvent Registered %d \n", registered);
}

void PerfettoTraceManager::StartNewSession(const std::string &trace_file_name)
{
    LOGI("Begin schedule trace session\n");
    m_tracing_worker.Schedule([trace_file_name] {
        LOGI("In StartTracing \n");
        perfetto::TraceConfig cfg;
        cfg.set_duration_ms(kTraceDurationMs);
        cfg.set_flush_period_ms(kFlushPeriodMs);

        auto *buffers = cfg.add_buffers();
        buffers->set_size_kb(kBufferSize);
        buffers->set_fill_policy(
        perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy_RING_BUFFER);

        perfetto::protos::gen::TrackEventConfig te_cfg;
        te_cfg.add_disabled_categories("*");
        te_cfg.add_enabled_categories("dive");

        auto *ds_cfg = cfg.add_data_sources()->mutable_config();
        ds_cfg->set_name("track_event");
        ds_cfg->set_track_event_config_raw(te_cfg.SerializeAsString());

        auto *ds_cfg_render = cfg.add_data_sources()->mutable_config();
        ds_cfg_render->set_name("gpu.renderstages");

        int fd = open(trace_file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (fd <= 0)
        {
            LOGE("Failed to create perfetto trace file\n");
        }

        auto tracing_session = perfetto::Tracing::NewTrace();
        tracing_session->Setup(cfg, fd);
        LOGI("StartBlocking \n");
        tracing_session->StartBlocking();

        std::this_thread::sleep_for(std::chrono::milliseconds(kTraceDurationMs));
        LOGI("StopBlocking \n");
        perfetto::TrackEvent::Flush();
        tracing_session->StopBlocking();
        close(fd);
        LOGI("Session ended \n");
    });

    perfetto::ProcessTrack                 process_track = perfetto::ProcessTrack::Current();
    perfetto::protos::gen::TrackDescriptor desc = process_track.Serialize();
    desc.mutable_process()->set_process_name("Dive");
    perfetto::TrackEvent::SetTrackDescriptor(process_track, desc);

    LOGI("End schedule work\n");
    WaitForSessionStart();
}

void PerfettoTraceManager::TraceStartFrame()
{
    PERFETTO_LOG("TRACE_EVENT_BEGIN dive start tracing");
    TRACE_EVENT_BEGIN("dive", "Dive trace");
}

void PerfettoTraceManager::TraceEndFrame()
{
    PERFETTO_LOG("TRACE_EVENT_END dive end tracing");
    TRACE_EVENT_END("dive");
}

void PerfettoTraceManager::TraceFrame(uint32_t frame_num)
{
    std::string event = "Frame " + std::to_string(frame_num);
    TRACE_EVENT("dive", event.c_str());
}

void PerfettoTraceManager::WaitForSessionStart()
{
    PerfettoSessionObserver session_observer;
    session_observer.WaitForTracingStart();
    PERFETTO_LOG("perfetto::TrackEvent::IsEnabled() %d", perfetto::TrackEvent::IsEnabled());
}

PerfettoTraceManager &GetPerfettoMgr()
{
    static PerfettoTraceManager mgr;
    return mgr;
}

}  // namespace Dive