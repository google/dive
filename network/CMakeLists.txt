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

# Define source files for the client library.
set(NETWORK_CLIENT_SRCS socket_connection.cc)

# Define header files for the client library.
set(NETWORK_CLIENT_HDRS socket_connection.h)

# Create the client library.
add_library(network_client SHARED ${NETWORK_CLIENT_SRCS} ${NETWORK_CLIENT_HDRS})

# Set include directories for the client library.
target_include_directories(network_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Define source files for the server library.
set(NETWORK_SERVER_SRCS socket_connection.cc)

# Define header files for the server library.
set(NETWORK_SERVER_HDRS socket_connection.h)

# Create the server library.
add_library(network_server SHARED ${NETWORK_SERVER_SRCS} ${NETWORK_SERVER_HDRS})

# Set include directories for the server library.
target_include_directories(network_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})