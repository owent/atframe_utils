function(atframe_add_test_executable TARGET_NAME)
  set(PROJECT_TEST_INC_DIRS "${PROJECT_TEST_INC_DIR}")
  if(MSVC)
    set(PROJECT_TEST_DEFINITIONS _CRT_SECURE_NO_WARNINGS=1)
  endif()

  # =========== find gtest ===========
  if(TARGET GTest::gtest OR GTest::GTest)
    list(APPEND PROJECT_TEST_DEFINITIONS PROJECT_TEST_MACRO_ENABLE_GTEST=1)
    if(TARGET GTest::GTest)
      list(APPEND PROJECT_TEST_LIB_LINK GTest::gtest)
    else()
      list(APPEND PROJECT_TEST_LIB_LINK GTest::GTest)
    endif()

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

      list(APPEND PROJECT_TEST_DEFINITIONS PROJECT_TEST_MACRO_ENABLE_BOOST_TEST=1)
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

  add_executable(${TARGET_NAME} ${ARGN})
  if(MSVC)
    set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "atframework/test")
    # add_target_properties(${TARGET_NAME} LINK_FLAGS /NODEFAULTLIB:library)
  endif()
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

  # add_test(NAME test COMMAND ${TARGET_NAME})
endfunction()
