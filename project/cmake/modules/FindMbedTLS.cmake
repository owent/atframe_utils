#.rst:
# FindMbedTLS
# --------
#
# Find the native mbemtls includes and library.
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
#   MbedTLS_INCLUDE_DIRS      - where to find uv.h, etc.
#   MbedTLS_LIBRARIES         - List of all libraries when using mbemtls.
#   MbedTLS_FOUND             - True if mbemtls found.
#   MbedTLS_TLS_LIBRARIES     - List of tls libraries when using mbemtls.
#   MbedTLS_CRYPTO_LIBRARIES  - List of crypto libraries when using mbemtls.
#   MbedTLS_X509_LIBRARIES    - List of x509 libraries when using mbemtls.
#
# ::
#
#
# Hints
# ^^^^^
#
# A user may set ``MBEDTLS_ROOT`` to a mbemtls installation root to tell this
# module where to look.

# =============================================================================
# Copyright 2014-2016 OWenT.
#
# Distributed under the OSI-approved BSD License (the "License"); see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE. See the License for more information.
# =============================================================================
# (To distribute this file outside of CMake, substitute the full License text for the above reference.)

unset(_MBEDTLS_SEARCH_ROOT_INC)
unset(_MBEDTLS_SEARCH_ROOT_LIB)

# Search MBEDTLS_ROOT first if it is set.
if(MbedTLS_ROOT)
  set(MBEDTLS_ROOT ${MbedTLS_ROOT})
endif()

if(MBEDTLS_ROOT)
  set(_MBEDTLS_SEARCH_ROOT_INC PATHS ${MBEDTLS_ROOT} ${MBEDTLS_ROOT}/include NO_DEFAULT_PATH)
  set(_MBEDTLS_SEARCH_ROOT_LIB PATHS ${MBEDTLS_ROOT} ${MBEDTLS_ROOT}/lib NO_DEFAULT_PATH)
endif()

# Normal search.
set(MbedTLS_CRYPTO_NAMES mbedcrypto libmbedcrypto)
set(MbedTLS_TLS_NAMES mbedtls libmbedtls)
set(MbedTLS_X509_NAMES mbedx509 libmbedx509)

# Try each search configuration.
find_path(MbedTLS_INCLUDE_DIRS NAMES mbedtls/config.h ${_MBEDTLS_SEARCH_ROOT_INC})
find_library(MbedTLS_TLS_LIBRARIES NAMES ${MbedTLS_TLS_NAMES} ${_MBEDTLS_SEARCH_ROOT_LIB})
find_library(MbedTLS_CRYPTO_LIBRARIES NAMES ${MbedTLS_CRYPTO_NAMES} ${_MBEDTLS_SEARCH_ROOT_LIB})
find_library(MbedTLS_X509_LIBRARIES NAMES ${MbedTLS_X509_NAMES} ${_MBEDTLS_SEARCH_ROOT_LIB})

mark_as_advanced(MbedTLS_INCLUDE_DIRS MbedTLS_TLS_LIBRARIES MbedTLS_CRYPTO_LIBRARIES MbedTLS_X509_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set MBEDTLS_FOUND to TRUE if all listed variables are TRUE
include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(
  MbedTLS
  REQUIRED_VARS MbedTLS_INCLUDE_DIRS MbedTLS_TLS_LIBRARIES
  FOUND_VAR MbedTLS_FOUND)

if(MbedTLS_FOUND)
  set(MBEDTLS_FOUND ${MbedTLS_FOUND})
  set(MbedTLS_LIBRARIES ${MbedTLS_TLS_LIBRARIES} ${MbedTLS_CRYPTO_LIBRARIES} ${MbedTLS_X509_LIBRARIES})
  if(NOT MbedTLS_CRYPTO_LIBRARIES)
    set(MbedTLS_CRYPTO_LIBRARIES ${MbedTLS_TLS_LIBRARIES})
  endif()
endif()
