
EchoWithColor(COLOR GREEN "-- Configure ${CMAKE_CURRENT_LIST_DIR}")
include("test.macro.cmake")

include("${CMAKE_CURRENT_LIST_DIR}/test.build_bin.cmake")

include_directories(${PROJECT_ALL_INCLUDE_DIR})

file(GLOB_RECURSE PROJECT_TEST_SRC_LIST
    ${PROJECT_TEST_SRC_DIR}/*.h
    ${PROJECT_TEST_SRC_DIR}/*.hpp
    ${PROJECT_TEST_SRC_DIR}/*.c
    ${PROJECT_TEST_SRC_DIR}/*.cpp
    ${PROJECT_TEST_SRC_DIR}/*.cc
    ${PROJECT_TEST_SRC_DIR}/*.cxx
)
source_group_by_dir(PROJECT_TEST_SRC_LIST)

# ============ test - coroutine test frame ============

if (CRYPTO_USE_OPENSSL OR CRYPTO_USE_LIBRESSL OR CRYPTO_USE_BORINGSSL)
    find_package(OpenSSL)
    list(APPEND 3RD_PARTY_C_CPP_UTILS_LINK_NAME ${OPENSSL_CRYPTO_LIBRARY})
elseif (CRYPTO_USE_MBEDTLS)
    find_package(MbedTLS)
    list(APPEND 3RD_PARTY_C_CPP_UTILS_LINK_NAME ${MbedTLS_CRYPTO_LIBRARIES})
endif()

if (LOG_STACKTRACE_USING_LIBUNWIND)
    list(APPEND 3RD_PARTY_C_CPP_UTILS_LINK_NAME ${Libunwind_LIBRARIES})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test")

add_executable(atframe_utils_unit_test ${PROJECT_TEST_SRC_LIST})
target_link_libraries(atframe_utils_unit_test
    ${PROJECT_LIB_LINK}
    ${PROJECT_TEST_LIB_LINK}
    ${3RD_PARTY_LIBUV_LINK_NAME}
    ${3RD_PARTY_C_CPP_UTILS_LINK_NAME}
    ${COMPILER_OPTION_EXTERN_CXX_LIBS}
)

add_test(NAME test COMMAND atframe_utils_unit_test)
