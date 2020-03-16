
aux_source_directory(. SRC_LIST_SAMPLE)

set(BIN_NAME "${PROJECT_ATFRAME_UTILS_LIB_NAME}_sample")

set(CMAKE_BUILD_RPATH "$ORIGIN/../lib" "$ORIGIN/../lib64")
if (NOT (WIN32 AND BUILD_SHARED_LIBS))
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/sample")
endif ()

add_executable(${BIN_NAME} ${SRC_LIST_SAMPLE})

target_link_libraries(${BIN_NAME} 
    ${PROJECT_ATFRAME_UTILS_LIB_NAME} 
)

target_compile_options(${BIN_NAME}
    PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS}
)

if (MSVC)
    set_property(TARGET ${BIN_NAME} PROPERTY FOLDER "atframework/sample")
endif (MSVC)

add_test(NAME test COMMAND ${BIN_NAME})
