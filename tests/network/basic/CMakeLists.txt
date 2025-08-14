# 网络模块基础测试
cmake_minimum_required(VERSION 3.16)

# 测试可执行文件
add_executable(test_network_basic
    test_network_basic.cpp
)

# 链接库
target_link_libraries(test_network_basic
    PRIVATE
        common_network
        common_spdlog
)

# 设置目标属性
set_target_properties(test_network_basic PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 包含目录
target_include_directories(test_network_basic PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 复制配置文件到构建目录
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/network_log_config.json
    ${CMAKE_BINARY_DIR}/bin/network_log_config.json
    COPYONLY
)

message(STATUS "Network basic test configured")