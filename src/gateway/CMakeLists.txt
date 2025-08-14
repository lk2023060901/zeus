# Zeus Gateway Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Gateway Module Configuration ===")

# Gateway module sources
set(GATEWAY_SOURCES
    gateway_server.cpp
    gateway_session.cpp
    protocol_router.cpp
)

# Gateway module headers
set(GATEWAY_HEADERS
    ${CMAKE_SOURCE_DIR}/include/gateway/gateway_server.h
    ${CMAKE_SOURCE_DIR}/include/gateway/gateway_session.h
    ${CMAKE_SOURCE_DIR}/include/gateway/protocol_router.h
)

# Create the gateway library
add_library(gateway_module ${GATEWAY_SOURCES} ${GATEWAY_HEADERS})

# Set target properties
set_target_properties(gateway_module PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "gateway_module"
)

# Include directories
target_include_directories(gateway_module
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(gateway_module
    PUBLIC
        common_network  # Zeus networking
        common_spdlog   # Zeus logging
)

# Protocol compilation definitions
if(USE_KCP_PROTOCOL)
    target_compile_definitions(gateway_module PUBLIC ZEUS_USE_KCP=1)
    message(STATUS "Gateway: Using KCP protocol")
else()
    target_compile_definitions(gateway_module PUBLIC ZEUS_USE_TCP=1)
    message(STATUS "Gateway: Using TCP protocol")
endif()

# Create the gateway server executable
add_executable(gateway_server main.cpp)

# Set target properties for executable
set_target_properties(gateway_server PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "gateway_server"
)

# Include directories for executable
target_include_directories(gateway_server
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies for executable
target_link_libraries(gateway_server
    PRIVATE
        gateway_module      # Gateway library
        zeus_core_app      # Zeus application framework
        common_network     # Zeus networking
        common_spdlog      # Zeus logging
)

# Protocol compilation definitions for executable
if(USE_KCP_PROTOCOL)
    target_compile_definitions(gateway_server PRIVATE ZEUS_USE_KCP=1)
    message(STATUS "Gateway Server: Using KCP protocol")
else()
    target_compile_definitions(gateway_server PRIVATE ZEUS_USE_TCP=1)
    message(STATUS "Gateway Server: Using TCP protocol")
endif()

message(STATUS "Gateway module configured successfully")
message(STATUS "Gateway server executable configured")
message(STATUS "======================================")