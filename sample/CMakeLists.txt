aux_source_directory(. SRC_LIST_SAMPLE)

set(BIN_NAME "${PROJECT_NAME}_sample")

if(NOT (WIN32 AND BUILD_SHARED_LIBS))
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/sample")
endif()

add_executable(${BIN_NAME} ${SRC_LIST_SAMPLE})

target_link_libraries(${BIN_NAME} ${PROJECT_NAME})

target_compile_options(${BIN_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})

set_target_properties(
  ${BIN_NAME}
  PROPERTIES INSTALL_RPATH_USE_LINK_PATH YES
             BUILD_WITH_INSTALL_RPATH NO
             BUILD_RPATH_USE_ORIGIN YES)

if(MSVC)
  set_property(TARGET ${BIN_NAME} PROPERTY FOLDER "atframework/sample")
endif(MSVC)

add_test(NAME "atframe_utils.sample" COMMAND "$<TARGET_FILE:${BIN_NAME}>")
set_tests_properties("atframe_utils.sample" PROPERTIES LABELS "atframe_utils;atframe_utils.sample")

if(NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/config.ini"
                        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/test.ini" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
