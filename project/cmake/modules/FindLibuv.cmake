#.rst:
# FindLibuv
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
#   Libuv_INCLUDE_DIRS   - where to find uv.h, etc.
#   Libuv_LIBRARIES      - List of libraries when using libuv.
#   Libuv_FOUND          - True if libuv found.
#
# ::
#
#
# Hints
# ^^^^^
#
# A user may set ``LIBUV_ROOT`` to a libuv installation root to tell this
# module where to look.

#=============================================================================
# Copyright 2014-2015 OWenT.
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

unset(_LIBUV_SEARCH_ROOT_INC)
unset(_LIBUV_SEARCH_ROOT_LIB)

# Search LIBUV_ROOT first if it is set.
if (Libuv_ROOT)
  set(LIBUV_ROOT ${Libuv_ROOT})
endif()

if(LIBUV_ROOT)
  set(_LIBUV_SEARCH_ROOT_INC PATHS ${LIBUV_ROOT} ${LIBUV_ROOT}/include NO_DEFAULT_PATH)
  set(_LIBUV_SEARCH_ROOT_LIB PATHS ${LIBUV_ROOT} ${LIBUV_ROOT}/lib NO_DEFAULT_PATH)
endif()

# Normal search.
set(Libuv_NAMES uv libuv)

# Try each search configuration.
find_path(Libuv_INCLUDE_DIRS    NAMES uv.h            ${_LIBUV_SEARCH_ROOT_INC})
find_library(Libuv_LIBRARIES    NAMES ${Libuv_NAMES}  ${_LIBUV_SEARCH_ROOT_LIB})

mark_as_advanced(Libuv_INCLUDE_DIRS Libuv_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBUV_FOUND to TRUE if
# all listed variables are TRUE
include("FindPackageHandleStandardArgs")
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libuv
  REQUIRED_VARS Libuv_INCLUDE_DIRS Libuv_LIBRARIES
  FOUND_VAR Libuv_FOUND
)

if(Libuv_FOUND)
    set(LIBUV_FOUND ${Libuv_FOUND})
endif()
