cmake_minimum_required(VERSION 3.12.0)
cmake_policy(SET CMP0022 NEW)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
  cmake_policy(SET CMP0074 NEW)
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

project(atframe_utils C CXX)

set(LIBATFRAME_UTILS_VERSION_MAJOR "1")
set(LIBATFRAME_UTILS_VERSION_MINOR "2")
set(LIBATFRAME_UTILS_VERSION_PATCH "2")
set(LIBATFRAME_UTILS_VERSION "${LIBATFRAME_UTILS_VERSION_MAJOR}.${LIBATFRAME_UTILS_VERSION_MINOR}.${LIBATFRAME_UTILS_VERSION_PATCH}")

set(PROJECT_CMAKE_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/project/cmake")

# 编译选项
# ##########################################################################################################################################
include("${PROJECT_CMAKE_MODULE_DIR}/CompilerOption.cmake")
include("${PROJECT_CMAKE_MODULE_DIR}/TargetOption.cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_CMAKE_MODULE_DIR}/modules")
include(IncludeDirectoryRecurse)
include(EchoWithColor)
include(FindConfigurePackage)

# 工程选项
unset(PROJECT_ATFRAME_UTILS_EXTENTION_LINK_LIB)
set(PROJECT_ATFRAME_UTILS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(PROJECT_ATFRAME_UTILS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
unset(PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS)
unset(PROJECT_ATFRAME_UTILS_PUBLIC_DEFINITIONS)
unset(PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES)
unset(PROJECT_ATFRAME_UTILS_PRIVATE_INCLUDE_DIRS)
unset(PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES)
unset(PROJECT_ATFRAME_UTILS_INTERFACE_LINK_NAMES)
unset(PROJECT_ATFRAME_UTILS_DEP_SYS_LINK_NAMES)

include("${PROJECT_CMAKE_MODULE_DIR}/ProjectBuildOption.cmake")

# 增加相对链接路径
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH YES)
set(CMAKE_BUILD_WITH_INSTALL_RPATH NO)
set(CMAKE_BUILD_RPATH_USE_ORIGIN YES)

# 源码
file(GLOB_RECURSE SRC_LIST "${PROJECT_ATFRAME_UTILS_SOURCE_DIR}/*.c" "${PROJECT_ATFRAME_UTILS_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_LIST "${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}/*.h" "${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}/*.hpp")

# lib名称
set(PROJECT_ATFRAME_UTILS_LIB_NAME atframe_utils)
set(PROJECT_LIB_EXPORT_NAME "${PROJECT_ATFRAME_UTILS_LIB_NAME}-target")

source_group_by_dir(HEADER_LIST)
source_group_by_dir(SRC_LIST)

message(STATUS "Configure ${PROJECT_ATFRAME_UTILS_LIB_NAME}")

# Internal implement of uuid
if(LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT)
  include(CheckIncludeFile)

  get_source_file_property(LIBATFRAME_UTILS_UUID_GENERATOR_DEFS "${PROJECT_ATFRAME_UTILS_SOURCE_DIR}/random/uuid_generator.cpp"
                           COMPILE_DEFINITIONS)
  macro(LIBATFRAME_UTILS_UUID_GENERATOR_CHECK_DEF FILE_PATH DEF_NAME)
    check_include_file(${FILE_PATH} ${DEF_NAME})
    if(${DEF_NAME})
      list(APPEND LIBATFRAME_UTILS_UUID_GENERATOR_DEFS "${DEF_NAME}=1")
    endif()
  endmacro()

  libatframe_utils_uuid_generator_check_def("fcntl.h" HAVE_FCNTL_H)
  libatframe_utils_uuid_generator_check_def("inttypes.h" HAVE_INTTYPES_H)
  libatframe_utils_uuid_generator_check_def("limits.h" HAVE_LIMITS_H)
  libatframe_utils_uuid_generator_check_def("netinet/in.h" HAVE_NETINET_IN_H)
  libatframe_utils_uuid_generator_check_def("stdlib.h" HAVE_STDLIB_H)
  libatframe_utils_uuid_generator_check_def("string.h" HAVE_STRING_H)
  libatframe_utils_uuid_generator_check_def("sys/file.h" HAVE_SYS_FILE_H)
  libatframe_utils_uuid_generator_check_def("sys/ioctl.h" HAVE_SYS_IOCTL_H)
  libatframe_utils_uuid_generator_check_def("sys/socket.h" HAVE_SYS_SOCKET_H)
  libatframe_utils_uuid_generator_check_def("sys/time.h" HAVE_SYS_TIME_H)
  libatframe_utils_uuid_generator_check_def("unistd.h" HAVE_UNISTD_H)
  libatframe_utils_uuid_generator_check_def("net/if.h" HAVE_NET_IF_H)
  libatframe_utils_uuid_generator_check_def("sys/un.h" HAVE_SYS_UN_H)
  libatframe_utils_uuid_generator_check_def("sys/sockio.h" HAVE_SYS_SOCKIO_H)
  libatframe_utils_uuid_generator_check_def("net/if_dl.h" HAVE_NET_IF_DL_H)

  set_source_files_properties("${PROJECT_ATFRAME_UTILS_SOURCE_DIR}/random/uuid_generator.cpp"
                              PROPERTIES COMPILE_DEFINITIONS "${LIBATFRAME_UTILS_UUID_GENERATOR_DEFS}")
  unset(LIBATFRAME_UTILS_UUID_GENERATOR_DEFS)
endif()

if(BUILD_SHARED_LIBS OR ATFRAMEWORK_USE_DYNAMIC_LIBRARY)
  add_library(${PROJECT_ATFRAME_UTILS_LIB_NAME} SHARED ${HEADER_LIST} ${SRC_LIST})
  set_target_properties(
    ${PROJECT_ATFRAME_UTILS_LIB_NAME}
    PROPERTIES C_VISIBILITY_PRESET "hidden"
               CXX_VISIBILITY_PRESET "hidden"
               VERSION ${LIBATFRAME_UTILS_VERSION}
               SOVERSION ${LIBATFRAME_UTILS_VERSION}
               INTERFACE_COMPILE_DEFINITIONS LIBATFRAME_UTILS_API_DLL=1)
  target_compile_definitions(${PROJECT_ATFRAME_UTILS_LIB_NAME} PRIVATE LIBATFRAME_UTILS_API_NATIVE=1 LIBATFRAME_UTILS_API_DLL=1)
  target_compile_options(${PROJECT_ATFRAME_UTILS_LIB_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})
else()
  add_library(${PROJECT_ATFRAME_UTILS_LIB_NAME} STATIC ${HEADER_LIST} ${SRC_LIST})
  set_target_properties(
    ${PROJECT_ATFRAME_UTILS_LIB_NAME}
    PROPERTIES C_VISIBILITY_PRESET "hidden"
               CXX_VISIBILITY_PRESET "hidden"
               VERSION ${LIBATFRAME_UTILS_VERSION})
  target_compile_definitions(${PROJECT_ATFRAME_UTILS_LIB_NAME} PRIVATE LIBATFRAME_UTILS_API_NATIVE=1)
  target_compile_options(${PROJECT_ATFRAME_UTILS_LIB_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})
endif()

if(PROJECT_ATFRAME_UTILS_PUBLIC_DEFINITIONS)
  target_compile_definitions(${PROJECT_ATFRAME_UTILS_LIB_NAME} PUBLIC ${PROJECT_ATFRAME_UTILS_PUBLIC_DEFINITIONS})
endif()

if(MSVC)
  set_property(TARGET ${PROJECT_ATFRAME_UTILS_LIB_NAME} PROPERTY FOLDER "atframework")
endif(MSVC)

if(LOG_STACKTRACE_USING_LIBUNWIND)
  list(APPEND PROJECT_ATFRAME_UTILS_DEP_SYS_LINK_NAMES ${Libunwind_LIBRARIES})
endif()

if(UNIX)
  find_library(PROJECT_ATFRAME_UTILS_TRY_LINK_DL dl)
  if(PROJECT_ATFRAME_UTILS_TRY_LINK_DL)
    list(APPEND PROJECT_ATFRAME_UTILS_DEP_SYS_LINK_NAMES dl)
  endif()

  find_library(PROJECT_ATFRAME_UTILS_TRY_LINK_RT rt)
  if(PROJECT_ATFRAME_UTILS_TRY_LINK_RT)
    list(APPEND PROJECT_ATFRAME_UTILS_DEP_SYS_LINK_NAMES rt)
  endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD|NetBSD|OpenBSD")
  list(APPEND PROJECT_ATFRAME_UTILS_DEP_SYS_LINK_NAMES kvm)
elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
  list(APPEND PROJECT_ATFRAME_UTILS_DEP_SYS_LINK_NAMES kstat nsl sendfile socket)
endif()

target_include_directories(${PROJECT_ATFRAME_UTILS_LIB_NAME} PUBLIC "$<BUILD_INTERFACE:${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}>"
                                                                    "$<INSTALL_INTERFACE:include>")
if(PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS)
  target_include_directories(${PROJECT_ATFRAME_UTILS_LIB_NAME} PUBLIC "$<BUILD_INTERFACE:${PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS}>")
endif()
if(PROJECT_ATFRAME_UTILS_PRIVATE_INCLUDE_DIRS)
  target_include_directories(${PROJECT_ATFRAME_UTILS_LIB_NAME} PRIVATE "$<BUILD_INTERFACE:${PROJECT_ATFRAME_UTILS_PRIVATE_INCLUDE_DIRS}>")
endif()

target_link_libraries(
  ${PROJECT_ATFRAME_UTILS_LIB_NAME}
  PUBLIC ${PROJECT_ATFRAME_UTILS_EXTENTION_LINK_LIB} ${PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES}
  INTERFACE ${PROJECT_ATFRAME_UTILS_INTERFACE_LINK_NAMES} ${COMPILER_OPTION_EXTERN_CXX_LIBS} ${PROJECT_ATFRAME_UTILS_DEP_SYS_LINK_NAMES})
if(PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES)
  target_link_libraries(${PROJECT_ATFRAME_UTILS_LIB_NAME} PRIVATE ${PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES})
endif()

configure_file("${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}/config/atframe_utils_build_feature.h.in"
               "${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}/config/atframe_utils_build_feature.h" @ONLY)

if(PROJECT_ENABLE_TOOLS)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/tools")
endif()

if(PROJECT_ENABLE_SAMPLE)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/sample")
endif()

if(PROJECT_ENABLE_UNITTEST)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/test")
endif()

install(
  TARGETS ${PROJECT_ATFRAME_UTILS_LIB_NAME}
  EXPORT ${PROJECT_LIB_EXPORT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}
  DESTINATION .
  FILES_MATCHING
  REGEX ".+\\.h(pp)?$"
  PATTERN ".svn" EXCLUDE
  PATTERN ".git" EXCLUDE)

# 生成文档和导入配置

# Install configuration
set(CMAKE_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake"
    CACHE STRING "Directory relative to CMAKE_INSTALL to install the cmake configuration files")

include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR include)

file(MAKE_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake")

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/libatframe_utils-config.cmake.in" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  PATH_VARS LIBATFRAME_UTILS_VERSION INCLUDE_INSTALL_DIR CMAKE_INSTALL_LIBDIR PROJECT_SOURCE_DIR
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file(
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config-version.cmake"
  VERSION ${LIBATFRAME_UTILS_VERSION}
  COMPATIBILITY SameMajorVersion)

export(
  TARGETS ${PROJECT_ATFRAME_UTILS_LIB_NAME}
  FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_LIB_EXPORT_NAME}.cmake"
  NAMESPACE atframe::
  # EXPORT_LINK_INTERFACE_LIBRARIES
)

install(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config.cmake"
              "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/libatframe_utils-config-version.cmake" DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

install(
  EXPORT ${PROJECT_LIB_EXPORT_NAME}
  NAMESPACE "atframe::"
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})
