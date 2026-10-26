# Copyright 2026 atframework

add_executable(ac_automation_tool ac_automation_tool.cpp)
target_link_libraries(ac_automation_tool PRIVATE ${PROJECT_NAME})
target_compile_options(ac_automation_tool PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})
set_property(TARGET ac_automation_tool PROPERTY FOLDER "atframework/tools")

add_executable(ac_automation_benchmark ac_automation_benchmark.cpp)
target_link_libraries(ac_automation_benchmark PRIVATE ${PROJECT_NAME}
                                                      ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_XXHASH_LINK_NAME})
target_compile_options(ac_automation_benchmark PRIVATE ${COMPILER_STRICT_EXTRA_CFLAGS} ${COMPILER_STRICT_CFLAGS})
set_property(TARGET ac_automation_benchmark PROPERTY FOLDER "atframework/tools")

add_test(NAME atframe_utils.ac_tool.help COMMAND ac_automation_tool --help)
add_test(NAME atframe_utils.ac_tool.invalid COMMAND ac_automation_tool build --unknown)
set_tests_properties(atframe_utils.ac_tool.invalid PROPERTIES WILL_FAIL TRUE)
set_tests_properties(atframe_utils.ac_tool.help atframe_utils.ac_tool.invalid
                     PROPERTIES LABELS "atframe_utils;atframe_utils.tools")
