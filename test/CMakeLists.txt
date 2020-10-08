
EchoWithColor(COLOR GREEN "-- Configure ${CMAKE_CURRENT_LIST_DIR}")
include("test.macro.cmake")

include("${CMAKE_CURRENT_LIST_DIR}/test.build_bin.cmake")

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
if (NOT (WIN32 AND BUILD_SHARED_LIBS))
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test")
endif ()

add_executable(atframe_utils_unit_test ${PROJECT_TEST_SRC_LIST})

set_target_properties(atframe_utils_unit_test PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH YES
    BUILD_WITH_INSTALL_RPATH NO
    BUILD_RPATH_USE_ORIGIN YES
)

if (MSVC)
    set_property(TARGET atframe_utils_unit_test PROPERTY FOLDER "atframework")
endif (MSVC)

target_link_libraries(atframe_utils_unit_test
    ${PROJECT_TEST_LIB_LINK}
    ${PROJECT_ATFRAME_UTILS_LIB_NAME}
)

target_compile_options(atframe_utils_unit_test
    PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS}
)

if (MSVC)
    set_property(TARGET atframe_utils_unit_test PROPERTY FOLDER "atframework/test")
endif (MSVC)

add_test(NAME test COMMAND atframe_utils_unit_test)
