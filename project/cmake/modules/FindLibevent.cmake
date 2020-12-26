# .rst: FindLibevent
# --------
#
# Find the native libevent includes and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
# Libevent_INCLUDE_DIRS   - where to find event2/event.h, etc. Libevent_LIBRARIES      - List of libraries when using libevent.
# Libevent_FOUND          - True if libevent found.
#
# ::
#
# Hints ^^^^^
#
# A user may set ``LIBEVENT_ROOT`` to a libevent installation root to tell this module where to look.

# =============================================================================
# Copyright 2014-2015 OWenT.
#
# Distributed under the OSI-approved BSD License (the "License"); see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE. See the License for more information.
# =============================================================================
# (To distribute this file outside of CMake, substitute the full License text for the above reference.)

unset(_LIBEVENT_SEARCH_ROOT_INC)
unset(_LIBEVENT_SEARCH_ROOT_LIB)

# Search LIBEVENT_ROOT first if it is set.
if(Libevent_ROOT)
  set(LIBEVENT_ROOT ${Libevent_ROOT})
endif()

if(LIBEVENT_ROOT)
  set(_LIBEVENT_SEARCH_ROOT_INC PATHS ${LIBEVENT_ROOT} ${LIBEVENT_ROOT}/include NO_DEFAULT_PATH)
  set(_LIBEVENT_SEARCH_ROOT_LIB PATHS ${LIBEVENT_ROOT} ${LIBEVENT_ROOT}/lib NO_DEFAULT_PATH)
endif()

# Normal search.
set(Libevent_NAMES event libevent)

# Try each search configuration.
find_path(Libevent_INCLUDE_DIRS NAMES event.h ${_LIBEVENT_SEARCH_ROOT_INC})
find_library(Libevent_LIBRARIES NAMES ${Libevent_NAMES} ${_LIBEVENT_SEARCH_ROOT_LIB})

mark_as_advanced(Libevent_INCLUDE_DIRS Libevent_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set LIBEVENT_FOUND to TRUE if all listed variables are TRUE
include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(
  Libevent
  REQUIRED_VARS Libevent_INCLUDE_DIRS Libevent_LIBRARIES
  FOUND_VAR Libevent_FOUND)

if(Libevent_FOUND)
  set(LIBEVENT_FOUND ${Libevent_FOUND})
endif()
