#.rst:
# FindMongoc
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
#   Mongoc_INCLUDE_DIRS   - where to find uv.h, etc.
#   Mongoc_LIBRARIES      - List of libraries when using libuv.
#   Mongoc_FOUND          - True if libuv found.
#
# ::
#
#
# Hints
# ^^^^^
#
# A user may set ``MONGOC_ROOT`` to a libuv installation root to tell this
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

unset(_MONGOC_SEARCH_ROOT_INC)
unset(_MONGOC_SEARCH_ROOT_LIB)

# Search MONGOC_ROOT first if it is set.
if(Mongoc_ROOT)
  set(MONGOC_ROOT ${Mongoc_ROOT})
endif()

if(MONGOC_ROOT)
  set(_MONGOC_SEARCH_ROOT_INC PATHS ${MONGOC_ROOT}/include/libmongoc-1.0 NO_DEFAULT_PATH)
  set(_MONGOC_SEARCH_ROOT_LIB PATHS ${MONGOC_ROOT}/lib NO_DEFAULT_PATH)

  set(_BSON_SEARCH_ROOT_INC PATHS ${MONGOC_ROOT}/include/libbson-1.0 NO_DEFAULT_PATH)
  set(_BSON_SEARCH_ROOT_LIB PATHS ${MONGOC_ROOT}/lib NO_DEFAULT_PATH)
endif()

# Normal search.
set(Mongoc_NAMES mongoc-1.0 libmongoc-1.0 mongoc-priv libmongoc-priv)

set(Bson_NAMES bson-1.0 libbson-1.0)

# Try each search configuration.
find_path(Mongoc_INCLUDE_DIRS NAMES mongoc.h ${_MONGOC_SEARCH_ROOT_INC})
find_library(Mongoc_LIBRARIES NAMES ${Mongoc_NAMES} ${_MONGOC_SEARCH_ROOT_LIB})

find_path(Bson_INCLUDE_DIRS NAMES bson.h ${_BSON_SEARCH_ROOT_INC})
find_library(Bson_LIBRARIES NAMES ${Bson_NAMES} ${_BSON_SEARCH_ROOT_LIB})

mark_as_advanced(Mongoc_INCLUDE_DIRS Mongoc_LIBRARIES Bson_INCLUDE_DIRS Bson_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set MONGOC_FOUND to TRUE if all listed variables are TRUE
include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(
  Mongoc
  REQUIRED_VARS Mongoc_INCLUDE_DIRS Mongoc_LIBRARIES
  FOUND_VAR Mongoc_FOUND)

if(Mongoc_FOUND)
  set(MONGOC_FOUND ${Mongoc_FOUND})
  set(Mongoc_INCLUDE_DIRS ${Mongoc_INCLUDE_DIRS} ${Bson_INCLUDE_DIRS})
  # EchoWithColor(COLOR RED "-- Dependency: Mongoc include found.(${Mongoc_INCLUDE_DIRS})")
  set(Mongoc_LIBRARIES ${Mongoc_LIBRARIES} ${Bson_LIBRARIES})
endif()
