# Zeus Zone Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Zone Module Configuration ===")

# Zone module sources (to be implemented)
set(ZONE_SOURCES
    # zone_main.cpp
    # zone_server.cpp
    # zone_manager.cpp
    # scene_manager.cpp
    # instance_manager.cpp
)

# Zone module headers (to be implemented)
set(ZONE_HEADERS
    # ${CMAKE_SOURCE_DIR}/include/zone/zone_server.h
    # ${CMAKE_SOURCE_DIR}/include/zone/zone_manager.h
    # ${CMAKE_SOURCE_DIR}/include/zone/scene_manager.h
    # ${CMAKE_SOURCE_DIR}/include/zone/instance_manager.h
)

# TODO: Uncomment when source files are implemented
# Create the zone library
# add_library(zone_module ${ZONE_SOURCES} ${ZONE_HEADERS})

# Set target properties
# set_target_properties(zone_module PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     OUTPUT_NAME "zone_module"
# )

# Include directories
# target_include_directories(zone_module
#     PUBLIC
#         ${CMAKE_SOURCE_DIR}/include
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# Link dependencies
# target_link_libraries(zone_module
#     PUBLIC
#         common_network  # Zeus networking
#         common_spdlog   # Zeus logging
# )

message(STATUS "Zone module configured (placeholder)")
message(STATUS "=====================================")