# Zeus Utilities Tests
cmake_minimum_required(VERSION 3.15)

# 添加cmake模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# 查找测试框架
find_package(GTest REQUIRED)

# 设置通用测试属性
set(UTILITIES_TEST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/common/utilities
)

set(UTILITIES_TEST_LIBRARIES
    zeus_common_utilities
    GTest::GTest
    GTest::Main
    Threads::Threads
)

# 添加子目录
add_subdirectory(singleton)
add_subdirectory(string_utils)

# 创建所有utilities测试的聚合目标
add_custom_target(run_utilities_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_CURRENT_BINARY_DIR} --output-on-failure
    COMMENT "Running all utilities tests"
)

# 添加依赖关系
add_dependencies(run_utilities_tests
    zeus_singleton_tests
    zeus_string_utils_tests
)