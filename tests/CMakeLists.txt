# 测试模块管理
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Test Modules Configuration ===")

# 添加各个测试模块
add_subdirectory(basic)
add_subdirectory(spdlog)
add_subdirectory(network)
add_subdirectory(utilities)

message(STATUS "All test modules have been added")
message(STATUS "==================================")