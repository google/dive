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

project(DiveService)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Proto file
get_filename_component(dive_service_proto "./protos/dive_service.proto" ABSOLUTE)
get_filename_component(dive_proto_path "${dive_service_proto}" PATH)

# Generated sources
set(dive_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/dive_service.pb.cc")
set(dive_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/dive_service.pb.h")
set(dive_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/dive_service.grpc.pb.cc")
set(dive_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/dive_service.grpc.pb.h")
add_custom_command(
      OUTPUT "${dive_proto_srcs}" "${dive_proto_hdrs}" "${dive_grpc_srcs}" "${dive_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${dive_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${dive_service_proto}"
      DEPENDS "${dive_service_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}"
"${CMAKE_SOURCE_DIR}/third_party/grpc/include"
"${CMAKE_SOURCE_DIR}/third_party/grpc/third_party/protobuf/src/"
"${CMAKE_SOURCE_DIR}/third_party/grpc/third_party/abseil-cpp/"
)

add_library(dive_grpc_proto
              ${dive_grpc_srcs}
              ${dive_grpc_hdrs}
              ${dive_proto_srcs}
              ${dive_proto_hdrs})

target_link_libraries(dive_grpc_proto PRIVATE
              ${_GRPC_GRPCPP}
              ${_PROTOBUF_LIBPROTOBUF})
set_property(TARGET dive_grpc_proto PROPERTY POSITION_INDEPENDENT_CODE 1)

if(WIN32)
  set(COMMAND_UTILS_SRC "command_utils_win32.cc")
  add_definitions(-DUNICODE -D_UNICODE)
else()
  set(COMMAND_UTILS_SRC "command_utils.cc")
endif()

if(ANDROID)
if(DIVE_ENABLE_PERFETTO)
  set(PERFETTO_DIVE_IMP_SRC perfetto_trace.cc)
else()
  set(PERFETTO_DIVE_IMP_SRC perfetto_trace_empty.cc)
endif()
  add_library(service
              service.cc
              trace_mgr.cc
              android_trace_mgr.cc
              ${PERFETTO_DIVE_IMP_SRC}
              ${COMMAND_UTILS_SRC})

  target_link_libraries(service PRIVATE  log)
  if(DIVE_ENABLE_PERFETTO)
    target_link_libraries(service PRIVATE  perfetto)
  endif()
else()
  add_library(service
              service.cc
              trace_mgr.cc
              ${COMMAND_UTILS_SRC})
endif()
target_include_directories(service PRIVATE
  ${CMAKE_SOURCE_DIR}/common
)
target_link_libraries(service PRIVATE
                      network
                      dive_grpc_proto
                      absl::flags
                      absl::flags_parse
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF})
set_property(TARGET service PROPERTY POSITION_INDEPENDENT_CODE 1)

add_executable(server server.cc
)

target_link_libraries(server PRIVATE
                      service
                      dive_grpc_proto
                      absl::flags
                      absl::flags_parse
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF})

if(ANDROID)
  target_link_libraries(service PRIVATE wrap)
else()

  add_library(client client.cc)
  target_link_libraries(client PRIVATE
                        dive_grpc_proto
                        ${_GRPC_GRPCPP}
                        ${_PROTOBUF_LIBPROTOBUF}
                        absl::flags
                        absl::flags_parse)
  set_property(TARGET client PROPERTY POSITION_INDEPENDENT_CODE 1)

  add_executable(client_cli client_cli.cc)
  target_link_libraries(client_cli
    dive_grpc_proto
    client
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    absl::flags
    absl::flags_parse
    absl::algorithm
    )

  add_library(device_mgr device_mgr.cc ${COMMAND_UTILS_SRC} android_application.cc)
  target_include_directories(device_mgr PRIVATE
    ${CMAKE_SOURCE_DIR}/common
  )
  target_link_libraries(device_mgr 
    absl::flags
    absl::flags_parse
    absl::algorithm
    absl::strings
    absl::statusor
    )

  add_executable(dive_client_cli dive_client_cli.cc )
  target_link_libraries(dive_client_cli
    network
    dive_grpc_proto
    device_mgr
    client
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    absl::flags
    absl::flags_parse
    absl::algorithm
    absl::strings
    )

    install(TARGETS dive_client_cli DESTINATION ${CMAKE_INSTALL_PREFIX})

  enable_testing()
  include(GoogleTest)
  add_executable(device_mgr_test device_mgr_test.cc)
  target_link_libraries(device_mgr_test
    device_mgr
    gtest
    gtest_main
    )
  gtest_discover_tests(device_mgr_test)
endif()