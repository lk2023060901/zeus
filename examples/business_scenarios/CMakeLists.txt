# Business Scenarios Example
cmake_minimum_required(VERSION 3.15)

# 示例可执行文件
add_executable(business_scenarios_example
    business_scenarios_example.cpp
)

# 设置C++标准
set_target_properties(business_scenarios_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接依赖库
target_link_libraries(business_scenarios_example
    PRIVATE
        zeus_structured_spdlog
        zeus_spdlog
        fmt::fmt
        Threads::Threads
)

# 包含目录
target_include_directories(business_scenarios_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# 编译定义
target_compile_definitions(business_scenarios_example
    PRIVATE
        ZEUS_EXAMPLE_BUILD=1
        ZEUS_BUSINESS_SCENARIOS=1
)

# 创建日志输出目录
add_custom_command(TARGET business_scenarios_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
    $<TARGET_FILE_DIR:business_scenarios_example>/logs
    COMMENT "Creating logs directory for business scenarios example"
)

# 运行目标
add_custom_target(run_business_scenarios
    COMMAND business_scenarios_example
    DEPENDS business_scenarios_example
    WORKING_DIRECTORY $<TARGET_FILE_DIR:business_scenarios_example>
    COMMENT "Running business scenarios example"
)