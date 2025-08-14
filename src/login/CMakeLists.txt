# Zeus Login Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Login Module Configuration ===")

# Login module sources (to be implemented)
set(LOGIN_SOURCES
    # login_main.cpp
    # login_server.cpp
    # login_service.cpp
    # auth_manager.cpp
)

# Login module headers (to be implemented)
set(LOGIN_HEADERS
    # ${CMAKE_SOURCE_DIR}/include/login/login_server.h
    # ${CMAKE_SOURCE_DIR}/include/login/login_service.h
    # ${CMAKE_SOURCE_DIR}/include/login/auth_manager.h
)

# TODO: Uncomment when source files are implemented
# Create the login library
# add_library(login_module ${LOGIN_SOURCES} ${LOGIN_HEADERS})

# Set target properties
# set_target_properties(login_module PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     OUTPUT_NAME "login_module"
# )

# Include directories
# target_include_directories(login_module
#     PUBLIC
#         ${CMAKE_SOURCE_DIR}/include
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# Link dependencies
# target_link_libraries(login_module
#     PUBLIC
#         common_network  # Zeus networking
#         common_spdlog   # Zeus logging
# )

message(STATUS "Login module configured (placeholder)")
message(STATUS "====================================")