echowithcolor(COLOR GREEN "-- Configure ${CMAKE_CURRENT_LIST_DIR}")
include("test.macro.cmake")

include("${CMAKE_CURRENT_LIST_DIR}/test.build_bin.cmake")

file(
  GLOB_RECURSE
  PROJECT_TEST_SRC_LIST
  ${PROJECT_TEST_SRC_DIR}/*.h
  ${PROJECT_TEST_SRC_DIR}/*.hpp
  ${PROJECT_TEST_SRC_DIR}/*.c
  ${PROJECT_TEST_SRC_DIR}/*.cpp
  ${PROJECT_TEST_SRC_DIR}/*.cc
  ${PROJECT_TEST_SRC_DIR}/*.cxx)
source_group_by_dir(PROJECT_TEST_SRC_LIST)

# ============ test - coroutine test frame ============
if(NOT (WIN32 AND BUILD_SHARED_LIBS))
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test")
endif()

atframe_add_test_executable(atframe_utils_unit_test ${PROJECT_TEST_SRC_LIST})

target_link_libraries(atframe_utils_unit_test ${PROJECT_NAME})
target_compile_options(atframe_utils_unit_test PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})

add_test(NAME "atframe_utils.unit_test" COMMAND "$<TARGET_FILE:atframe_utils_unit_test>")
set_tests_properties("atframe_utils.unit_test" PROPERTIES LABELS "atframe_utils;atframe_utils.unit_test")
