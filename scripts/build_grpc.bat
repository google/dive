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


set PROJECT_ROOT=%~dp0\..
echo "PROJECT_ROOT: " %PROJECT_ROOT%
set BUILD_DIR= %PROJECT_ROOT%\\build\\grpc
echo "BUILD_DIR: " %BUILD_DIR%
set SRC_DIR= %PROJECT_ROOT%\\third_party\\grpc

mkdir -p %BUILD_DIR%

pushd %BUILD_DIR%
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
    -Dprotobuf_WITH_ZLIB=OFF ^
    -Dprotobuf_BUILD_TESTS=OFF ^
    -DCMAKE_INSTALL_PREFIX="%BUILD_DIR%\\grpc_bin"  ^
    -DCMAKE_BUILD_TYPE=Release ^
    %SRC_DIR% ^
    -B%BUILD_DIR%
cmake --build %BUILD_DIR% --config=Release --parallel %NUMBER_OF_PROCESSORS% -- /p:CL_MPcount=%NUMBER_OF_PROCESSORS%
cmake --install .

copy %BUILD_DIR%\\Release\\grpc_cpp_plugin.exe %PROJECT_ROOT%\\bin\\
copy %BUILD_DIR%\\third_party\\protobuf\\Release\\protoc.exe %PROJECT_ROOT%\\bin\\

popd