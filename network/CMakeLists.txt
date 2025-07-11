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

set(NETWORK_SRCS
  socket_connection.cc
  messages.cc
  tcp_client.cc
  unix_domain_server.cc
)

set(NETWORK_HDRS
  platform_net.h
  socket_connection.h
  serializable.h
  messages.h
  tcp_client.h
  message_handler.h
  unix_domain_server.h
)

add_library(network STATIC ${NETWORK_SRCS} ${NETWORK_HDRS})

set(NETWORK_LINK_LIBS
  absl::status
  absl::statusor
)

if(ANDROID)
  list(APPEND NETWORK_LINK_LIBS log)
endif()

target_link_libraries(
  network
  PRIVATE
  ${NETWORK_LINK_LIBS}
)

target_include_directories(network PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(NOT ANDROID)
  enable_testing()
  include(GoogleTest)

  add_executable(messages_test messages_test.cc)
  target_link_libraries(messages_test PRIVATE
    network
    gtest
    gtest_main
    absl::status
    absl::statusor
    absl::status_matchers
  )
  gtest_discover_tests(messages_test)

  if(NOT WIN32)
    add_executable(unix_domain_server_test unix_domain_server_test.cc)
    target_link_libraries(unix_domain_server_test PRIVATE
      network
      gtest
      gtest_main
      absl::status
      absl::statusor
    )
    add_test(NAME unix_domain_server_test COMMAND unix_domain_server_test)
    set_tests_properties(unix_domain_server_test PROPERTIES TIMEOUT 2)

    add_executable(tcp_client_test tcp_client_test.cc)
    target_link_libraries(tcp_client_test PRIVATE
      network
      gtest
      gtest_main
      absl::status
      absl::statusor
    )
    add_test(NAME tcp_client_test COMMAND tcp_client_test)
    set_tests_properties(tcp_client_test PROPERTIES TIMEOUT 2)
  endif()
endif()