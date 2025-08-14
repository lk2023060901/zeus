# HTTP网络模块测试
cmake_minimum_required(VERSION 3.16)

message(STATUS "Configuring HTTP network tests...")

# HTTP客户端测试
add_executable(test_http_client
    test_http_client.cpp
)

target_link_libraries(test_http_client
    PRIVATE
        common_network
        common_spdlog
)

set_target_properties(test_http_client PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(test_http_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# HTTPS客户端测试
add_executable(test_https_client
    test_https_client.cpp
)

target_link_libraries(test_https_client
    PRIVATE
        common_network
        common_spdlog
)

set_target_properties(test_https_client PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(test_https_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

message(STATUS "HTTP network tests configured successfully - HTTP and HTTPS client tests added")