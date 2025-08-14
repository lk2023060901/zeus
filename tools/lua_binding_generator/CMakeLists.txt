cmake_minimum_required(VERSION 3.16)
project(lua_binding_generator VERSION 2.0.0 LANGUAGES CXX)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 使用项目根目录的 cmake 模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# 按正确顺序查找依赖库 - 使用项目的依赖管理系统
find_package(Zstd REQUIRED)    # LLVM 依赖 zstd
find_package(LLVM REQUIRED)
find_package(Clang REQUIRED) 
find_package(Lua REQUIRED)
find_package(Sol2 REQUIRED)

# 编译器特定设置
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    # 修复 Sol2 库中的 GCC 特定警告在 Clang 中的问题
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option")
    endif()
endif()

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include    # 添加项目根目录的include，用于访问common/lua/export_macros.h
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    ${SOL2_INCLUDE_DIRS}
)

# 源文件列表 - 重构后的新架构
set(GENERATOR_SOURCES
    src/ast_visitor.cpp
    src/direct_binding_generator.cpp
    src/smart_inference_engine.cpp
    src/incremental_generator.cpp
)

# 头文件列表 - 重构后的新架构
set(GENERATOR_HEADERS
    include/lua_binding_generator/ast_visitor.h
    include/lua_binding_generator/direct_binding_generator.h
    include/lua_binding_generator/smart_inference_engine.h
    include/lua_binding_generator/incremental_generator.h
)

# 创建可执行文件
add_executable(lua_binding_generator 
    main.cpp
    ${GENERATOR_SOURCES}
    ${GENERATOR_HEADERS}
)

# 设置目标属性
set_target_properties(lua_binding_generator PROPERTIES
    OUTPUT_NAME lua_binding_generator
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 链接库 - 使用 Find 模块提供的导入目标
target_link_libraries(lua_binding_generator
    Clang::LibTooling    # 使用 FindClang.cmake 提供的 LibTooling 目标
    LLVM::LLVM           # 使用 FindLLVM.cmake 提供的目标
    zstd::zstd           # LLVM 依赖的压缩库
    lua::lua             # 使用 FindLua.cmake 提供的目标
    sol2::sol2           # 使用 FindSol2.cmake 提供的目标
)

# 编译选项
target_compile_options(lua_binding_generator PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    # 注意：保留异常处理，因为代码需要异常处理
)

# 安装配置
install(TARGETS lua_binding_generator
    RUNTIME DESTINATION bin
    COMPONENT tools
)

# 安装示例文件
install(DIRECTORY examples/
    DESTINATION share/lua_binding_generator/examples
    COMPONENT examples
    FILES_MATCHING PATTERN "*.h" PATTERN "*.json"
)

# 显示配置信息
message(STATUS "=== Lua Binding Generator Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "LLVM Version: ${LLVM_VERSION}")
if(CLANG_VERSION)
    message(STATUS "Clang Version: ${CLANG_VERSION}")
endif()
if(LUA_VERSION_STRING)
    message(STATUS "Lua Version: ${LUA_VERSION_STRING}")
endif()
if(SOL2_VERSION_STRING)
    message(STATUS "Sol2 Version: ${SOL2_VERSION_STRING}")
endif()
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "============================================")

# 添加一些开发辅助目标

# 格式化代码目标
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i -style=file
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
            ${GENERATOR_SOURCES}
            ${GENERATOR_HEADERS}
        COMMENT "Formatting source code"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# 静态分析目标
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
            ${GENERATOR_SOURCES}
            --
            -I${CMAKE_CURRENT_SOURCE_DIR}/include
            -I${LLVM_INCLUDE_DIRS}
            -I${CLANG_INCLUDE_DIRS}
            ${LLVM_DEFINITIONS}
            ${CLANG_DEFINITIONS}
        COMMENT "Running clang-tidy"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# 测试目标
if(BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()

# 文档目标
find_program(DOXYGEN_EXECUTABLE doxygen)
if(DOXYGEN_EXECUTABLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs")
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        COMMENT "Generating documentation"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
    )
endif()