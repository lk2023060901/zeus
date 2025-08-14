cmake_minimum_required(VERSION 3.16)

# 查找依赖库
find_package(Lua REQUIRED)

# 简单绑定测试（使用系统 Lua）
add_executable(test_lua_binding_simple 
    simple_binding_test.cpp
)

set_target_properties(test_lua_binding_simple PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(test_lua_binding_simple PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${LUA_INCLUDE_DIRS}
)

target_link_libraries(test_lua_binding_simple
    lua::lua
)

# 如果 Sol2 可用，添加完整的绑定测试
if(TARGET Sol2::Sol2 OR Sol2_FOUND)
    # 完整的绑定测试（需要 Sol2）
    add_executable(test_lua_binding_full 
        test_bindings_impl.cpp
    )
    
    set_target_properties(test_lua_binding_full PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    
    target_include_directories(test_lua_binding_full PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${LUA_INCLUDE_DIRS}
        ${SOL2_INCLUDE_DIRS}
    )
    
    target_link_libraries(test_lua_binding_full
        ${LUA_LIBRARIES}
    )
    
    if(TARGET Sol2::Sol2)
        target_link_libraries(test_lua_binding_full Sol2::Sol2)
    endif()
    
    message(STATUS "Lua binding full test configured with Sol2")
else()
    message(STATUS "Sol2 not found - only simple binding test will be built")
endif()

# 测试已在顶级CMakeLists.txt中启用

add_test(
    NAME lua_binding_simple_test
    COMMAND test_lua_binding_simple
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(TARGET test_lua_binding_full)
    add_test(
        NAME lua_binding_full_test  
        COMMAND test_lua_binding_full
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# 复制 Lua 测试脚本到构建目录
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_lua_script.lua
    ${CMAKE_CURRENT_BINARY_DIR}/test_lua_script.lua
    COPYONLY
)

message(STATUS "Lua binding generator tests configured")