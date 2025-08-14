# Simple Application Example
cmake_minimum_required(VERSION 3.15)

# 示例可执行文件
add_executable(simple_application_example
    simple_application_example.cpp
)

# 设置C++标准
set_target_properties(simple_application_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# 链接依赖库
target_link_libraries(simple_application_example
    PRIVATE
        zeus_core_app           # Zeus应用程序框架
        zeus_network           # Zeus网络模块
        zeus_spdlog           # Zeus日志模块
        Boost::system
        Boost::thread
        Boost::filesystem
        Boost::asio
        nlohmann_json::nlohmann_json
        fmt::fmt
        Threads::Threads
)

# 包含目录
target_include_directories(simple_application_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# 编译定义
target_compile_definitions(simple_application_example
    PRIVATE
        ZEUS_EXAMPLE_BUILD=1
        ZEUS_SIMPLE_APPLICATION=1
)

# 创建日志和配置目录
add_custom_command(TARGET simple_application_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
    $<TARGET_FILE_DIR:simple_application_example>/logs
    COMMENT "Creating logs directory for simple application example"
)

add_custom_command(TARGET simple_application_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
    $<TARGET_FILE_DIR:simple_application_example>/scripts
    COMMENT "Creating scripts directory for simple application example"
)

# 运行目标
add_custom_target(run_simple_application
    COMMAND simple_application_example
    DEPENDS simple_application_example
    WORKING_DIRECTORY $<TARGET_FILE_DIR:simple_application_example>
    COMMENT "Running simple application example"
)