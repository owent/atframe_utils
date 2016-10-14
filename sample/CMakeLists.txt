
aux_source_directory(. SRC_LIST_SAMPLE)

add_executable(${PROJECT_LIB_LINK}_sample ${SRC_LIST_SAMPLE})
    
target_link_libraries(${PROJECT_LIB_LINK}_sample ${PROJECT_LIB_LINK} ${COMPILER_OPTION_EXTERN_CXX_LIBS})
