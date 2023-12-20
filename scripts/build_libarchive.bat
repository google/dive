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
set SRC_DIR= %PROJECT_ROOT%\\third_party\\libarchive
set INSTALL_ROOT=%PROJECT_ROOT%\\prebuild
set BUILD_TYPE=Debug Release
set startTime=%time%


(for %%b in (%BUILD_TYPE%) do (
    setlocal enabledelayedexpansion
    set build_type=%%b
    echo "build type is " !build_type!

    set BUILD_DIR= %PROJECT_ROOT%\\build\\libarchive\\!build_type!
    set INSTALL_DIR= %INSTALL_ROOT%\\libarchive\\
    md !BUILD_DIR!
    echo "BUILD_DIR: " !BUILD_DIR!
    echo "INSTALL_DIR: " !INSTALL_DIR!
    pushd !BUILD_DIR!

    pwd
    cmake -DENABLE_TEST=OFF ^
        -DLIBARCHIVE_STATIC=ON ^
        -DBUILD_SHARED_LIBS=OFF ^
        -DENABLE_OPENSSL=OFF ^
        -DENABLE_LIBB2=OFF ^
        -DENABLE_LZ4=OFF ^
        -DENABLE_LZMA=OFF ^
        -DENABLE_ZSTD=OFF ^
        -DENABLE_BZip2=OFF ^
        -DENABLE_CNG=OFF ^
        -DENABLE_TAR=OFF ^
        -DENABLE_CPIO=OFF ^
        -DENABLE_CAT=OFF ^
        -DENABLE_ACL=OFF ^
        -DENABLE_INSTALL=ON ^
        -DCMAKE_CXX_STANDARD=17 ^
        -DCMAKE_BUILD_TYPE=!build_type! ^
        %SRC_DIR% ^
        -DCMAKE_DEBUG_POSTFIX=d ^
        -G "Visual Studio 16 2019" -A x64

    cmake --build . --config=!build_type!  
    cmake --install . --config=!build_type! --prefix="!INSTALL_DIR!"  
))

popd


echo Start Time: %startTime%
echo Finish Time: %time%