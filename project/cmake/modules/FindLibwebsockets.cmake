#.rst:
# FindLibwebsockets
# --------
#
# Find the native libwebsockets includes and library.
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
#   Libwebsockets_INCLUDE_DIRS   - where to find uv.h, etc.
#   Libwebsockets_LIBRARIES      - List of libraries when using libwebsockets.
#   Libwebsockets_FOUND          - True if libwebsockets found.
#
# ::
#
#
# Hints
# ^^^^^
#
# A user may set ``LIBWEBSOCKETS_ROOT`` to a libwebsockets installation root to tell this
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

unset(_LIBWEBSOCKETS_SEARCH_ROOT)

# Search LIBWEBSOCKETS_ROOT first if it is set.
if (Libwebsockets_ROOT)
  set(LIBWEBSOCKETS_ROOT ${Libwebsockets_ROOT})
endif()

if(LIBWEBSOCKETS_ROOT)
  set(_LIBWEBSOCKETS_SEARCH_ROOT PATHS ${LIBWEBSOCKETS_ROOT} NO_DEFAULT_PATH)
endif()

# Normal search.
set(Libwebsockets_NAMES websockets libwebsockets)

# Try each search configuration.
find_path(Libwebsockets_INCLUDE_DIRS    NAMES libwebsockets.h            ${_LIBWEBSOCKETS_SEARCH_ROOT})
find_library(Libwebsockets_LIBRARIES    NAMES ${Libwebsockets_NAMES}  ${_LIBWEBSOCKETS_SEARCH_ROOT})

mark_as_advanced(Libwebsockets_INCLUDE_DIRS Libwebsockets_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBWEBSOCKETS_FOUND to TRUE if
# all listed variables are TRUE
include("FindPackageHandleStandardArgs")
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libwebsockets
  REQUIRED_VARS Libwebsockets_INCLUDE_DIRS Libwebsockets_LIBRARIES
  FOUND_VAR Libwebsockets_FOUND
)

if(Libwebsockets_FOUND)
    set(LIBWEBSOCKETS_FOUND ${Libwebsockets_FOUND})
endif()
