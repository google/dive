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
set SRC_DIR= %PROJECT_ROOT%\\third_party\\grpc
set INSTALL_ROOT=%PROJECT_ROOT%\\prebuild
set BUILD_TYPE=Debug Release
set startTime=%time%


(for %%b in (%BUILD_TYPE%) do (
    setlocal enabledelayedexpansion
    set build_type=%%b
    echo "build type is " !build_type!

    set BUILD_DIR= %PROJECT_ROOT%\\build\\grpc\\!build_type!
    set INSTALL_DIR= %INSTALL_ROOT%\\grpc\\!build_type!
    md !BUILD_DIR!
    echo "BUILD_DIR: " !BUILD_DIR!
    echo "INSTALL_DIR: " !INSTALL_DIR!
    pushd !BUILD_DIR!

    pwd
    cmake -DgRPC_BUILD_CODEGEN=ON ^
        -DgRPC_BUILD_GRPC_CPP_PLUGIN=ON ^
        -DgRPC_BUILD_CSHARP_EXT=OFF ^
        -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF ^
        -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF ^
        -DgRPC_BUILD_TESTS=OFF ^
        -DgRPC_INSTALL=ON ^
        -Dprotobuf_WITH_ZLIB=OFF ^
        -Dprotobuf_BUILD_TESTS=OFF ^
        -Dprotobuf_INSTALL=ON ^
        -DABSL_PROPAGATE_CXX_STD=ON ^
        -DABSL_ENABLE_INSTALL=ON ^
        -DCMAKE_CXX_STANDARD=17 ^
        -DCMAKE_BUILD_TYPE=!build_type! ^
        %SRC_DIR% ^
        -D_WIN32_WINNT=0x600 ^
        -G "Visual Studio 16 2019" -A x64

    cmake --build . --config=!build_type!  --parallel %NUMBER_OF_PROCESSORS% -- /p:CL_MPcount=%NUMBER_OF_PROCESSORS%
    cmake --install . --config=!build_type! --prefix="!INSTALL_DIR!"  

    @REM copy %BUILD_DIR%\\Release\\grpc_cpp_plugin.exe %PROJECT_ROOT%\\bin\\
    @REM copy %BUILD_DIR%\\third_party\\protobuf\\Release\\protoc.exe %PROJECT_ROOT%\\bin\\
))

popd

pushd %INSTALL_ROOT%\\grpc\\Debug\\lib
tar -zcvf grpc.tar.gz grpc.lib
tar -zcvf grpc_unsecure.tar.gz grpc_unsecure.lib
tar -zcvf grpc_authorization_provider.tar.gz grpc_authorization_provider.lib
tar -zcvf libprotocd.tar.gz libprotocd.lib
tar -zcvf libprotobufd.tar.gz libprotobufd.lib

del grpc.lib
del grpc_unsecure.lib
del grpc_authorization_provider.lib
del libprotocd.lib
del libprotobufd.lib
popd

echo Start Time: %startTime%
echo Finish Time: %time%