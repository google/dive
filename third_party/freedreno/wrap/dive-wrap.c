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

#include "dive-wrap.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>

// TODO: (renfeng) update the capture interface so that user can set this property value.
#define DIVE_MAX_BUFFER_SIZE_PROPERTY_NAME "debug.dive.pm4.max_buffer_size"
#define DIVE_REPLAY_PM4_CAPTURE_PROPERTY_NAME "debug.dive.replay.capture_pm4"
#define DIVE_REPLAY_PM4_CAPTURE_FILE_NAME_PROPERTY_NAME "debug.dive.replay.capture_pm4_file_name"

#define MIN_CAPTURE_BUFFER_SIZE 1 * 1024 * 1014  // 1M

static pthread_mutex_t capture_state_lock = PTHREAD_MUTEX_INITIALIZER;
static int             capture_state = 0;

void collect_trace_file(const char* capture_file_path);
int  IsCapturing()
{
    pthread_mutex_lock(&capture_state_lock);
    int is_capturing = (capture_state == 1);
    pthread_mutex_unlock(&capture_state_lock);
    return is_capturing;
}

void SetCaptureState(int state)
{
    LOGD("SetCaptureState %d", state);
    pthread_mutex_lock(&capture_state_lock);

    if (state == 0 && capture_state == 1)
    {
        char path[1024];
        if (IsGfrxReplayCapture())
        {

            char prop_str[PROP_VALUE_MAX];
            int  len = __system_property_get(DIVE_REPLAY_PM4_CAPTURE_FILE_NAME_PROPERTY_NAME,
                                            prop_str);

            if (len <= 0)
            {
                LOGI("Property %s not set.\n", DIVE_REPLAY_PM4_CAPTURE_FILE_NAME_PROPERTY_NAME);
                return;
            }
            LOGD("Property %s is set to %s\n",
                 DIVE_REPLAY_PM4_CAPTURE_FILE_NAME_PROPERTY_NAME,
                 prop_str);

            snprintf(path, 1024, "/sdcard/Download/%s", prop_str);
        }
        else
        {

            const char* num = getenv("TESTNUM");
            if (!num)
                num = "0";
            int frame_num = atoi(num);
            snprintf(path, 1024, "/sdcard/Download/trace-frame-%04u.rd", frame_num);
        }
        collect_trace_file(path);
    }

    capture_state = state;
    pthread_mutex_unlock(&capture_state_lock);
}

void StartCapture()
{
    SetCaptureState(1);
}

void StopCapture()
{
    SetCaptureState(0);
}

// If PROPERTY_NAME has been set, then set the max capture buffer size, otherwise do nothing, which
// means don't limit the buffer size.
void SetMaxCaptureBufferSize()
{
    char prop_str[PROP_VALUE_MAX];
    int  len = __system_property_get(DIVE_MAX_BUFFER_SIZE_PROPERTY_NAME, prop_str);

    if (len <= 0)
    {
        LOGI("Property %s not set.\n", DIVE_MAX_BUFFER_SIZE_PROPERTY_NAME);
        return;
    }

    LOGI("Value of %s: %s\n", DIVE_MAX_BUFFER_SIZE_PROPERTY_NAME, prop_str);

    int buffer_size = atoi(prop_str);
    if (buffer_size < MIN_CAPTURE_BUFFER_SIZE)
    {
        buffer_size = MIN_CAPTURE_BUFFER_SIZE;
        snprintf(prop_str, PROP_VALUE_MAX, "%d", buffer_size);
    }

    LOGI("Set WRAP_BUF_LEN_CAP to %s\n", prop_str);
    setenv("WRAP_BUF_LEN_CAP", prop_str, 1);
}

void SetCaptureName(const char* name, const char* frame_num)
{
    if (name)
    {
        setenv("TESTNAME", name, 1);
    }
    if (frame_num)
    {
        setenv("TESTNUM", frame_num, 1);
    }

    SetMaxCaptureBufferSize();
}

// Returns 1 if the property is set to "1" or "true", 0 otherwise.
int IsGfrxReplayCapture()
{
    char prop_str[PROP_VALUE_MAX];
    int  len = __system_property_get(DIVE_REPLAY_PM4_CAPTURE_PROPERTY_NAME, prop_str);

    if (len <= 0)
    {
        LOGI("Property %s not set.\n", DIVE_REPLAY_PM4_CAPTURE_PROPERTY_NAME);
        return 0;
    }

    LOGD("Property %s is set to %s\n", DIVE_REPLAY_PM4_CAPTURE_PROPERTY_NAME, prop_str);

    if (strcmp("1", prop_str) == 0 || strcmp("true", prop_str) == 0)
    {
        return 1;
    }

    return 0;
}

void SetCaptureFileNameByProperty()
{
    char prop_str[PROP_VALUE_MAX];
    int  len = __system_property_get(DIVE_REPLAY_PM4_CAPTURE_FILE_NAME_PROPERTY_NAME, prop_str);

    if (len <= 0)
    {
        LOGI("Property %s not set.\n", DIVE_REPLAY_PM4_CAPTURE_FILE_NAME_PROPERTY_NAME);
        return;
    }
    LOGD("Property %s is set to %s\n", DIVE_REPLAY_PM4_CAPTURE_FILE_NAME_PROPERTY_NAME, prop_str);

    char full_path[256];
    sprintf(full_path, "/sdcard/Download/%s", prop_str);

    SetCaptureName(full_path, "1");
}