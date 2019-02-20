find_package(GTest)

# =========== find gtest ===========
if (GTEST_FOUND)
    EchoWithColor(COLOR GREEN "-- GTest Found: ${GTEST_LIBRARIES}")
    add_compiler_define(PROJECT_TEST_MACRO_ENABLE_GTEST=1)
	list(APPEND PROJECT_TEST_LIB_LINK ${GTEST_LIBRARIES})
    include_directories(${GTEST_INCLUDE_DIRS})

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

        add_compiler_define(PROJECT_TEST_MACRO_ENABLE_BOOST_TEST=1)
        list(APPEND PROJECT_TEST_LIB_LINK ${Boost_LIBRARIES})
        include_directories(${Boost_INCLUDE_DIRS})
        EchoWithColor(COLOR GREEN "-- Boost.test Found: ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")

        if (NOT Boost_USE_STATIC_LIBS)
            EchoWithColor(COLOR GREEN "-- Boost.test using dynamic library define BOOST_TEST_DYN_LINK")
            add_compiler_define(BOOST_TEST_DYN_LINK)
        endif()
    else()
        EchoWithColor(COLOR RED "-- Enable boost unit test but boost.test not found.")
    endif()
endif()

include_directories(${PROJECT_TEST_INC_DIR})

# ================ multi thread ================
find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
    add_definitions(-D_POSIX_MT_)
    list(APPEND PROJECT_TEST_LIB_LINK pthread)
endif ()
