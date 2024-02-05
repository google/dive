:: Copyright 2023 Google LLC
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::      https://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.


@echo off
set PROJECT_ROOT=%~dp0\..
echo "PROJECT_ROOT: " %PROJECT_ROOT%
set PERFETTO_SRC= %PROJECT_ROOT%\third_party\perfetto
set INSTALL_DIR=%PROJECT_ROOT%\prebuild\perfetto\windows
set BUILD_DIR= %PROJECT_ROOT%\build\perfetto_trace_processor
set startTime=%time%
md %BUILD_DIR%
md %INSTALL_DIR%
echo "BUILD_DIR: " %BUILD_DIR%
echo "INSTALL_DIR: " %INSTALL_DIR%
echo "PERFETTO_SRC: " %PERFETTO_SRC%
pushd %PERFETTO_SRC%

where python
python %PERFETTO_SRC%\\tools\\install-build-deps
python %PERFETTO_SRC%\\tools\\gn clean %BUILD_DIR%\\release
python %PERFETTO_SRC%\\tools\\gn gen %BUILD_DIR%\\release --args="is_debug = false enable_perfetto_trace_processor=true enable_perfetto_trace_processor_json=true enable_perfetto_trace_processor_linenoise=true enable_perfetto_trace_processor_sqlite=true enable_perfetto_trace_processor_percentile=true use_custom_libcxx = false custom_libcxx_is_static = false is_hermetic_clang=false is_system_compiler=true is_clang=false skip_buildtools_check=true extra_cflags=\"/wd4146 /wd4369\""

python %PERFETTO_SRC%\\tools\\ninja -C %BUILD_DIR%\\release src/trace_processor:trace_processor
cp %BUILD_DIR%\\release\\trace_processor.lib %INSTALL_DIR%
pushd %INSTALL_DIR%
tar -zcvf trace_processor.lib.tar.gz trace_processor.lib
popd
cp %BUILD_DIR%\\release\\gen\\build_config\\perfetto_build_flags.h %INSTALL_DIR%\
popd

echo Start Time: %startTime%
echo Finish Time: %time%