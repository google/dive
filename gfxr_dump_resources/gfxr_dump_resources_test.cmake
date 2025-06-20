# Usage in CMakeLists.txt:
#
# enable_testing()
# add_test(NAME MyTest
#   COMMAND ${CMAKE_COMMAND} -D TEST_EXECUTABLE=$<TARGET_FILE:gfxr_dump_resources> -D PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/gfxr_dump_resources_test.cmake
# )

execute_process(
  COMMAND ${TEST_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/vs_triangle_300_20221211T232110.gfxr test_output.json
  RESULT_VARIABLE exit_code
)
# In CMake 3.19+, prefer COMMAND_ERROR_IS_FATAL (more succinct)
if (NOT exit_code EQUAL 0)
  message(FATAL_ERROR "${TEST_EXECUTABLE} failed: ${exit_code}")
endif()

# Since git lets users specify newline style, normalize both input and output to compare_files.
# This is required so that compare_files works regardless of platform and setting.
# In CMake 3.14+, prefer compare_files --ignore-eol (more succinct)
configure_file(test_output.json test_output_unix_newline.json NEWLINE_STYLE UNIX)
configure_file(${PROJECT_SOURCE_DIR}/tests/golden/vs_triangle_300_20221211T232110_dump_resources_golden.json golden_unix_newline.json NEWLINE_STYLE UNIX)

execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files golden_unix_newline.json test_output_unix_newline.json
  RESULT_VARIABLE exit_code
)
# In CMake 3.19+, prefer COMMAND_ERROR_IS_FATAL (more succinct)
if (NOT exit_code EQUAL 0)
  message(FATAL_ERROR "Output differs from golden file")
endif()
