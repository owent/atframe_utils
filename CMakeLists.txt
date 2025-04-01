cmake_minimum_required(VERSION 3.24.0)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27.0")
  cmake_policy(SET CMP0144 NEW)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

project(
  atframe_utils
  VERSION "2.8.1"
  HOMEPAGE_URL "https://github.com/atframework/atframe_utils"
  LANGUAGES C CXX)

if(NOT DEFINED __COMPILER_OPTION_LOADED)
  if(MSVC)
    string(REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/GR-?" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    string(REGEX REPLACE "-f(no-)?exceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "-f(no-)?rtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  endif()
endif()

# 编译选项
# ######################################################################################################################
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

include("${CMAKE_CURRENT_LIST_DIR}/project/cmake/ProjectBuildOption.cmake")

# Link RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH YES)
set(CMAKE_BUILD_WITH_INSTALL_RPATH NO)
set(CMAKE_BUILD_RPATH_USE_ORIGIN YES)

set(ATFRAMEWORK_UTILS_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(ATFRAMEWORK_UTILS_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(ATFRAMEWORK_UTILS_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(ATFRAMEWORK_UTILS_VERSION "${PROJECT_VERSION}")

set(ATFRAMEWORK_UTILS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/_generated")
file(MAKE_DIRECTORY "${ATFRAMEWORK_UTILS_GENERATED_DIR}/include/config")
file(MAKE_DIRECTORY "${ATFRAMEWORK_UTILS_GENERATED_DIR}/temp")
configure_file("${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}/config/atframe_utils_build_feature.h.in"
               "${ATFRAMEWORK_UTILS_GENERATED_DIR}/temp/atframe_utils_build_feature.h" @ONLY)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ATFRAMEWORK_UTILS_GENERATED_DIR}/temp/atframe_utils_build_feature.h"
          "${ATFRAMEWORK_UTILS_GENERATED_DIR}/include/config")

# Source
set(SRC_LIST
    "${CMAKE_CURRENT_LIST_DIR}/src/algorithm/base64.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/algorithm/crc.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/algorithm/crypto_cipher.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/algorithm/crypto_dh.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/algorithm/murmur_hash.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/algorithm/sha.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/algorithm/xxtea.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cli/cmd_option_list.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cli/cmd_option_value.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cli/shell_font.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/demangle.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/demangle_cxx_abi.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/demangle_windows.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/file_system.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/platform_compat.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/string_oprs.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/config/ini_loader.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/log/log_formatter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/log/log_sink_file_backend.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/log/log_sink_syslog_backend.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/log/log_stacktrace.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/log/log_wrapper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/log/lua_log_adaptor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/memory/lru_object_pool.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/memory/rc_ptr.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/network/http_content_type.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/network/http_request.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/random/uuid_generator.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/string/tquerystring.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/time/time_utility.cpp")
set(HEADER_LIST
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/base64.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/crc.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/crypto_cipher.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/crypto_dh.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/hash.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/mixed_int.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/murmur_hash.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/sha.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/algorithm/xxtea.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_bind.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_bindt_base.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_bindt_cc.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_bindt_mf_cc.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_bind_param_list.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_list.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_phoenix.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_string.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/cmd_option_value.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/cli/shell_font.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/common/compiler_message.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/common/demangle.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/common/file_system.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/common/platform_compat.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/common/string_oprs.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compiler/migrate_prefix.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compiler/migrate_suffix.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compiler/protobuf_prefix.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compiler/protobuf_suffix.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compiler/template_prefix.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compiler/template_suffix.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compiler_features.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/compile_optimize.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/config/ini_loader.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/data_structure/finite_state_machine.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/data_structure/lock_free_array.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/design_pattern/nomovable.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/design_pattern/noncopyable.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/design_pattern/singleton.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/gsl/select-gsl.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/lock/atomic_int_type.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/lock/lock_holder.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/lock/seq_alloc.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/lock/spin_lock.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/lock/spin_rw_lock.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/log/log_formatter.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/log/log_sink_file_backend.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/log/log_stacktrace.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/log/log_wrapper.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/log/lua_log_adaptor.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/mem_pool/lru_map.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/mem_pool/lru_object_pool.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/memory/lru_map.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/memory/rc_ptr.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/memory/lru_object_pool.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/memory/allocator_ptr.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/network/http_content_type.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/network/http_request.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/nostd/function_ref.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/nostd/nullability.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/nostd/string_view.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/nostd/type_traits.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/nostd/utility_data_size.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/nostd/utility_sequence.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/random/random_generator.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/random/random_mt_core.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/random/random_xor_combine_core.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/random/random_xoshiro_core.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/random/uuid_generator.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/array.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/chrono.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/explicit_declare.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/functional.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/intrusive_ptr.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/ref.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/smart_ptr.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/static_assert.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/thread.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/tuple.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/std/utility.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/string/ac_automation.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/string/tquerystring.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/string/utf8_char_t.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/time/jiffies_timer.h"
    "${CMAKE_CURRENT_LIST_DIR}/include/time/time_utility.h")

# lib名称
set(PROJECT_LIB_EXPORT_NAME "${PROJECT_NAME}-target")

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${HEADER_LIST} ${SRC_LIST})

list(APPEND HEADER_LIST "${ATFRAMEWORK_UTILS_GENERATED_DIR}/include/config/atframe_utils_build_feature.h")
source_group(TREE "${ATFRAMEWORK_UTILS_GENERATED_DIR}"
             FILES "${ATFRAMEWORK_UTILS_GENERATED_DIR}/include/config/atframe_utils_build_feature.h")

# Internal implement of uuid
if(ATFRAMEWORK_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT)
  include(CheckIncludeFile)

  get_source_file_property(ATFRAMEWORK_UTILS_UUID_GENERATOR_DEFS
                           "${PROJECT_ATFRAME_UTILS_SOURCE_DIR}/random/uuid_generator.cpp" COMPILE_DEFINITIONS)
  macro(ATFRAMEWORK_UTILS_UUID_GENERATOR_CHECK_DEF FILE_PATH DEF_NAME)
    check_include_file(${FILE_PATH} ${DEF_NAME})
    if(${DEF_NAME})
      list(APPEND ATFRAMEWORK_UTILS_UUID_GENERATOR_DEFS "${DEF_NAME}=1")
    endif()
  endmacro()

  atframework_utils_uuid_generator_check_def("fcntl.h" HAVE_FCNTL_H)
  atframework_utils_uuid_generator_check_def("inttypes.h" HAVE_INTTYPES_H)
  atframework_utils_uuid_generator_check_def("limits.h" HAVE_LIMITS_H)
  atframework_utils_uuid_generator_check_def("netinet/in.h" HAVE_NETINET_IN_H)
  atframework_utils_uuid_generator_check_def("stdlib.h" HAVE_STDLIB_H)
  atframework_utils_uuid_generator_check_def("string.h" HAVE_STRING_H)
  atframework_utils_uuid_generator_check_def("sys/file.h" HAVE_SYS_FILE_H)
  atframework_utils_uuid_generator_check_def("sys/ioctl.h" HAVE_SYS_IOCTL_H)
  atframework_utils_uuid_generator_check_def("sys/socket.h" HAVE_SYS_SOCKET_H)
  atframework_utils_uuid_generator_check_def("sys/time.h" HAVE_SYS_TIME_H)
  atframework_utils_uuid_generator_check_def("unistd.h" HAVE_UNISTD_H)
  atframework_utils_uuid_generator_check_def("net/if.h" HAVE_NET_IF_H)
  atframework_utils_uuid_generator_check_def("sys/un.h" HAVE_SYS_UN_H)
  atframework_utils_uuid_generator_check_def("sys/sockio.h" HAVE_SYS_SOCKIO_H)
  atframework_utils_uuid_generator_check_def("net/if_dl.h" HAVE_NET_IF_DL_H)

  set_source_files_properties("${PROJECT_ATFRAME_UTILS_SOURCE_DIR}/random/uuid_generator.cpp"
                              PROPERTIES COMPILE_DEFINITIONS "${ATFRAMEWORK_UTILS_UUID_GENERATOR_DEFS}")
  unset(ATFRAMEWORK_UTILS_UUID_GENERATOR_DEFS)
endif()

if(BUILD_SHARED_LIBS OR ATFRAMEWORK_USE_DYNAMIC_LIBRARY)
  add_library(${PROJECT_NAME} SHARED ${HEADER_LIST} ${SRC_LIST})
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION}
               INTERFACE_COMPILE_DEFINITIONS ATFRAMEWORK_UTILS_API_DLL=1)
  if(NOT APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES C_VISIBILITY_PRESET "hidden" CXX_VISIBILITY_PRESET "hidden")
  endif()
  target_compile_definitions(${PROJECT_NAME} PRIVATE ATFRAMEWORK_UTILS_API_NATIVE=1 ATFRAMEWORK_UTILS_API_DLL=1)
  target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS}
                                                 ${COMPILER_STRICT_RECOMMEND_EXTRA_CFLAGS})
else()
  add_library(${PROJECT_NAME} STATIC ${HEADER_LIST} ${SRC_LIST})
  set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
  if(NOT APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES C_VISIBILITY_PRESET "hidden" CXX_VISIBILITY_PRESET "hidden")
  endif()
  target_compile_definitions(${PROJECT_NAME} PRIVATE ATFRAMEWORK_UTILS_API_NATIVE=1)
  target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS}
                                                 ${COMPILER_STRICT_RECOMMEND_EXTRA_CFLAGS})
endif()

if(PROJECT_ATFRAME_UTILS_PUBLIC_DEFINITIONS)
  target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_ATFRAME_UTILS_PUBLIC_DEFINITIONS})
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "atframework")

include(AtframeworkToolsetCommonDefinitions)
if(BUILD_SHARED_LIBS OR ATFRAMEWORK_USE_DYNAMIC_LIBRARY)
  if(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND AND ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_LIBUNWIND_LINK_NAME)
    list(APPEND PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_LIBUNWIND_LINK_NAME})
  endif()

  if(CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD|NetBSD|OpenBSD")
    list(APPEND PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES kvm)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    list(APPEND PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES kstat nsl sendfile socket)
  endif()

  list(APPEND PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LIBRARIES})
else()
  if(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND AND ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_LIBUNWIND_LINK_NAME)
    list(APPEND PROJECT_ATFRAME_UTILS_INTERFACE_LINK_NAMES ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_LIBUNWIND_LINK_NAME})
  endif()

  if(CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD|NetBSD|OpenBSD")
    list(APPEND PROJECT_ATFRAME_UTILS_INTERFACE_LINK_NAMES kvm)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
    list(APPEND PROJECT_ATFRAME_UTILS_INTERFACE_LINK_NAMES kstat nsl sendfile socket)
  endif()

  list(APPEND PROJECT_ATFRAME_UTILS_INTERFACE_LINK_NAMES ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LIBRARIES})
endif()

target_include_directories(
  ${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${PROJECT_ATFRAME_UTILS_INCLUDE_DIR}>"
                         "$<BUILD_INTERFACE:${ATFRAMEWORK_UTILS_GENERATED_DIR}/include>" "$<INSTALL_INTERFACE:include>")
if(PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS)
  target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS}>")
endif()
if(PROJECT_ATFRAME_UTILS_PRIVATE_INCLUDE_DIRS)
  target_include_directories(${PROJECT_NAME} PRIVATE "$<BUILD_INTERFACE:${PROJECT_ATFRAME_UTILS_PRIVATE_INCLUDE_DIRS}>")
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC ${PROJECT_ATFRAME_UTILS_EXTENTION_LINK_LIB} ${PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES}
  INTERFACE ${PROJECT_ATFRAME_UTILS_INTERFACE_LINK_NAMES} ${COMPILER_OPTION_EXTERN_CXX_LIBS})
if(PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_ATFRAME_UTILS_PRIVATE_LINK_NAMES})
endif()

if(PROJECT_ENABLE_TOOLS)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/tools")
endif()

if(PROJECT_ENABLE_SAMPLE)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/sample")
endif()

if(PROJECT_ENABLE_UNITTEST OR BUILD_TESTING)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/test")
endif()

install(
  TARGETS ${PROJECT_NAME}
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
    "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    CACHE STRING "Directory relative to CMAKE_INSTALL to install the cmake configuration files")

include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR include)

file(MAKE_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}")

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/atframe_utils-config.cmake.in"
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  PATH_VARS PROJECT_VERSION
            PROJECT_VERSION_MAJOR
            PROJECT_VERSION_MINOR
            PROJECT_VERSION_PATCH
            INCLUDE_INSTALL_DIR
            CMAKE_INSTALL_LIBDIR
            PROJECT_SOURCE_DIR
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file(
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

export(
  EXPORT ${PROJECT_LIB_EXPORT_NAME}
  NAMESPACE atframework::
  FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_LIB_EXPORT_NAME}.cmake")

install(FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config.cmake"
              "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${PROJECT_NAME}/${PROJECT_NAME}-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR})
install(FILES "${ATFRAMEWORK_UTILS_GENERATED_DIR}/include/config/atframe_utils_build_feature.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/config/")

install(
  EXPORT ${PROJECT_LIB_EXPORT_NAME}
  NAMESPACE "atframework::"
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

set(CPACK_PACKAGE_VENDOR "atframework")
# set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}") set(CPACK_PACKAGE_VERSION_MINOR
# "${PROJECT_VERSION_MINOR}") set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION
    "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "admin@owent.net")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/atframework/")
include(CPack)
