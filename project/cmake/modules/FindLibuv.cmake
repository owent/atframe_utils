# .rst: FindLibuv
# --------
#
# Find the native libuv includes and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
# ::
#
# libuv
#
# ::
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
# Libuv_INCLUDE_DIRS   - where to find uv.h, etc. Libuv_LIBRARIES      - List of libraries when using libuv. Libuv_FOUND          - True if
# libuv found.
#
# ::
#
# Hints ^^^^^
#
# A user may set ``LIBUV_ROOT`` to a libuv installation root to tell this module where to look.

# =============================================================================
# Copyright 2014-2020 OWenT.
#
# Distributed under the OSI-approved BSD License (the "License"); see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE. See the License for more information.
# =============================================================================
# (To distribute this file outside of CMake, substitute the full License text for the above reference.)

unset(_LIBUV_SEARCH_ROOT_INC)
unset(_LIBUV_SEARCH_ROOT_LIB)

# Search LIBUV_ROOT first if it is set.
if(Libuv_ROOT)
  set(LIBUV_ROOT ${Libuv_ROOT})
endif()

if(LIBUV_ROOT)
  set(_LIBUV_SEARCH_ROOT_INC PATHS ${LIBUV_ROOT} "${LIBUV_ROOT}/include" NO_DEFAULT_PATH)
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    if(MSVC AND CMAKE_BUILD_TYPE)
      set(_LIBUV_SEARCH_ROOT_LIB
          PATHS
          ${LIBUV_ROOT}
          "${LIBUV_ROOT}/lib64/${CMAKE_BUILD_TYPE}"
          "${LIBUV_ROOT}/lib/${CMAKE_BUILD_TYPE}"
          "${LIBUV_ROOT}/lib64"
          "${LIBUV_ROOT}/lib"
          NO_DEFAULT_PATH)
    else()
      set(_LIBUV_SEARCH_ROOT_LIB PATHS ${LIBUV_ROOT} "${LIBUV_ROOT}/lib64" "${LIBUV_ROOT}/lib" NO_DEFAULT_PATH)
    endif()
  else()
    if(MSVC AND CMAKE_BUILD_TYPE)
      set(_LIBUV_SEARCH_ROOT_LIB PATHS ${LIBUV_ROOT} "${LIBUV_ROOT}/lib/${CMAKE_BUILD_TYPE}" "${LIBUV_ROOT}/lib" NO_DEFAULT_PATH)
    else()
      set(_LIBUV_SEARCH_ROOT_LIB PATHS ${LIBUV_ROOT} "${LIBUV_ROOT}/lib" NO_DEFAULT_PATH)
    endif()
  endif()
endif()

# Normal search.
set(Libuv_NAMES uv_a libuv_a uv libuv)

# Try each search configuration.
find_path(Libuv_INCLUDE_DIRS NAMES uv.h ${_LIBUV_SEARCH_ROOT_INC})
find_library(Libuv_LIBRARIES NAMES ${Libuv_NAMES} ${_LIBUV_SEARCH_ROOT_LIB})

mark_as_advanced(Libuv_INCLUDE_DIRS Libuv_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBUV_FOUND to TRUE if all listed variables are TRUE
include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(
  Libuv
  REQUIRED_VARS Libuv_INCLUDE_DIRS Libuv_LIBRARIES
  FOUND_VAR Libuv_FOUND)

if(Libuv_FOUND)
  set(LIBUV_FOUND ${Libuv_FOUND})
  if(NOT TARGET libuv)
    add_library(libuv UNKNOWN IMPORTED)
    set_target_properties(libuv PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${Libuv_INCLUDE_DIRS})
    set_target_properties(libuv PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C;CXX;RC" IMPORTED_LOCATION ${Libuv_LIBRARIES})

    if(WIN32)
      set_target_properties(libuv PROPERTIES INTERFACE_LINK_LIBRARIES "psapi;user32;advapi32;iphlpapi;userenv;ws2_32")
    else()
      unset(uv_libraries)
      if(NOT CMAKE_SYSTEM_NAME MATCHES "Android|OS390")
        list(APPEND uv_libraries pthread)
      endif()
      if(CMAKE_SYSTEM_NAME STREQUAL "AIX")
        list(APPEND uv_libraries perfstat)
      endif()
      if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        list(APPEND uv_libraries dl)
      endif()
      if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND uv_libraries dl rt)
      endif()
      if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        list(APPEND uv_libraries kvm)
      endif()
      if(CMAKE_SYSTEM_NAME STREQUAL "OS390")
        list(APPEND uv_libraries -Wl,xplink)
      endif()
      if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
        list(APPEND uv_libraries kstat nsl sendfile socket)
      endif()
      if(uv_libraries)
        set_target_properties(libuv PROPERTIES INTERFACE_LINK_LIBRARIES "${uv_libraries}")
        unset(uv_libraries)
      endif()
    endif()
  endif()
else()
  unset(Libuv_INCLUDE_DIRS CACHE)
  unset(Libuv_LIBRARIES CACHE)
endif()
