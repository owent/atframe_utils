# Copyright 2026 atframework

include_guard(GLOBAL)

# Create the atframework test framework library targets once per build tree. Called lazily from
# atframe_add_test_executable so the integration mode (private runner / GoogleTest / Boost.Test)
# detected by the first caller applies. CMAKE_CURRENT_FUNCTION_LIST_DIR always points at the directory
# of this script no matter which project included it, so all consumers share one framework source copy.
function(atframework_test_ensure_library_targets)
  if(TARGET atframework.test.library)
    return()
  endif()
  set(ATFRAMEWORK_TEST_FRAME_DIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

  set(ATFRAMEWORK_TEST_LIBRARY_SOURCES
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_framework_export.h"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_case_base.h"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_case_base.cpp"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_event_listener.h"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_event_listener.cpp"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_manager.h"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_manager.cpp"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_macros.h"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_resource_limit.h"
      "${ATFRAMEWORK_TEST_FRAME_DIR}/frame/test_resource_limit.cpp")

  if(BUILD_SHARED_LIBS OR ATFRAMEWORK_USE_DYNAMIC_LIBRARY)
    add_library(atframework.test.library SHARED ${ATFRAMEWORK_TEST_LIBRARY_SOURCES})
    # Consumers must see ATFRAMEWORK_TEST_API_DLL=1 to import the symbols on Windows.
    target_compile_definitions(atframework.test.library PUBLIC ATFRAMEWORK_TEST_API_DLL=1)
    if(NOT APPLE)
      set_target_properties(atframework.test.library PROPERTIES C_VISIBILITY_PRESET "hidden"
                                                                CXX_VISIBILITY_PRESET "hidden")
    endif()
  else()
    add_library(atframework.test.library STATIC ${ATFRAMEWORK_TEST_LIBRARY_SOURCES})
  endif()
  add_library(atframework::test::library ALIAS atframework.test.library)
  set_property(TARGET atframework.test.library PROPERTY FOLDER "atframework/test")
  target_compile_definitions(atframework.test.library PRIVATE ATFRAMEWORK_TEST_API_NATIVE=1)
  target_include_directories(atframework.test.library PUBLIC "$<BUILD_INTERFACE:${ATFRAMEWORK_TEST_FRAME_DIR}>")

  # The integration mode macros shape the public headers (test_macros.h, test_event_listener.h), so
  # they must propagate to consumers; the external framework libraries are linked the same way.
  if(ATFRAMEWORK_TEST_DEFINITIONS)
    target_compile_definitions(atframework.test.library PUBLIC ${ATFRAMEWORK_TEST_DEFINITIONS})
  endif()
  if(ATFRAMEWORK_TEST_LIB_LINK)
    target_link_libraries(atframework.test.library PUBLIC ${ATFRAMEWORK_TEST_LIB_LINK})
  endif()
  if(ATFRAMEWORK_TEST_INC_DIRS)
    target_include_directories(atframework.test.library PUBLIC ${ATFRAMEWORK_TEST_INC_DIRS})
  endif()
  if(TARGET atframework::atframe_utils)
    target_link_libraries(atframework.test.library PUBLIC atframework::atframe_utils)
  elseif(TARGET atframe_utils)
    target_link_libraries(atframework.test.library PUBLIC atframe_utils)
  endif()

  if(NOT TARGET atframework.test.main)
    # The main entry library is always static, like GTest::Main.
    add_library(atframework.test.main STATIC "${ATFRAMEWORK_TEST_FRAME_DIR}/app/main.cpp")
    add_library(atframework::test::main ALIAS atframework.test.main)
    set_property(TARGET atframework.test.main PROPERTY FOLDER "atframework/test")
    target_link_libraries(atframework.test.main PUBLIC atframework::test::library)
  endif()
endfunction()

# atframe_add_test_executable(<target>
#   [sources...]
#   [LINK_TEST_LIBRARY]   # link atframework::test::library instead of compiling frame sources in
#   [LINK_TEST_MAIN]      # link atframework::test::main (implies the library, provides main())
# )
function(atframe_add_test_executable TARGET_NAME)
  cmake_parse_arguments(ATFRAMEWORK_TEST_EXE "LINK_TEST_LIBRARY;LINK_TEST_MAIN" "" "" ${ARGN})
  set(PROJECT_TEST_SOURCES ${ATFRAMEWORK_TEST_EXE_UNPARSED_ARGUMENTS})

  set(PROJECT_TEST_INC_DIRS "${PROJECT_TEST_INC_DIR}")
  if(MSVC)
    set(PROJECT_TEST_DEFINITIONS _CRT_SECURE_NO_WARNINGS=1)
  endif()

  # =========== find gtest ===========
  if(TARGET GTest::gtest OR GTest::GTest)
    list(APPEND PROJECT_TEST_DEFINITIONS ATFW_UTILS_TEST_MACRO_TEST_ENABLE_GTEST=1)
    if(TARGET GTest::GTest)
      list(APPEND PROJECT_TEST_LIB_LINK GTest::GTest)
    else()
      list(APPEND PROJECT_TEST_LIB_LINK GTest::gtest)
    endif()
    project_build_tools_patch_default_imported_config(GTest::GTest GTest::gtest)

    # =========== enable find boost.test ===========
  elseif(PROJECT_TEST_ENABLE_BOOST_UNIT_TEST)

    find_package(Boost COMPONENTS unit_test_framework)
    set(Boost_AUTO_LIBS "${Boost_LIBRARIES}")

    if(Boost_FOUND)
      set(Boost_USE_STATIC_LIBS ON)
      find_package(Boost COMPONENTS unit_test_framework)
      if(NOT Boost_FOUND)
        set(Boost_USE_STATIC_LIBS OFF)
        find_package(Boost COMPONENTS unit_test_framework)
      elseif(NOT "${Boost_LIBRARIES}" EQUAL "${Boost_AUTO_LIBS}")
        set(Boost_USE_STATIC_LIBS OFF)
        find_package(Boost COMPONENTS unit_test_framework)
      endif()

      list(APPEND PROJECT_TEST_DEFINITIONS ATFW_UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST=1)
      list(APPEND PROJECT_TEST_LIB_LINK ${Boost_LIBRARIES})
      list(APPEND PROJECT_TEST_INC_DIRS ${Boost_INCLUDE_DIRS})
      echowithcolor(COLOR GREEN "-- Boost.test Found: ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")

      if(NOT Boost_USE_STATIC_LIBS)
        echowithcolor(COLOR GREEN "-- Boost.test using dynamic library define BOOST_TEST_DYN_LINK")
        list(APPEND PROJECT_TEST_DEFINITIONS BOOST_TEST_DYN_LINK)
      endif()
    else()
      echowithcolor(COLOR RED "-- Enable boost unit test but boost.test not found.")
    endif()
  endif()

  if(ATFRAMEWORK_TEST_EXE_LINK_TEST_LIBRARY OR ATFRAMEWORK_TEST_EXE_LINK_TEST_MAIN)
    # Share the detected integration mode with the framework library targets.
    set(ATFRAMEWORK_TEST_DEFINITIONS ${PROJECT_TEST_DEFINITIONS})
    set(ATFRAMEWORK_TEST_LIB_LINK ${PROJECT_TEST_LIB_LINK})
    set(ATFRAMEWORK_TEST_INC_DIRS ${PROJECT_TEST_INC_DIRS})
    atframework_test_ensure_library_targets()
  endif()

  add_executable(${TARGET_NAME} ${PROJECT_TEST_SOURCES})
  if(PROJECT_TEST_RUNTIME_OUTPUT_DIRECTORY)
    atframe_target_set_runtime_output_directory(${TARGET_NAME} "${PROJECT_TEST_RUNTIME_OUTPUT_DIRECTORY}")
  endif()
  set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "atframework/test")
  # add_target_properties(${TARGET_NAME} LINK_FLAGS /NODEFAULTLIB:library)
  set_target_properties(
    ${TARGET_NAME}
    PROPERTIES INSTALL_RPATH_USE_LINK_PATH YES
               BUILD_WITH_INSTALL_RPATH NO
               BUILD_RPATH_USE_ORIGIN YES)

  target_include_directories(${TARGET_NAME} PRIVATE "${PROJECT_TEST_INC_DIRS}")
  if(PROJECT_TEST_LIB_LINK)
    target_link_libraries(${TARGET_NAME} ${PROJECT_TEST_LIB_LINK})
  endif()
  if(PROJECT_TEST_DEFINITIONS)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${PROJECT_TEST_DEFINITIONS})
  endif()

  if(ATFRAMEWORK_TEST_EXE_LINK_TEST_MAIN)
    target_link_libraries(${TARGET_NAME} atframework::test::main)
  elseif(ATFRAMEWORK_TEST_EXE_LINK_TEST_LIBRARY)
    target_link_libraries(${TARGET_NAME} atframework::test::library)
  endif()

  # add_test(NAME test-name COMMAND "$<TARGET_FILE:${TARGET_NAME}>") set_tests_properties(test-name PROPERTIES LABELS
  # "label1;label2")
endfunction()
