#.rst:
# FindLibhiredis
# --------
#
# Find the native libuv includes and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   Libhiredis_INCLUDE_DIRS   - where to find uv.h, etc.
#   Libhiredis_LIBRARIES      - List of libraries when using libuv.
#   Libhiredis_FOUND          - True if libuv found.
#
# ::
#
#
# Hints
# ^^^^^
#
# A user may set ``LIBHIREDIS_ROOT`` to a libuv installation root to tell this
# module where to look.

# =============================================================================
# Copyright 2014-2015 OWenT.
#
# Distributed under the OSI-approved BSD License (the "License"); see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE. See the License for more information.
# =============================================================================
# (To distribute this file outside of CMake, substitute the full License text for the above reference.)

unset(_LIBHIREDIS_SEARCH_ROOT)
unset(_LIBHIREDIS_SEARCH_INCLUDE)
unset(_LIBHIREDIS_SEARCH_LIB)

# Search LIBHIREDIS_ROOT first if it is set.
if(Libhiredis_ROOT)
  set(LIBHIREDIS_ROOT ${Libhiredis_ROOT})
endif()

if(LIBHIREDIS_ROOT)
  set(_LIBHIREDIS_SEARCH_ROOT PATHS ${LIBHIREDIS_ROOT} NO_DEFAULT_PATH)
  set(_LIBHIREDIS_SEARCH_INCLUDE PATHS ${LIBHIREDIS_ROOT}/include NO_DEFAULT_PATH)
  set(_LIBHIREDIS_SEARCH_LIB PATHS ${LIBHIREDIS_ROOT}/lib NO_DEFAULT_PATH)
endif()

# Normal search.
set(Libhiredis_NAMES hiredis libhiredis)

# Try each search configuration.
find_path(Libhiredis_INCLUDE_DIRS NAMES hiredis/hiredis.h ${_LIBHIREDIS_SEARCH_INCLUDE})
find_library(Libhiredis_LIBRARIES NAMES ${Libhiredis_NAMES} ${_LIBHIREDIS_SEARCH_LIB})

mark_as_advanced(Libhiredis_INCLUDE_DIRS Libhiredis_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBHIREDIS_FOUND to TRUE if all listed variables are TRUE
include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(
  Libhiredis
  REQUIRED_VARS Libhiredis_INCLUDE_DIRS Libhiredis_LIBRARIES
  FOUND_VAR Libhiredis_FOUND)

if(Libhiredis_FOUND)
  set(LIBHIREDIS_FOUND ${Libhiredis_FOUND})
else()
  unset(Libhiredis_INCLUDE_DIRS CACHE)
  unset(Libhiredis_LIBRARIES CACHE)
endif()
