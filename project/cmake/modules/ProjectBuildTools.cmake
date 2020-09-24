#.rst:
# ProjectBuildTools
# ----------------
#
# build tools
#

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.10")
    include_guard(GLOBAL)
endif()

set (PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_C
    CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
    CMAKE_C_COMPILER CMAKE_C_COMPILER_TARGET CMAKE_C_COMPILER_LAUNCHER CMAKE_C_COMPILER_AR CMAKE_C_COMPILER_RANLIB CMAKE_C_LINK_LIBRARY_SUFFIX
    CMAKE_C_STANDARD_INCLUDE_DIRECTORIES CMAKE_C_STANDARD_LIBRARIES
    CMAKE_OBJC_EXTENSIONS
)

set (PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_CXX
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL
    CMAKE_CXX_COMPILER CMAKE_CXX_COMPILER_TARGET CMAKE_CXX_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_AR CMAKE_CXX_COMPILER_RANLIB CMAKE_CXX_LINK_LIBRARY_SUFFIX
    ANDROID_CPP_FEATURES ANDROID_STL
    CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES CMAKE_CXX_STANDARD_LIBRARIES
    CMAKE_OBJCXX_EXTENSIONS
)

set (PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_ASM
    CMAKE_ASM_FLAGS CMAKE_ASM_COMPILER CMAKE_ASM_COMPILER_TARGET CMAKE_ASM_COMPILER_LAUNCHER CMAKE_ASM_COMPILER_AR CMAKE_ASM_COMPILER_RANLIB CMAKE_ASM_LINK_LIBRARY_SUFFIX
)

set (PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_COMMON
    CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_STATIC_LINKER_FLAGS
    CMAKE_TOOLCHAIN_FILE CMAKE_AR CMAKE_RANLIB
    PROJECT_ATFRAME_TARGET_CPU_ABI 
    CMAKE_SYSROOT CMAKE_SYSROOT_COMPILE # CMAKE_SYSTEM_LIBRARY_PATH # CMAKE_SYSTEM_LIBRARY_PATH ninja里解出的参数不对，原因未知
    CMAKE_OSX_SYSROOT CMAKE_OSX_ARCHITECTURES CMAKE_OSX_DEPLOYMENT_TARGET CMAKE_MACOSX_RPATH
    ANDROID_TOOLCHAIN ANDROID_ABI ANDROID_PIE ANDROID_PLATFORM
    ANDROID_ALLOW_UNDEFINED_SYMBOLS ANDROID_ARM_MODE ANDROID_ARM_NEON ANDROID_DISABLE_NO_EXECUTE ANDROID_DISABLE_RELRO
    ANDROID_DISABLE_FORMAT_STRING_CHECKS ANDROID_CCACHE
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
    # Set CMAKE_SYSTEM_NAME will cause cmake to set CMAKE_CROSSCOMPILING to TRUE, so we don't set it when not crosscompiling
    list(APPEND PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_COMMON CMAKE_SYSTEM_NAME CMAKE_SYSTEM_PROCESSOR CMAKE_SYSTEM_VERSION)
endif ()

set (PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS 
    ${PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_COMMON}
    ${PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_C}
    ${PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_CXX}
    ${PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_ASM}
)

macro(project_build_tools_append_cmake_inherit_options OUTVAR)
    list (APPEND ${ARGV0} "-G" "${CMAKE_GENERATOR}")

    set(project_build_tools_append_cmake_inherit_options_DISABLE_C_FLAGS FALSE)
    set(project_build_tools_append_cmake_inherit_options_DISABLE_CXX_FLAGS FALSE)
    set(project_build_tools_append_cmake_inherit_options_DISABLE_ASM_FLAGS FALSE)
    set(project_build_tools_append_cmake_inherit_options_VARS PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_COMMON)
    foreach(ARG ${ARGN})
        if ("${ARG}" STREQUAL "DISABLE_C_FLAGS")
            set(project_build_tools_append_cmake_inherit_options_DISABLE_C_FLAGS TRUE)
        endif ()
        if ("${ARG}" STREQUAL "DISABLE_CXX_FLAGS")
            set(project_build_tools_append_cmake_inherit_options_DISABLE_CXX_FLAGS TRUE)
        endif ()
        if ("${ARG}" STREQUAL "DISABLE_ASM_FLAGS")
            set(project_build_tools_append_cmake_inherit_options_DISABLE_ASM_FLAGS TRUE)
        endif ()
    endforeach()

    if (NOT project_build_tools_append_cmake_inherit_options_DISABLE_C_FLAGS)
        list (APPEND project_build_tools_append_cmake_inherit_options_VARS PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_C)
    endif ()
    if (NOT project_build_tools_append_cmake_inherit_options_DISABLE_CXX_FLAGS)
        list (APPEND project_build_tools_append_cmake_inherit_options_VARS PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_CXX)
    endif ()
    if (NOT project_build_tools_append_cmake_inherit_options_DISABLE_ASM_FLAGS)
        list (APPEND project_build_tools_append_cmake_inherit_options_VARS PROJECT_BUILD_TOOLS_CMAKE_INHERIT_VARS_ASM)
    endif ()


    foreach (VAR_NAME IN LISTS ${project_build_tools_append_cmake_inherit_options_VARS})
        if (DEFINED ${VAR_NAME})
            set(VAR_VALUE "${${VAR_NAME}}")
            # message("DEBUG============ ${VAR_NAME}=${VAR_VALUE}")
            if (VAR_VALUE)
                list (APPEND ${ARGV0} "-D${VAR_NAME}=${VAR_VALUE}")
            endif ()
            unset(VAR_VALUE)
        endif ()
    endforeach ()

    if (CMAKE_GENERATOR_PLATFORM)
        list (APPEND ${ARGV0} "-A" "${CMAKE_GENERATOR_PLATFORM}")
    endif ()

    if (CMAKE_GENERATOR_TOOLSET)
        list (APPEND ${ARGV0} "-T" "${CMAKE_GENERATOR_TOOLSET}")
    endif ()

    unset(project_build_tools_append_cmake_inherit_options_DISABLE_C_FLAGS)
    unset(project_build_tools_append_cmake_inherit_options_DISABLE_CXX_FLAGS)
    unset(project_build_tools_append_cmake_inherit_options_DISABLE_ASM_FLAGS)
    unset(project_build_tools_append_cmake_inherit_options_VARS)
endmacro ()

macro(project_build_tools_append_cmake_build_type_for_lib OUTVAR)
    if (CMAKE_BUILD_TYPE)
        if (MSVC)
            list (APPEND ${ARGV0} "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
            list (APPEND ${ARGV0} "-DCMAKE_BUILD_TYPE=RelWithDebInfo")
        else ()
            list (APPEND ${ARGV0} "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        endif ()
    endif ()
endmacro ()

macro(project_build_tools_append_cmake_cxx_standard_options)
    unset(project_build_tools_append_cmake_cxx_standard_options_OUTVAR)
    set(project_build_tools_append_cmake_cxx_standard_options_DISABLE_C_FLAGS FALSE)
    set(project_build_tools_append_cmake_cxx_standard_options_DISABLE_CXX_FLAGS FALSE)
    foreach(ARG ${ARGN})
        if(NOT project_build_tools_append_cmake_cxx_standard_options_OUTVAR)
            set(project_build_tools_append_cmake_cxx_standard_options_OUTVAR ${ARG})
        endif ()
        if ("${ARG}" STREQUAL "DISABLE_C_FLAGS")
            set(project_build_tools_append_cmake_cxx_standard_options_DISABLE_C_FLAGS TRUE)
        endif ()
        if ("${ARG}" STREQUAL "DISABLE_CXX_FLAGS")
            set(project_build_tools_append_cmake_cxx_standard_options_DISABLE_CXX_FLAGS TRUE)
        endif ()
    endforeach()
    if (CMAKE_C_STANDARD AND NOT project_build_tools_append_cmake_cxx_standard_options_DISABLE_C_FLAGS)
        list (APPEND ${project_build_tools_append_cmake_cxx_standard_options_OUTVAR} "-DCMAKE_C_STANDARD=${CMAKE_C_STANDARD}")
    endif ()
    if (CMAKE_OBJC_STANDARD AND NOT project_build_tools_append_cmake_cxx_standard_options_DISABLE_C_FLAGS)
        list (APPEND ${project_build_tools_append_cmake_cxx_standard_options_OUTVAR} "-DCMAKE_OBJC_STANDARD=${CMAKE_OBJC_STANDARD}")
    endif ()
    if (CMAKE_CXX_STANDARD AND NOT project_build_tools_append_cmake_cxx_standard_options_DISABLE_CXX_FLAGS)
        list (APPEND ${project_build_tools_append_cmake_cxx_standard_options_OUTVAR} "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}")
    endif ()
    if (CMAKE_OBJCXX_STANDARD AND NOT project_build_tools_append_cmake_cxx_standard_options_DISABLE_CXX_FLAGS)
        list (APPEND ${project_build_tools_append_cmake_cxx_standard_options_OUTVAR} "-DCMAKE_OBJCXX_STANDARD=${CMAKE_OBJCXX_STANDARD}")
    endif ()

    unset(project_build_tools_append_cmake_cxx_standard_options_OUTVAR)
    unset(project_build_tools_append_cmake_cxx_standard_options_DISABLE_C_FLAGS)
    unset(project_build_tools_append_cmake_cxx_standard_options_DISABLE_CXX_FLAGS)
endmacro()

macro(project_build_tools_append_cmake_options_for_lib OUTVAR)
    project_build_tools_append_cmake_inherit_options(${ARGV0} ${ARGN})
    project_build_tools_append_cmake_build_type_for_lib(${ARGV0})
    project_build_tools_append_cmake_cxx_standard_options(${ARGV0} ${ARGN})
    list (APPEND ${ARGV0}
        "-DCMAKE_POLICY_DEFAULT_CMP0075=NEW" 
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    )
endmacro ()

function(project_make_executable)
    if (UNIX OR MINGW OR CYGWIN OR APPLE OR CMAKE_HOST_APPLE OR CMAKE_HOST_UNIX)
        foreach(ARG ${ARGN})
            execute_process(COMMAND chmod -R +x ${ARG})
        endforeach()
    endif()
endfunction()

function (project_make_writable)
    if (CMAKE_HOST_APPLE OR APPLE OR UNIX OR MINGW OR MSYS OR CYGWIN)
        execute_process(COMMAND chmod -R +w ${ARGN})
    else ()
        foreach(arg ${ARGN})
            execute_process(COMMAND attrib -R "${arg}" /S /D /L)
        endforeach()
    endif ()
endfunction()


# 如果仅仅是设置环境变量的话可以用 ${CMAKE_COMMAND} -E env M4=/foo/bar 代替
macro (project_expand_list_for_command_line OUTPUT INPUT)
    set(project_expand_list_for_command_line_ENABLE_CONVERT ON)
    foreach(ARG IN LISTS ${INPUT})
        if (ARG STREQUAL "DISABLE_CONVERT")
            set(project_expand_list_for_command_line_ENABLE_CONVERT OFF)
        elseif (ARG STREQUAL "ENABLE_CONVERT")
            set(project_expand_list_for_command_line_ENABLE_CONVERT ON)
        else ()
            if (project_expand_list_for_command_line_ENABLE_CONVERT)
                string(REPLACE "\\" "\\\\" project_expand_list_for_command_line_OUT_VAR ${ARG})
                string(REPLACE "\"" "\\\"" project_expand_list_for_command_line_OUT_VAR ${project_expand_list_for_command_line_OUT_VAR})
            else ()
                set (project_expand_list_for_command_line_OUT_VAR ${ARG})
            endif()
            set (${OUTPUT} "${${OUTPUT}} \"${project_expand_list_for_command_line_OUT_VAR}\"")
            unset (project_expand_list_for_command_line_OUT_VAR)
        endif()
    endforeach()
    unset(project_expand_list_for_command_line_ENABLE_CONVERT)
endmacro()

function (project_expand_list_for_command_line_to_file)
    unset (project_expand_list_for_command_line_to_file_OUTPUT)
    unset (project_expand_list_for_command_line_to_file_LINE)
    set(project_expand_list_for_command_line_to_file_ENABLE_CONVERT ON)
    foreach(ARG ${ARGN})
        if (ARG STREQUAL "DISABLE_CONVERT")
            set(project_expand_list_for_command_line_to_file_ENABLE_CONVERT OFF)
        elseif (ARG STREQUAL "ENABLE_CONVERT")
            set(project_expand_list_for_command_line_to_file_ENABLE_CONVERT ON)
        elseif (NOT project_expand_list_for_command_line_to_file_OUTPUT)
            set (project_expand_list_for_command_line_to_file_OUTPUT "${ARG}")
        else ()
            if (project_expand_list_for_command_line_to_file_ENABLE_CONVERT)
                string(REPLACE "\\" "\\\\" project_expand_list_for_command_line_OUT_VAR ${ARG})
                string(REPLACE "\"" "\\\"" project_expand_list_for_command_line_OUT_VAR ${project_expand_list_for_command_line_OUT_VAR})
            else ()
                set (project_expand_list_for_command_line_OUT_VAR ${ARG})
            endif ()
            if (project_expand_list_for_command_line_to_file_LINE)
                set (project_expand_list_for_command_line_to_file_LINE "${project_expand_list_for_command_line_to_file_LINE} \"${project_expand_list_for_command_line_OUT_VAR}\"")
            else ()
                set (project_expand_list_for_command_line_to_file_LINE "\"${project_expand_list_for_command_line_OUT_VAR}\"")
            endif ()
            unset (project_expand_list_for_command_line_OUT_VAR)
        endif ()
    endforeach()

    if (project_expand_list_for_command_line_to_file_OUTPUT)
        file(APPEND "${project_expand_list_for_command_line_to_file_OUTPUT}" "${project_expand_list_for_command_line_to_file_LINE}${PROJECT_THIRD_PARTY_BUILDTOOLS_BASH_EOL}")
    endif ()
    unset (project_expand_list_for_command_line_to_file_OUTPUT)
    unset (project_expand_list_for_command_line_to_file_LINE)
    unset (project_expand_list_for_command_line_to_file_ENABLE_CONVERT)
endfunction()

if (CMAKE_HOST_WIN32)
    set (PROJECT_THIRD_PARTY_BUILDTOOLS_EOL "\r\n")
    set (PROJECT_THIRD_PARTY_BUILDTOOLS_BASH_EOL "\n")
elseif (CMAKE_HOST_APPLE OR APPLE)
    set (PROJECT_THIRD_PARTY_BUILDTOOLS_EOL "\r")
    set (PROJECT_THIRD_PARTY_BUILDTOOLS_BASH_EOL "\n")
else ()
    set (PROJECT_THIRD_PARTY_BUILDTOOLS_EOL "\n")
    set (PROJECT_THIRD_PARTY_BUILDTOOLS_BASH_EOL "\n")
endif ()


function (project_git_clone_3rd_party)
    if (CMAKE_VERSION VERSION_LESS_EQUAL "3.4")
        include(CMakeParseArguments)
    endif ()
    set(oneValueArgs URL WORKING_DIRECTORY REPO_DIRECTORY DEPTH BRANCH TAG CHECK_PATH)
    cmake_parse_arguments(project_git_clone_3rd_party "" "${oneValueArgs}" "" ${ARGN} )

    if (NOT project_git_clone_3rd_party_URL)
        message(FATAL_ERROR "URL is required")
    endif ()
    if (NOT project_git_clone_3rd_party_REPO_DIRECTORY)
        message(FATAL_ERROR "REPO_DIRECTORY is required")
    endif ()
    if (NOT project_git_clone_3rd_party_WORKING_DIRECTORY)
        get_filename_component(project_git_clone_3rd_party_WORKING_DIRECTORY ${project_git_clone_3rd_party_REPO_DIRECTORY} DIRECTORY)
    endif ()
    if (NOT project_git_clone_3rd_party_CHECK_PATH)
        set (project_git_clone_3rd_party_CHECK_PATH ".git")
    endif ()
    unset (CLONE_OPTIONS)
    unset (UPDATE_OPTIONS)
    unset (FETCH_OPTIONS)
    if (project_git_clone_3rd_party_DEPTH)
        list (APPEND CLONE_OPTIONS --depth ${project_git_clone_3rd_party_DEPTH})
        list (APPEND FETCH_OPTIONS --depth ${project_git_clone_3rd_party_DEPTH})
    endif ()
    if (project_git_clone_3rd_party_TAG)
        list (APPEND CLONE_OPTIONS -b "${project_git_clone_3rd_party_TAG}")
        list (APPEND UPDATE_OPTIONS "tags/${project_git_clone_3rd_party_TAG}")
        list (APPEND FETCH_OPTIONS --tags)
    elseif (project_git_clone_3rd_party_BRANCH)
        list (APPEND CLONE_OPTIONS -b "${project_git_clone_3rd_party_BRANCH}")
        list (APPEND UPDATE_OPTIONS "origin/${project_git_clone_3rd_party_BRANCH}")
    else ()
        list (APPEND CLONE_OPTIONS -b master)
        list (APPEND UPDATE_OPTIONS "origin/master")
    endif ()
    find_package(Git)
    if (NOT GIT_FOUND AND NOT Git_FOUND)
        message(FATAL_ERROR "git not found")
    endif ()
    if(NOT EXISTS "${project_git_clone_3rd_party_REPO_DIRECTORY}/${project_git_clone_3rd_party_CHECK_PATH}")
        if (EXISTS ${project_git_clone_3rd_party_REPO_DIRECTORY})
            file(REMOVE_RECURSE ${project_git_clone_3rd_party_REPO_DIRECTORY})
        endif ()
        execute_process(COMMAND ${GIT_EXECUTABLE} clone ${CLONE_OPTIONS} ${project_git_clone_3rd_party_URL} ${project_git_clone_3rd_party_REPO_DIRECTORY}
            WORKING_DIRECTORY ${project_git_clone_3rd_party_WORKING_DIRECTORY}
        )
    elseif(PROJECT_RESET_DENPEND_REPOSITORIES)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} fetch -f ${FETCH_OPTIONS} origin
            COMMAND ${GIT_EXECUTABLE} clean -dfx
            COMMAND ${GIT_EXECUTABLE} reset --hard ${UPDATE_OPTIONS}
            WORKING_DIRECTORY ${project_git_clone_3rd_party_REPO_DIRECTORY}
            RESULT_VARIABLE LAST_GIT_RESET_RESULT
        )

        if (LAST_GIT_RESET_RESULT AND NOT LAST_GIT_RESET_RESULT EQUAL 0)
            if (EXISTS ${project_git_clone_3rd_party_REPO_DIRECTORY})
                file(REMOVE_RECURSE ${project_git_clone_3rd_party_REPO_DIRECTORY})
            endif ()
            execute_process(COMMAND ${GIT_EXECUTABLE} clone ${CLONE_OPTIONS} ${project_git_clone_3rd_party_URL} ${project_git_clone_3rd_party_REPO_DIRECTORY}
                WORKING_DIRECTORY ${project_git_clone_3rd_party_WORKING_DIRECTORY}
            )
        endif ()
    endif()
endfunction()

if (NOT PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS_SET)
    if (MSVC)
        unset(PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS CACHE)
        set(PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS /wd4244 /wd4251 /wd4309)

        if (MSVC_VERSION GREATER_EQUAL 1922)
            # see https://docs.microsoft.com/en-us/cpp/overview/cpp-conformance-improvements?view=vs-2019#improvements_162 for detail
            list(APPEND PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS /wd5054)
        endif ()
        
        if (MSVC_VERSION GREATER_EQUAL 1925)
            list(APPEND PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS /wd4996)
        endif ()

        if (MSVC_VERSION LESS 1910)
            list(APPEND PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS /wd4800)
        endif ()
    else()
        unset(PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS CACHE)
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag(-Wno-unused-parameter project_build_tools_patch_protobuf_sources_LINT_NO_UNUSED_PARAMETER)
        if (project_build_tools_patch_protobuf_sources_LINT_NO_UNUSED_PARAMETER)
            list(APPEND PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS -Wno-unused-parameter)
        endif ()
        check_cxx_compiler_flag(-Wno-deprecated-declarations project_build_tools_patch_protobuf_sources_LINT_NO_DEPRECATED_DECLARATIONS)
        if (project_build_tools_patch_protobuf_sources_LINT_NO_DEPRECATED_DECLARATIONS)
            list(APPEND PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS -Wno-deprecated-declarations)
        endif ()
        
    endif ()
    set(PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS_SET TRUE)
    set(PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS ${PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS} CACHE INTERNAL "Options to disable warning of generated protobuf sources" FORCE)
endif ()

function (project_build_tools_patch_protobuf_sources)
    if (PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS)
        foreach(PROTO_SRC ${ARGN})
            unset(PROTO_SRC_OPTIONS)
            get_source_file_property(PROTO_SRC_OPTIONS ${PROTO_SRC} COMPILE_OPTIONS)
            if (PROTO_SRC_OPTIONS)
                list(APPEND PROTO_SRC_OPTIONS ${PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS})
            else()
                set(PROTO_SRC_OPTIONS ${PROJECT_BUILD_TOOLS_PATCH_PROTOBUF_SOURCES_OPTIONS})
            endif()

            set_source_files_properties(${PROTO_SRC} PROPERTIES COMPILE_OPTIONS "${PROTO_SRC_OPTIONS}")
        endforeach()
        unset(PROTO_SRC)
        unset(PROTO_SRC_OPTIONS)
    endif ()
endfunction()

function(project_build_tools_patch_imported_link_interface_libraries TARGET_NAME)
    if (CMAKE_VERSION VERSION_LESS_EQUAL "3.4")
        include(CMakeParseArguments)
    endif ()
    set(multiValueArgs ADD_LIBRARIES REMOVE_LIBRARIES)
    cmake_parse_arguments(PATCH_OPTIONS "" "" "${multiValueArgs}" ${ARGN})

    get_target_property(OLD_LINK_LIBRARIES ${TARGET_NAME} INTERFACE_LINK_LIBRARIES)
    set(PROPERTY_NAME "")
    if (OLD_LINK_LIBRARIES)
        set(PROPERTY_NAME "INTERFACE_LINK_LIBRARIES")
    endif ()
    if (NOT PROPERTY_NAME)
        get_target_property(OLD_LINK_LIBRARIES ${TARGET_NAME} IMPORTED_LINK_INTERFACE_LIBRARIES)
        if (OLD_LINK_LIBRARIES)
            set(PROPERTY_NAME "IMPORTED_LINK_INTERFACE_LIBRARIES")
        endif ()
    endif ()
    if (NOT PROPERTY_NAME)
        get_target_property(OLD_IMPORTED_CONFIGURATIONS ${TARGET_NAME} IMPORTED_CONFIGURATIONS)
        get_target_property(OLD_LINK_LIBRARIES ${TARGET_NAME} "IMPORTED_LINK_INTERFACE_LIBRARIES_${OLD_IMPORTED_CONFIGURATIONS}")
        if (OLD_LINK_LIBRARIES)
            set(PROPERTY_NAME "IMPORTED_LINK_INTERFACE_LIBRARIES_${OLD_IMPORTED_CONFIGURATIONS}")
        endif ()
    endif ()
    if (NOT PROPERTY_NAME)
        set(PROPERTY_NAME "INTERFACE_LINK_LIBRARIES")
    endif ()

    if(NOT OLD_LINK_LIBRARIES)
        set(OLD_LINK_LIBRARIES "") # Reset NOTFOUND
    endif()
    unset(PATCH_INNER_LIBS)
    if (OLD_LINK_LIBRARIES AND PATCH_OPTIONS_REMOVE_LIBRARIES)
        foreach(DEP_PATH IN LISTS OLD_LINK_LIBRARIES)
            set(MATCH_ANY_RULES FALSE)
            foreach(MATCH_RULE IN LISTS PATCH_OPTIONS_REMOVE_LIBRARIES)
                if(DEP_PATH MATCHES ${MATCH_RULE})
                    set(MATCH_ANY_RULES TRUE)
                    break()
                endif()
            endforeach()
            
            if (NOT MATCH_ANY_RULES)
                list(APPEND PATCH_INNER_LIBS ${DEP_PATH})
            endif ()
        endforeach()
        if (PATCH_OPTIONS_ADD_LIBRARIES)
            list(APPEND PATCH_INNER_LIBS ${PATCH_OPTIONS_ADD_LIBRARIES})
        endif()
    elseif (OLD_LINK_LIBRARIES)
        set(PATCH_INNER_LIBS ${OLD_LINK_LIBRARIES})
        if (PATCH_OPTIONS_ADD_LIBRARIES)
            list(APPEND PATCH_INNER_LIBS ${PATCH_OPTIONS_ADD_LIBRARIES})
        endif()
    elseif (PATCH_OPTIONS_ADD_LIBRARIES)
        set(PATCH_INNER_LIBS ${PATCH_OPTIONS_ADD_LIBRARIES})
    else()
        set(PATCH_INNER_LIBS "")
    endif ()

    if (PATCH_INNER_LIBS)
        list(REMOVE_DUPLICATES PATCH_INNER_LIBS)
    endif()

    if (NOT OLD_LINK_LIBRARIES STREQUAL PATCH_INNER_LIBS)
        set_target_properties(${TARGET_NAME} PROPERTIES ${PROPERTY_NAME} "${PATCH_INNER_LIBS}")
        message(STATUS "Patch: ${PROPERTY_NAME} of ${TARGET_NAME} from \"${OLD_LINK_LIBRARIES}\" to \"${PATCH_INNER_LIBS}\"")
    endif ()
endfunction()
