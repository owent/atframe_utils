aux_source_directory(. SRC_LIST_SAMPLE)

set(BIN_NAME "uuidgen")

add_executable(${BIN_NAME} ${SRC_LIST_SAMPLE})

set_target_properties(
  ${BIN_NAME}
  PROPERTIES INSTALL_RPATH_USE_LINK_PATH YES
             BUILD_WITH_INSTALL_RPATH NO
             BUILD_RPATH_USE_ORIGIN YES)

target_link_libraries(${BIN_NAME} ${PROJECT_NAME})

target_compile_options(${BIN_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})

set_property(TARGET ${BIN_NAME} PROPERTY FOLDER "atframework/tools")
if(MSVC)
  add_target_properties(${BIN_NAME} LINK_FLAGS /NODEFAULTLIB:library)
endif(MSVC)

add_test(NAME test-uuidgen COMMAND "$<TARGET_FILE:${BIN_NAME}>")
add_test(NAME test-uuidgen-r COMMAND "$<TARGET_FILE:${BIN_NAME}>" -r)
add_test(NAME test-uuidgen-t COMMAND "$<TARGET_FILE:${BIN_NAME}>" -t)
add_test(NAME test-uuidgen-V COMMAND "$<TARGET_FILE:${BIN_NAME}>" -V)
add_test(NAME test-uuidgen-h COMMAND "$<TARGET_FILE:${BIN_NAME}>" -h)
add_test(NAME test-uuidgen--bad-params COMMAND ${BIN_NAME} --bad-params)

set_tests_properties(test-uuidgen test-uuidgen-r test-uuidgen-t test-uuidgen-V test-uuidgen-h test-uuidgen--bad-params
                     PROPERTIES LABELS "atframe_utils;atframe_utils.tools")
