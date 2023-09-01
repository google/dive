#!/bin/bash

# Copyright 2023 Google LLC
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
readonly BUILD_DIR=${PROJECT_ROOT}/build/grpc
readonly INSTALL_DIR=${PROJECT_ROOT}/grpc_bin

echo "PROJECT_ROOT " ${PROJECT_ROOT}
mkdir -p ${BUILD_DIR}
pushd ${BUILD_DIR}

cmake -DgRPC_BUILD_CODEGEN=ON \
    -DgRPC_BUILD_GRPC_CPP_PLUGIN=ON \
    -DgRPC_BUILD_CSHARP_EXT=OFF \
    -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
    -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
    -DgRPC_BUILD_TESTS=OFF \
    -Dprotobuf_WITH_ZLIB=OFF \
    -Dprotobuf_BUILD_TESTS=OFF \
    -DgRPC_INSTALL=ON \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
    -DABSL_ENABLE_INSTALL=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -G"Ninja" \
    ${PROJECT_ROOT}/third_party/grpc/
# cmake --build ${BUILD_DIR} --config=Release -j
# cmake --install .
ninja
ninja install

mkdir -p ${PROJECT_ROOT}/bin
cp ${INSTALL_DIR}/bin/protoc  ${PROJECT_ROOT}/bin/
cp ${INSTALL_DIR}/bin/grpc_cpp_plugin  ${PROJECT_ROOT}/bin/

popd