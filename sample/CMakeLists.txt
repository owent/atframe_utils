# Copyright 2026 atframework
aux_source_directory(. SRC_LIST_SAMPLE)

set(BIN_NAME "${PROJECT_NAME}_sample")

add_executable(${BIN_NAME} ${SRC_LIST_SAMPLE})

if(PROJECT_SAMPLE_RUNTIME_OUTPUT_DIRECTORY)
  atframe_target_set_runtime_output_directory(${BIN_NAME} "${PROJECT_SAMPLE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries(${BIN_NAME} ${PROJECT_NAME})

target_compile_options(${BIN_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})

set_target_properties(
  ${BIN_NAME}
  PROPERTIES INSTALL_RPATH_USE_LINK_PATH YES
             BUILD_WITH_INSTALL_RPATH NO
             BUILD_RPATH_USE_ORIGIN YES)

set_property(TARGET ${BIN_NAME} PROPERTY FOLDER "atframework/sample")

add_test(NAME "atframe_utils.sample" COMMAND "$<TARGET_FILE:${BIN_NAME}>")
set_tests_properties("atframe_utils.sample" PROPERTIES LABELS "atframe_utils;atframe_utils.sample")
atframe_test_setup_run_environment("atframe_utils.sample" "${PROJECT_SAMPLE_RUNTIME_OUTPUT_DIRECTORY}")

# The sample loads config.ini/test.ini relative to the executable directory.
if(PROJECT_SAMPLE_RUNTIME_OUTPUT_DIRECTORY)
  set(PROJECT_SAMPLE_RESOURCE_OUTPUT_DIRECTORY "${PROJECT_SAMPLE_RUNTIME_OUTPUT_DIRECTORY}")
else()
  set(PROJECT_SAMPLE_RESOURCE_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()
if(NOT EXISTS ${PROJECT_SAMPLE_RESOURCE_OUTPUT_DIRECTORY})
  file(MAKE_DIRECTORY ${PROJECT_SAMPLE_RESOURCE_OUTPUT_DIRECTORY})
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/config.ini"
                        ${PROJECT_SAMPLE_RESOURCE_OUTPUT_DIRECTORY})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/test.ini"
                        ${PROJECT_SAMPLE_RESOURCE_OUTPUT_DIRECTORY})
