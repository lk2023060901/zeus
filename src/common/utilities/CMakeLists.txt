# Zeus Common Utilities Library

# 设置库名称
set(UTILITIES_LIB_NAME zeus_common_utilities)

# 收集头文件
file(GLOB_RECURSE UTILITIES_HEADERS
    "${CMAKE_SOURCE_DIR}/include/common/utilities/*.h"
)

# 收集源文件
file(GLOB_RECURSE UTILITIES_SOURCES
    "*.cpp"
)

# 创建静态库
add_library(${UTILITIES_LIB_NAME} STATIC
    ${UTILITIES_HEADERS}
    ${UTILITIES_SOURCES}
)

# 设置目标属性
set_target_properties(${UTILITIES_LIB_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# 设置包含目录
target_include_directories(${UTILITIES_LIB_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接依赖库
target_link_libraries(${UTILITIES_LIB_NAME}
    PUBLIC
        # 标准库线程支持
        ${CMAKE_THREAD_LIBS_INIT}
)

# 查找线程库
find_package(Threads REQUIRED)
if(Threads_FOUND)
    target_link_libraries(${UTILITIES_LIB_NAME} PUBLIC Threads::Threads)
endif()

# 编译选项
target_compile_options(${UTILITIES_LIB_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# 编译定义
target_compile_definitions(${UTILITIES_LIB_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
)

# 安装头文件
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/common/utilities"
    DESTINATION include/common
    FILES_MATCHING PATTERN "*.h"
)

# 安装库文件
install(TARGETS ${UTILITIES_LIB_NAME}
    EXPORT zeus_common_utilities_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# 导出目标
install(EXPORT zeus_common_utilities_targets
    FILE zeus_common_utilities_targets.cmake
    NAMESPACE zeus::
    DESTINATION lib/cmake/zeus_common_utilities
)

# 创建配置文件
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/zeus_common_utilities_config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/zeus_common_utilities_config.cmake"
    INSTALL_DESTINATION lib/cmake/zeus_common_utilities
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/zeus_common_utilities_config_version.cmake"
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/zeus_common_utilities_config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/zeus_common_utilities_config_version.cmake"
    DESTINATION lib/cmake/zeus_common_utilities
)

# 添加别名以便在同一项目中使用
add_library(zeus::common_utilities ALIAS ${UTILITIES_LIB_NAME})

# 显示构建信息
message(STATUS "Zeus Common Utilities Library configured:")
message(STATUS "  Library name: ${UTILITIES_LIB_NAME}")
message(STATUS "  Headers: ${UTILITIES_HEADERS}")
message(STATUS "  Sources: ${UTILITIES_SOURCES}")
message(STATUS "  Include directories: ${CMAKE_SOURCE_DIR}/include")

# 可选：添加测试支持
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# 可选：添加示例
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()