# Zeus Structured Logging Tests
cmake_minimum_required(VERSION 3.15)

# 查找测试框架
find_package(GTest REQUIRED)

# 测试源文件
set(TEST_SOURCES
    test_field.cpp
    # 未来可以添加更多测试文件
    # test_structured_logger.cpp
    # test_formatters.cpp
)

# 创建测试可执行文件
add_executable(zeus_structured_log_tests ${TEST_SOURCES})

# 设置C++标准
set_target_properties(zeus_structured_log_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 链接库
target_link_libraries(zeus_structured_log_tests
    PRIVATE
        zeus_structured_spdlog
        GTest::GTest
        GTest::Main
        Threads::Threads
)

# 包含目录
target_include_directories(zeus_structured_log_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/common/spdlog/structured
)

# 编译定义
target_compile_definitions(zeus_structured_log_tests
    PRIVATE
        ZEUS_STRUCTURED_LOG_TESTING=1
)

# 发现测试
include(GoogleTest)
gtest_discover_tests(zeus_structured_log_tests)

# 添加自定义测试目标
add_custom_target(run_structured_log_tests
    COMMAND zeus_structured_log_tests
    DEPENDS zeus_structured_log_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# 创建测试输出目录
add_custom_command(TARGET zeus_structured_log_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
    $<TARGET_FILE_DIR:zeus_structured_log_tests>/test_logs
)