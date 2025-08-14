# 测试模块管理
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Test Modules Configuration ===")

# 添加各个测试模块
add_subdirectory(basic)
add_subdirectory(spdlog)
add_subdirectory(network)
add_subdirectory(utilities)

# 如果启用了工具构建，添加 lua_binding_generator 测试
if(BUILD_TOOLS)
    add_subdirectory(lua_binding_generator)
endif()

message(STATUS "All test modules have been added")
message(STATUS "==================================")