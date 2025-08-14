# Spdlog配置测试模块
cmake_minimum_required(VERSION 3.16)

# 创建配置测试可执行程序
add_executable(test_spdlog_config test_config.cpp)

# 链接common库
target_link_libraries(test_spdlog_config common_spdlog)

# 设置输出目录
set_target_properties(test_spdlog_config PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 复制配置文件到输出目录
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/log_config.json ${CMAKE_BINARY_DIR}/bin/spdlog_config_log_config.json COPYONLY)

message(STATUS "Added Spdlog Config Test Module")