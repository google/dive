#!/bin/bash

find \
    capture_service/ cli/ dive_core/ gfxr_dump_resources/ gfxr_ext/ \
    gpu_time/ host_cli/ layer/ lrz_validator/ network/ plugins/ \
    runtime_layer/ src/ trace_stats/ ui/ \
    -iname '*.hpp' -o -iname '*.h' -o -iname '*.cpp' -o -iname '*.cc' \
    | xargs clang-format-21 -i
find \
    capture_service/ cli/ dive_core/ gfxr_dump_resources/ gfxr_ext/ \
    gpu_time/ host_cli/ layer/ lrz_validator/ network/ plugins/ \
    runtime_layer/ src/ trace_stats/ ui/ \
    -iname '*.hpp' -o -iname '*.h' -o -iname '*.cpp' -o -iname '*.cc' \
    | xargs clang-format-18 -i
find \
    capture_service/ cli/ dive_core/ gfxr_dump_resources/ gfxr_ext/ \
    gpu_time/ host_cli/ layer/ lrz_validator/ network/ plugins/ \
    runtime_layer/ src/ trace_stats/ ui/ \
    -iname '*.hpp' -o -iname '*.h' -o -iname '*.cpp' -o -iname '*.cc' \
    | xargs clang-format-18 -i
