#.rst:
# FindPbc
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
#   Pbc_INCLUDE_DIRS   - where to find pbc.h, etc.
#   Pbc_LIBRARIES      - List of libraries when using pbc.
#   Pbc_FOUND          - True if libpbc found.
#
# ::
#
#
# Hints
# ^^^^^
#
# This module reads hints about search locations from variables:
#  Pbc_ROOT            - Preferred installation prefix
#   (or Pbc_ROOT)
#
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

unset(_PBC_SEARCH_ROOT)

if(Pbc_ROOT)
  set(PBC_ROOT ${Pbc_ROOT})
endif()

# Search LIBUV_ROOT first if it is set.
if(PBC_ROOT)
  set(_PBC_SEARCH_ROOT PATHS ${PBC_ROOT} NO_DEFAULT_PATH)
endif()

set(Pbc_NAMES pbc libpbc)

# Try each search configuration.
find_path(Pbc_INCLUDE_DIR NAMES pbc.h ${_PBC_SEARCH_ROOT})
find_library(Pbc_LIBRARY NAMES ${Pbc_NAMES} ${_PBC_SEARCH_ROOT})

mark_as_advanced(Pbc_LIBRARY Pbc_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set LIBUV_FOUND to TRUE if all listed variables are TRUE
include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(
  Pbc
  REQUIRED_VARS Pbc_LIBRARY Pbc_INCLUDE_DIR
  FOUND_VAR Pbc_FOUND)

if(Pbc_FOUND)
  set(Pbc_INCLUDE_DIRS ${Pbc_INCLUDE_DIR})
  set(Pbc_LIBRARIES ${Pbc_LIBRARY})

  set(PBC_FOUND ${Pbc_FOUND})
endif()
