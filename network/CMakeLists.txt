#
# Copyright 2025 Google LLC
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
#

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(network)

set(NETWORK_SRCS socket_connection.cc messages.cc tcp_client.cc)

set(NETWORK_HDRS socket_connection.h serializable.h messages.h tcp_client.h)

add_library(network SHARED ${NETWORK_SRCS} ${NETWORK_HDRS})

target_link_libraries(
  network
  PRIVATE
  absl::statusor
)

target_include_directories(network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})