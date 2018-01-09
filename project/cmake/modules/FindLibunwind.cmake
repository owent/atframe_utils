#.rst:
# FindLibUnwind
# -----------
#
# Find LibUnwind
#
# Find LibUnwind headers and library
#
# ::
#
#   Libunwind_FOUND                     - True if libunwind is found.
#   Libunwind_INCLUDE_DIRS              - Directory where libunwind headers are located.
#   Libunwind_LIBRARIES                 - Unwind libraries to link against.
#   Libunwind_HAS_UNW_GETCONTEXT        - True if unw_getcontext() is found (optional).
#   Libunwind_HAS_UNW_INIT_LOCAL        - True if unw_init_local() is found (optional).
#   Libunwind_HAS_UNW_BACKTRACE         - True if unw_backtrace() is found (required).
#   Libunwind_HAS_UNW_BACKTRACE_SKIP    - True if unw_backtrace_skip() is found (optional).
#   Libunwind_VERSION_STRING            - version number as a string (ex: "5.0.3")

#=============================================================================
# Copyright 2018 OWenT.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

if (Libunwind_ROOT)
  set(LIBUNWIND_ROOT ${Libunwind_ROOT})
endif()

if(LIBUNWIND_ROOT)
  set(_LIBUNWIND_SEARCH_ROOT PATHS ${LIBUNWIND_ROOT} NO_DEFAULT_PATH)
  set(_LIBUNWIND_SEARCH_INCLUDE PATHS ${LIBUNWIND_ROOT}/include NO_DEFAULT_PATH)
  set(_LIBUNWIND_SEARCH_LIB PATHS ${LIBUNWIND_ROOT}/lib NO_DEFAULT_PATH)
endif()

find_path(Libunwind_INCLUDE_DIR NAMES libunwind.h ${_LIBUNWIND_SEARCH_INCLUDE} )
if(NOT EXISTS "${Libunwind_INCLUDE_DIR}/unwind.h")
  MESSAGE("Found libunwind.h but corresponding unwind.h is absent!")
  SET(Libunwind_INCLUDE_DIR "")
endif()

find_library(Libunwind_LIBRARY NAMES unwind ${_LIBUNWIND_SEARCH_LIB})

if(Libunwind_INCLUDE_DIR AND EXISTS "${Libunwind_INCLUDE_DIR}/libunwind-common.h")
  file(STRINGS "${Libunwind_INCLUDE_DIR}/libunwind-common.h" Libunwind_HEADER_CONTENTS REGEX "#define UNW_VERSION_[A-Z]+\t[0-9]*")

  string(REGEX REPLACE ".*#define UNW_VERSION_MAJOR\t([0-9]*).*" "\\1" Libunwind_VERSION_MAJOR "${Libunwind_HEADER_CONTENTS}")
  string(REGEX REPLACE ".*#define UNW_VERSION_MINOR\t([0-9]*).*" "\\1" Libunwind_VERSION_MINOR "${Libunwind_HEADER_CONTENTS}")
  string(REGEX REPLACE ".*#define UNW_VERSION_EXTRA\t([0-9]*).*" "\\1" Libunwind_VERSION_EXTRA "${Libunwind_HEADER_CONTENTS}")

  if(Libunwind_VERSION_EXTRA)
    set(Libunwind_VERSION_STRING "${Libunwind_VERSION_MAJOR}.${Libunwind_VERSION_MINOR}.${Libunwind_VERSION_EXTRA}")
  else(not Libunwind_VERSION_EXTRA)
    set(Libunwind_VERSION_STRING "${Libunwind_VERSION_MAJOR}.${Libunwind_VERSION_MINOR}")
  endif()
  unset(Libunwind_HEADER_CONTENTS)
endif()

if (Libunwind_LIBRARY)
  include (CheckLibraryExists)
  set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
  set(CMAKE_REQUIRED_QUIET ${LibUnwind_FIND_QUIETLY})
  check_library_exists(${Libunwind_LIBRARY} unw_getcontext "" Libunwind_HAS_UNW_GETCONTEXT)
  check_library_exists(${Libunwind_LIBRARY} unw_init_local "" Libunwind_HAS_UNW_INIT_LOCAL)
  check_library_exists(${Libunwind_LIBRARY} unw_backtrace "" Libunwind_HAS_UNW_BACKTRACE)
  check_library_exists (${Libunwind_LIBRARY} unw_backtrace_skip "" Libunwind_HAS_UNW_BACKTRACE_SKIP)
  set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibUnwind  REQUIRED_VARS  
    Libunwind_INCLUDE_DIR
    Libunwind_LIBRARY
    Libunwind_HAS_UNW_BACKTRACE
    VERSION_VAR Libunwind_VERSION_STRING
)

if (Libunwind_FOUND)
  set(LIBUNWIND_FOUND ${Libunwind_FOUND})
  set(Libunwind_LIBRARIES ${Libunwind_LIBRARY})
  set(Libunwind_INCLUDE_DIRS ${Libunwind_INCLUDE_DIR})
else()
  unset(Libunwind_INCLUDE_DIR CACHE )
  unset(Libunwind_LIBRARY CACHE )
  unset(Libunwind_HAS_UNW_BACKTRACE CACHE )
  unset(Libunwind_VERSION_STRING CACHE )
endif ()

mark_as_advanced( Libunwind_INCLUDE_DIR Libunwind_LIBRARY )