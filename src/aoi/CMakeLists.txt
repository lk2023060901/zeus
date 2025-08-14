# Zeus AOI Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== AOI Module Configuration ===")

# AOI module sources (to be implemented)
set(AOI_SOURCES
    # aoi_main.cpp
    # aoi_server.cpp
    # aoi_manager.cpp
    # spatial_index.cpp
    # grid_manager.cpp
)

# AOI module headers (to be implemented)
set(AOI_HEADERS
    # ${CMAKE_SOURCE_DIR}/include/aoi/aoi_server.h
    # ${CMAKE_SOURCE_DIR}/include/aoi/aoi_manager.h
    # ${CMAKE_SOURCE_DIR}/include/aoi/spatial_index.h
    # ${CMAKE_SOURCE_DIR}/include/aoi/grid_manager.h
)

# TODO: Uncomment when source files are implemented
# Create the AOI library
# add_library(aoi_module ${AOI_SOURCES} ${AOI_HEADERS})

# Set target properties
# set_target_properties(aoi_module PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     OUTPUT_NAME "aoi_module"
# )

# Include directories
# target_include_directories(aoi_module
#     PUBLIC
#         ${CMAKE_SOURCE_DIR}/include
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# Link dependencies
# target_link_libraries(aoi_module
#     PUBLIC
#         common_network  # Zeus networking
#         common_spdlog   # Zeus logging
# )

message(STATUS "AOI module configured (placeholder)")
message(STATUS "===================================")