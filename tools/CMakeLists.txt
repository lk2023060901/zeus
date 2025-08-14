# Zeus 开发工具模块
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Zeus Tools Configuration ===")

# 工具构建状态
set(TOOLS_BUILT 0)

# Lua 绑定生成器工具 - 可以通过 BUILD_LUA_BINDINGS 或 BUILD_TOOLS 启用
if(BUILD_LUA_BINDINGS OR BUILD_TOOLS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lua_binding_generator/CMakeLists.txt")
        message(STATUS "Building Lua Binding Generator...")
        add_subdirectory(lua_binding_generator)
        math(EXPR TOOLS_BUILT "${TOOLS_BUILT} + 1")
    else()
        message(WARNING "Lua Binding Generator source not found")
    endif()
endif()

# 其他开发工具可以在这里添加
# 当 BUILD_TOOLS=ON 时，所有工具都会被构建
if(BUILD_TOOLS)
    # 示例：如果有其他工具目录，可以在这里添加
    # if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/code_generator/CMakeLists.txt")
    #     message(STATUS "Building Code Generator...")
    #     add_subdirectory(code_generator)
    #     math(EXPR TOOLS_BUILT "${TOOLS_BUILT} + 1")
    # endif()
    
    # if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/asset_processor/CMakeLists.txt")
    #     message(STATUS "Building Asset Processor...")
    #     add_subdirectory(asset_processor)
    #     math(EXPR TOOLS_BUILT "${TOOLS_BUILT} + 1")
    # endif()
endif()

# 显示工具构建统计
if(TOOLS_BUILT GREATER 0)
    message(STATUS "Total tools to be built: ${TOOLS_BUILT}")
else()
    message(STATUS "No tools will be built")
endif()

message(STATUS "=================================")