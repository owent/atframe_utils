#.rst:
# FindLibcopp
# --------
#
# Find the native libcopp includes and library.
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
#   Libcopp_INCLUDE_DIRS   - where to find uv.h, etc.
#   Libcopp_LIBRARIES      - List of libraries when using libcopp.
#   Libcotask_LIBRARIES    - List of libraries when using libcotask.
#   Libcopp_FOUND          - True if libcopp found.
#
# ::
#
#
# Hints
# ^^^^^
#
# A user may set ``LIBCOPP_ROOT`` to a libcopp installation root to tell this
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

unset(_LIBCOPP_SEARCH_ROOT)

# Search LIBCOPP_ROOT first if it is set.
if(Libcopp_ROOT)
  set(LIBCOPP_ROOT ${Libcopp_ROOT})
endif()

if(LIBCOPP_ROOT)
  set(_LIBCOPP_SEARCH_ROOT PATHS ${LIBCOPP_ROOT} NO_DEFAULT_PATH)
endif()

# Normal search.
set(Libcopp_NAMES copp libcopp)
set(Libcotask_NAMES cotask libcotask)

# Try each search configuration.
find_path(Libcopp_INCLUDE_DIRS NAMES libcopp/coroutine/coroutine_context_base.h ${_LIBCOPP_SEARCH_ROOT})
find_library(
  Libcopp_LIBRARIES
  NAMES ${Libcopp_NAMES} ${_LIBCOPP_SEARCH_ROOT}
  PATH_SUFFIXES lib lib64)
find_library(
  Libcotask_LIBRARIES
  NAMES ${Libcotask_NAMES} ${_LIBCOPP_SEARCH_ROOT}
  PATH_SUFFIXES lib lib64)

mark_as_advanced(Libcopp_INCLUDE_DIRS Libcopp_LIBRARIES Libcotask_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBCOPP_FOUND to TRUE if all listed variables are TRUE
include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(
  Libcopp
  FOUND_VAR Libcopp_FOUND
  REQUIRED_VARS Libcopp_INCLUDE_DIRS Libcopp_LIBRARIES)

if(Libcopp_FOUND)
  set(LIBCOPP_FOUND ${Libcopp_FOUND})
endif()
