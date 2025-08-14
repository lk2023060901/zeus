# Zeus String Utils Tests
cmake_minimum_required(VERSION 3.15)

# 查找测试框架
find_package(GTest REQUIRED)

# 测试源文件
set(STRING_UTILS_TEST_SOURCES
    test_string_utils_basic.cpp
    test_string_utils_parsing.cpp
    test_string_utils_datetime.cpp
    test_string_utils_advanced.cpp
)

# 创建测试可执行文件
add_executable(zeus_string_utils_tests ${STRING_UTILS_TEST_SOURCES})

# 设置C++标准
set_target_properties(zeus_string_utils_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接库
target_link_libraries(zeus_string_utils_tests
    PRIVATE
        zeus_common_utilities
        GTest::GTest
        GTest::Main
        Threads::Threads
)

# 包含目录
target_include_directories(zeus_string_utils_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/common/utilities
)

# 编译定义
target_compile_definitions(zeus_string_utils_tests
    PRIVATE
        ZEUS_UTILITIES_TESTING=1
)

# 发现测试
include(GoogleTest)
gtest_discover_tests(zeus_string_utils_tests)

# 添加自定义测试目标
add_custom_target(run_string_utils_tests
    COMMAND zeus_string_utils_tests
    DEPENDS zeus_string_utils_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)