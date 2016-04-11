
EchoWithColor(COLOR GREEN "-- Configure Test Projects")
include("test.macro.cmake")

enable_testing()
include("${CMAKE_CURRENT_LIST_DIR}/test.build_bin.cmake")

include_directories(${PROJECT_ALL_INCLUDE_DIR})

file(GLOB_RECURSE PROJECT_TEST_SRC_LIST
    ${PROJECT_TEST_SRC_DIR}/*.h
    ${PROJECT_TEST_SRC_DIR}/*.hpp
    ${PROJECT_TEST_SRC_DIR}/*.c
    ${PROJECT_TEST_SRC_DIR}/*.cpp
    ${PROJECT_TEST_SRC_DIR}/*.cc
    ${PROJECT_TEST_SRC_DIR}/*.cxx
)
source_group_by_dir(PROJECT_TEST_SRC_LIST)

# ============ test - coroutine test frame ============
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/test")

if (MINGW)
    list(APPEND EXTENTION_LINK_LIB stdc++)
endif()

add_executable(atframe_utils_unit_test ${PROJECT_TEST_SRC_LIST})
target_link_libraries(atframe_utils_unit_test
    ${PROJECT_LIB_LINK}
    ${PROJECT_TEST_LIB_LINK}
    ${3RD_PARTY_LIBUV_LINK_NAME}
    ${3RD_PARTY_C_CPP_UTILS_LINK_NAME}
    ${EXTENTION_LINK_LIB}
)

add_test(test atframe_utils_unit_test)
