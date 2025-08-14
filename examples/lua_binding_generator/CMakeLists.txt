cmake_minimum_required(VERSION 3.16)

# Lua Binding Generator 示例

# 查找依赖库
find_package(Lua REQUIRED)
find_package(Sol2 REQUIRED)

# 编译器特定设置 - 修复 Sol2 库警告
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option")
endif()

# 手动 Sol2 绑定示例
add_executable(manual_sol2_bindings_example 
    test_manual_bindings.cpp
)

set_target_properties(manual_sol2_bindings_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(manual_sol2_bindings_example PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/lua_binding_generator
    ${LUA_INCLUDE_DIRS}
)

# 将 Sol2 头文件设置为系统头文件以抑制警告
target_include_directories(manual_sol2_bindings_example SYSTEM PRIVATE
    ${SOL2_INCLUDE_DIRS}
)

target_link_libraries(manual_sol2_bindings_example
    lua::lua
)

# 为目标添加编译选项以抑制 Sol2 警告
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(manual_sol2_bindings_example PRIVATE
        -Wno-unknown-warning-option
        -Wno-ignored-pragmas
    )
endif()

# 添加到示例运行目标
add_custom_target(run_lua_binding_examples
    DEPENDS manual_sol2_bindings_example
    COMMENT "Building Lua binding generator examples"
)

add_custom_target(run_manual_sol2_example
    COMMAND manual_sol2_bindings_example
    DEPENDS manual_sol2_bindings_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running manual Sol2 bindings example"
)

message(STATUS "Lua binding generator examples configured")