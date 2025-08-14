# Basic Structured Logging Example
cmake_minimum_required(VERSION 3.15)

# 示例可执行文件
add_executable(basic_structured_logging_example
    basic_structured_logging_example.cpp
)

# 设置C++标准
set_target_properties(basic_structured_logging_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接依赖库
target_link_libraries(basic_structured_logging_example
    PRIVATE
        zeus_structured_spdlog
        zeus_spdlog
        fmt::fmt
        Threads::Threads
)

# 包含目录
target_include_directories(basic_structured_logging_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# 编译定义
target_compile_definitions(basic_structured_logging_example
    PRIVATE
        ZEUS_EXAMPLE_BUILD=1
)

# 创建日志输出目录
add_custom_command(TARGET basic_structured_logging_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
    $<TARGET_FILE_DIR:basic_structured_logging_example>/logs
    COMMENT "Creating logs directory for basic structured logging example"
)

# 添加到运行所有示例的目标中
add_custom_target(run_basic_structured_logging
    COMMAND basic_structured_logging_example
    DEPENDS basic_structured_logging_example
    WORKING_DIRECTORY $<TARGET_FILE_DIR:basic_structured_logging_example>
    COMMENT "Running basic structured logging example"
)