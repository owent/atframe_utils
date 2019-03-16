
aux_source_directory(. SRC_LIST_SAMPLE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/sample")

set(BIN_NAME "${PROJECT_LIB_LINK}_sample")

find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
    add_definitions(-D_POSIX_MT_)
    list(APPEND COMPILER_OPTION_EXTERN_CXX_LIBS pthread)
endif ()

add_executable(${BIN_NAME} ${SRC_LIST_SAMPLE})

target_link_libraries(${BIN_NAME} ${PROJECT_LIB_LINK} ${COMPILER_OPTION_EXTERN_CXX_LIBS})

if (MSVC)
    set_property(TARGET ${BIN_NAME} PROPERTY FOLDER "atframework/sample")
endif (MSVC)

add_test(NAME test COMMAND ${BIN_NAME})