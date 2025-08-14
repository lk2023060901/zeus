# Custom Formatters Example
cmake_minimum_required(VERSION 3.15)

# 示例可执行文件
add_executable(custom_formatters_example
    custom_formatters_example.cpp
)

# 设置C++标准
set_target_properties(custom_formatters_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接依赖库
target_link_libraries(custom_formatters_example
    PRIVATE
        zeus_structured_spdlog
        zeus_spdlog
        fmt::fmt
        Threads::Threads
)

# 包含目录
target_include_directories(custom_formatters_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# 编译定义
target_compile_definitions(custom_formatters_example
    PRIVATE
        ZEUS_EXAMPLE_BUILD=1
        ZEUS_CUSTOM_FORMATTERS=1
)

# 创建日志输出目录
add_custom_command(TARGET custom_formatters_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
    $<TARGET_FILE_DIR:custom_formatters_example>/logs
    COMMENT "Creating logs directory for custom formatters example"
)

# 运行目标
add_custom_target(run_custom_formatters
    COMMAND custom_formatters_example
    DEPENDS custom_formatters_example
    WORKING_DIRECTORY $<TARGET_FILE_DIR:custom_formatters_example>
    COMMENT "Running custom formatters example"
)