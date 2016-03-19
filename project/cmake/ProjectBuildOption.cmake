# 默认配置选项
#####################################################################
option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." OFF)

# atbus 选项
set(ATBUS_MACRO_BUSID_TYPE "uint64_t" CACHE STRING "atbus的busid类型")
set(ATBUS_MACRO_DATA_NODE_SIZE 128 CACHE STRING "atbus的内存通道node大小（必须是2的倍数）")
set(ATBUS_MACRO_DATA_ALIGN_TYPE "uint64_t" CACHE STRING "atbus的内存内存块对齐类型（用于优化memcpy和校验）")
set(ATBUS_MACRO_DATA_SMALL_SIZE 512 CACHE STRING "流通道小数据块大小（用于优化减少内存拷贝）")

set(ATBUS_MACRO_HUGETLB_SIZE 4194304 CACHE STRING "大页表分页大小（用于优化共享内存分页）")
set(ATBUS_MACRO_MSG_LIMIT 65536 CACHE STRING "默认消息体大小限制")
set(ATBUS_MACRO_CONNECTION_CONFIRM_TIMEOUT 30 CACHE STRING "默认连接确认时限")
set(ATBUS_MACRO_CONNECTION_BACKLOG 128 CACHE STRING "默认握手队列的最大连接数")

# libuv选项
set(LIBUV_ROOT "" CACHE STRING "libuv root directory")

# 测试配置选项
set(GTEST_ROOT "" CACHE STRING "GTest root directory")
set(BOOST_ROOT "" CACHE STRING "Boost root directory")
option(PROJECT_TEST_ENABLE_BOOST_UNIT_TEST "Enable boost unit test." OFF)

