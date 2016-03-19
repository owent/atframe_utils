
EchoWithColor(COLOR GREEN "-- Configure Test Projects")

enable_testing()
find_package(GTest)
set (PROJECT_TEST_LIB_LINK "")

# =========== find gtest ===========
if (GTEST_FOUND)
    EchoWithColor(COLOR GREEN "-- GTest Found: ${GTEST_LIBRARIES}")
    add_compiler_define(PROJECT_TEST_MACRO_ENABLE_GTEST=1)
	set (PROJECT_TEST_LIB_LINK ${GTEST_LIBRARIES})
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
        set (PROJECT_TEST_LIB_LINK ${Boost_LIBRARIES})
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
include_directories(${PROJECT_ROOT_SRC_DIR})

file(GLOB_RECURSE PROJECT_TEST_SRC_LIST 
    ${PROJECT_TEST_SRC_DIR}/*.h 
    ${PROJECT_TEST_SRC_DIR}/*.hpp 
    ${PROJECT_TEST_SRC_DIR}/*.c 
    ${PROJECT_TEST_SRC_DIR}/*.cpp
    ${PROJECT_TEST_SRC_DIR}/*.cc 
    ${PROJECT_TEST_SRC_DIR}/*.cxx
)
source_group_by_dir(PROJECT_TEST_SRC_LIST)


# ================ multi thread ================
if ( NOT MSVC )
    add_definitions(-D_POSIX_MT_)
    set (PROJECT_TEST_LIB_LINK ${PROJECT_TEST_LIB_LINK} pthread)
endif()

# ============ test - coroutine test frame ============
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/test")

add_executable(unit_test ${PROJECT_TEST_SRC_LIST})
target_link_libraries(unit_test
    ${PROJECT_LIB_LINK} 
    ${PROJECT_TEST_LIB_LINK}
    ${3RD_PARTY_LIBUV_LINK_NAME}
    ${3RD_PARTY_C_CPP_UTILS_LINK_NAME}
)

add_test(test unit_test)
