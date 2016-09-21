#.rst:
# FindLibuuid
# --------
#
# Find the native libuuid includes and library.
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
#   Libuuid_INCLUDE_DIRS   - where to find uv.h, etc.
#   Libuuid_LIBRARIES      - List of libraries when using libuuid.
#   Libuuid_FOUND          - True if libuuid found.
#
# ::
#
#
# Hints
# ^^^^^
#
# A user may set ``LIBUUID_ROOT`` to a libuuid installation root to tell this
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

unset(_LIBUUID_SEARCH_ROOT_INC)
unset(_LIBUUID_SEARCH_ROOT_LIB)

# Search LIBUUID_ROOT first if it is set.
if (Libuuid_ROOT)
  set(LIBUUID_ROOT ${Libuuid_ROOT})
endif()

if(LIBUUID_ROOT)
  set(_LIBUUID_SEARCH_ROOT_INC PATHS ${LIBUUID_ROOT} ${LIBUUID_ROOT}/include NO_DEFAULT_PATH)
  set(_LIBUUID_SEARCH_ROOT_LIB PATHS ${LIBUUID_ROOT} ${LIBUUID_ROOT}/lib NO_DEFAULT_PATH)
endif()

# Normal search.
set(Libuuid_NAMES uuid libuuid)

# Try each search configuration.
find_path(Libuuid_INCLUDE_DIRS    NAMES uuid/uuid.h           ${_LIBUUID_SEARCH_ROOT_INC})
find_library(Libuuid_LIBRARIES    NAMES ${Libuuid_NAMES}    ${_LIBUUID_SEARCH_ROOT_LIB})

mark_as_advanced(Libuuid_INCLUDE_DIRS Libuuid_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBUUID_FOUND to TRUE if
# all listed variables are TRUE
include("FindPackageHandleStandardArgs")
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libuuid
  REQUIRED_VARS Libuuid_INCLUDE_DIRS Libuuid_LIBRARIES
  FOUND_VAR Libuuid_FOUND
)

if(Libuuid_FOUND)
    set(LIBUUID_FOUND ${Libuuid_FOUND})
endif()
