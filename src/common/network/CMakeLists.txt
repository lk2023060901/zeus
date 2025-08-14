# Zeus Network Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Network module sources
set(NETWORK_SOURCES
    network_logger.cpp
    network_events.cpp
    connection.cpp
    tcp_connector.cpp
    kcp_connector.cpp
    tcp_acceptor.cpp
    kcp_acceptor.cpp
    zeus_network.cpp
    # HTTP module sources
    http/http_common.cpp
    http/http_message.cpp
    http/http_client.cpp
    http/http_server.cpp
    http/http_router.cpp
    http/http_middleware.cpp
)

# Network module headers
set(NETWORK_HEADERS
    ${CMAKE_SOURCE_DIR}/include/common/network/network_logger.h
    ${CMAKE_SOURCE_DIR}/include/common/network/network_events.h
    ${CMAKE_SOURCE_DIR}/include/common/network/connection.h
    ${CMAKE_SOURCE_DIR}/include/common/network/tcp_connector.h
    ${CMAKE_SOURCE_DIR}/include/common/network/kcp_connector.h
    ${CMAKE_SOURCE_DIR}/include/common/network/tcp_acceptor.h
    ${CMAKE_SOURCE_DIR}/include/common/network/kcp_acceptor.h
    ${CMAKE_SOURCE_DIR}/include/common/network/zeus_network.h
    # HTTP module headers
    ${CMAKE_SOURCE_DIR}/include/common/network/http/http_common.h
    ${CMAKE_SOURCE_DIR}/include/common/network/http/http_message.h
    ${CMAKE_SOURCE_DIR}/include/common/network/http/http_client.h
    ${CMAKE_SOURCE_DIR}/include/common/network/http/http_server.h
    ${CMAKE_SOURCE_DIR}/include/common/network/http/http_router.h
    ${CMAKE_SOURCE_DIR}/include/common/network/http/http_middleware.h
)

# Create the network library
add_library(common_network ${NETWORK_SOURCES} ${NETWORK_HEADERS})

# Set target properties
set_target_properties(common_network PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "common_network"
)

# Include directories
target_include_directories(common_network
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(common_network
    PUBLIC
        common_spdlog  # Zeus logging module
        kcp::kcp       # KCP library
)

# Boost libraries (using direct library names for compatibility)
if(Boost_FOUND)
    target_link_libraries(common_network PUBLIC ${Boost_LIBRARIES})
    target_include_directories(common_network PUBLIC ${Boost_INCLUDE_DIRS})
endif()

# Find and link OpenSSL (required for HTTPS/SSL support)
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    target_include_directories(common_network
        PUBLIC
            ${OPENSSL_INCLUDE_DIR}
    )
    target_link_libraries(common_network 
        PUBLIC 
            ${OPENSSL_LIBRARIES}
    )
else()
    message(FATAL_ERROR "OpenSSL is required but not found")
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(common_network PRIVATE ws2_32 mswsock)
elseif(UNIX)
    target_link_libraries(common_network PUBLIC pthread)
endif()

# Compiler definitions
target_compile_definitions(common_network
    PUBLIC
        BOOST_ASIO_HAS_STD_CHRONO=1
        BOOST_ASIO_HAS_STD_THREAD=1
        BOOST_ASIO_NO_DEPRECATED=1
    PRIVATE
        $<$<CONFIG:Debug>:ZEUS_NETWORK_DEBUG=1>
        $<$<CONFIG:Release>:ZEUS_NETWORK_OPTIMIZED=1>
)

# 网络协议编译定义
if(USE_KCP_PROTOCOL)
    target_compile_definitions(common_network PUBLIC ZEUS_USE_KCP=1)
    message(STATUS "Network: Using KCP protocol")
else()
    target_compile_definitions(common_network PUBLIC ZEUS_USE_TCP=1) 
    message(STATUS "Network: Using TCP protocol")
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(common_network PRIVATE
        /W4                     # High warning level
        /WX                     # Treat warnings as errors
        /wd4251                 # Suppress DLL interface warnings
        /wd4275                 # Suppress DLL base class warnings
    )
else()
    target_compile_options(common_network PRIVATE
        -Wall                   # Enable most warnings
        -Wextra                 # Enable extra warnings
        -Werror                 # Treat warnings as errors
        -Wno-unused-parameter   # Don't warn about unused parameters (common in callbacks)
        -Wno-unused-variable    # Don't warn about unused variables in debug builds
    )
    
    # Additional GCC/Clang specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(common_network PRIVATE
            -Wno-maybe-uninitialized  # GCC false positives with std::optional
        )
    endif()
endif()

# Link-time optimizations for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)
    if(ipo_supported)
        set_target_properties(common_network PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE
        )
    endif()
endif()

# Installation rules
if(NOT BUILD_TESTS_ONLY)
    install(TARGETS common_network
        EXPORT zeus-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/network
        DESTINATION include/common
        FILES_MATCHING PATTERN "*.h"
    )
endif()

# Display configuration information
message(STATUS "=== Network Module Configuration ===")
message(STATUS "Library type: ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "Building shared network library")
else()
    message(STATUS "Building static network library")
endif()
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")

# Find Boost components and display versions
find_package(Boost REQUIRED COMPONENTS system thread)
if(Boost_FOUND)
    message(STATUS "Boost version: ${Boost_VERSION}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
else()
    message(WARNING "Boost not found - network module may not compile correctly")
endif()

# Find OpenSSL (required dependency)
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION_STRING}")
message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

message(STATUS "Network module sources: ${NETWORK_SOURCES}")
message(STATUS "=====================================")