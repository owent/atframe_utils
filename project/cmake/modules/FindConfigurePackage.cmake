# .rst: FindConfigurePackage
# ----------------
#
# find package, and try to configure it when not found in system.
#
# FindConfigurePackage( PACKAGE <name> BUILD_WITH_CONFIGURE BUILD_WITH_CMAKE BUILD_WITH_SCONS BUILD_WITH_CUSTOM_COMMAND CONFIGURE_FLAGS
# [configure options...] CMAKE_FLAGS [cmake options...] FIND_PACKAGE_FLAGS [options will be passed into find_package(...)]
# CMAKE_INHIRT_BUILD_ENV CMAKE_INHIRT_BUILD_ENV_DISABLE_C_FLAGS CMAKE_INHIRT_BUILD_ENV_DISABLE_CXX_FLAGS
# CMAKE_INHIRT_BUILD_ENV_DISABLE_ASM_FLAGS SCONS_FLAGS [scons options...] CUSTOM_BUILD_COMMAND [custom build cmd...] MAKE_FLAGS [make
# options...] PREBUILD_COMMAND [run cmd before build ...] AFTERBUILD_COMMAND [run cmd after build ...] RESET_FIND_VARS [cmake vars]
# WORKING_DIRECTORY <work directory> BUILD_DIRECTORY <build directory> PREFIX_DIRECTORY <prefix directory> SRC_DIRECTORY_NAME <source
# directory name> MSVC_CONFIGURE <Debug/Release/RelWithDebInfo/MinSizeRel> INSTALL_TARGET [install targets...] ZIP_URL <zip url> TAR_URL
# <tar url> SVN_URL <svn url> GIT_URL <git url> GIT_BRANCH <git branch> GIT_COMMIT <git commit sha> GIT_FETCH_DEPTH <fetch depth/deepen> )
#
# ::
#
# <configure options>     - flags added to configure command <cmake options>         - flags added to cmake command <scons options> - flags
# added to scons command <custom build cmd>      - custom commands for build <make options>          - flags added to make command <pre
# build cmd>         - commands to run before build tool <work directory>        - work directory <build directory>       - where to execute
# configure and make <prefix directory>      - prefix directory(default: <work directory>) <source directory name> - source directory
# name(default detected by download url) <install targets>       - which target(s) used to install package(default: install) <zip url> -
# from where to download zip when find package failed <tar url>               - from where to download tar.* or tgz when find package failed
# <svn url>               - from where to svn co when find package failed <git url>               - from where to git clone when find
# package failed <git branch>            - git branch or tag to fetch <git commit>            - git commit to fetch, server must support
# --deepen=<depth>. if both <git branch> and <git commit> is set, we will use <git branch> <fetch depth/deepen>    - --deepen or --depth for
# git fetch depend using <git branch> or <git commit>

# =============================================================================
# Copyright 2014-2020 OWenT.
#
# Distributed under the OSI-approved BSD License (the "License"); see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE. See the License for more information.
# =============================================================================
# (To distribute this file outside of CMake, substitute the full License text for the above reference.)

include("${CMAKE_CURRENT_LIST_DIR}/ProjectBuildTools.cmake")

function(FindConfigurePackageDownloadFile from to)
  find_program(WGET_FULL_PATH wget)
  if(WGET_FULL_PATH)
    execute_process(COMMAND ${WGET_FULL_PATH} --no-check-certificate -v ${from} -O ${to})
  else()
    find_program(CURL_FULL_PATH curl)
    if(CURL_FULL_PATH)
      execute_process(COMMAND ${CURL_FULL_PATH} --insecure -L ${from} -o ${to})
    else()
      file(DOWNLOAD ${from} ${to} SHOW_PROGRESS)
    endif()
  endif()
endfunction()

function(FindConfigurePackageUnzip src work_dir)
  if(CMAKE_HOST_UNIX)
    find_program(FindConfigurePackage_UNZIP_BIN unzip)
    if(FindConfigurePackage_UNZIP_BIN)
      execute_process(COMMAND ${FindConfigurePackage_UNZIP_BIN} -uo ${src} WORKING_DIRECTORY ${work_dir})
      return()
    endif()
  endif()
  # fallback
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ${src} WORKING_DIRECTORY ${work_dir})
endfunction()

function(FindConfigurePackageTarXV src work_dir)
  if(CMAKE_HOST_UNIX)
    find_program(FindConfigurePackage_TAR_BIN tar)
    if(FindConfigurePackage_TAR_BIN)
      execute_process(COMMAND ${FindConfigurePackage_TAR_BIN} -xvf ${src} WORKING_DIRECTORY ${work_dir})
      return()
    endif()
  endif()
  # fallback
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ${src} WORKING_DIRECTORY ${work_dir})
endfunction()

function(FindConfigurePackageRemoveEmptyDir DIR)
  if(EXISTS ${DIR})
    file(GLOB RESULT "${DIR}/*")
    list(LENGTH RESULT RES_LEN)
    if(${RES_LEN} EQUAL 0)
      file(REMOVE_RECURSE ${DIR})
    endif()
  endif()
endfunction()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.12")
  set(FindConfigurePackageCMakeBuildMultiJobs
      "--parallel"
      CACHE INTERNAL "Build options for multi-jobs")
  unset(CPU_CORE_NUM)
elseif(
  (CMAKE_MAKE_PROGRAM STREQUAL "make")
  OR (CMAKE_MAKE_PROGRAM STREQUAL "gmake")
  OR (CMAKE_MAKE_PROGRAM STREQUAL "ninja"))
  include(ProcessorCount)
  ProcessorCount(CPU_CORE_NUM)
  set(FindConfigurePackageCMakeBuildMultiJobs
      "--" "-j${CPU_CORE_NUM}"
      CACHE INTERNAL "Build options for multi-jobs")
  unset(CPU_CORE_NUM)
elseif(CMAKE_MAKE_PROGRAM STREQUAL "xcodebuild")
  include(ProcessorCount)
  ProcessorCount(CPU_CORE_NUM)
  set(FindConfigurePackageCMakeBuildMultiJobs
      "--" "-jobs" ${CPU_CORE_NUM}
      CACHE INTERNAL "Build options for multi-jobs")
  unset(CPU_CORE_NUM)
elseif(CMAKE_VS_MSBUILD_COMMAND)
  set(FindConfigurePackageCMakeBuildMultiJobs
      "--" "/m"
      CACHE INTERNAL "Build options for multi-jobs")
endif()

if(NOT FindConfigurePackageGitFetchDepth)
  set(FindConfigurePackageGitFetchDepth
      100
      CACHE STRING "Defalut depth of git clone/fetch")
endif()

macro(FindConfigurePackage)
  if(CMAKE_VERSION VERSION_LESS_EQUAL "3.4")
    include(CMakeParseArguments)
  endif()
  set(optionArgs
      BUILD_WITH_CONFIGURE
      BUILD_WITH_CMAKE
      BUILD_WITH_SCONS
      BUILD_WITH_CUSTOM_COMMAND
      GIT_ENABLE_SUBMODULE
      CMAKE_INHIRT_BUILD_ENV
      CMAKE_INHIRT_BUILD_ENV_DISABLE_C_FLAGS
      CMAKE_INHIRT_BUILD_ENV_DISABLE_CXX_FLAGS
      CMAKE_INHIRT_BUILD_ENV_DISABLE_ASM_FLAGS)
  set(oneValueArgs
      PACKAGE
      WORKING_DIRECTORY
      BUILD_DIRECTORY
      PREFIX_DIRECTORY
      SRC_DIRECTORY_NAME
      PROJECT_DIRECTORY
      MSVC_CONFIGURE
      ZIP_URL
      TAR_URL
      SVN_URL
      GIT_URL
      GIT_BRANCH
      GIT_COMMIT
      GIT_FETCH_DEPTH)
  set(multiValueArgs
      CONFIGURE_CMD
      CONFIGURE_FLAGS
      CMAKE_FLAGS
      FIND_PACKAGE_FLAGS
      RESET_FIND_VARS
      SCONS_FLAGS
      MAKE_FLAGS
      CUSTOM_BUILD_COMMAND
      PREBUILD_COMMAND
      AFTERBUILD_COMMAND
      INSTALL_TARGET)
  foreach(RESTORE_VAR IN LISTS optionArgs oneValueArgs multiValueArgs)
    unset(FindConfigurePackage_${RESTORE_VAR})
  endforeach()

  cmake_parse_arguments(FindConfigurePackage "${optionArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FindConfigurePackage_INSTALL_TARGET)
    set(FindConfigurePackage_INSTALL_TARGET "install")
  endif()
  # some module is not match standard, using upper case but package name
  string(TOUPPER "${FindConfigurePackage_PACKAGE}_FOUND" FIND_CONFIGURE_PACKAGE_UPPER_NAME)

  unset(FindConfigurePackage_BACKUP_CMAKE_FIND_ROOT_PATH)
  unset(FindConfigurePackage_BACKUP_CMAKE_PREFIX_PATH)

  # step 1. find using standard method
  find_package(${FindConfigurePackage_PACKAGE} QUIET ${FindConfigurePackage_FIND_PACKAGE_FLAGS})
  if(NOT ${FindConfigurePackage_PACKAGE}_FOUND AND NOT ${FIND_CONFIGURE_PACKAGE_UPPER_NAME})
    if(NOT FindConfigurePackage_PREFIX_DIRECTORY)
      # prefix
      set(FindConfigurePackage_PREFIX_DIRECTORY ${FindConfigurePackage_WORK_DIRECTORY})
    endif()

    set(FindConfigurePackage_BACKUP_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
    set(FindConfigurePackage_BACKUP_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    list(APPEND CMAKE_FIND_ROOT_PATH ${FindConfigurePackage_PREFIX_DIRECTORY})
    list(APPEND CMAKE_PREFIX_PATH ${FindConfigurePackage_PREFIX_DIRECTORY})

    # step 2. find in prefix
    find_package(${FindConfigurePackage_PACKAGE} QUIET ${FindConfigurePackage_FIND_PACKAGE_FLAGS})

    # step 3. build
    if(NOT ${FindConfigurePackage_PACKAGE}_FOUND AND NOT ${FIND_CONFIGURE_PACKAGE_UPPER_NAME})
      set(FindConfigurePackage_UNPACK_SOURCE NO)

      # tar package
      if(NOT FindConfigurePackage_UNPACK_SOURCE AND FindConfigurePackage_TAR_URL)
        get_filename_component(DOWNLOAD_FILENAME "${FindConfigurePackage_TAR_URL}" NAME)
        if(NOT FindConfigurePackage_SRC_DIRECTORY_NAME)
          string(REGEX REPLACE "\\.tar\\.[A-Za-z0-9]+$" "" FindConfigurePackage_SRC_DIRECTORY_NAME "${DOWNLOAD_FILENAME}")
        endif()
        set(FindConfigurePackage_DOWNLOAD_SOURCE_DIR "${FindConfigurePackage_WORKING_DIRECTORY}/${FindConfigurePackage_SRC_DIRECTORY_NAME}")
        findconfigurepackageremoveemptydir(${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})

        if(NOT EXISTS "${FindConfigurePackage_WORKING_DIRECTORY}/${DOWNLOAD_FILENAME}")
          message(STATUS "start to download ${DOWNLOAD_FILENAME} from ${FindConfigurePackage_TAR_URL}")
          findconfigurepackagedownloadfile("${FindConfigurePackage_TAR_URL}"
                                           "${FindConfigurePackage_WORKING_DIRECTORY}/${DOWNLOAD_FILENAME}")
        endif()

        findconfigurepackagetarxv("${FindConfigurePackage_WORKING_DIRECTORY}/${DOWNLOAD_FILENAME}"
                                  ${FindConfigurePackage_WORKING_DIRECTORY})

        if(EXISTS ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
          set(FindConfigurePackage_UNPACK_SOURCE YES)
        endif()
      endif()

      # zip package
      if(NOT FindConfigurePackage_UNPACK_SOURCE AND FindConfigurePackage_ZIP_URL)
        get_filename_component(DOWNLOAD_FILENAME "${FindConfigurePackage_ZIP_URL}" NAME)
        if(NOT FindConfigurePackage_SRC_DIRECTORY_NAME)
          string(REGEX REPLACE "\\.zip$" "" FindConfigurePackage_SRC_DIRECTORY_NAME "${DOWNLOAD_FILENAME}")
        endif()
        set(FindConfigurePackage_DOWNLOAD_SOURCE_DIR "${FindConfigurePackage_WORKING_DIRECTORY}/${FindConfigurePackage_SRC_DIRECTORY_NAME}")
        findconfigurepackageremoveemptydir(${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})

        if(NOT EXISTS "${FindConfigurePackage_WORKING_DIRECTORY}/${DOWNLOAD_FILENAME}")
          message(STATUS "start to download ${DOWNLOAD_FILENAME} from ${FindConfigurePackage_ZIP_URL}")
          findconfigurepackagedownloadfile("${FindConfigurePackage_ZIP_URL}"
                                           "${FindConfigurePackage_WORKING_DIRECTORY}/${DOWNLOAD_FILENAME}")
        endif()

        if(NOT EXISTS ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
          findconfigurepackageunzip("${FindConfigurePackage_WORKING_DIRECTORY}/${DOWNLOAD_FILENAME}"
                                    ${FindConfigurePackage_WORKING_DIRECTORY})
        endif()

        if(EXISTS ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
          set(FindConfigurePackage_UNPACK_SOURCE YES)
        endif()
      endif()

      # git package
      if(NOT FindConfigurePackage_UNPACK_SOURCE AND FindConfigurePackage_GIT_URL)
        get_filename_component(DOWNLOAD_FILENAME "${FindConfigurePackage_GIT_URL}" NAME)
        if(NOT FindConfigurePackage_SRC_DIRECTORY_NAME)
          get_filename_component(FindConfigurePackage_SRC_DIRECTORY_FULL_NAME "${DOWNLOAD_FILENAME}" NAME)
          string(REGEX REPLACE "\\.git$" "" FindConfigurePackage_SRC_DIRECTORY_NAME "${FindConfigurePackage_SRC_DIRECTORY_FULL_NAME}")
        endif()
        set(FindConfigurePackage_DOWNLOAD_SOURCE_DIR "${FindConfigurePackage_WORKING_DIRECTORY}/${FindConfigurePackage_SRC_DIRECTORY_NAME}")
        findconfigurepackageremoveemptydir(${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})

        if(EXISTS "${FindConfigurePackage_DOWNLOAD_SOURCE_DIR}/.git")
          execute_process(
            COMMAND ${GIT_EXECUTABLE} ls-files
            WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR}
            OUTPUT_VARIABLE FindConfigurePackage_GIT_CHECK_REPO)
          string(STRIP "${FindConfigurePackage_GIT_CHECK_REPO}" FindConfigurePackage_GIT_CHECK_REPO)
          if(FindConfigurePackage_GIT_CHECK_REPO STREQUAL "")
            message(STATUS "${FindConfigurePackage_DOWNLOAD_SOURCE_DIR} is not a valid git reposutory, remove it...")
            file(REMOVE_RECURSE ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
          endif()
          unset(FindConfigurePackage_GIT_CHECK_REPO)
        endif()
        if(NOT EXISTS "${FindConfigurePackage_DOWNLOAD_SOURCE_DIR}/.git")
          find_package(Git)
          if(GIT_FOUND)
            if(NOT EXISTS ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
              file(MAKE_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
            endif()
            execute_process(COMMAND ${GIT_EXECUTABLE} init WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
            execute_process(COMMAND ${GIT_EXECUTABLE} remote add origin "${FindConfigurePackage_GIT_URL}"
                            WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
            if(NOT FindConfigurePackage_GIT_BRANCH AND NOT FindConfigurePackage_GIT_COMMIT)
              execute_process(
                COMMAND ${GIT_EXECUTABLE} ls-remote --symref origin HEAD
                WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR}
                OUTPUT_VARIABLE FindConfigurePackage_GIT_CHECK_REPO)
              string(REGEX REPLACE ".*refs/heads/([^ \t]*)[ \t]*HEAD.*" "\\1" FindConfigurePackage_GIT_BRANCH
                                   ${FindConfigurePackage_GIT_CHECK_REPO})
              if(NOT FindConfigurePackage_GIT_BRANCH OR FindConfigurePackage_GIT_BRANCH STREQUAL FindConfigurePackage_GIT_CHECK_REPO)
                string(REGEX REPLACE "([^ \t]*)[ \t]*HEAD.*" "\\1" FindConfigurePackage_GIT_BRANCH ${FindConfigurePackage_GIT_CHECK_REPO})
                if(NOT FindConfigurePackage_GIT_BRANCH OR FindConfigurePackage_GIT_BRANCH STREQUAL FindConfigurePackage_GIT_CHECK_REPO)
                  set(FindConfigurePackage_GIT_BRANCH master)
                endif()
              endif()
              unset(FindConfigurePackage_GIT_CHECK_REPO)
            endif()
            if(NOT FindConfigurePackage_GIT_FETCH_DEPTH)
              set(FindConfigurePackage_GIT_FETCH_DEPTH ${FindConfigurePackageGitFetchDepth})
            endif()
            if(FindConfigurePackage_GIT_BRANCH)
              execute_process(COMMAND ${GIT_EXECUTABLE} fetch "--depth=${FindConfigurePackage_GIT_FETCH_DEPTH}" "-n" origin
                                      ${FindConfigurePackage_GIT_BRANCH} WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
            else()
              if(GIT_VERSION_STRING VERSION_GREATER_EQUAL "2.11.0")
                execute_process(COMMAND ${GIT_EXECUTABLE} fetch "--deepen=${FindConfigurePackage_GIT_FETCH_DEPTH}" "-n" origin
                                        ${FindConfigurePackage_GIT_COMMIT} WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
              else()
                message(WARNING "It's recommended to use git 2.11.0 or upper to only fetch partly of repository.")
                execute_process(COMMAND ${GIT_EXECUTABLE} fetch "-n" origin ${FindConfigurePackage_GIT_COMMIT}
                                WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
              endif()
            endif()
            execute_process(COMMAND ${GIT_EXECUTABLE} reset --hard FETCH_HEAD WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
            if(FindConfigurePackage_GIT_ENABLE_SUBMODULE)
              execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -f
                              WORKING_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
            endif()
          else()
            message(STATUS "git not found, skip ${FindConfigurePackage_GIT_URL}")
          endif()
        endif()

        if(EXISTS ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
          set(FindConfigurePackage_UNPACK_SOURCE YES)
        endif()
      endif()

      # svn package
      if(NOT FindConfigurePackage_UNPACK_SOURCE AND FindConfigurePackage_SVN_URL)
        get_filename_component(DOWNLOAD_FILENAME "${FindConfigurePackage_SVN_URL}" NAME)
        if(NOT FindConfigurePackage_SRC_DIRECTORY_NAME)
          get_filename_component(FindConfigurePackage_SRC_DIRECTORY_NAME "${DOWNLOAD_FILENAME}" NAME)
        endif()
        set(FindConfigurePackage_DOWNLOAD_SOURCE_DIR "${FindConfigurePackage_WORKING_DIRECTORY}/${FindConfigurePackage_SRC_DIRECTORY_NAME}")
        findconfigurepackageremoveemptydir(${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})

        if(NOT EXISTS ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
          find_package(Subversion)
          if(SUBVERSION_FOUND)
            execute_process(
              COMMAND ${Subversion_SVN_EXECUTABLE} co "${FindConfigurePackage_SVN_URL}" "${FindConfigurePackage_SRC_DIRECTORY_NAME}"
              WORKING_DIRECTORY "${FindConfigurePackage_WORKING_DIRECTORY}")
          else()
            message(STATUS "svn not found, skip ${FindConfigurePackage_SVN_URL}")
          endif()
        endif()

        if(EXISTS ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
          set(FindConfigurePackage_UNPACK_SOURCE YES)
        endif()
      endif()

      if(NOT FindConfigurePackage_UNPACK_SOURCE)
        message(FATAL_ERROR "Can not download source for ${FindConfigurePackage_PACKAGE}")
      endif()

      # init build dir
      if(NOT FindConfigurePackage_BUILD_DIRECTORY)
        set(FindConfigurePackage_BUILD_DIRECTORY ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
      endif()
      if(NOT EXISTS ${FindConfigurePackage_BUILD_DIRECTORY})
        file(MAKE_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
      endif()

      # prebuild commands
      foreach(cmd ${FindConfigurePackage_PREBUILD_COMMAND})
        message(STATUS "FindConfigurePackage - Run: ${cmd} @ ${FindConfigurePackage_BUILD_DIRECTORY}")
        execute_process(COMMAND ${cmd} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
      endforeach()

      # build using configure and make
      if(FindConfigurePackage_BUILD_WITH_CONFIGURE)
        if(FindConfigurePackage_PROJECT_DIRECTORY)
          file(RELATIVE_PATH CONFIGURE_EXEC_FILE ${FindConfigurePackage_BUILD_DIRECTORY}
               "${FindConfigurePackage_PROJECT_DIRECTORY}/configure")
        else()
          file(RELATIVE_PATH CONFIGURE_EXEC_FILE ${FindConfigurePackage_BUILD_DIRECTORY}
               "${FindConfigurePackage_WORKING_DIRECTORY}/${FindConfigurePackage_SRC_DIRECTORY_NAME}/configure")
        endif()
        if(${CONFIGURE_EXEC_FILE} STREQUAL "configure")
          set(CONFIGURE_EXEC_FILE "./configure")
        endif()
        message(
          STATUS
            "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${CONFIGURE_EXEC_FILE} --prefix=${FindConfigurePackage_PREFIX_DIRECTORY} ${FindConfigurePackage_CONFIGURE_FLAGS}"
        )
        execute_process(COMMAND ${CONFIGURE_EXEC_FILE} "--prefix=${FindConfigurePackage_PREFIX_DIRECTORY}"
                                ${FindConfigurePackage_CONFIGURE_FLAGS} WORKING_DIRECTORY "${FindConfigurePackage_BUILD_DIRECTORY}")

        if(PROJECT_FIND_CONFIGURE_PACKAGE_PARALLEL_BUILD)
          set(FindConfigurePackageCMakeBuildParallelFlags "-j")
        else()
          unset(FindConfigurePackageCMakeBuildParallelFlags)
        endif()
        execute_process(COMMAND "make" ${FindConfigurePackage_MAKE_FLAGS} ${FindConfigurePackage_INSTALL_TARGET}
                                ${FindConfigurePackageCMakeBuildParallelFlags} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
        unset(FindConfigurePackageCMakeBuildParallelFlags)

        # build using cmake and make
      elseif(FindConfigurePackage_BUILD_WITH_CMAKE)
        if(FindConfigurePackage_PROJECT_DIRECTORY)
          file(RELATIVE_PATH BUILD_WITH_CMAKE_PROJECT_DIR ${FindConfigurePackage_BUILD_DIRECTORY} ${FindConfigurePackage_PROJECT_DIRECTORY})
        else()
          file(RELATIVE_PATH BUILD_WITH_CMAKE_PROJECT_DIR ${FindConfigurePackage_BUILD_DIRECTORY}
               ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
        endif()
        if(NOT BUILD_WITH_CMAKE_PROJECT_DIR)
          set(BUILD_WITH_CMAKE_PROJECT_DIR ".")
        endif()

        set(FindConfigurePackage_BUILD_WITH_CMAKE_GENERATOR "-DCMAKE_INSTALL_PREFIX=${FindConfigurePackage_PREFIX_DIRECTORY}")

        if(FindConfigurePackage_CMAKE_INHIRT_BUILD_ENV)
          set(FindConfigurePackage_CMAKE_INHIRT_BUILD_ENV OFF)
          set(project_build_tools_append_cmake_inherit_options_CALL_VARS FindConfigurePackage_BUILD_WITH_CMAKE_GENERATOR)
          if(FindConfigurePackage_CMAKE_INHIRT_BUILD_ENV_DISABLE_C_FLAGS)
            list(APPEND project_build_tools_append_cmake_inherit_options_CALL_VARS DISABLE_C_FLAGS)
          endif()
          if(FindConfigurePackage_CMAKE_INHIRT_BUILD_ENV_DISABLE_CXX_FLAGS)
            list(APPEND project_build_tools_append_cmake_inherit_options_CALL_VARS DISABLE_CXX_FLAGS)
          endif()
          if(FindConfigurePackage_CMAKE_INHIRT_BUILD_ENV_DISABLE_ASM_FLAGS)
            list(APPEND project_build_tools_append_cmake_inherit_options_CALL_VARS DISABLE_ASM_FLAGS)
          endif()
          project_build_tools_append_cmake_inherit_options(${project_build_tools_append_cmake_inherit_options_CALL_VARS})
          unset(project_build_tools_append_cmake_inherit_options_CALL_VARS)
        endif()

        message(
          STATUS
            "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${CMAKE_COMMAND} ${BUILD_WITH_CMAKE_PROJECT_DIR} ${FindConfigurePackage_BUILD_WITH_CMAKE_GENERATOR} ${FindConfigurePackage_CMAKE_FLAGS}"
        )

        execute_process(COMMAND ${CMAKE_COMMAND} ${BUILD_WITH_CMAKE_PROJECT_DIR} ${FindConfigurePackage_BUILD_WITH_CMAKE_GENERATOR}
                                ${FindConfigurePackage_CMAKE_FLAGS} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})

        # cmake --build and install
        if(PROJECT_FIND_CONFIGURE_PACKAGE_PARALLEL_BUILD)
          set(FindConfigurePackageCMakeBuildParallelFlags "-j")
        else()
          unset(FindConfigurePackageCMakeBuildParallelFlags)
        endif()
        if(MSVC)
          if(FindConfigurePackage_MSVC_CONFIGURE)
            message(
              STATUS
                "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config ${FindConfigurePackage_MSVC_CONFIGURE} ${FindConfigurePackageCMakeBuildParallelFlags}"
            )
            execute_process(
              COMMAND ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config
                      ${FindConfigurePackage_MSVC_CONFIGURE} ${FindConfigurePackageCMakeBuildParallelFlags}
              WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
          else()
            message(
              STATUS
                "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config Debug ${FindConfigurePackageCMakeBuildParallelFlags}"
            )
            execute_process(
              COMMAND ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config Debug
                      ${FindConfigurePackageCMakeBuildParallelFlags} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
            message(
              STATUS
                "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config Release ${FindConfigurePackageCMakeBuildParallelFlags}"
            )
            execute_process(
              COMMAND ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config Release
                      ${FindConfigurePackageCMakeBuildParallelFlags} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
            if(CMAKE_BUILD_TYPE)
              message(
                STATUS
                  "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config ${CMAKE_BUILD_TYPE} ${FindConfigurePackageCMakeBuildParallelFlags}"
              )
              execute_process(
                COMMAND ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} --config ${CMAKE_BUILD_TYPE}
                        ${FindConfigurePackageCMakeBuildParallelFlags} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
            endif()
          endif()

        else()
          message(
            STATUS
              "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET} ${FindConfigurePackageCMakeBuildParallelFlags}"
          )
          execute_process(COMMAND ${CMAKE_COMMAND} --build . --target ${FindConfigurePackage_INSTALL_TARGET}
                                  ${FindConfigurePackageCMakeBuildParallelFlags} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
        endif()
        unset(FindConfigurePackageCMakeBuildParallelFlags)

        # adaptor for new cmake module
        set("${FindConfigurePackage_PACKAGE}_ROOT" ${FindConfigurePackage_PREFIX_DIRECTORY})

        # build using scons
      elseif(FindConfigurePackage_BUILD_WITH_SCONS)
        if(FindConfigurePackage_PROJECT_DIRECTORY)
          file(RELATIVE_PATH BUILD_WITH_SCONS_PROJECT_DIR ${FindConfigurePackage_BUILD_DIRECTORY} ${FindConfigurePackage_PROJECT_DIRECTORY})
        else()
          file(RELATIVE_PATH BUILD_WITH_SCONS_PROJECT_DIR ${FindConfigurePackage_BUILD_DIRECTORY}
               ${FindConfigurePackage_DOWNLOAD_SOURCE_DIR})
        endif()
        if(NOT BUILD_WITH_SCONS_PROJECT_DIR)
          set(BUILD_WITH_SCONS_PROJECT_DIR ".")
        endif()

        set(OLD_ENV_PREFIX $ENV{prefix})
        set(ENV{prefix} ${FindConfigurePackage_PREFIX_DIRECTORY})
        message(
          STATUS "@${FindConfigurePackage_BUILD_DIRECTORY} Run: scons ${FindConfigurePackage_SCONS_FLAGS} ${BUILD_WITH_SCONS_PROJECT_DIR}")
        execute_process(COMMAND "scons" ${FindConfigurePackage_SCONS_FLAGS} ${BUILD_WITH_SCONS_PROJECT_DIR}
                        WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
        set(ENV{prefix} ${OLD_ENV_PREFIX})

        # build using custom commands(such as gyp)
      elseif(FindConfigurePackage_BUILD_WITH_CUSTOM_COMMAND)
        foreach(cmd ${FindConfigurePackage_CUSTOM_BUILD_COMMAND})
          message(STATUS "@${FindConfigurePackage_BUILD_DIRECTORY} Run: ${cmd}")
          execute_process(COMMAND ${cmd} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
        endforeach()

      else()
        message(FATAL_ERROR "build type is required")
      endif()

      # afterbuild commands
      foreach(cmd ${FindConfigurePackage_AFTERBUILD_COMMAND})
        message(STATUS "FindConfigurePackage - Run: ${cmd} @ ${FindConfigurePackage_BUILD_DIRECTORY}")
        execute_process(COMMAND ${cmd} WORKING_DIRECTORY ${FindConfigurePackage_BUILD_DIRECTORY})
      endforeach()

      # reset vars before retry to find package
      foreach(RESET_VAR ${FindConfigurePackage_RESET_FIND_VARS})
        unset(${RESET_VAR} CACHE)
        unset(${RESET_VAR})
      endforeach()
      unset(${FindConfigurePackage_PACKAGE}_FOUND CACHE)
      unset(${FindConfigurePackage_PACKAGE}_FOUND)
      find_package(${FindConfigurePackage_PACKAGE} ${FindConfigurePackage_FIND_PACKAGE_FLAGS})
    endif()
  endif()

  # Cleanup vars
  unset(FindConfigurePackage_INSTALL_TARGET)
  if(DEFINED FindConfigurePackage_BACKUP_CMAKE_FIND_ROOT_PATH)
    set(CMAKE_FIND_ROOT_PATH ${FindConfigurePackage_BACKUP_CMAKE_FIND_ROOT_PATH})
    unset(FindConfigurePackage_BACKUP_CMAKE_FIND_ROOT_PATH)
  endif()
  if(DEFINED FindConfigurePackage_BACKUP_CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH ${FindConfigurePackage_BACKUP_CMAKE_PREFIX_PATH})
    unset(FindConfigurePackage_BACKUP_CMAKE_PREFIX_PATH)
  endif()
endmacro(FindConfigurePackage)
