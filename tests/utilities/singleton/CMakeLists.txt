# Zeus Singleton Tests
cmake_minimum_required(VERSION 3.15)

# 查找测试框架
find_package(GTest REQUIRED)

# 测试源文件
set(SINGLETON_TEST_SOURCES
    test_singleton_basic.cpp
    test_singleton_thread_safety.cpp
    test_singleton_policies.cpp
)

# 创建测试可执行文件
add_executable(zeus_singleton_tests ${SINGLETON_TEST_SOURCES})

# 设置C++标准
set_target_properties(zeus_singleton_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接库
target_link_libraries(zeus_singleton_tests
    PRIVATE
        zeus_common_utilities
        GTest::GTest
        GTest::Main
        Threads::Threads
)

# 包含目录
target_include_directories(zeus_singleton_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src/common/utilities
)

# 编译定义
target_compile_definitions(zeus_singleton_tests
    PRIVATE
        ZEUS_UTILITIES_TESTING=1
)

# 发现测试
include(GoogleTest)
gtest_discover_tests(zeus_singleton_tests)

# 添加自定义测试目标
add_custom_target(run_singleton_tests
    COMMAND zeus_singleton_tests
    DEPENDS zeus_singleton_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)