#.rst:
# FindLibreSSL
# -----------
#
# Find the LibreSSL encryption library.
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``LibreSSL::SSL``
#   The LibreSSL ``ssl`` library, if found.
# ``LibreSSL::Crypto``
#   The LibreSSL ``crypto`` library, if found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``LIBRESSL_FOUND``
#   System has the LibreSSL library.
# ``LIBRESSL_INCLUDE_DIR``
#   The LibreSSL include directory.
# ``LIBRESSL_CRYPTO_LIBRARY``
#   The LibreSSL crypto library.
# ``LIBRESSL_SSL_LIBRARY``
#   The LibreSSL SSL library.
# ``LIBRESSL_LIBRARIES``
#   All LibreSSL libraries.
# ``LIBRESSL_VERSION``
#   This is set to ``$major.$minor.$revision$patch`` (e.g. ``0.9.8s``).
#
# Hints
# ^^^^^
#
# Set ``LIBRESSL_ROOT_DIR`` to the root directory of an LibreSSL installation.
# Set ``LIBRESSL_USE_STATIC_LIBS`` to ``TRUE`` to look for static libraries.
# Set ``LIBRESSL_MSVC_STATIC_RT`` set ``TRUE`` to choose the MT version of the lib.

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
# Copyright 2009-2011 Mathieu Malaterre <mathieu.malaterre@gmail.com>
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

if (UNIX)
  find_package(PkgConfig QUIET)
  pkg_check_modules(_LIBRESSL QUIET openssl)
endif ()

# Support preference of static libs by adjusting CMAKE_FIND_LIBRARY_SUFFIXES
if(LIBRESSL_USE_STATIC_LIBS)
  set(_openssl_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a )
  endif()
endif()

if (WIN32)
  # http://slproweb.com/products/Win32OpenSSL.html
  set(_LIBRESSL_ROOT_HINTS
    ${LIBRESSL_ROOT_DIR}
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\LibreSSL (32-bit)_is1;Inno Setup: App Path]"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\LibreSSL (64-bit)_is1;Inno Setup: App Path]"
    ENV LIBRESSL_ROOT_DIR
    )
  file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
  set(_LIBRESSL_ROOT_PATHS
    "${_programfiles}/LibreSSL"
    "${_programfiles}/LibreSSL-Win32"
    "${_programfiles}/LibreSSL-Win64"
    "C:/LibreSSL/"
    "C:/LibreSSL-Win32/"
    "C:/LibreSSL-Win64/"
    )
  unset(_programfiles)
else ()
  set(_LIBRESSL_ROOT_HINTS
    ${LIBRESSL_ROOT_DIR}
    ENV LIBRESSL_ROOT_DIR
    )
endif ()

set(_LIBRESSL_ROOT_HINTS_AND_PATHS
    HINTS ${_LIBRESSL_ROOT_HINTS}
    PATHS ${_LIBRESSL_ROOT_PATHS}
    )

find_path(LIBRESSL_INCLUDE_DIR
  NAMES
    openssl/ssl.h
  ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
  HINTS
    ${_LIBRESSL_INCLUDEDIR}
  PATH_SUFFIXES
    include
)

if(WIN32 AND NOT CYGWIN)
  if(MSVC)
    # /MD and /MDd are the standard values - if someone wants to use
    # others, the libnames have to change here too
    # use also ssl and ssleay32 in debug as fallback for openssl < 0.9.8b
    # enable LIBRESSL_MSVC_STATIC_RT to get the libs build /MT (Multithreaded no-DLL)
    # In Visual C++ naming convention each of these four kinds of Windows libraries has it's standard suffix:
    #   * MD for dynamic-release
    #   * MDd for dynamic-debug
    #   * MT for static-release
    #   * MTd for static-debug

    # Implementation details:
    # We are using the libraries located in the VC subdir instead of the parent directory eventhough :
    # libeay32MD.lib is identical to ../libeay32.lib, and
    # ssleay32MD.lib is identical to ../ssleay32.lib
    # enable LIBRESSL_USE_STATIC_LIBS to use the static libs located in lib/VC/static

    if (LIBRESSL_MSVC_STATIC_RT)
      set(_LIBRESSL_MSVC_RT_MODE "MT")
    else ()
      set(_LIBRESSL_MSVC_RT_MODE "MD")
    endif ()

    if(LIBRESSL_USE_STATIC_LIBS)
      set(_LIBRESSL_PATH_SUFFIXES
        "lib"
        "VC/static"
        "lib/VC/static"
        )
    else()
      set(_LIBRESSL_PATH_SUFFIXES
        "lib"
        "VC"
        "lib/VC"
        )
    endif ()

    find_library(LIB_EAY_DEBUG
      NAMES
        libeay32${_LIBRESSL_MSVC_RT_MODE}d
        libeay32d
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        ${_LIBRESSL_PATH_SUFFIXES}
    )

    find_library(LIB_EAY_RELEASE
      NAMES
        libeay32${_LIBRESSL_MSVC_RT_MODE}
        libeay32
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        ${_LIBRESSL_PATH_SUFFIXES}
    )

    find_library(SSL_EAY_DEBUG
      NAMES
        ssleay32${_LIBRESSL_MSVC_RT_MODE}d
        ssleay32d
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        ${_LIBRESSL_PATH_SUFFIXES}
    )

    find_library(SSL_EAY_RELEASE
      NAMES
        ssleay32${_LIBRESSL_MSVC_RT_MODE}
        ssleay32
        ssl
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        ${_LIBRESSL_PATH_SUFFIXES}
    )

    set(LIB_EAY_LIBRARY_DEBUG "${LIB_EAY_DEBUG}")
    set(LIB_EAY_LIBRARY_RELEASE "${LIB_EAY_RELEASE}")
    set(SSL_EAY_LIBRARY_DEBUG "${SSL_EAY_DEBUG}")
    set(SSL_EAY_LIBRARY_RELEASE "${SSL_EAY_RELEASE}")

    include(${CMAKE_CURRENT_LIST_DIR}/SelectLibraryConfigurations.cmake)
    select_library_configurations(LIB_EAY)
    select_library_configurations(SSL_EAY)

    mark_as_advanced(LIB_EAY_LIBRARY_DEBUG LIB_EAY_LIBRARY_RELEASE
                     SSL_EAY_LIBRARY_DEBUG SSL_EAY_LIBRARY_RELEASE)
    set(LIBRESSL_SSL_LIBRARY ${SSL_EAY_LIBRARY} )
    set(LIBRESSL_CRYPTO_LIBRARY ${LIB_EAY_LIBRARY} )
    set(LIBRESSL_LIBRARIES ${SSL_EAY_LIBRARY} ${LIB_EAY_LIBRARY} )
  elseif(MINGW)
    # same player, for MinGW
    set(LIB_EAY_NAMES crypto libeay32)
    set(SSL_EAY_NAMES ssl ssleay32)
    find_library(LIB_EAY
      NAMES
        ${LIB_EAY_NAMES}
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        "lib"
        "lib/MinGW"
    )

    find_library(SSL_EAY
      NAMES
        ${SSL_EAY_NAMES}
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        "lib"
        "lib/MinGW"
    )

    mark_as_advanced(SSL_EAY LIB_EAY)
    set(LIBRESSL_SSL_LIBRARY ${SSL_EAY} )
    set(LIBRESSL_CRYPTO_LIBRARY ${LIB_EAY} )
    set(LIBRESSL_LIBRARIES ${SSL_EAY} ${LIB_EAY} )
    unset(LIB_EAY_NAMES)
    unset(SSL_EAY_NAMES)
  else()
    # Not sure what to pick for -say- intel, let's use the toplevel ones and hope someone report issues:
    find_library(LIB_EAY
      NAMES
        libeay32
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      HINTS
        ${_LIBRESSL_LIBDIR}
      PATH_SUFFIXES
        lib
    )

    find_library(SSL_EAY
      NAMES
        ssleay32
      NAMES_PER_DIR
      ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
      HINTS
        ${_LIBRESSL_LIBDIR}
      PATH_SUFFIXES
        lib
    )

    mark_as_advanced(SSL_EAY LIB_EAY)
    set(LIBRESSL_SSL_LIBRARY ${SSL_EAY} )
    set(LIBRESSL_CRYPTO_LIBRARY ${LIB_EAY} )
    set(LIBRESSL_LIBRARIES ${SSL_EAY} ${LIB_EAY} )
  endif()
else()

  find_library(LIBRESSL_SSL_LIBRARY
    NAMES
      ssl
      ssleay32
      ssleay32MD
    NAMES_PER_DIR
    ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
    HINTS
      ${_LIBRESSL_LIBDIR}
    PATH_SUFFIXES
      lib
  )

  find_library(LIBRESSL_CRYPTO_LIBRARY
    NAMES
      crypto
    NAMES_PER_DIR
    ${_LIBRESSL_ROOT_HINTS_AND_PATHS}
    HINTS
      ${_LIBRESSL_LIBDIR}
    PATH_SUFFIXES
      lib
  )

  mark_as_advanced(LIBRESSL_CRYPTO_LIBRARY LIBRESSL_SSL_LIBRARY)

  # compat defines
  set(LIBRESSL_SSL_LIBRARIES ${LIBRESSL_SSL_LIBRARY})
  set(LIBRESSL_CRYPTO_LIBRARIES ${LIBRESSL_CRYPTO_LIBRARY})

  set(LIBRESSL_LIBRARIES ${LIBRESSL_SSL_LIBRARY} ${LIBRESSL_CRYPTO_LIBRARY})

endif()

function(from_hex HEX DEC)
  string(TOUPPER "${HEX}" HEX)
  set(_res 0)
  string(LENGTH "${HEX}" _strlen)

  while (_strlen GREATER 0)
    math(EXPR _res "${_res} * 16")
    string(SUBSTRING "${HEX}" 0 1 NIBBLE)
    string(SUBSTRING "${HEX}" 1 -1 HEX)
    if (NIBBLE STREQUAL "A")
      math(EXPR _res "${_res} + 10")
    elseif (NIBBLE STREQUAL "B")
      math(EXPR _res "${_res} + 11")
    elseif (NIBBLE STREQUAL "C")
      math(EXPR _res "${_res} + 12")
    elseif (NIBBLE STREQUAL "D")
      math(EXPR _res "${_res} + 13")
    elseif (NIBBLE STREQUAL "E")
      math(EXPR _res "${_res} + 14")
    elseif (NIBBLE STREQUAL "F")
      math(EXPR _res "${_res} + 15")
    else()
      math(EXPR _res "${_res} + ${NIBBLE}")
    endif()

    string(LENGTH "${HEX}" _strlen)
  endwhile()

  set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

if (LIBRESSL_INCLUDE_DIR)
  if(LIBRESSL_INCLUDE_DIR AND EXISTS "${LIBRESSL_INCLUDE_DIR}/openssl/opensslv.h")
    file(STRINGS "${LIBRESSL_INCLUDE_DIR}/openssl/opensslv.h" openssl_version_str
         REGEX "^#[\t ]*define[\t ]+LIBRESSL_VERSION_NUMBER[\t ]+0x([0-9a-fA-F])+.*")

    # The version number is encoded as 0xMNNFFPPS: major minor fix patch status
    # The status gives if this is a developer or prerelease and is ignored here.
    # Major, minor, and fix directly translate into the version numbers shown in
    # the string. The patch field translates to the single character suffix that
    # indicates the bug fix state, which 00 -> nothing, 01 -> a, 02 -> b and so
    # on.

    string(REGEX REPLACE "^.*LIBRESSL_VERSION_NUMBER[\t ]+0x([0-9a-fA-F])([0-9a-fA-F][0-9a-fA-F])([0-9a-fA-F][0-9a-fA-F])([0-9a-fA-F][0-9a-fA-F])([0-9a-fA-F]).*$"
           "\\1;\\2;\\3;\\4;\\5" LIBRESSL_VERSION_LIST "${openssl_version_str}")
    list(GET LIBRESSL_VERSION_LIST 0 LIBRESSL_VERSION_MAJOR)
    list(GET LIBRESSL_VERSION_LIST 1 LIBRESSL_VERSION_MINOR)
    from_hex("${LIBRESSL_VERSION_MINOR}" LIBRESSL_VERSION_MINOR)
    list(GET LIBRESSL_VERSION_LIST 2 LIBRESSL_VERSION_FIX)
    from_hex("${LIBRESSL_VERSION_FIX}" LIBRESSL_VERSION_FIX)
    list(GET LIBRESSL_VERSION_LIST 3 LIBRESSL_VERSION_PATCH)

    if (NOT LIBRESSL_VERSION_PATCH STREQUAL "00")
      from_hex("${LIBRESSL_VERSION_PATCH}" _tmp)
      # 96 is the ASCII code of 'a' minus 1
      math(EXPR LIBRESSL_VERSION_PATCH_ASCII "${_tmp} + 96")
      unset(_tmp)
      # Once anyone knows how LibreSSL would call the patch versions beyond 'z'
      # this should be updated to handle that, too. This has not happened yet
      # so it is simply ignored here for now.
      string(ASCII "${LIBRESSL_VERSION_PATCH_ASCII}" LIBRESSL_VERSION_PATCH_STRING)
    endif ()

    set(LIBRESSL_VERSION "${LIBRESSL_VERSION_MAJOR}.${LIBRESSL_VERSION_MINOR}.${LIBRESSL_VERSION_FIX}${LIBRESSL_VERSION_PATCH_STRING}")
  endif ()
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)

if (LIBRESSL_VERSION)
  find_package_handle_standard_args(LibreSSL
    REQUIRED_VARS
      LIBRESSL_LIBRARIES
      LIBRESSL_INCLUDE_DIR
    VERSION_VAR
      LIBRESSL_VERSION
    FAIL_MESSAGE
      "Could NOT find LibreSSL, try to set the path to LibreSSL root folder in the system variable LIBRESSL_ROOT_DIR"
  )
else ()
  find_package_handle_standard_args(LibreSSL "Could NOT find LibreSSL, try to set the path to LibreSSL root folder in the system variable LIBRESSL_ROOT_DIR"
    LIBRESSL_LIBRARIES
    LIBRESSL_INCLUDE_DIR
  )
endif ()

mark_as_advanced(LIBRESSL_INCLUDE_DIR LIBRESSL_LIBRARIES)

if(LIBRESSL_FOUND)
  if(NOT TARGET LibreSSL::Crypto AND
      (EXISTS "${LIBRESSL_CRYPTO_LIBRARY}" OR
        EXISTS "${LIB_EAY_LIBRARY_DEBUG}" OR
        EXISTS "${LIB_EAY_LIBRARY_RELEASE}")
      )
    add_library(LibreSSL::Crypto UNKNOWN IMPORTED)
    set_target_properties(LibreSSL::Crypto PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LIBRESSL_INCLUDE_DIR}")
    if(EXISTS "${LIBRESSL_CRYPTO_LIBRARY}")
      set_target_properties(LibreSSL::Crypto PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${LIBRESSL_CRYPTO_LIBRARY}")
    endif()
    if(EXISTS "${LIB_EAY_LIBRARY_DEBUG}")
      set_property(TARGET LibreSSL::Crypto APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(LibreSSL::Crypto PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
        IMPORTED_LOCATION_DEBUG "${LIB_EAY_LIBRARY_DEBUG}")
    endif()
    if(EXISTS "${LIB_EAY_LIBRARY_RELEASE}")
      set_property(TARGET LibreSSL::Crypto APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(LibreSSL::Crypto PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
        IMPORTED_LOCATION_RELEASE "${LIB_EAY_LIBRARY_RELEASE}")
    endif()
  endif()
  if(NOT TARGET LibreSSL::SSL AND
      (EXISTS "${LIBRESSL_SSL_LIBRARY}" OR
        EXISTS "${SSL_EAY_LIBRARY_DEBUG}" OR
        EXISTS "${SSL_EAY_LIBRARY_RELEASE}")
      )
    add_library(LibreSSL::SSL UNKNOWN IMPORTED)
    set_target_properties(LibreSSL::SSL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LIBRESSL_INCLUDE_DIR}")
    if(EXISTS "${LIBRESSL_SSL_LIBRARY}")
      set_target_properties(LibreSSL::SSL PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${LIBRESSL_SSL_LIBRARY}")
    endif()
    if(EXISTS "${SSL_EAY_LIBRARY_DEBUG}")
      set_property(TARGET LibreSSL::SSL APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(LibreSSL::SSL PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
        IMPORTED_LOCATION_DEBUG "${SSL_EAY_LIBRARY_DEBUG}")
    endif()
    if(EXISTS "${SSL_EAY_LIBRARY_RELEASE}")
      set_property(TARGET LibreSSL::SSL APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(LibreSSL::SSL PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
        IMPORTED_LOCATION_RELEASE "${SSL_EAY_LIBRARY_RELEASE}")
    endif()
    if(TARGET LibreSSL::Crypto)
      set_target_properties(LibreSSL::SSL PROPERTIES
        INTERFACE_LINK_LIBRARIES LibreSSL::Crypto)
    endif()
  endif()
endif()

# Restore the original find library ordering
if(LIBRESSL_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_openssl_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
endif()
