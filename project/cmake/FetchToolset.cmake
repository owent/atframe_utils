include_guard(GLOBAL)

include(FetchContent)
find_package(Git REQUIRED)

set(ATFRAMEWORK_CMAKE_TOOLSET_DIR
    "${PROJECT_SOURCE_DIR}/project/cmake/toolset"
    CACHE PATH "PATH to cmake-toolset")
set(ATFRAMEWORK_CMAKE_TOOLSET_GIT_URL
    "https://github.com/atframework/cmake-toolset.git"
    CACHE STRING "Git URL of cmake-toolset")
set(ATFRAMEWORK_CMAKE_TOOLSET_VERSION
    "v1"
    CACHE STRING "Branch or tag of cmake-toolset")

FetchContent_Declare(
  downdload_cmake_toolset
  SOURCE_DIR "${ATFRAMEWORK_CMAKE_TOOLSET_DIR}"
  GIT_REPOSITORY "${ATFRAMEWORK_CMAKE_TOOLSET_GIT_URL}"
  GIT_TAG "${ATFRAMEWORK_CMAKE_TOOLSET_VERSION}")

if(NOT downdload_cmake_toolset_POPULATED)
  FetchContent_Populate(downdload_cmake_toolset)
endif()
