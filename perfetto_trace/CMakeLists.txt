cmake_minimum_required(VERSION 3.8)

project(PerfettoTrace)
set(target perfetto_trace_reader)

add_library(${target} trace_reader.h trace_reader.cpp)
target_link_libraries(${target}  trace_processor)
target_include_directories(${target} PRIVATE 
    "${THIRDPARTY_DIRECTORY}/perfetto/"
    "${THIRDPARTY_DIRECTORY}/perfetto/include"
    ${PERFETTO_BUILD_HEADER_DIR})