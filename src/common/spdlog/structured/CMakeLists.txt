# Zeus Structured Logging Module
cmake_minimum_required(VERSION 3.15)

# 定义模块名称
set(MODULE_NAME zeus_structured_spdlog)

# 收集源文件
file(GLOB_RECURSE STRUCTURED_LOGGING_SOURCES
    "*.cpp"
    "*.h"
    "${CMAKE_SOURCE_DIR}/include/common/spdlog/structured/*.h"
)

# 创建静态库
add_library(${MODULE_NAME} STATIC
    ${STRUCTURED_LOGGING_SOURCES}
)

# 设置目标属性
set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# 包含目录
target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/common/spdlog/structured
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 依赖库
target_link_libraries(${MODULE_NAME}
    PUBLIC
        # Zeus日志基础模块
        zeus_spdlog
        
        # fmt库（用于高性能格式化）
        fmt::fmt
        
        # spdlog（如果是外部依赖）
        spdlog::spdlog
        
    PRIVATE
        # 线程库
        Threads::Threads
)

# 编译选项
target_compile_options(${MODULE_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -O3>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic -O3>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
)

# 编译定义
target_compile_definitions(${MODULE_NAME}
    PUBLIC
        ZEUS_STRUCTURED_LOG_VERSION_MAJOR=1
        ZEUS_STRUCTURED_LOG_VERSION_MINOR=0
        ZEUS_STRUCTURED_LOG_VERSION_PATCH=0
        
        # fmt相关定义
        FMT_HEADER_ONLY=1
        
    PRIVATE
        $<$<CONFIG:Debug>:ZEUS_STRUCTURED_LOG_DEBUG>
        $<$<CONFIG:Release>:ZEUS_STRUCTURED_LOG_RELEASE>
        
        # 性能优化定义
        FMT_USE_CONSTEXPR=1
        SPDLOG_FMT_EXTERNAL=1
)

# 如果找到了性能分析工具，启用性能优化
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PERF QUIET libperftools)
    if(PERF_FOUND)
        target_compile_definitions(${MODULE_NAME} PRIVATE ZEUS_ENABLE_PROFILING=1)
        target_link_libraries(${MODULE_NAME} PRIVATE ${PERF_LIBRARIES})
    endif()
endif()

# 安装规则
install(TARGETS ${MODULE_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/spdlog/structured/
    DESTINATION include/common/spdlog/structured
    FILES_MATCHING PATTERN "*.h"
)

install(FILES ${CMAKE_SOURCE_DIR}/include/common/spdlog/zeus_structured_log.h
    DESTINATION include/common/spdlog
)

# 调试信息
message(STATUS "Zeus Structured Logging Module Configuration:")
message(STATUS "  Module Name: ${MODULE_NAME}")
message(STATUS "  Source Files: ${STRUCTURED_LOGGING_SOURCES}")
message(STATUS "  CXX Standard: C++17")

# 可选：创建示例可执行文件
option(ZEUS_BUILD_STRUCTURED_LOG_EXAMPLES "Build Zeus Structured Log examples" ON)
if(ZEUS_BUILD_STRUCTURED_LOG_EXAMPLES)
    # 构建示例程序
    add_executable(structured_logging_example
        ${CMAKE_SOURCE_DIR}/examples/structured_logging_field_example.cpp
    )
    
    target_link_libraries(structured_logging_example
        PRIVATE
            ${MODULE_NAME}
            zeus_spdlog
    )
    
    set_target_properties(structured_logging_example PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    
    # 创建输出目录
    add_custom_command(TARGET structured_logging_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:structured_logging_example>/logs
    )
endif()

# 可选：创建性能测试
option(ZEUS_BUILD_STRUCTURED_LOG_BENCHMARKS "Build Zeus Structured Log benchmarks" OFF)
if(ZEUS_BUILD_STRUCTURED_LOG_BENCHMARKS)
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_subdirectory(benchmarks)
    else()
        message(STATUS "Google Benchmark not found, skipping benchmark builds")
    endif()
endif()

# 可选：创建单元测试
option(ZEUS_BUILD_STRUCTURED_LOG_TESTS "Build Zeus Structured Log tests" OFF)
if(ZEUS_BUILD_STRUCTURED_LOG_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()