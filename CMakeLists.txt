cmake_minimum_required(VERSION 3.7.0)

enable_testing()

project(atframe_utils C CXX)

set(LIBATFRAME_UTILS_VERSION_MAJOR "1")
set(LIBATFRAME_UTILS_VERSION_MINOR "0")
set(LIBATFRAME_UTILS_VERSION_PATCH "0")
set(LIBATFRAME_UTILS_VERSION "${LIBATFRAME_UTILS_VERSION_MAJOR}.${LIBATFRAME_UTILS_VERSION_MINOR}.${LIBATFRAME_UTILS_VERSION_PATCH}")


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


if (CRYPTO_USE_OPENSSL OR CRYPTO_USE_LIBRESSL OR CRYPTO_USE_BORINGSSL)
    find_package(OpenSSL)
    list(APPEND PROJECT_DEP_LINK_NAMES ${OPENSSL_CRYPTO_LIBRARY})
    if(UNIX)
        find_library(TRY_FIND_DL dl)
        if (TRY_FIND_DL)
            list(APPEND PROJECT_DEP_LINK_NAMES dl)
        endif()
    endif()
elseif (CRYPTO_USE_MBEDTLS)
    find_package(MbedTLS)
    list(APPEND PROJECT_DEP_LINK_NAMES ${MbedTLS_CRYPTO_LIBRARIES})
endif()

if(LIBSODIUM_FOUND)
    list(APPEND PROJECT_DEP_LINK_NAMES ${Libsodium_LIBRARIES})
endif()

if (LOG_STACKTRACE_USING_LIBUNWIND)
    list(APPEND PROJECT_DEP_LINK_NAMES ${Libunwind_LIBRARIES})
endif()

target_link_libraries(${PROJECT_LIB_LINK} ${PROJECT_DEP_LINK_NAMES})

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
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${PROJECT_ALL_INCLUDE_DIR}
    DESTINATION .
	FILES_MATCHING REGEX ".+\\.h(pp)?$"
    PATTERN ".svn" EXCLUDE
    PATTERN ".git" EXCLUDE
)


## 生成文档和导入配置

# Install configuration
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake" CACHE STRING "Directory relative to CMAKE_INSTALL to install the cmake configuration files")

include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR include)

file(MAKE_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake")

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/libatframe_utils-config.cmake.in"
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
    PATH_VARS LIBATFRAME_UTILS_VERSION INCLUDE_INSTALL_DIR CMAKE_INSTALL_LIBDIR PROJECT_SOURCE_DIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config-version.cmake"
    VERSION ${LIBATFRAME_UTILS_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config.cmake" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)