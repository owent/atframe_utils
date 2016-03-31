# 默认配置选项
#####################################################################
set(ENABLE_MIXEDINT_MAGIC_MASK 0 CACHE STRING "Integer mixed magic mask")

# 测试配置选项
set(GTEST_ROOT "" CACHE STRING "GTest root directory")
set(BOOST_ROOT "" CACHE STRING "Boost root directory")
option(PROJECT_TEST_ENABLE_BOOST_UNIT_TEST "Enable boost unit test." OFF)

option(PROJECT_ENABLE_UNITTEST "Enable unit test" OFF)
option(PROJECT_ENABLE_SAMPLE "Enable sample" OFF)
