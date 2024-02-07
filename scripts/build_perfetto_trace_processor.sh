#!/bin/bash

# Copyright 2024 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

PROJECT_ROOT="$(readlink -f $0)"
PROJECT_ROOT="${PROJECT_ROOT%/*}/.."
readonly PROJECT_ROOT="$(readlink -f ${PROJECT_ROOT})"
readonly BUILD_DIR=${PROJECT_ROOT}/build/perfetto_trace_processor
readonly INSTALL_DIR=${PROJECT_ROOT}/prebuild/perfetto/linux
readonly PERFETTO_SRC=${PROJECT_ROOT}/third_party/perfetto

mkdir -p ${INSTALL_DIR}
mkdir -p ${BUILD_DIR}
pushd ${PERFETTO_SRC}
echo `pwd`

${PERFETTO_SRC}/tools/install-build-deps
${PERFETTO_SRC}/tools/gn clean ${BUILD_DIR}/release
${PERFETTO_SRC}/tools/gn gen ${BUILD_DIR}/release --args="\
    enable_perfetto_trace_processor=true \
    enable_perfetto_trace_processor_json=true \
    enable_perfetto_trace_processor_linenoise=true \
    enable_perfetto_trace_processor_sqlite=true \
    enable_perfetto_trace_processor_percentile=true \
    target_os = \"linux\" \
    target_cpu = \"x64\" \
    is_debug = false \
    use_custom_libcxx = false \
    custom_libcxx_is_static = false \
    is_hermetic_clang=false \
    is_system_compiler=true \
    is_clang=false \
    skip_buildtools_check=true \
"

${PERFETTO_SRC}/tools/ninja -C ${BUILD_DIR}/release src/trace_processor:trace_processor
cp ${BUILD_DIR}/release/libtrace_processor.a ${INSTALL_DIR}
pushd ${INSTALL_DIR}
tar -Jcvf libtrace_processor.a.tar.xz libtrace_processor.a
popd
cp ${BUILD_DIR}/release/gen/build_config/perfetto_build_flags.h ${INSTALL_DIR}
popd
