cmake_minimum_required(VERSION 3.8)

project(PerfettoTrace)
set(target perfetto_trace_reader)

add_library(${target} trace_reader.h trace_reader.cpp)
target_link_libraries(${target}  libtrace_processor.a )
target_link_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/prebuild/perfetto )
target_include_directories(${target} PRIVATE 
    "${THIRDPARTY_DIRECTORY}/perfetto/"
    "${THIRDPARTY_DIRECTORY}/perfetto/include"
    "${CMAKE_SOURCE_DIR}/prebuild/perfetto")