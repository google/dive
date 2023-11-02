#
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
#

cmake_minimum_required(VERSION 3.8)

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

if(ANDROID)
  add_library(service
              service.cc
              trace_mgr.cc
              android_trace_mgr.cc
              command_utils.cc)
else()
  add_library(service
              service.cc
              trace_mgr.cc
              command_utils.cc)
endif()
target_link_libraries(service PRIVATE
                      dive_grpc_proto
                      absl::flags
                      absl::flags_parse
                      ${_GRPC_GRPCPP}
                      ${_PROTOBUF_LIBPROTOBUF})
set_property(TARGET service PROPERTY POSITION_INDEPENDENT_CODE 1)

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

  add_library(device_mgr device_mgr.cc command_utils.cc android_application.cc)
  target_link_libraries(device_mgr 
  absl::flags
  absl::flags_parse
  absl::algorithm
  absl::strings)

  add_executable(dive_client_cli dive_client_cli.cc )
  target_link_libraries(dive_client_cli
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

endif()