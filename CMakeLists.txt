cmake_minimum_required(VERSION 2.8.9)

project(atframe_utils_cpp)
set (PROJECT_CMAKE_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}/project/cmake")

# 编译选项
#####################################################################
include("${PROJECT_CMAKE_MODULE_DIR}/ProjectBuildOption.cmake")
include("${PROJECT_CMAKE_MODULE_DIR}/CompilerOption.cmake")
include("${PROJECT_CMAKE_MODULE_DIR}/TargetOption.cmake")
list (APPEND CMAKE_MODULE_PATH "${PROJECT_CMAKE_MODULE_DIR}/modules")
include(IncludeDirectoryRecurse)
include(EchoWithColor)
include(FindConfigurePackage)

if (NOT ENABLE_MIXEDINT_MAGIC_MASK)
    set(ENABLE_MIXEDINT_MAGIC_MASK 0)
endif()

if (${ENABLE_MIXEDINT_MAGIC_MASK} GREATER 0)
    add_compiler_define("ENABLE_MIXEDINT_MAGIC_MASK=${ENABLE_MIXEDINT_MAGIC_MASK}")
endif()

set(EXTENTION_LINK_LIB)
# 查找Lua
find_package(Lua51)
if (LUA51_FOUND)
    include_directories(${LUA_INCLUDE_DIR})
    list(APPEND EXTENTION_LINK_LIB ${LUA_LIBRARIES})
    message(STATUS "Lua support enabled.(lua 5.1 detected)")
elseif(ENABLE_LUA_SUPPORT)
    message(STATUS "Lua support enabled.(enabled by custom)")
else()
    message(STATUS "Lua support disabled")
	add_compiler_define("LOG_WRAPPER_DISABLE_LUA_SUPPORT=1")
endif()

# 设置输出目录
set(PROJECT_ALL_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(PROJECT_ALL_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
include_directories ("${PROJECT_ALL_INCLUDE_DIR}")

file(GLOB_RECURSE SRC_LIST "${PROJECT_ALL_SOURCE_DIR}/*.c" "${PROJECT_ALL_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_LIST "${PROJECT_ALL_INCLUDE_DIR}/*.h" "${PROJECT_ALL_INCLUDE_DIR}/*.hpp")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
link_directories (${LIBRARY_OUTPUT_PATH})

# lib名称
set(PROJECT_LIB_LINK atframe_utils)

source_group_by_dir(HEADER_LIST)
source_group_by_dir(SRC_LIST)
add_library(${PROJECT_LIB_LINK} ${HEADER_LIST} ${SRC_LIST})

install(TARGETS ${PROJECT_LIB_LINK}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
	)

install(DIRECTORY "${PROJECT_ALL_INCLUDE_DIR}" DESTINATION .
	FILES_MATCHING REGEX ".+\\.h(pp)?$"
)

if (PROJECT_ENABLE_SAMPLE)
    add_subdirectory("${PROJECT_SOURCE_DIR}/sample")
endif()

if (PROJECT_ENABLE_UNITTEST)
    add_subdirectory("${PROJECT_SOURCE_DIR}/test")
endif()
