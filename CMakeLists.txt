cmake_minimum_required(VERSION 3.16)
project(Zeus VERSION 1.0.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型选项 - 控制静态库/动态库
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" ON)

# 测试编译选项 - 控制是否编译测试用例
option(BUILD_TESTS "Build test programs" OFF)

# 示例编译选项 - 控制是否编译示例程序（默认关闭）
option(BUILD_EXAMPLES "Build example programs" OFF)

# 网络协议选择选项 - 控制Zeus所有服务使用的网络协议（默认KCP）
option(USE_KCP_PROTOCOL "Use KCP protocol for all Zeus services instead of TCP" ON)

# Gateway模块编译选项
option(BUILD_GATEWAY "Build Gateway module" ON)

# 添加common库子项目
add_subdirectory(src/common)

# 添加Gateway模块（如果启用）
if(BUILD_GATEWAY)
    add_subdirectory(src/gateway)
endif()

# 添加测试模块（如果启用）
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# 添加示例模块（如果启用）
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 显示编译信息
message(STATUS "=== Zeus Project Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(BUILD_SHARED_LIBS)
    message(STATUS "Library type: SHARED")
else()
    message(STATUS "Library type: STATIC")
endif()
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "Build Gateway: ${BUILD_GATEWAY}")
if(USE_KCP_PROTOCOL)
    message(STATUS "Network Protocol: KCP (Low-latency)")
else()
    message(STATUS "Network Protocol: TCP (Reliable)")
endif()
message(STATUS "===================================")