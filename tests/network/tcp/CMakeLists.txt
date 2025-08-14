# TCP网络模块测试
cmake_minimum_required(VERSION 3.16)

message(STATUS "Configuring TCP network tests...")

# TCP连接测试
add_executable(test_tcp_connector
    test_tcp_connector.cpp
)

target_link_libraries(test_tcp_connector
    PRIVATE
        common_network
        common_spdlog
)

set_target_properties(test_tcp_connector PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(test_tcp_connector PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TCP服务器测试
add_executable(test_tcp_acceptor
    test_tcp_acceptor.cpp
)

target_link_libraries(test_tcp_acceptor
    PRIVATE
        common_network
        common_spdlog
)

set_target_properties(test_tcp_acceptor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(test_tcp_acceptor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

message(STATUS "TCP network tests configured successfully")