# Zeus Examples
cmake_minimum_required(VERSION 3.15)

message(STATUS "Building Zeus examples...")

# 通用的示例配置函数
function(add_zeus_example EXAMPLE_NAME SOURCE_DIR)
    message(STATUS "Adding example: ${EXAMPLE_NAME}")
    add_subdirectory(${SOURCE_DIR})
endfunction()

# 添加各个示例
add_zeus_example("Basic Structured Logging" "basic_structured_logging")
add_zeus_example("Performance Comparison" "performance_comparison")  
add_zeus_example("Business Scenarios" "business_scenarios")
add_zeus_example("Custom Formatters" "custom_formatters")
add_zeus_example("Simple Application Framework" "simple_application")
add_zeus_example("Enhanced Signal Handling" "enhanced_signal_handling")

# 创建一个运行所有示例的目标
add_custom_target(run_all_examples
    COMMENT "Running all Zeus examples"
)

# 依赖所有示例可执行文件
add_dependencies(run_all_examples
    basic_structured_logging_example
    performance_comparison_example
    business_scenarios_example
    custom_formatters_example
    simple_application_example
    enhanced_signal_demo
)