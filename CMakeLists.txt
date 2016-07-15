cmake_minimum_required(VERSION 3.1.0)

project(atframe_utils_cpp)
set (PROJECT_CMAKE_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/project/cmake")

# 编译选项
#####################################################################
include("${PROJECT_CMAKE_MODULE_DIR}/CompilerOption.cmake")
include("${PROJECT_CMAKE_MODULE_DIR}/TargetOption.cmake")
list (APPEND CMAKE_MODULE_PATH "${PROJECT_CMAKE_MODULE_DIR}/modules")
include(IncludeDirectoryRecurse)
include(EchoWithColor)
include(FindConfigurePackage)

# 工程选项
set(EXTENTION_LINK_LIB)

set(PROJECT_ALL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(PROJECT_ALL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
include_directories ("${PROJECT_ALL_INCLUDE_DIR}")

include("${PROJECT_CMAKE_MODULE_DIR}/ProjectBuildOption.cmake")

# 源码
file(GLOB_RECURSE SRC_LIST "${PROJECT_ALL_SOURCE_DIR}/*.c" "${PROJECT_ALL_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_LIST "${PROJECT_ALL_INCLUDE_DIR}/*.h" "${PROJECT_ALL_INCLUDE_DIR}/*.hpp")

# lib名称
set(PROJECT_LIB_LINK atframe_utils)

source_group_by_dir(HEADER_LIST)
source_group_by_dir(SRC_LIST)
add_library(${PROJECT_LIB_LINK} ${HEADER_LIST} ${SRC_LIST})

configure_file(
    "${PROJECT_ALL_INCLUDE_DIR}/config/atframe_utils_build_feature.h.in"
    "${PROJECT_ALL_INCLUDE_DIR}/config/atframe_utils_build_feature.h"
    @ONLY
)

if (PROJECT_ENABLE_SAMPLE)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/sample")
endif()

if (PROJECT_ENABLE_UNITTEST)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/test")
endif()

install(TARGETS ${PROJECT_LIB_LINK}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${PLATFORM_SUFFIX}
    ARCHIVE DESTINATION lib${PLATFORM_SUFFIX}
)

install(DIRECTORY ${PROJECT_ALL_INCLUDE_DIR}
    DESTINATION .
	FILES_MATCHING REGEX ".+\\.h(pp)?$"
    PATTERN ".svn" EXCLUDE
    PATTERN ".git" EXCLUDE
)