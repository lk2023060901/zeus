# network模块测试管理
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Network Test Modules Configuration ===")

# 添加各个network测试模块
add_subdirectory(basic)
add_subdirectory(tcp)
add_subdirectory(kcp)
add_subdirectory(http)
add_subdirectory(integration)

message(STATUS "All network test modules have been added")
message(STATUS "===========================================")