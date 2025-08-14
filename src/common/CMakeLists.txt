# Common库的CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 添加cmake模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# 静态库模式时，强制所有依赖也使用静态库
if(NOT BUILD_SHARED_LIBS)
    # 设置第三方库为静态链接
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    
    # 针对spdlog设置静态编译
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    
    # 针对boost设置静态链接
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    
    message(STATUS "Common: Building static libraries - all dependencies will be statically linked")
else()
    # 动态库模式
    set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
    set(Boost_USE_STATIC_LIBS OFF)
    message(STATUS "Common: Building shared libraries - dependencies will be dynamically linked")
endif()

# 查找依赖
find_package(Fmt REQUIRED)
find_package(Kcp REQUIRED)
find_package(Spdlog REQUIRED)
find_package(NlohmannJson REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Common Spdlog库源文件
set(COMMON_SPDLOG_SOURCES
    spdlog/zeus_log_config.cpp
    spdlog/zeus_log_manager.cpp
)

# 创建Common库 - 根据BUILD_SHARED_LIBS自动选择静态或动态
add_library(common_spdlog ${COMMON_SPDLOG_SOURCES})

# 设置库的导出符号 (适用于Windows平台)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(common_spdlog PRIVATE COMMON_EXPORTS)
endif()

# 链接依赖
target_link_libraries(common_spdlog 
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Boost::boost
)

# 网络协议编译定义
if(USE_KCP_PROTOCOL)
    target_compile_definitions(common_spdlog PUBLIC ZEUS_USE_KCP=1)
    message(STATUS "Common: Enabled KCP protocol support")
else()
    target_compile_definitions(common_spdlog PUBLIC ZEUS_USE_TCP=1)
    message(STATUS "Common: Enabled TCP protocol support")
endif()

# 包含目录
target_include_directories(common_spdlog PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 设置输出目录
set_target_properties(common_spdlog PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 静态库模式下的额外设置
if(NOT BUILD_SHARED_LIBS)
    set_target_properties(common_spdlog PROPERTIES
        POSITION_INDEPENDENT_CODE ON  # 支持在共享库中使用
    )
endif()

# 创建别名目标以供示例程序使用
add_library(zeus_spdlog ALIAS common_spdlog)

# 添加网络模块子目录
add_subdirectory(network)

# 添加工具模块子目录
add_subdirectory(utilities)

# 显示编译信息
message(STATUS "=== Common Library Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(BUILD_SHARED_LIBS)
    message(STATUS "Library type: SHARED")
else()
    message(STATUS "Library type: STATIC")
endif()
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "=====================================")