cmake_minimum_required(VERSION 3.16)
project(lua_binding_generator VERSION 2.0.0 LANGUAGES CXX)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 检测 LLVM 安装
if(EXISTS "/opt/homebrew/opt/llvm")
    # macOS Homebrew LLVM
    set(LLVM_ROOT "/opt/homebrew/opt/llvm")
    set(LLVM_INCLUDE_DIRS "${LLVM_ROOT}/include")
    set(LLVM_LIBRARY_DIRS "${LLVM_ROOT}/lib")
    message(STATUS "使用 Homebrew LLVM: ${LLVM_ROOT}")
elseif(EXISTS "/usr/local/opt/llvm")
    # macOS Homebrew LLVM (Intel)
    set(LLVM_ROOT "/usr/local/opt/llvm")
    set(LLVM_INCLUDE_DIRS "${LLVM_ROOT}/include")
    set(LLVM_LIBRARY_DIRS "${LLVM_ROOT}/lib")
    message(STATUS "使用 Homebrew LLVM (Intel): ${LLVM_ROOT}")
elseif(EXISTS "/usr/include/clang")
    # 系统安装的 LLVM/Clang
    set(LLVM_INCLUDE_DIRS "/usr/include")
    set(LLVM_LIBRARY_DIRS "/usr/lib")
    message(STATUS "使用系统 LLVM/Clang")
else()
    message(FATAL_ERROR "未找到 LLVM/Clang 开发库。请安装：brew install llvm")
endif()

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include    # Zeus 项目包含路径
    ${LLVM_INCLUDE_DIRS}
)

# 源文件列表 - 零配置版本
set(GENERATOR_SOURCES
    src/compiler_detector.cpp
    src/dynamic_compilation_database.cpp
    src/smart_inference_engine.cpp
    src/ast_visitor.cpp
    src/direct_binding_generator.cpp
    src/incremental_generator.cpp
)

# 头文件列表 - 零配置版本
set(GENERATOR_HEADERS
    include/lua_binding_generator/compiler_detector.h
    include/lua_binding_generator/dynamic_compilation_database.h
    include/lua_binding_generator/smart_inference_engine.h
    include/lua_binding_generator/ast_visitor.h
    include/lua_binding_generator/direct_binding_generator.h
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

# 链接库目录
link_directories(${LLVM_LIBRARY_DIRS})

# 查找必要的 Clang/LLVM 库
find_library(CLANG_TOOLING_LIB clangTooling PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_FRONTEND_LIB clangFrontend PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_AST_LIB clangAST PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_BASIC_LIB clangBasic PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_LEX_LIB clangLex PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_PARSE_LIB clangParse PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_SEMA_LIB clangSema PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_EDIT_LIB clangEdit PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_ANALYSIS_LIB clangAnalysis PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_DRIVER_LIB clangDriver PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_SERIALIZATION_LIB clangSerialization PATHS ${LLVM_LIBRARY_DIRS})

# 使用 clang-cpp 共享库而不是单独的库
find_library(CLANG_CPP_LIB clang-cpp PATHS ${LLVM_LIBRARY_DIRS})
find_library(LLVM_LIB LLVM PATHS ${LLVM_LIBRARY_DIRS})

# 获取 LLVM 系统依赖
execute_process(
    COMMAND ${LLVM_ROOT}/bin/llvm-config --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 链接库
if(CLANG_CPP_LIB AND LLVM_LIB)
    # 使用共享库
    target_link_libraries(lua_binding_generator
        ${CLANG_CPP_LIB}
        ${LLVM_LIB}
        ${LLVM_SYSTEM_LIBS}
    )
    message(STATUS "使用共享库: clang-cpp + LLVM")
else()
    # 回退到单独的静态库
    target_link_libraries(lua_binding_generator
        ${CLANG_TOOLING_LIB}
        ${CLANG_FRONTEND_LIB}
        ${CLANG_AST_LIB}
        ${CLANG_BASIC_LIB}
        ${CLANG_LEX_LIB}
        ${CLANG_PARSE_LIB}
        ${CLANG_SEMA_LIB}
        ${CLANG_EDIT_LIB}
        ${CLANG_ANALYSIS_LIB}
        ${CLANG_DRIVER_LIB}
        ${CLANG_SERIALIZATION_LIB}
        ${LLVM_SYSTEM_LIBS}
    )
    message(STATUS "使用静态库")
endif()

# 编译选项
target_compile_options(lua_binding_generator PRIVATE
    -fno-rtti
)

# 显示配置信息
message(STATUS "=== 零配置 Lua Binding Generator ===")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "LLVM 根目录: ${LLVM_ROOT}")
message(STATUS "LLVM 包含目录: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM 库目录: ${LLVM_LIBRARY_DIRS}")
if(CLANG_TOOLING_LIB)
    message(STATUS "Clang Tooling 库: ${CLANG_TOOLING_LIB}")
else()
    message(WARNING "未找到 Clang Tooling 库")
endif()
message(STATUS "====================================")

# 安装配置
install(TARGETS lua_binding_generator
    RUNTIME DESTINATION bin
    COMPONENT tools
)