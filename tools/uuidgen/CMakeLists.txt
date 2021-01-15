aux_source_directory(. SRC_LIST_SAMPLE)

set(BIN_NAME "uuidgen")

add_executable(${BIN_NAME} ${SRC_LIST_SAMPLE})

set_target_properties(
  ${BIN_NAME}
  PROPERTIES INSTALL_RPATH_USE_LINK_PATH YES
             BUILD_WITH_INSTALL_RPATH NO
             BUILD_RPATH_USE_ORIGIN YES)

target_link_libraries(${BIN_NAME} ${PROJECT_ATFRAME_UTILS_LIB_NAME})

target_compile_options(${BIN_NAME} PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})

if(MSVC)
  set_property(TARGET ${BIN_NAME} PROPERTY FOLDER "atframework/tools")
  add_target_properties(${BIN_NAME} LINK_FLAGS /NODEFAULTLIB:library)
endif(MSVC)

add_test(NAME test COMMAND ${BIN_NAME})
