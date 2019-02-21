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
  find_library(Libunwind_LIBRARY_EXTRA NAMES unwind-${CMAKE_SYSTEM_PROCESSOR} ${_LIBUNWIND_SEARCH_LIB})
  if (NOT Libunwind_LIBRARY_EXTRA AND ${CMAKE_SYSTEM_PROCESSOR} EQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
    find_library(Libunwind_LIBRARY_EXTRA NAMES unwind-generic ${_LIBUNWIND_SEARCH_LIB})
  endif()
  include (CheckCSourceCompiles)
  set(CMAKE_REQUIRED_QUIET_SAVE ${CMAKE_REQUIRED_QUIET})
  set(CMAKE_REQUIRED_QUIET ${LibUnwind_FIND_QUIETLY})
  set(CMAKE_REQUIRED_LIBRARIES_SAVE ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES ${Libunwind_LIBRARY} ${Libunwind_LIBRARY_EXTRA})
  set(CMAKE_REQUIRED_INCLUDES_SAVE ${CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_INCLUDES ${Libunwind_INCLUDE_DIR})
  check_c_source_compiles("
  #include <libunwind.h>
  int main() {
    unw_context_t unw_ctx;
    unw_getcontext(&unw_ctx);
    return 0;
  }" Libunwind_HAS_UNW_GETCONTEXT)
  check_c_source_compiles("
  #include <libunwind.h>
  int main() {
    unw_context_t unw_ctx;
    unw_cursor_t unw_cur;
    unw_getcontext(&unw_ctx);
    unw_init_local(&unw_cur, &unw_ctx);
    return 0;
  }" Libunwind_HAS_UNW_INIT_LOCAL)
  set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_SAVE})
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_SAVE})
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVE})
  unset(CMAKE_REQUIRED_QUIET_SAVE)
  unset(CMAKE_REQUIRED_LIBRARIES_SAVE)
  unset(CMAKE_REQUIRED_INCLUDES_SAVE)
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libunwind  REQUIRED_VARS  
    Libunwind_INCLUDE_DIR
    Libunwind_LIBRARY
    VERSION_VAR Libunwind_VERSION_STRING
)

if (Libunwind_FOUND)
  set(LIBUNWIND_FOUND ${Libunwind_FOUND})
  set(Libunwind_LIBRARIES ${Libunwind_LIBRARY} ${Libunwind_LIBRARY_EXTRA})
  set(LIBUNWIND_LIBRARIES ${Libunwind_LIBRARIES})
  set(Libunwind_INCLUDE_DIRS ${Libunwind_INCLUDE_DIR})
  set(LIBUNWIND_INCLUDE_DIRS ${Libunwind_INCLUDE_DIRS})
else()
  unset(Libunwind_INCLUDE_DIR CACHE )
  unset(Libunwind_LIBRARY CACHE )
  unset(Libunwind_VERSION_STRING CACHE )
endif ()

mark_as_advanced( Libunwind_INCLUDE_DIR Libunwind_LIBRARY Libunwind_LIBRARY_EXTRA )