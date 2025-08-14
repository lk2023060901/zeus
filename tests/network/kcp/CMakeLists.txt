# KCP网络模块测试
cmake_minimum_required(VERSION 3.16)

message(STATUS "Configuring KCP network tests...")

# KCP连接测试
add_executable(test_kcp_connector
    test_kcp_connector.cpp
)

target_link_libraries(test_kcp_connector
    PRIVATE
        common_network
        common_spdlog
)

set_target_properties(test_kcp_connector PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(test_kcp_connector PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# KCP服务器测试
add_executable(test_kcp_acceptor
    test_kcp_acceptor.cpp
)

target_link_libraries(test_kcp_acceptor
    PRIVATE
        common_network
        common_spdlog
)

set_target_properties(test_kcp_acceptor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(test_kcp_acceptor PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

message(STATUS "KCP network tests configured successfully")