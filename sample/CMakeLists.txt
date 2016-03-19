
aux_source_directory(. SRC_LIST_SAMPLE)

add_executable(owent_utils_sample ${SRC_LIST_SAMPLE})

target_link_libraries(owent_utils_sample owent_utils ${EXTENTION_LINK_LIB})

			
