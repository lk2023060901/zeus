# spdlog模块测试管理
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== spdlog Test Modules Configuration ===")

# 添加各个spdlog测试模块
add_subdirectory(basic)
add_subdirectory(config)
add_subdirectory(multithread)
add_subdirectory(performance)

message(STATUS "All spdlog test modules have been added")
message(STATUS "===========================================")