# Performance Comparison Example
cmake_minimum_required(VERSION 3.15)

# 示例可执行文件
add_executable(performance_comparison_example
    performance_comparison_example.cpp
)

# 设置C++标准
set_target_properties(performance_comparison_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接依赖库
target_link_libraries(performance_comparison_example
    PRIVATE
        common_spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json  # 用于JSON对比
        Threads::Threads
)

# 包含目录
target_include_directories(performance_comparison_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# 编译定义 - 启用性能优化
target_compile_definitions(performance_comparison_example
    PRIVATE
        ZEUS_EXAMPLE_BUILD=1
        ZEUS_PERFORMANCE_TEST=1
)

# 性能优化编译选项
target_compile_options(performance_comparison_example
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-O3 -march=native>
        $<$<CXX_COMPILER_ID:Clang>:-O3 -march=native>
        $<$<CXX_COMPILER_ID:MSVC>:/O2>
)

# 创建日志输出目录
add_custom_command(TARGET performance_comparison_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
    $<TARGET_FILE_DIR:performance_comparison_example>/logs
    COMMENT "Creating logs directory for performance comparison example"
)

# 运行目标
add_custom_target(run_performance_comparison
    COMMAND performance_comparison_example
    DEPENDS performance_comparison_example
    WORKING_DIRECTORY $<TARGET_FILE_DIR:performance_comparison_example>
    COMMENT "Running performance comparison example"
)