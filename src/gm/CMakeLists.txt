# Zeus GM Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== GM Module Configuration ===")

# GM module sources (to be implemented)
set(GM_SOURCES
    # gm_main.cpp
    # gm_server.cpp
    # gm_service.cpp
    # admin_interface.cpp
    # monitoring_service.cpp
)

# GM module headers (to be implemented)
set(GM_HEADERS
    # ${CMAKE_SOURCE_DIR}/include/gm/gm_server.h
    # ${CMAKE_SOURCE_DIR}/include/gm/gm_service.h
    # ${CMAKE_SOURCE_DIR}/include/gm/admin_interface.h
    # ${CMAKE_SOURCE_DIR}/include/gm/monitoring_service.h
)

# TODO: Uncomment when source files are implemented
# Create the GM library
# add_library(gm_module ${GM_SOURCES} ${GM_HEADERS})

# Set target properties
# set_target_properties(gm_module PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     OUTPUT_NAME "gm_module"
# )

# Include directories
# target_include_directories(gm_module
#     PUBLIC
#         ${CMAKE_SOURCE_DIR}/include
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# Link dependencies
# target_link_libraries(gm_module
#     PUBLIC
#         common_network  # Zeus networking
#         common_spdlog   # Zeus logging
# )

message(STATUS "GM module configured (placeholder)")
message(STATUS "==================================")