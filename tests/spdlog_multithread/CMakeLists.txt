# Spdlog多线程测试模块
cmake_minimum_required(VERSION 3.16)

# 创建多线程测试可执行程序
add_executable(test_spdlog_multithread test_multithread.cpp)

# 链接common库
target_link_libraries(test_spdlog_multithread common_spdlog)

# 设置输出目录
set_target_properties(test_spdlog_multithread PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 复制配置文件到输出目录
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/log_config.json ${CMAKE_BINARY_DIR}/bin/spdlog_multithread_log_config.json COPYONLY)

message(STATUS "Added Spdlog Multithread Test Module")