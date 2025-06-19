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
if (NOT exit_code EQUAL 0)
  message(FATAL_ERROR "${TEST_EXECUTABLE} failed: ${exit_code}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files ${PROJECT_SOURCE_DIR}/tests/golden/vs_triangle_300_20221211T232110_dump_resources_golden.json test_output.json
  RESULT_VARIABLE exit_code
)
if (NOT exit_code EQUAL 0)
  message(FATAL_ERROR "Output differs from golden file")
endif()
