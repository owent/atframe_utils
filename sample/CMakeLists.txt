
aux_source_directory(. SRC_LIST_SAMPLE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/sample")

set(BIN_NAME "${PROJECT_LIB_LINK}_sample")
add_executable(${BIN_NAME} ${SRC_LIST_SAMPLE})

if (CRYPTO_USE_OPENSSL OR CRYPTO_USE_LIBRESSL OR CRYPTO_USE_BORINGSSL)
    find_package(OpenSSL)
    list(APPEND PROJECT_LIB_LINK ${OPENSSL_CRYPTO_LIBRARY})
    if(UNIX)
        find_library(TRY_FIND_DL dl)
        if (TRY_FIND_DL)
            list(APPEND PROJECT_LIB_LINK dl)
        endif()
    endif()
elseif (CRYPTO_USE_MBEDTLS)
    find_package(MbedTLS)
    list(APPEND PROJECT_LIB_LINK ${MbedTLS_CRYPTO_LIBRARIES})
endif()

if(LIBSODIUM_FOUND)
    list(APPEND PROJECT_LIB_LINK ${Libsodium_LIBRARIES})
endif()

if (LOG_STACKTRACE_USING_LIBUNWIND)
    list(APPEND PROJECT_LIB_LINK ${Libunwind_LIBRARIES})
endif()

target_link_libraries(${BIN_NAME} ${PROJECT_LIB_LINK} ${COMPILER_OPTION_EXTERN_CXX_LIBS})

add_test(NAME test COMMAND ${BIN_NAME})