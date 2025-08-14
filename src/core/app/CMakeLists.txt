# Zeus Core Application Framework Module
cmake_minimum_required(VERSION 3.15)

# 定义模块名称
set(MODULE_NAME zeus_core_app)

# 收集源文件
file(GLOB_RECURSE ZEUS_CORE_APP_SOURCES
    "*.cpp"
    "*.h"
    "${CMAKE_SOURCE_DIR}/include/core/app/*.h"
)

# 收集配置提供者源文件
file(GLOB_RECURSE CONFIG_PROVIDER_SOURCES
    "config_providers/*.cpp"
    "config_providers/*.h"
)

# 创建静态库
add_library(${MODULE_NAME} STATIC
    ${ZEUS_CORE_APP_SOURCES}
    ${CONFIG_PROVIDER_SOURCES}
)

# 设置目标属性
set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# 包含目录
target_include_directories(${MODULE_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/core/app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 依赖库
target_link_libraries(${MODULE_NAME}
    PUBLIC
        # Zeus 网络模块
        zeus_network
        
        # Zeus 日志模块
        zeus_spdlog
)

# 编译选项
target_compile_options(${MODULE_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# 编译定义
target_compile_definitions(${MODULE_NAME}
    PUBLIC
        ZEUS_CORE_APP_VERSION_MAJOR=1
        ZEUS_CORE_APP_VERSION_MINOR=0
        ZEUS_CORE_APP_VERSION_PATCH=0
        
    PRIVATE
        $<$<CONFIG:Debug>:ZEUS_CORE_APP_DEBUG>
        $<$<CONFIG:Release>:ZEUS_CORE_APP_RELEASE>
        BOOST_ASIO_HAS_STD_ADDRESSOF
        BOOST_ASIO_HAS_STD_ARRAY
        BOOST_ASIO_HAS_CSTDINT
        BOOST_ASIO_HAS_STD_SHARED_PTR
        BOOST_ASIO_HAS_STD_TYPE_TRAITS
)

# 安装规则
install(TARGETS ${MODULE_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/core/app/
    DESTINATION include/core/app
    FILES_MATCHING PATTERN "*.h"
)

# 调试信息
message(STATUS "Zeus Core App Module Configuration:")
message(STATUS "  Module Name: ${MODULE_NAME}")
message(STATUS "  Source Files: ${ZEUS_CORE_APP_SOURCES}")
message(STATUS "  Config Provider Files: ${CONFIG_PROVIDER_SOURCES}")
message(STATUS "  Include Directories: ${CMAKE_SOURCE_DIR}/include")

# 可选：创建示例可执行文件
option(ZEUS_BUILD_CORE_APP_EXAMPLES "Build Zeus Core App examples" OFF)
if(ZEUS_BUILD_CORE_APP_EXAMPLES)
    add_subdirectory(examples)
endif()

# 可选：创建测试
option(ZEUS_BUILD_CORE_APP_TESTS "Build Zeus Core App tests" OFF)
if(ZEUS_BUILD_CORE_APP_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()